// ColorUtils.h: Color conversion utilities for renderer-agnostic color handling
//
// All interfaces use RGB888 format (8 bits per channel, 0-255 range).
// This utility provides conversions from legacy 565/555 formats.
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>

namespace ColorUtils {

// Create RGB888 color from 8-bit components (0-255 each)
// Returns color in 0x00RRGGBB format
inline uint32_t RGB888(uint8_t r, uint8_t g, uint8_t b)
{
    return static_cast<uint32_t>(r) | (static_cast<uint32_t>(g) << 8) | (static_cast<uint32_t>(b) << 16);
}

// Create RGB888 color with alpha from 8-bit components (0-255 each)
// Returns color in 0xAARRGGBB format
inline uint32_t RGBA8888(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
    return static_cast<uint32_t>(r) | (static_cast<uint32_t>(g) << 8) |
           (static_cast<uint32_t>(b) << 16) | (static_cast<uint32_t>(a) << 24);
}

// Extract components from RGB888 color (0x00RRGGBB format)
inline void ToComponents(uint32_t color, uint8_t& r, uint8_t& g, uint8_t& b)
{
    r = static_cast<uint8_t>(color & 0xFF);
    g = static_cast<uint8_t>((color >> 8) & 0xFF);
    b = static_cast<uint8_t>((color >> 16) & 0xFF);
}

// Extract components from RGBA8888 color (0xAARRGGBB format)
inline void ToComponentsWithAlpha(uint32_t color, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a)
{
    r = static_cast<uint8_t>(color & 0xFF);
    g = static_cast<uint8_t>((color >> 8) & 0xFF);
    b = static_cast<uint8_t>((color >> 16) & 0xFF);
    a = static_cast<uint8_t>((color >> 24) & 0xFF);
}

// Convert RGB565 packed color to RGB888
// RGB565: RRRRRGGGGGGBBBBB (5-6-5 bits)
inline uint32_t FromRGB565(uint16_t color565)
{
    // Extract 5-6-5 components
    uint8_t r5 = (color565 >> 11) & 0x1F;  // 5 bits
    uint8_t g6 = (color565 >> 5) & 0x3F;   // 6 bits
    uint8_t b5 = color565 & 0x1F;          // 5 bits

    // Scale to 8 bits: (val << 3) | (val >> 2) for 5-bit, (val << 2) | (val >> 4) for 6-bit
    uint8_t r8 = (r5 << 3) | (r5 >> 2);
    uint8_t g8 = (g6 << 2) | (g6 >> 4);
    uint8_t b8 = (b5 << 3) | (b5 >> 2);

    return RGB888(r8, g8, b8);
}

// Convert RGB555 packed color to RGB888
// RGB555: 0RRRRRGGGGGBBBBB (5-5-5 bits)
inline uint32_t FromRGB555(uint16_t color555)
{
    // Extract 5-5-5 components
    uint8_t r5 = (color555 >> 10) & 0x1F;  // 5 bits
    uint8_t g5 = (color555 >> 5) & 0x1F;   // 5 bits
    uint8_t b5 = color555 & 0x1F;          // 5 bits

    // Scale to 8 bits: (val << 3) | (val >> 2)
    uint8_t r8 = (r5 << 3) | (r5 >> 2);
    uint8_t g8 = (g5 << 3) | (g5 >> 2);
    uint8_t b8 = (b5 << 3) | (b5 >> 2);

    return RGB888(r8, g8, b8);
}

// Convert RGB888 to RGB565 packed color
inline uint16_t ToRGB565(uint32_t color888)
{
    uint8_t r, g, b;
    ToComponents(color888, r, g, b);

    uint16_t r5 = (r >> 3) & 0x1F;
    uint16_t g6 = (g >> 2) & 0x3F;
    uint16_t b5 = (b >> 3) & 0x1F;

    return (r5 << 11) | (g6 << 5) | b5;
}

// Convert RGB888 to RGB555 packed color
inline uint16_t ToRGB555(uint32_t color888)
{
    uint8_t r, g, b;
    ToComponents(color888, r, g, b);

    uint16_t r5 = (r >> 3) & 0x1F;
    uint16_t g5 = (g >> 3) & 0x1F;
    uint16_t b5 = (b >> 3) & 0x1F;

    return (r5 << 10) | (g5 << 5) | b5;
}

// Scale a 5-bit color component (0-31) to 8-bit (0-255)
inline uint8_t Scale5To8(uint8_t val5)
{
    return (val5 << 3) | (val5 >> 2);
}

// Scale a 6-bit color component (0-63) to 8-bit (0-255)
inline uint8_t Scale6To8(uint8_t val6)
{
    return (val6 << 2) | (val6 >> 4);
}

// Scale an 8-bit color component (0-255) to 5-bit (0-31)
inline uint8_t Scale8To5(uint8_t val8)
{
    return val8 >> 3;
}

// Scale an 8-bit color component (0-255) to 6-bit (0-63)
inline uint8_t Scale8To6(uint8_t val8)
{
    return val8 >> 2;
}

} // namespace ColorUtils
