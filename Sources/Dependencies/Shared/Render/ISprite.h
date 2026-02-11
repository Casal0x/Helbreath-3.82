// ISprite.h: Abstract sprite interface for renderer abstraction
//
// Part of the shared interface layer between client and renderers
//////////////////////////////////////////////////////////////////////

#pragma once

#include "SpriteTypes.h"
#include <cstdint>

namespace hb::shared::sprite {

class ISprite {
public:
    virtual ~ISprite() = default;

    //------------------------------------------------------------------
    // Core Drawing
    //------------------------------------------------------------------

    // Primary draw method - renderer implements effect handling internally
    virtual void Draw(int x, int y, int frame, const DrawParams& params = DrawParams{}) = 0;

    // Draw to a specific destination (e.g., background surface)
    virtual void DrawToSurface(void* destSurface, int x, int y, int frame, const DrawParams& params = DrawParams{}) = 0;

    //------------------------------------------------------------------
    // Convenience Draw Methods (call Draw() with appropriate params)
    //------------------------------------------------------------------

    // Opaque drawing with color key
    void DrawFast(int x, int y, int frame) {
        Draw(x, y, frame, DrawParams::Opaque());
    }

    // Opaque drawing without color key
    void DrawFastNoColorKey(int x, int y, int frame) {
        Draw(x, y, frame, DrawParams::NoColorKey());
    }

    // Partial width drawing (for progress bars, etc.)
    virtual void DrawWidth(int x, int y, int frame, int width, bool vertical = false) = 0;

    // Alpha blending at specific levels
    void DrawAlpha(int x, int y, int frame, float alpha) {
        Draw(x, y, frame, DrawParams::Alpha(alpha));
    }

    void DrawAlpha70(int x, int y, int frame) {
        Draw(x, y, frame, DrawParams::Alpha(AlphaPreset::Alpha70));
    }

    void DrawAlpha50(int x, int y, int frame) {
        Draw(x, y, frame, DrawParams::Alpha(AlphaPreset::Alpha50));
    }

    void DrawAlpha25(int x, int y, int frame) {
        Draw(x, y, frame, DrawParams::Alpha(AlphaPreset::Alpha25));
    }

    // Alpha blending without color key (NoColorKey variants)
    void DrawAlphaNoColorKey(int x, int y, int frame, float alpha) {
        DrawParams p = DrawParams::Alpha(alpha);
        p.useColorKey = false;
        Draw(x, y, frame, p);
    }

    void DrawAlpha70NoColorKey(int x, int y, int frame) {
        DrawParams p = DrawParams::Alpha(AlphaPreset::Alpha70);
        p.useColorKey = false;
        Draw(x, y, frame, p);
    }

    void DrawAlpha50NoColorKey(int x, int y, int frame) {
        DrawParams p = DrawParams::Alpha(AlphaPreset::Alpha50);
        p.useColorKey = false;
        Draw(x, y, frame, p);
    }

    void DrawAlpha25NoColorKey(int x, int y, int frame) {
        DrawParams p = DrawParams::Alpha(AlphaPreset::Alpha25);
        p.useColorKey = false;
        Draw(x, y, frame, p);
    }

    // hb::shared::render::Color tinting
    void DrawTinted(int x, int y, int frame, int16_t r, int16_t g, int16_t b) {
        Draw(x, y, frame, DrawParams::Tint(r, g, b));
    }

    // Alpha + color tinting combined
    void DrawTintedAlpha(int x, int y, int frame, int16_t r, int16_t g, int16_t b, float alpha) {
        Draw(x, y, frame, DrawParams::TintedAlpha(r, g, b, alpha));
    }

    // Shadow projection
    void DrawShadow(int x, int y, int frame) {
        Draw(x, y, frame, DrawParams::Shadow());
    }

    // Fade effect
    void DrawFade(int x, int y, int frame) {
        Draw(x, y, frame, DrawParams::Fade());
    }

    //------------------------------------------------------------------
    // Shifted Drawing (position offset)
    //------------------------------------------------------------------

    virtual void DrawShifted(int x, int y, int shiftX, int shiftY, int frame, const DrawParams& params = DrawParams{}) = 0;

    //------------------------------------------------------------------
    // Frame Information
    //------------------------------------------------------------------

    virtual int GetFrameCount() const = 0;
    virtual SpriteRect GetFrameRect(int frame) const = 0;
    virtual void GetBoundingRect(int x, int y, int frame, int& left, int& top, int& right, int& bottom) = 0;

    // Calculate and store bounds for later retrieval (without drawing)
    // Used for pre-checking collision before deciding how to draw
    virtual void CalculateBounds(int x, int y, int frame) = 0;

    // Get bounds from the last Draw() or CalculateBounds() call
    // Returns true if valid bounds exist (top != -1), false otherwise
    virtual bool GetLastDrawBounds(int& left, int& top, int& right, int& bottom) const = 0;

    // Get bounds as a struct (for easier legacy code compatibility)
    virtual BoundRect GetBoundRect() const = 0;

    //------------------------------------------------------------------
    // Collision Detection
    //------------------------------------------------------------------

    virtual bool CheckCollision(int spriteX, int spriteY, int frame, int pointX, int pointY) = 0;

    //------------------------------------------------------------------
    // Resource Management
    //------------------------------------------------------------------

    virtual void Preload() = 0;
    virtual void Unload() = 0;
    virtual bool IsLoaded() const = 0;
    virtual void Restore() = 0;  // Restore after device loss (DDraw specific, no-op for modern)

    // Check if sprite is currently in use (e.g., in a critical section during draw)
    virtual bool IsInUse() const = 0;

    // Get timestamp of last access (for cache eviction)
    virtual uint32_t GetLastAccessTime() const = 0;
};

} // namespace hb::shared::sprite
