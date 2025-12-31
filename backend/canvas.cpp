#include "canvas.h"
#include "snapshot.h"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <vector>

namespace fs = std::filesystem;

// Constants
const int CANVAS_SIZE = 50;
const int EPISODE_DURATION = 900;  // 15 minutes in seconds
const int USER_COOLDOWN = 5;       // 5 seconds
const int GUEST_COOLDOWN = 10;     // 10 seconds
const int SEASON_INTERVAL = 180;   // 3 minutes
const int SNAPSHOT_INTERVAL = 10;  // 10 seconds
const int FREEZE_DURATION = 10;    // 10 seconds

Canvas::Canvas(Database* db)
    : db_(db), running_(false), episodeNumber_(1), episodeStartTime_(0),
      episodeFrozen_(false), currentSeason_(Season::Calm) {
    
    // Initialize canvas
    canvas_.resize(CANVAS_SIZE);
    for (int i = 0; i < CANVAS_SIZE; ++i) {
        canvas_[i].resize(CANVAS_SIZE);
        for (int j = 0; j < CANVAS_SIZE; ++j) {
            canvas_[i][j] = {j, i, 15, PixelMood::Calm, 0, 0};  // White, calm
        }
    }
    
    // Initialize quests
    quests_.push_back({"Place 40 blue pixels", 0, 40, false});
    quests_.push_back({"Fill top-left 10x10 area", 0, 100, false});
    quests_.push_back({"Place 20 calm pixels", 0, 20, false});
}

Canvas::~Canvas() {
    stop();
}

void Canvas::start() {
    running_ = true;
    episodeStartTime_ = getCurrentTime();
    
    episodeThread_ = std::thread(&Canvas::episodeLoop, this);
    seasonThread_ = std::thread(&Canvas::seasonLoop, this);
    snapshotThread_ = std::thread(&Canvas::snapshotLoop, this);
    
    std::cout << "Canvas started. Episode " << episodeNumber_ << " begins!" << std::endl;
}

void Canvas::stop() {
    running_ = false;
    cv_.notify_all();
    
    if (episodeThread_.joinable()) episodeThread_.join();
    if (seasonThread_.joinable()) seasonThread_.join();
    if (snapshotThread_.joinable()) snapshotThread_.join();
}

bool Canvas::placePixel(int x, int y, uint8_t color, uint8_t mood, uint32_t userId, bool isLoggedIn) {
    std::cerr << "[Canvas] placePixel called x=" << x << " y=" << y << " color=" << (int)color << " uid=" << userId << " loggedIn=" << isLoggedIn << std::endl;
    std::lock_guard lock(canvasMutex_);
    
    // Check if episode is frozen
    if (episodeFrozen_) {
        std::cerr << "[Canvas] placePixel failed: episode frozen" << std::endl;
        return false;
    }
    
    // Check bounds
    if (x < 0 || x >= CANVAS_SIZE || y < 0 || y >= CANVAS_SIZE) {
        std::cerr << "[Canvas] placePixel failed: out of bounds" << std::endl;
        return false;
    }
    
    // Check cooldown
    uint64_t now = getCurrentTime();
    int cooldown = isLoggedIn ? USER_COOLDOWN : GUEST_COOLDOWN;
    
    auto it = cooldowns_.find(userId);
    if (it != cooldowns_.end()) {
        if (now - it->second < (uint64_t)cooldown) {
            std::cerr << "[Canvas] placePixel failed: cooldown active" << std::endl;
            return false;  // Still in cooldown
        }
    }
    
    // Place pixel
    canvas_[y][x] = Pixel{x, y, color, static_cast<PixelMood>(mood), now, userId};
    cooldowns_[userId] = now;
    
    // Update quests
    updateQuests(x, y, color, static_cast<PixelMood>(mood));

    std::cerr << "[Canvas] placePixel success x=" << x << " y=" << y << " uid=" << userId << std::endl;
    return true;
}

std::vector<Pixel> Canvas::getRegion(int x, int y, int width, int height) {
    std::cerr << "[Canvas] getRegion called x=" << x << " y=" << y << " w=" << width << " h=" << height << std::endl;
    std::lock_guard lock(canvasMutex_);
    
    std::vector<Pixel> result;
    
    int endX = std::min(x + width, CANVAS_SIZE);
    int endY = std::min(y + height, CANVAS_SIZE);
    
    for (int j = y; j < endY; ++j) {
        for (int i = x; i < endX; ++i) {
            result.push_back(canvas_[j][i]);
        }
    }
    std::cerr << "[Canvas] getRegion returning " << result.size() << " pixels" << std::endl;
    return result;
}

std::vector<Pixel> Canvas::getAllPixels() {
    std::cerr << "[Canvas] getAllPixels called" << std::endl;
    std::lock_guard lock(canvasMutex_);
    return getAllPixelsUnlocked();
}

// Assumes caller already holds `canvasMutex_` if needed. Copies current canvas state without locking.
std::vector<Pixel> Canvas::getAllPixelsUnlocked() {
    std::vector<Pixel> result;
    result.reserve(CANVAS_SIZE * CANVAS_SIZE);
    for (int j = 0; j < CANVAS_SIZE; ++j) {
        for (int i = 0; i < CANVAS_SIZE; ++i) {
            result.push_back(canvas_[j][i]);
        }
    }
    return result;
}

EpisodeInfo Canvas::getEpisodeInfo() {
    std::lock_guard lock(canvasMutex_);
    
    uint64_t now = getCurrentTime();
    int elapsed = now - episodeStartTime_;
    int remaining = std::max(0, EPISODE_DURATION - elapsed);
    std::cerr << "[Canvas] getEpisodeInfo called remaining=" << remaining << " episode=" << episodeNumber_ << std::endl;

    return {episodeNumber_, remaining, !episodeFrozen_, episodeFrozen_};
}

std::vector<std::vector<Pixel>> Canvas::getSnapshots() {
    std::lock_guard lock(canvasMutex_);
    return snapshots_;
}

std::string Canvas::getCurrentSeason() {
    switch (currentSeason_) {
        case Season::Bloom: return "Bloom";
        case Season::Frost: return "Frost";
        case Season::Warm: return "Warm";
        case Season::Calm: return "Calm";
        default: return "Calm";
    }
}

std::vector<Quest> Canvas::getQuests() {
    std::lock_guard lock(canvasMutex_);
    return quests_;
}

void Canvas::addChatMessage(const std::string& username, const std::string& message) {
    std::lock_guard lock(chatMutex_);
    
    ChatMessage msg{username, message, getCurrentTime()};
    chatMessages_.push_back(msg);
    
    // Keep only last 100 messages
    if (chatMessages_.size() > 100) {
        chatMessages_.erase(chatMessages_.begin());
    }
}

std::vector<ChatMessage> Canvas::getChatMessages() {
    std::lock_guard lock(chatMutex_);
    return chatMessages_;
}

void Canvas::episodeLoop() {
    std::unique_lock<std::mutex> cvLock(cvMutex_);
    while (running_) {
        uint64_t now = getCurrentTime();
        int elapsed = now - episodeStartTime_;
        
        if (elapsed >= EPISODE_DURATION) {
            endEpisode();
        }
        
        cv_.wait_for(cvLock, std::chrono::seconds(1));
    }
}

void Canvas::seasonLoop() {
    std::unique_lock<std::mutex> cvLock(cvMutex_);
    int seasonIndex = 0;
    Season seasons[] = {Season::Bloom, Season::Frost, Season::Warm, Season::Calm};
    
    while (running_) {
        cv_.wait_for(cvLock, std::chrono::seconds(SEASON_INTERVAL));
        
        if (!running_) break;
        
        seasonIndex = (seasonIndex + 1) % 4;
        currentSeason_ = seasons[seasonIndex];
        
        applySeason();
        
        std::cout << "Season changed to: " << getCurrentSeason() << std::endl;
    }
}

void Canvas::snapshotLoop() {
    std::unique_lock<std::mutex> cvLock(cvMutex_);
    while (running_) {
        cv_.wait_for(cvLock, std::chrono::seconds(SNAPSHOT_INTERVAL));
        
        if (!running_) break;
        
        if (!episodeFrozen_) {
            std::lock_guard canvasLock(canvasMutex_);

            std::cerr << "[Canvas] snapshotLoop: taking snapshot" << std::endl;
            // Store snapshot (we already hold the lock; use unlocked helper to avoid re-locking)
            std::vector<Pixel> snapshot = getAllPixelsUnlocked();
            snapshots_.push_back(std::move(snapshot));

            std::cout << "Snapshot taken (" << snapshots_.size() << " total)" << std::endl;
        }
    }
}

void Canvas::resetCanvas() {
    for (int i = 0; i < CANVAS_SIZE; ++i) {
        for (int j = 0; j < CANVAS_SIZE; ++j) {
            canvas_[i][j] = {j, i, 15, PixelMood::Calm, 0, 0};  // White, calm
        }
    }
    
    // Reset quests
    for (auto& quest : quests_) {
        quest.progress = 0;
        quest.completed = false;
    }
    
    cooldowns_.clear();
}

void Canvas::endEpisode() {
    std::lock_guard lock(canvasMutex_);

    std::cerr << "[Canvas] endEpisode called for episode " << episodeNumber_ << std::endl;
    std::cout << "Episode " << episodeNumber_ << " ended!" << std::endl;
    
    // Freeze canvas
    episodeFrozen_ = true;
    
    // Save final snapshot (we hold the lock; use unlocked helper to avoid deadlock)
    fs::create_directories("exports");
    std::string filename = "exports/episode_" + std::to_string(episodeNumber_) + ".png";
    auto finalSnapshot = getAllPixelsUnlocked();
    Snapshot::exportPNG(finalSnapshot, CANVAS_SIZE, CANVAS_SIZE, filename);
    
    // Save episode metadata
    db_->saveEpisode(episodeNumber_, episodeStartTime_, getCurrentTime());
    
    // Wait for freeze duration
    std::this_thread::sleep_for(std::chrono::seconds(FREEZE_DURATION));
    
    // Reset for next episode
    snapshots_.clear();
    resetCanvas();
    episodeNumber_++;
    episodeStartTime_ = getCurrentTime();
    episodeFrozen_ = false;
    
    std::cout << "Episode " << episodeNumber_ << " started!" << std::endl;
    std::cerr << "[Canvas] endEpisode completed, new episode " << episodeNumber_ << std::endl;
}

void Canvas::updateQuests(int x, int y, uint8_t color, PixelMood mood) {
    // Quest 0: Place 40 blue pixels
    if (color == 2 && !quests_[0].completed) {  // Blue
        quests_[0].progress++;
        if (quests_[0].progress >= quests_[0].target) {
            quests_[0].completed = true;
            std::cout << "Quest completed: " << quests_[0].description << std::endl;
        }
    }
    
    // Quest 1: Fill top-left 10x10 area
    if (x < 10 && y < 10 && !quests_[1].completed) {
        quests_[1].progress++;
        if (quests_[1].progress >= quests_[1].target) {
            quests_[1].completed = true;
            std::cout << "Quest completed: " << quests_[1].description << std::endl;
        }
    }
    
    // Quest 2: Place 20 calm pixels
    if (mood == PixelMood::Calm && !quests_[2].completed) {
        quests_[2].progress++;
        if (quests_[2].progress >= quests_[2].target) {
            quests_[2].completed = true;
            std::cout << "Quest completed: " << quests_[2].description << std::endl;
        }
    }
}

void Canvas::applySeason() {
    // Simple seasonal effects - adjust colors slightly
    // This is a simplified version; in production, you'd apply more sophisticated effects
}

uint64_t Canvas::getCurrentTime() {
    return (uint64_t)std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}