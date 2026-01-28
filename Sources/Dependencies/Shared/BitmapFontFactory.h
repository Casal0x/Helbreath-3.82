// BitmapFontFactory.h: Factory interface for creating bitmap fonts from sprites
//
// Part of the shared interface layer between client and renderers
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IBitmapFont.h"
#include "ISprite.h"
#include <vector>
#include <memory>

// Undefine Windows CreateFont macro to avoid naming conflict
#ifdef CreateFont
#undef CreateFont
#endif

namespace TextLib {

struct FontSpacing
{
    std::vector<int> charWidths;  // Per-character widths (if not using dynamic)
    int defaultWidth = 5;          // Default width when charWidths is empty
    bool useDynamicSpacing = false; // Use sprite frame widths for spacing
};

class BitmapFontFactory
{
public:
    virtual ~BitmapFontFactory() = default;

    // Create font with explicit character width array
    virtual std::unique_ptr<IBitmapFont> CreateFont(SpriteLib::ISprite* sprite, char firstChar, char lastChar,
                                                    int frameOffset, const FontSpacing& spacing) = 0;

    // Create font using dynamic spacing from sprite frame dimensions
    virtual std::unique_ptr<IBitmapFont> CreateFontDynamic(SpriteLib::ISprite* sprite, char firstChar, char lastChar,
                                                           int frameOffset) = 0;
};

// Global accessor - set by RendererFactory during initialization
BitmapFontFactory* GetBitmapFontFactory();
void SetBitmapFontFactory(BitmapFontFactory* factory);

// Convenience function to create a bitmap font using the global factory
inline std::unique_ptr<IBitmapFont> CreateBitmapFont(SpriteLib::ISprite* sprite, char firstChar, char lastChar,
                                                     int frameOffset, const FontSpacing& spacing)
{
    BitmapFontFactory* factory = GetBitmapFontFactory();
    if (!factory)
        return nullptr;
    return factory->CreateFont(sprite, firstChar, lastChar, frameOffset, spacing);
}

inline std::unique_ptr<IBitmapFont> CreateBitmapFontDynamic(SpriteLib::ISprite* sprite, char firstChar, char lastChar,
                                                            int frameOffset)
{
    BitmapFontFactory* factory = GetBitmapFontFactory();
    if (!factory)
        return nullptr;
    return factory->CreateFontDynamic(sprite, firstChar, lastChar, frameOffset);
}

} // namespace TextLib
