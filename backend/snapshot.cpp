#include "snapshot.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <vector>
#include <cstdint>
#include <iostream>
#include <string>

bool Snapshot::exportPNG(const std::vector<Pixel>& pixels, int width, int height, const std::string& filename) {
    // Create RGB image buffer
    std::vector<uint8_t> image(width * height * 3);
    
    for (const auto& pixel : pixels) {
        if (pixel.x >= 0 && pixel.x < width && pixel.y >= 0 && pixel.y < height) {
            int index = (pixel.y * width + pixel.x) * 3;
            
            uint8_t r, g, b;
            getRGB(pixel.color, r, g, b);
            
            image[index + 0] = r;
            image[index + 1] = g;
            image[index + 2] = b;
        }
    }
    
    // Write PNG
    int result = stbi_write_png(filename.c_str(), width, height, 3, image.data(), width * 3);
    
    if (result) {
        std::cout << "PNG exported: " << filename << std::endl;
        return true;
    } else {
        std::cerr << "Failed to export PNG: " << filename << std::endl;
        return false;
    }
}

void Snapshot::getRGB(uint8_t colorIndex, uint8_t& r, uint8_t& g, uint8_t& b) {
    // 16-color palette
    static const uint8_t palette[16][3] = {
        {0, 0, 0},       // 0: Black
        {255, 0, 0},     // 1: Red
        {0, 0, 255},     // 2: Blue
        {0, 255, 0},     // 3: Green
        {255, 255, 0},   // 4: Yellow
        {255, 0, 255},   // 5: Magenta
        {0, 255, 255},   // 6: Cyan
        {255, 128, 0},   // 7: Orange
        {128, 0, 255},   // 8: Purple
        {0, 128, 0},     // 9: Dark Green
        {128, 128, 128}, // 10: Gray
        {255, 192, 203}, // 11: Pink
        {165, 42, 42},   // 12: Brown
        {255, 215, 0},   // 13: Gold
        {64, 224, 208},  // 14: Turquoise
        {255, 255, 255}  // 15: White
    };
    
    if (colorIndex < 16) {
        r = palette[colorIndex][0];
        g = palette[colorIndex][1];
        b = palette[colorIndex][2];
    } else {
        r = g = b = 255;  // Default to white
    }
}