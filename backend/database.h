#ifndef DATABASE_H
#define DATABASE_H

#include "btree.h"
#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <fstream>

// User structure
struct User {
    uint32_t id;
    std::string email;
    std::string username;
    std::string passwordHash;
    uint64_t registrationTime;
};

// Episode metadata
struct EpisodeMetadata {
    uint32_t episodeNumber;
    uint64_t startTimestamp;
    uint64_t endTimestamp;
};

class Database {
public:
    Database(const std::string& filename);
    ~Database();
    
    bool load();
    bool save();
    void initialize();
    
    // User management
    uint32_t registerUser(const std::string& email, const std::string& username, const std::string& password);
    uint32_t authenticateUser(const std::string& email, const std::string& password);
    std::shared_ptr<User> getUserById(uint32_t userId);
    std::shared_ptr<User> getUserByEmail(const std::string& email);
    
    // Session management
    void createSession(const std::string& sessionId, uint32_t userId);
    uint32_t getUserIdFromSession(const std::string& sessionId);
    void removeSession(const std::string& sessionId);
    
    // Episode management
    void saveEpisode(uint32_t episodeNumber, uint64_t startTime, uint64_t endTime);
    std::vector<EpisodeMetadata> getEpisodeHistory(int count);
    
private:
    std::string filename_;
    uint32_t nextUserId_;
    
    // In-memory structures
    BTree userIdIndex_;          // B-Tree: userId -> User offset
    std::unordered_map<std::string, uint32_t> emailToUserId_;  // Hash: email -> userId
    std::unordered_map<std::string, uint32_t> sessions_;       // Hash: sessionId -> userId
    std::vector<User> users_;
    std::vector<EpisodeMetadata> episodes_;
    
    // Helper functions
    std::string hashPassword(const std::string& password);
    bool verifyPassword(const std::string& password, const std::string& hash);
    
    // Serialization
    void serialize(std::ofstream& out);
    void deserialize(std::ifstream& in);
};

#endif