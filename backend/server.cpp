#include "server.h"
#include "snapshot.h"
#include "video_export.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <random>
#include <string>
#include <functional>

namespace fs = std::filesystem;

Server::Server(int port, Database* db, Canvas* canvas)
    : port_(port), db_(db), canvas_(canvas) {
}

Server::~Server() {
    stop();
}

void Server::start() {
    setupRoutes();
    serveStatic();
    
    std::cout << "Server listening on port " << port_ << std::endl;
    server_.listen("0.0.0.0", port_);
}

void Server::stop() {
    server_.stop();
}

void Server::setupRoutes() {
    // API routes
    server_.Get("/api/canvas", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetCanvas(req, res);
    });
    
    server_.Post("/api/place_pixel", [this](const httplib::Request& req, httplib::Response& res) {
        handlePlacePixel(req, res);
    });
    
    server_.Post("/api/register", [this](const httplib::Request& req, httplib::Response& res) {
        handleRegister(req, res);
    });
    
    server_.Post("/api/login", [this](const httplib::Request& req, httplib::Response& res) {
        handleLogin(req, res);
    });
    
    server_.Get("/api/chat", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetChat(req, res);
    });
    
    server_.Post("/api/chat", [this](const httplib::Request& req, httplib::Response& res) {
        handlePostChat(req, res);
    });
    
    server_.Get("/api/quests", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetQuests(req, res);
    });
    
    server_.Get("/api/season", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetSeason(req, res);
    });
    
    server_.Get("/api/episode", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetEpisode(req, res);
    });
    
    server_.Get("/api/export_png", [this](const httplib::Request& req, httplib::Response& res) {
        handleExportPNG(req, res);
    });
    
    server_.Get("/api/export_video", [this](const httplib::Request& req, httplib::Response& res) {
        handleExportVideo(req, res);
    });
    
    server_.Get("/api/history", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetHistory(req, res);
    });
    
    server_.Get("/test", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{}", "application/json");
    });
}

void Server::serveStatic() {
    // Serve frontend files
    server_.Get("/", [](const httplib::Request&, httplib::Response& res) {
        std::cerr << "[HTTP] Serving index.html" << std::endl;
        std::ifstream file("frontend/index.html");
        if (file) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            res.set_content(buffer.str(), "text/html");
        } else {
            res.status = 404;
        }
    });
    
    server_.Get("/style.css", [](const httplib::Request&, httplib::Response& res) {
        std::ifstream file("frontend/style.css");
        if (file) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            res.set_content(buffer.str(), "text/css");
        } else {
            res.status = 404;
        }
    });
    
    server_.Get("/app.js", [](const httplib::Request&, httplib::Response& res) {
        std::ifstream file("frontend/app.js");
        if (file) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            res.set_content(buffer.str(), "application/javascript");
        } else {
            res.status = 404;
        }
    });
}

void Server::handleGetCanvas(const httplib::Request& req, httplib::Response& res) {
    std::cerr << "[HTTP] handleGetCanvas called" << std::endl;
    // Get visible region parameters
    int x = req.has_param("x") ? std::stoi(req.get_param_value("x")) : 0;
    int y = req.has_param("y") ? std::stoi(req.get_param_value("y")) : 0;
    int width = req.has_param("width") ? std::stoi(req.get_param_value("width")) : 50;
    int height = req.has_param("height") ? std::stoi(req.get_param_value("height")) : 50;
    
    // Get canvas data
    auto pixels = canvas_->getRegion(x, y, width, height);
    std::cerr << "[HTTP] handleGetCanvas sending " << pixels.size() << " pixels" << std::endl;
    
    // Build JSON response
    std::stringstream json;
    json << "{\"pixels\":[";
    
    bool first = true;
    for (const auto& pixel : pixels) {
        if (!first) json << ",";
        first = false;
        
        json << "{"
             << "\"x\":" << pixel.x << ","
             << "\"y\":" << pixel.y << ","
             << "\"color\":" << (int)pixel.color << ","
             << "\"mood\":" << (int)pixel.mood << ","
             << "\"timestamp\":" << pixel.timestamp << ","
             << "\"userId\":" << pixel.userId
             << "}";
    }
    
    json << "],\"timestamp\":" << canvas_->getCurrentTime() << "}";
    
    res.set_content(json.str(), "application/json");
}

void Server::handlePlacePixel(const httplib::Request& req, httplib::Response& res) {
    std::cerr << "[HTTP] handlePlacePixel called" << std::endl;
    // Parse request body (JSON)
    std::string body = req.body;
    
    // Simple JSON parsing (in production, use a proper JSON library)
    int x = -1, y = -1, color = -1, mood = 0;
    std::string sessionId;
    
    // Extract values (basic parsing)
    size_t pos;
    if ((pos = body.find("\"x\":")) != std::string::npos) {
        x = std::stoi(body.substr(pos + 4));
    }
    if ((pos = body.find("\"y\":")) != std::string::npos) {
        y = std::stoi(body.substr(pos + 4));
    }
    if ((pos = body.find("\"color\":")) != std::string::npos) {
        color = std::stoi(body.substr(pos + 8));
    }
    if ((pos = body.find("\"mood\":")) != std::string::npos) {
        mood = std::stoi(body.substr(pos + 7));
    }
    if ((pos = body.find("\"sessionId\":\"")) != std::string::npos) {
        size_t start = pos + 13;
        size_t end = body.find("\"", start);
        sessionId = body.substr(start, end - start);
    }
    
    // Validate input
    if (x < 0 || x >= 50 || y < 0 || y >= 50 || color < 0 || color >= 16) {
        res.set_content("{\"error\":\"Invalid coordinates or color\"}", "application/json");
        res.status = 400;
        return;
    }
    
    // Check if user is logged in
    uint32_t userId = 0;
    bool isLoggedIn = isUserLoggedIn(sessionId, userId);
    
    // Try to place pixel
    bool success = canvas_->placePixel(x, y, color, mood, userId, isLoggedIn);
    
    if (success) {
        res.set_content("{\"success\":true}", "application/json");
    } else {
        res.set_content("{\"error\":\"Cooldown active or episode ended\"}", "application/json");
        res.status = 429;
    }
}

void Server::handleRegister(const httplib::Request& req, httplib::Response& res) {
    std::cerr << "[HTTP] handleRegister called" << std::endl;
    std::string body = req.body;
    
    // Parse email, username, password
    std::string email, username, password;
    size_t pos;
    
    if ((pos = body.find("\"email\":\"")) != std::string::npos) {
        size_t start = pos + 9;
        size_t end = body.find("\"", start);
        email = body.substr(start, end - start);
    }
    if ((pos = body.find("\"username\":\"")) != std::string::npos) {
        size_t start = pos + 12;
        size_t end = body.find("\"", start);
        username = body.substr(start, end - start);
    }
    if ((pos = body.find("\"password\":\"")) != std::string::npos) {
        size_t start = pos + 12;
        size_t end = body.find("\"", start);
        password = body.substr(start, end - start);
    }
    
    // Validate
    if (email.empty() || username.empty() || password.empty()) {
        res.set_content("{\"error\":\"Missing fields\"}", "application/json");
        res.status = 400;
        return;
    }
    
    // Register user
    uint32_t userId = db_->registerUser(email, username, password);
    
    if (userId == 0) {
        res.set_content("{\"error\":\"Email already registered\"}", "application/json");
        res.status = 409;
        return;
    }
    
    // Generate session
    std::string sessionId = generateSessionId();
    db_->createSession(sessionId, userId);
    
    std::stringstream json;
    json << "{\"success\":true,\"sessionId\":\"" << sessionId << "\",\"userId\":" << userId << "}";
    res.set_content(json.str(), "application/json");
}

void Server::handleLogin(const httplib::Request& req, httplib::Response& res) {
    std::cerr << "[HTTP] handleLogin called" << std::endl;
    std::string body = req.body;
    
    // Parse email and password
    std::string email, password;
    size_t pos;
    
    if ((pos = body.find("\"email\":\"")) != std::string::npos) {
        size_t start = pos + 9;
        size_t end = body.find("\"", start);
        email = body.substr(start, end - start);
    }
    if ((pos = body.find("\"password\":\"")) != std::string::npos) {
        size_t start = pos + 12;
        size_t end = body.find("\"", start);
        password = body.substr(start, end - start);
    }
    
    // Authenticate
    uint32_t userId = db_->authenticateUser(email, password);
    
    if (userId == 0) {
        res.set_content("{\"error\":\"Invalid credentials\"}", "application/json");
        res.status = 401;
        return;
    }
    
    // Generate session
    std::string sessionId = generateSessionId();
    db_->createSession(sessionId, userId);
    
    std::stringstream json;
    json << "{\"success\":true,\"sessionId\":\"" << sessionId << "\",\"userId\":" << userId << "}";
    res.set_content(json.str(), "application/json");
}

void Server::handleGetChat(const httplib::Request&, httplib::Response& res) {
    std::cerr << "[HTTP] handleGetChat called" << std::endl;
    auto messages = canvas_->getChatMessages();
    
    std::cerr << "[HTTP] handleGetChat returning " << messages.size() << " messages" << std::endl;
    std::stringstream json;
    json << "{\"messages\":[";
    
    bool first = true;
    for (const auto& msg : messages) {
        if (!first) json << ",";
        first = false;
        
        json << "{"
             << "\"username\":\"" << msg.username << "\","
             << "\"message\":\"" << msg.message << "\","
             << "\"timestamp\":" << msg.timestamp
             << "}";
    }
    
    json << "]}";
    
    res.set_content(json.str(), "application/json");
}

void Server::handlePostChat(const httplib::Request& req, httplib::Response& res) {
    std::cerr << "[HTTP] handlePostChat called" << std::endl;
    std::string body = req.body;
    
    // Parse message and sessionId
    std::string message, sessionId;
    size_t pos;
    
    if ((pos = body.find("\"message\":\"")) != std::string::npos) {
        size_t start = pos + 11;
        size_t end = body.find("\"", start);
        message = body.substr(start, end - start);
    }
    if ((pos = body.find("\"sessionId\":\"")) != std::string::npos) {
        size_t start = pos + 13;
        size_t end = body.find("\"", start);
        sessionId = body.substr(start, end - start);
    }
    
    // Check if user is logged in
    uint32_t userId = 0;
    if (!isUserLoggedIn(sessionId, userId)) {
        std::cerr << "[HTTP] handlePostChat: not logged in, sessionId=" << sessionId << std::endl;
        res.set_content("{\"error\":\"Must be logged in to chat\"}", "application/json");
        res.status = 401;
        return;
    }
    
    std::cerr << "[HTTP] handlePostChat: userId=" << userId << " message=\"" << message << "\"" << std::endl;
    
    // Get username
    auto user = db_->getUserById(userId);
    if (!user) {
        res.set_content("{\"error\":\"User not found\"}", "application/json");
        res.status = 404;
        return;
    }
    
    // Add message
    canvas_->addChatMessage(user->username, message);
    
    res.set_content("{\"success\":true}", "application/json");
}

void Server::handleGetQuests(const httplib::Request&, httplib::Response& res) {
    std::cerr << "[HTTP] handleGetQuests called" << std::endl;
    auto quests = canvas_->getQuests();
    
    std::stringstream json;
    json << "{\"quests\":[";
    
    bool first = true;
    for (const auto& quest : quests) {
        if (!first) json << ",";
        first = false;
        
        json << "{"
             << "\"description\":\"" << quest.description << "\","
             << "\"progress\":" << quest.progress << ","
             << "\"target\":" << quest.target << ","
             << "\"completed\":" << (quest.completed ? "true" : "false")
             << "}";
    }
    
    json << "]}";
    
    res.set_content(json.str(), "application/json");
}

void Server::handleGetSeason(const httplib::Request&, httplib::Response& res) {
    std::cerr << "[HTTP] handleGetSeason called" << std::endl;
    std::string season = canvas_->getCurrentSeason();
    
    std::stringstream json;
    json << "{\"season\":\"" << season << "\",\"timestamp\":" << canvas_->getCurrentTime() << "}";
    
    res.set_content(json.str(), "application/json");
}

void Server::handleGetEpisode(const httplib::Request&, httplib::Response& res) {
    std::cerr << "[HTTP] handleGetEpisode called" << std::endl;
    auto info = canvas_->getEpisodeInfo();
    
    std::stringstream json;
    json << "{"
         << "\"episodeNumber\":" << info.episodeNumber << ","
         << "\"timeRemaining\":" << info.timeRemaining << ","
         << "\"isActive\":" << (info.isActive ? "true" : "false") << ","
         << "\"isFrozen\":" << (info.isFrozen ? "true" : "false") << ","
         << "\"timestamp\":" << canvas_->getCurrentTime()
         << "}";
    
    res.set_content(json.str(), "application/json");
}

void Server::handleExportPNG(const httplib::Request&, httplib::Response& res) {
    std::cerr << "[HTTP] handleExportPNG called" << std::endl;
    std::string filename = "exports/episode_" + std::to_string(canvas_->getEpisodeNumber()) + ".png";
    
    bool success = Snapshot::exportPNG(canvas_->getAllPixels(), 50, 50, filename);
    
    if (!success) {
        res.set_content("{\"error\":\"Failed to generate PNG\"}", "application/json");
        res.status = 500;
        return;
    }
    
    // Read file and send
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        res.set_content("{\"error\":\"Failed to read PNG\"}", "application/json");
        res.status = 500;
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    res.set_content(buffer.str(), "image/png");
    res.set_header("Content-Disposition", "attachment; filename=\"canvas.png\"");
}

void Server::handleExportVideo(const httplib::Request&, httplib::Response& res) {
    std::cerr << "[HTTP] handleExportVideo called" << std::endl;
    std::string filename = "exports/videos/episode_" + std::to_string(canvas_->getEpisodeNumber()) + ".mp4";
    
    // Create directories if needed
    fs::create_directories("exports/videos");
    
    bool success = VideoExport::generateVideo(canvas_->getSnapshots(), 50, 50, filename);
    
    if (!success) {
        res.set_content("{\"error\":\"Failed to generate video. Ensure FFmpeg is installed.\"}", "application/json");
        res.status = 500;
        return;
    }
    
    // Read file and send
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        res.set_content("{\"error\":\"Failed to read video\"}", "application/json");
        res.status = 500;
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    res.set_content(buffer.str(), "video/mp4");
    res.set_header("Content-Disposition", "attachment; filename=\"canvas_replay.mp4\"");
}

void Server::handleGetHistory(const httplib::Request&, httplib::Response& res) {
    std::cerr << "[HTTP] handleGetHistory called" << std::endl;
    auto history = db_->getEpisodeHistory(10);
    
    std::stringstream json;
    json << "{\"episodes\":[";
    
    bool first = true;
    for (const auto& episode : history) {
        if (!first) json << ",";
        first = false;
        
        json << "{"
             << "\"episodeNumber\":" << episode.episodeNumber << ","
             << "\"timestamp\":" << episode.endTimestamp << ","
             << "\"thumbnail\":\"exports/episode_" << episode.episodeNumber << ".png\""
             << "}";
    }
    
    json << "]}";
    
    res.set_content(json.str(), "application/json");
}

std::string Server::getSessionId(const httplib::Request& req) {
    if (req.has_header("Authorization")) {
        return req.get_header_value("Authorization");
    }
    return "";
}

bool Server::isUserLoggedIn(const std::string& sessionId, uint32_t& userId) {
    if (sessionId.empty()) return false;
    
    userId = db_->getUserIdFromSession(sessionId);
    return userId != 0;
}

std::string Server::generateSessionId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    const char* hex = "0123456789abcdef";
    std::string sessionId;
    
    for (int i = 0; i < 32; ++i) {
        sessionId += hex[dis(gen)];
    }
    
    return sessionId;
}