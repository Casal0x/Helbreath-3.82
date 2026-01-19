// PixelConversion.h: 16-bit to 32-bit pixel conversion utilities
//
// Part of SFMLEngine static library
// Converts between PAK file 16-bit formats (RGB565/RGB555) and SFML's RGBA8888
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>

namespace PixelConversion {

// Convert RGB565 pixel to RGBA8888 (SFML format: sf::Color is RGBA)
inline uint32_t RGB565toRGBA8888(uint16_t pixel, uint16_t colorKey)
{
    if (pixel == colorKey)
        return 0x00000000;  // Fully transparent

    // Extract RGB565 components
    uint8_t r = static_cast<uint8_t>(((pixel >> 11) & 0x1F) << 3);  // 5-bit to 8-bit
    uint8_t g = static_cast<uint8_t>(((pixel >> 5) & 0x3F) << 2);   // 6-bit to 8-bit
    uint8_t b = static_cast<uint8_t>((pixel & 0x1F) << 3);          // 5-bit to 8-bit

    // Fill in the low bits for better precision
    r |= (r >> 5);
    g |= (g >> 6);
    b |= (b >> 5);

    // Return as RGBA (R in highest byte for sf::Color compatibility)
    return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}

// Convert RGB565 pixel to ABGR8888 (sf::Image pixel format)
inline uint32_t RGB565toABGR8888(uint16_t pixel, uint16_t colorKey)
{
    if (pixel == colorKey)
        return 0x00000000;  // Fully transparent

    // Extract RGB565 components
    uint8_t r = static_cast<uint8_t>(((pixel >> 11) & 0x1F) << 3);
    uint8_t g = static_cast<uint8_t>(((pixel >> 5) & 0x3F) << 2);
    uint8_t b = static_cast<uint8_t>((pixel & 0x1F) << 3);

    // Fill in the low bits for better precision
    r |= (r >> 5);
    g |= (g >> 6);
    b |= (b >> 5);

    // Return as ABGR (for direct sf::Image pixel manipulation)
    // sf::Image stores pixels as RGBA in memory, but on little-endian systems
    // we need to write them as R, G, B, A bytes
    return (0xFF << 24) | (b << 16) | (g << 8) | r;
}

// Convert RGB555 pixel to RGBA8888
inline uint32_t RGB555toRGBA8888(uint16_t pixel, uint16_t colorKey)
{
    if (pixel == colorKey)
        return 0x00000000;

    // Extract RGB555 components
    uint8_t r = static_cast<uint8_t>(((pixel >> 10) & 0x1F) << 3);
    uint8_t g = static_cast<uint8_t>(((pixel >> 5) & 0x1F) << 3);
    uint8_t b = static_cast<uint8_t>((pixel & 0x1F) << 3);

    // Fill in the low bits
    r |= (r >> 5);
    g |= (g >> 5);
    b |= (b >> 5);

    return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}

// Convert RGBA8888 back to RGB565 (for LockBackBuffer emulation)
inline uint16_t RGBA8888toRGB565(uint32_t pixel)
{
    uint8_t r = (pixel >> 24) & 0xFF;
    uint8_t g = (pixel >> 16) & 0xFF;
    uint8_t b = (pixel >> 8) & 0xFF;

    return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

// Create RGB565 from RGB components
inline uint16_t MakeRGB565(uint8_t r, uint8_t g, uint8_t b)
{
    return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

// Extract RGB from RGB565
inline void ExtractRGB565(uint16_t pixel, uint8_t& r, uint8_t& g, uint8_t& b)
{
    r = static_cast<uint8_t>(((pixel >> 11) & 0x1F) << 3);
    g = static_cast<uint8_t>(((pixel >> 5) & 0x3F) << 2);
    b = static_cast<uint8_t>((pixel & 0x1F) << 3);
}

// Apply tint to RGB values
inline void ApplyTint(uint8_t& r, uint8_t& g, uint8_t& b, int16_t tintR, int16_t tintG, int16_t tintB)
{
    int newR = r + tintR;
    int newG = g + tintG;
    int newB = b + tintB;

    r = static_cast<uint8_t>(newR < 0 ? 0 : (newR > 255 ? 255 : newR));
    g = static_cast<uint8_t>(newG < 0 ? 0 : (newG > 255 ? 255 : newG));
    b = static_cast<uint8_t>(newB < 0 ? 0 : (newB > 255 ? 255 : newB));
}

// Convert pixel data buffer from 16-bit to 32-bit RGBA
// srcData: Source pixel data (RGB565)
// width, height: Dimensions
// srcPitch: Source pitch in uint16_t units (pixels per row)
// colorKey: Color key value for transparency
// destData: Destination RGBA buffer (must be width * height * 4 bytes)
inline void ConvertBuffer16to32(
    const uint16_t* srcData,
    int width,
    int height,
    int srcPitch,
    uint16_t colorKey,
    uint8_t* destData)
{
    for (int y = 0; y < height; y++)
    {
        const uint16_t* srcRow = srcData + y * srcPitch;
        uint8_t* destRow = destData + y * width * 4;

        for (int x = 0; x < width; x++)
        {
            uint16_t pixel = srcRow[x];
            uint8_t* dest = destRow + x * 4;

            if (pixel == colorKey)
            {
                dest[0] = 0;  // R
                dest[1] = 0;  // G
                dest[2] = 0;  // B
                dest[3] = 0;  // A (transparent)
            }
            else
            {
                uint8_t r, g, b;
                ExtractRGB565(pixel, r, g, b);
                dest[0] = r;
                dest[1] = g;
                dest[2] = b;
                dest[3] = 255;  // A (opaque)
            }
        }
    }
}

} // namespace PixelConversion
