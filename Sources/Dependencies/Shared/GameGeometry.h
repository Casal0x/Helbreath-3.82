// GameGeometry.h: Shared geometry types for Client and Server
//
// Cross-platform point and rectangle types with optional Windows conversion helpers.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>

// Note: This header does NOT include <windows.h> to avoid winsock ordering
// issues on the server. The Windows conversion helpers (ToPOINT, FromPOINT,
// ToRECT, FromRECT) are available when <windows.h> has already been included
// before this header (detected via the _WINDEF_ include guard).

// ============================================================================
// Point Type
// ============================================================================

struct GamePoint
{
    int x = 0;
    int y = 0;

    constexpr GamePoint() = default;
    constexpr GamePoint(int x_, int y_) : x(x_), y(y_) {}

#ifdef _WINDEF_
    POINT ToPOINT() const { return POINT{ x, y }; }
    static GamePoint FromPOINT(const POINT& p) { return GamePoint(p.x, p.y); }
#endif
};

// ============================================================================
// Rectangle Type
// ============================================================================

// Renderer-agnostic rectangle for game use
// Can be converted to platform-specific types (RECT, sf::IntRect) as needed
struct GameRectangle
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    constexpr GameRectangle() = default;
    constexpr GameRectangle(int x_, int y_, int w, int h)
        : x(x_), y(y_), width(w), height(h) {}

    // Edge accessors for compatibility
    constexpr int Left() const { return x; }
    constexpr int Top() const { return y; }
    constexpr int Right() const { return x + width; }
    constexpr int Bottom() const { return y + height; }

#ifdef _WINDEF_
    // Convert to Windows RECT
    RECT ToRECT() const { return RECT{ x, y, x + width, y + height }; }

    // Construct from Windows RECT
    static GameRectangle FromRECT(const RECT& r) {
        return GameRectangle(r.left, r.top, r.right - r.left, r.bottom - r.top);
    }
#endif
};
