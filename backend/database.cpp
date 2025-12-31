#include "database.h"
#include "sha256.h"
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <ctime>
#include <stdexcept>

namespace fs = std::filesystem;

Database::Database(const std::string& filename)
    : filename_(filename), nextUserId_(1), userIdIndex_(5) {
}

Database::~Database() {
    save();
}

bool Database::load() {
    std::ifstream in(filename_, std::ios::binary);
    if (!in) {
        return false;
    }
    
    try {
        deserialize(in);
        return true;
    } catch (...) {
        return false;
    }
}

bool Database::save() {
    // Create data directory if it doesn't exist
    fs::create_directories("data");
    
    std::ofstream out(filename_, std::ios::binary);
    if (!out) {
        return false;
    }
    
    serialize(out);
    return true;
}

void Database::initialize() {
    nextUserId_ = 1;
    users_.clear();
    episodes_.clear();
    emailToUserId_.clear();
    sessions_.clear();
    userIdIndex_ = BTree(5);
    
    std::cout << "Database initialized with default values." << std::endl;

    // Add default test users when initializing a fresh database
    if (users_.empty()) {
        uint32_t id1 = registerUser("bscs24045@itu.edu.pk", "israr", "itu123");
        if (id1) {
            std::cout << "Default user created: bscs24045@itu.edu.pk (ID: " << id1 << ")" << std::endl;
        }
        uint32_t id2 = registerUser("bscs24009@itu.edu.pk", "abdullah", "itu123");
        if (id2) {
            std::cout << "Default user created: bscs24009@itu.edu.pk (ID: " << id2 << ")" << std::endl;
        }
        uint32_t id3 = registerUser("bscs24017@itu.edu.pk", "ali", "itu123");
        if (id3) {
            std::cout << "Default user created: bscs24017@itu.edu.pk (ID: " << id3 << ")" << std::endl;
        }
    }
}

uint32_t Database::registerUser(const std::string& email, const std::string& username, const std::string& password) {
    // Check if email already exists
    if (emailToUserId_.find(email) != emailToUserId_.end()) {
        return 0;  // Email already registered
    }
    
    // Create new user
    User user;
    user.id = nextUserId_++;
    user.email = email;
    user.username = username;
    user.passwordHash = hashPassword(password);
    user.registrationTime = static_cast<uint64_t>(std::time(nullptr));
    
    // Store user
    users_.push_back(user);
    emailToUserId_[email] = user.id;
    userIdIndex_.insert(user.id, users_.size() - 1);
    
    std::cout << "User registered: " << username << " (ID: " << user.id << ")" << std::endl;
    
    return user.id;
}

uint32_t Database::authenticateUser(const std::string& email, const std::string& password) {
    auto it = emailToUserId_.find(email);
    if (it == emailToUserId_.end()) {
        return 0;  // User not found
    }
    
    uint32_t userId = it->second;
    auto user = getUserById(userId);
    
    if (!user) {
        return 0;
    }
    
    if (verifyPassword(password, user->passwordHash)) {
        return userId;
    }
    
    return 0;  // Invalid password
}

std::shared_ptr<User> Database::getUserById(uint32_t userId) {
    int userIndex = userIdIndex_.search(userId);
    if (userIndex >= 0 && userIndex < (int)users_.size()) {
        return std::make_shared<User>(users_[userIndex]);
    }
    return nullptr;
}

std::shared_ptr<User> Database::getUserByEmail(const std::string& email) {
    auto it = emailToUserId_.find(email);
    if (it != emailToUserId_.end()) {
        return getUserById(it->second);
    }
    return nullptr;
}

void Database::createSession(const std::string& sessionId, uint32_t userId) {
    sessions_[sessionId] = userId;
}

uint32_t Database::getUserIdFromSession(const std::string& sessionId) {
    auto it = sessions_.find(sessionId);
    if (it != sessions_.end()) {
        return it->second;
    }
    return 0;
}

void Database::removeSession(const std::string& sessionId) {
    sessions_.erase(sessionId);
}

void Database::saveEpisode(uint32_t episodeNumber, uint64_t startTime, uint64_t endTime) {
    EpisodeMetadata episode;
    episode.episodeNumber = episodeNumber;
    episode.startTimestamp = startTime;
    episode.endTimestamp = endTime;
    
    episodes_.push_back(episode);
    
    // Keep only last 10 episodes
    if (episodes_.size() > 10) {
        episodes_.erase(episodes_.begin());
    }
}

std::vector<EpisodeMetadata> Database::getEpisodeHistory(int count) {
    int start = std::max(0, (int)episodes_.size() - count);
    return std::vector<EpisodeMetadata>(episodes_.begin() + start, episodes_.end());
}

std::string Database::hashPassword(const std::string& password) {
    return sha256(password);
}

bool Database::verifyPassword(const std::string& password, const std::string& hash) {
    return sha256(password) == hash;
}

void Database::serialize(std::ofstream& out) {
    // Write magic number
    uint32_t magic = 0x4F4D4E49;  // "OMNI"
    out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    
    // Write next user ID
    out.write(reinterpret_cast<const char*>(&nextUserId_), sizeof(nextUserId_));
    
    // Write users
    uint32_t userCount = (uint32_t)users_.size();
    out.write(reinterpret_cast<const char*>(&userCount), sizeof(userCount));
    
    for (const auto& user : users_) {
        out.write(reinterpret_cast<const char*>(&user.id), sizeof(user.id));
        
        uint32_t emailLen = user.email.size();
        out.write(reinterpret_cast<const char*>(&emailLen), sizeof(emailLen));
        out.write(user.email.c_str(), emailLen);
        
        uint32_t usernameLen = user.username.size();
        out.write(reinterpret_cast<const char*>(&usernameLen), sizeof(usernameLen));
        out.write(user.username.c_str(), usernameLen);
        
        uint32_t hashLen = user.passwordHash.size();
        out.write(reinterpret_cast<const char*>(&hashLen), sizeof(hashLen));
        out.write(user.passwordHash.c_str(), hashLen);
        
        out.write(reinterpret_cast<const char*>(&user.registrationTime), sizeof(user.registrationTime));
    }
    
    // Write episodes
    uint32_t episodeCount = (uint32_t)episodes_.size();
    out.write(reinterpret_cast<const char*>(&episodeCount), sizeof(episodeCount));
    
    for (const auto& episode : episodes_) {
        out.write(reinterpret_cast<const char*>(&episode.episodeNumber), sizeof(episode.episodeNumber));
        out.write(reinterpret_cast<const char*>(&episode.startTimestamp), sizeof(episode.startTimestamp));
        out.write(reinterpret_cast<const char*>(&episode.endTimestamp), sizeof(episode.endTimestamp));
    }

    // Write sessions
    uint32_t sessionCount = (uint32_t)sessions_.size();
    out.write(reinterpret_cast<const char*>(&sessionCount), sizeof(sessionCount));
    for (const auto &p : sessions_) {
        const std::string &sid = p.first;
        uint32_t uid = p.second;
        uint32_t sidLen = (uint32_t)sid.size();
        out.write(reinterpret_cast<const char*>(&sidLen), sizeof(sidLen));
        out.write(sid.c_str(), sidLen);
        out.write(reinterpret_cast<const char*>(&uid), sizeof(uid));
    }
}

void Database::deserialize(std::ifstream& in) {
    // Read magic number
    uint32_t magic;
    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != 0x4F4D4E49) {
        throw std::runtime_error("Invalid database file format");
    }
    
    // Read next user ID
    in.read(reinterpret_cast<char*>(&nextUserId_), sizeof(nextUserId_));
    
    // Read users
    uint32_t userCount;
    in.read(reinterpret_cast<char*>(&userCount), sizeof(userCount));
    
    users_.clear();
    emailToUserId_.clear();
    userIdIndex_ = BTree(5);
    
    for (uint32_t i = 0; i < userCount; ++i) {
        User user;
        in.read(reinterpret_cast<char*>(&user.id), sizeof(user.id));
        
        uint32_t emailLen;
        in.read(reinterpret_cast<char*>(&emailLen), sizeof(emailLen));
        user.email.resize(emailLen);
        in.read(&user.email[0], emailLen);
        
        uint32_t usernameLen;
        in.read(reinterpret_cast<char*>(&usernameLen), sizeof(usernameLen));
        user.username.resize(usernameLen);
        in.read(&user.username[0], usernameLen);
        
        uint32_t hashLen;
        in.read(reinterpret_cast<char*>(&hashLen), sizeof(hashLen));
        user.passwordHash.resize(hashLen);
        in.read(&user.passwordHash[0], hashLen);
        
        in.read(reinterpret_cast<char*>(&user.registrationTime), sizeof(user.registrationTime));
        
        users_.push_back(user);
        emailToUserId_[user.email] = user.id;
        userIdIndex_.insert(user.id, users_.size() - 1);
    }
    
    // Read episodes
    uint32_t episodeCount;
    in.read(reinterpret_cast<char*>(&episodeCount), sizeof(episodeCount));
    
    episodes_.clear();
    
    for (uint32_t i = 0; i < episodeCount; ++i) {
        EpisodeMetadata episode;
        in.read(reinterpret_cast<char*>(&episode.episodeNumber), sizeof(episode.episodeNumber));
        in.read(reinterpret_cast<char*>(&episode.startTimestamp), sizeof(episode.startTimestamp));
        in.read(reinterpret_cast<char*>(&episode.endTimestamp), sizeof(episode.endTimestamp));
        
        episodes_.push_back(episode);
    }
    
    // Read sessions
    uint32_t sessionCount = 0;
    in.read(reinterpret_cast<char*>(&sessionCount), sizeof(sessionCount));
    for (uint32_t i = 0; i < sessionCount; ++i) {
        uint32_t sidLen = 0;
        in.read(reinterpret_cast<char*>(&sidLen), sizeof(sidLen));
        std::string sid;
        sid.resize(sidLen);
        in.read(&sid[0], sidLen);
        uint32_t uid = 0;
        in.read(reinterpret_cast<char*>(&uid), sizeof(uid));
        sessions_[sid] = uid;
    }
    std::cout << "Database loaded: " << userCount << " users, " << episodeCount << " episodes" << std::endl;
}