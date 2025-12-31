#ifndef CANVAS_H
#define CANVAS_H

#include "database.h"
#include <vector>
#include <string>
#include <cstdint>
#include <atomic>
#include <thread>
#include <mutex>
#include <map>
#include <condition_variable>

// Pixel moods
enum class PixelMood : uint8_t {
    Happy = 0,
    Sad = 1,
    Calm = 2,
    Energetic = 3
};

// Pixel structure
struct Pixel {
    int x, y;
    uint8_t color;
    PixelMood mood;
    uint64_t timestamp;
    uint32_t userId;
};

// Season types
enum class Season {
    Bloom,
    Frost,
    Warm,
    Calm
};

// Quest structure
struct Quest {
    std::string description;
    int progress;
    int target;
    bool completed;
};

// Chat message
struct ChatMessage {
    std::string username;
    std::string message;
    uint64_t timestamp;
};

// Episode info
struct EpisodeInfo {
    uint32_t episodeNumber;
    int timeRemaining;
    bool isActive;
    bool isFrozen;
};

class Canvas {
public:
    Canvas(Database* db);
    ~Canvas();
    
    void start();
    void stop();
    
    // Pixel operations
    bool placePixel(int x, int y, uint8_t color, uint8_t mood, uint32_t userId, bool isLoggedIn);
    std::vector<Pixel> getRegion(int x, int y, int width, int height);
    std::vector<Pixel> getAllPixels();
    
    // Episode management
    uint32_t getEpisodeNumber() const { return episodeNumber_; }
    EpisodeInfo getEpisodeInfo();
    std::vector<std::vector<Pixel>> getSnapshots();
    
    // Season management
    std::string getCurrentSeason();
    
    // Quest management
    std::vector<Quest> getQuests();
    
    // Chat
    void addChatMessage(const std::string& username, const std::string& message);
    std::vector<ChatMessage> getChatMessages();
    
    // Helper
    uint64_t getCurrentTime();
    
private:
    Database* db_;
    std::atomic<bool> running_;
    std::thread episodeThread_;
    std::thread seasonThread_;
    std::thread snapshotThread_;
    std::mutex canvasMutex_;
    std::mutex chatMutex_;
    std::mutex cvMutex_;
    std::condition_variable cv_;
    
    // Canvas state
    std::vector<std::vector<Pixel>> canvas_;
    uint32_t episodeNumber_;
    uint64_t episodeStartTime_;
    bool episodeFrozen_;
    
    // Cooldowns (userId -> timestamp)
    std::map<uint32_t, uint64_t> cooldowns_;
    
    // Season
    Season currentSeason_;
    
    // Quests
    std::vector<Quest> quests_;
    
    // Chat
    std::vector<ChatMessage> chatMessages_;
    
    // Snapshots
    std::vector<std::vector<Pixel>> snapshots_;
    
    // Thread functions
    void episodeLoop();
    void seasonLoop();
    void snapshotLoop();

    // Internal helper: copy all pixels without taking the mutex (caller must hold lock if needed)
    std::vector<Pixel> getAllPixelsUnlocked();
    
    // Helper functions
    void resetCanvas();
    void endEpisode();
    void updateQuests(int x, int y, uint8_t color, PixelMood mood);
    void applySeason();
};

#endif