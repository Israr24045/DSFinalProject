#ifndef VIDEO_EXPORT_H
#define VIDEO_EXPORT_H

#include "canvas.h"
#include <vector>
#include <string>
#include <filesystem>

class VideoExport {
public:
    static bool generateVideo(const std::vector<std::vector<Pixel>>& snapshots, 
                             int width, int height, 
                             const std::string& outputFilename);
};

#endif