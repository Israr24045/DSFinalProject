#ifndef SERVER_H
#define SERVER_H

#include "httplib.h"
#include "database.h"
#include "canvas.h"
#include <string>
#include <cstdint>

class Server {
public:
    Server(int port, Database* db, Canvas* canvas);
    ~Server();
    
    void start();
    void stop();
    
private:
    int port_;
    Database* db_;
    Canvas* canvas_;
    httplib::Server server_;
    
    // Route handlers
    void setupRoutes();
    void serveStatic();
    
    // API handlers
    void handleGetCanvas(const httplib::Request& req, httplib::Response& res);
    void handlePlacePixel(const httplib::Request& req, httplib::Response& res);
    void handleRegister(const httplib::Request& req, httplib::Response& res);
    void handleLogin(const httplib::Request& req, httplib::Response& res);
    void handleGetChat(const httplib::Request& req, httplib::Response& res);
    void handlePostChat(const httplib::Request& req, httplib::Response& res);
    void handleGetQuests(const httplib::Request& req, httplib::Response& res);
    void handleGetSeason(const httplib::Request& req, httplib::Response& res);
    void handleGetEpisode(const httplib::Request& req, httplib::Response& res);
    void handleExportPNG(const httplib::Request& req, httplib::Response& res);
    void handleExportVideo(const httplib::Request& req, httplib::Response& res);
    void handleGetHistory(const httplib::Request& req, httplib::Response& res);
    
    // Utility functions
    std::string getSessionId(const httplib::Request& req);
    bool isUserLoggedIn(const std::string& sessionId, uint32_t& userId);
    std::string generateSessionId();
};

#endif