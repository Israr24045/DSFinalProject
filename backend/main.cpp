#include "server.h"
#include "database.h"
#include "canvas.h"

// Global instances
Database* g_database = nullptr;
Canvas* g_canvas = nullptr;
Server* g_server = nullptr;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    std::cout << "\nShutting down gracefully..." << std::endl;
    
    if (g_server) {
        g_server->stop();
    }
    
    if (g_canvas) {
        g_canvas->stop();
    }
    
    if (g_database) {
        g_database->save();
    }
}

int main() {
    // Register signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    std::cout << "=== Season Canvas Server ===" << std::endl;
    std::cout << "Initializing..." << std::endl;
    
    // Initialize database
    g_database = new Database("data/canvas.omni");
    if (!g_database->load()) {
        std::cerr << "Failed to load database, creating new one..." << std::endl;
        g_database->initialize();
    }
    std::cout << "Database loaded." << std::endl;
    
    // Initialize canvas
    g_canvas = new Canvas(g_database);
    g_canvas->start();
    std::cout << "Canvas initialized." << std::endl;
    
    // Initialize and start server
    g_server = new Server(8080, g_database, g_canvas);
    std::cout << "Starting server on http://localhost:8080" << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;
    
    g_server->start();
    
    // Cleanup (unreachable in normal flow due to server blocking)
    delete g_server;
    delete g_canvas;
    delete g_database;
    
    return 0;
}