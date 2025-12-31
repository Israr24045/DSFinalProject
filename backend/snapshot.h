#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include "canvas.h"
#include <vector>
#include <string>
#include <cstdint>

class Snapshot {
public:
    static bool exportPNG(const std::vector<Pixel>& pixels, int width, int height, const std::string& filename);
    
private:
    static void getRGB(uint8_t colorIndex, uint8_t& r, uint8_t& g, uint8_t& b);
};

#endif