// IBitmapFont.h: Abstract interface for sprite-based bitmap font rendering
//
// Part of the shared interface layer between client and renderers
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>

namespace TextLib {

struct BitmapTextParams
{
    int16_t tintR = 0;
    int16_t tintG = 0;
    int16_t tintB = 0;
    float alpha = 1.0f;
    bool shadow = false;
    bool isColorReplace = false;  // true = direct RGB color, false = offset-based tint

    // Factory methods for common configurations
    static BitmapTextParams Default()
    {
        return BitmapTextParams{};
    }

    // ColorReplace: Direct RGB color - what you specify is what you get
    // Use this for bitmap fonts where you want a specific output color
    static BitmapTextParams ColorReplace(int16_t r, int16_t g, int16_t b)
    {
        BitmapTextParams p;
        p.tintR = r;
        p.tintG = g;
        p.tintB = b;
        p.isColorReplace = true;
        return p;
    }

    static BitmapTextParams ColorReplaceWithShadow(int16_t r, int16_t g, int16_t b)
    {
        BitmapTextParams p;
        p.tintR = r;
        p.tintG = g;
        p.tintB = b;
        p.shadow = true;
        p.isColorReplace = true;
        return p;
    }

    static BitmapTextParams ColorReplaceWithAlpha(int16_t r, int16_t g, int16_t b, float a)
    {
        BitmapTextParams p;
        p.tintR = r;
        p.tintG = g;
        p.tintB = b;
        p.alpha = a;
        p.isColorReplace = true;
        return p;
    }

    // Tinted: Offset-based tinting (legacy system using gray base)
    // Values are offsets from base gray (96): final = 96 + tint
    static BitmapTextParams Tinted(int16_t r, int16_t g, int16_t b)
    {
        BitmapTextParams p;
        p.tintR = r;
        p.tintG = g;
        p.tintB = b;
        p.isColorReplace = false;
        return p;
    }

    static BitmapTextParams WithShadow()
    {
        BitmapTextParams p;
        p.shadow = true;
        return p;
    }

    static BitmapTextParams TintedWithAlpha(int16_t r, int16_t g, int16_t b, float a)
    {
        BitmapTextParams p;
        p.tintR = r;
        p.tintG = g;
        p.tintB = b;
        p.alpha = a;
        return p;
    }

    static BitmapTextParams TintedWithShadow(int16_t r, int16_t g, int16_t b)
    {
        BitmapTextParams p;
        p.tintR = r;
        p.tintG = g;
        p.tintB = b;
        p.shadow = true;
        return p;
    }
};

class IBitmapFont
{
public:
    virtual ~IBitmapFont() = default;

    // Text measurement
    virtual int MeasureText(const char* text) const = 0;
    virtual int GetCharWidth(char c) const = 0;

    // Drawing
    virtual void DrawText(int x, int y, const char* text, const BitmapTextParams& params = BitmapTextParams::Default()) = 0;
    virtual void DrawTextCentered(int x1, int x2, int y, const char* text, const BitmapTextParams& params = BitmapTextParams::Default()) = 0;
};

} // namespace TextLib
