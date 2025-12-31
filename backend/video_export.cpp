#include "video_export.h"
#include "snapshot.h"
#include <filesystem>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

bool VideoExport::generateVideo(const std::vector<std::vector<Pixel>>& snapshots, 
                                 int width, int height, 
                                 const std::string& outputFilename) {
    
    if (snapshots.empty()) {
        std::cerr << "No snapshots to export" << std::endl;
        return false;
    }
    
    // Create temporary directory for frames
    fs::path tempDir = "exports/temp_frames";
    fs::create_directories(tempDir);
    
    // Export each snapshot as a PNG frame
    std::cout << "Generating " << snapshots.size() << " frames..." << std::endl;
    
    for (size_t i = 0; i < snapshots.size(); ++i) {
        std::string frameFilename = tempDir.string() + "/frame_" + 
                                   std::to_string(i) + ".png";
        
        if (!Snapshot::exportPNG(snapshots[i], width, height, frameFilename)) {
            std::cerr << "Failed to export frame " << i << std::endl;
            return false;
        }
    }
    
    // Use FFmpeg to create video from frames
    std::stringstream cmd;
    cmd << "ffmpeg -y -framerate 10 -i " << tempDir.string() << "/frame_%d.png "
        << "-c:v libx264 -pix_fmt yuv420p -movflags +faststart "
        << outputFilename;
    
    std::cout << "Running FFmpeg: " << cmd.str() << std::endl;
    
    int result = system(cmd.str().c_str());
    
    if (result != 0) {
        std::cerr << "FFmpeg failed. Ensure FFmpeg is installed and in PATH." << std::endl;
        return false;
    }
    
    // Cleanup temporary frames
    fs::remove_all(tempDir);
    
    std::cout << "Video exported: " << outputFilename << std::endl;
    return true;
}