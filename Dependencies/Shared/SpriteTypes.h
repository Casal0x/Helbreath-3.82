// SpriteTypes.h: Shared sprite types for renderer abstraction
//
// Part of the shared interface layer between client and renderers
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>

namespace SpriteLib {

// Frame rectangle - matches PAKLib::sprite_rect binary layout exactly
struct SpriteRect {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    int16_t pivotX;
    int16_t pivotY;
};

// Bounding rectangle from last draw operation
// Used for hit testing and collision detection
struct BoundRect {
    int left = 0;
    int top = -1;  // -1 indicates invalid/not drawn
    int right = 0;
    int bottom = 0;

    bool IsValid() const { return top != -1; }
};

// Alpha blend presets matching legacy transparency levels
enum class AlphaPreset {
    Opaque = 100,   // No transparency
    Alpha70 = 70,   // 70% opacity
    Alpha50 = 50,   // 50% opacity
    Alpha25 = 25    // 25% opacity
};

// Drawing parameters for sprite rendering
struct DrawParams {
    // Alpha/transparency (0.0 = invisible, 1.0 = opaque)
    float alpha = 1.0f;

    // Color tint offset (-255 to +255 for each channel)
    int16_t tintR = 0;
    int16_t tintG = 0;
    int16_t tintB = 0;

    // Rendering flags
    bool useColorKey = true;    // false = NoColorKey variants
    bool isShadow = false;      // Shadow projection effect
    bool isReverse = false;     // Reverse blend effect
    bool isFade = false;        // Fade effect
    bool isColorReplace = false; // true = tint values are direct RGB, false = offset from base
    bool isAdditive = false;    // true = additive blending (for bitmap fonts on black sprites)

    // Static factory methods for common configurations
    static DrawParams Opaque() {
        return {};
    }

    static DrawParams Alpha(float a) {
        DrawParams p;
        p.alpha = a;
        return p;
    }

    static DrawParams Alpha(AlphaPreset preset) {
        DrawParams p;
        p.alpha = static_cast<float>(preset) / 100.0f;
        return p;
    }

    static DrawParams Tint(int16_t r, int16_t g, int16_t b) {
        DrawParams p;
        p.tintR = r;
        p.tintG = g;
        p.tintB = b;
        return p;
    }

    static DrawParams ColorReplace(int16_t r, int16_t g, int16_t b) {
        DrawParams p;
        p.tintR = r;
        p.tintG = g;
        p.tintB = b;
        p.isColorReplace = true;
        return p;
    }

    static DrawParams ColorReplaceAlpha(int16_t r, int16_t g, int16_t b, float a) {
        DrawParams p;
        p.alpha = a;
        p.tintR = r;
        p.tintG = g;
        p.tintB = b;
        p.isColorReplace = true;
        return p;
    }

    static DrawParams TintedAlpha(int16_t r, int16_t g, int16_t b, float a) {
        DrawParams p;
        p.alpha = a;
        p.tintR = r;
        p.tintG = g;
        p.tintB = b;
        return p;
    }

    static DrawParams Shadow() {
        DrawParams p;
        p.isShadow = true;
        return p;
    }

    static DrawParams NoColorKey() {
        DrawParams p;
        p.useColorKey = false;
        return p;
    }

    static DrawParams Fade() {
        DrawParams p;
        p.isFade = true;
        return p;
    }
};

} // namespace SpriteLib
