// SFMLBitmapFont.h: SFML implementation of IBitmapFont
//
// Part of SFMLEngine static library
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IBitmapFont.h"
#include "BitmapFontFactory.h"
#include "ISprite.h"
#include <vector>

// Undefine Windows CreateFont macro to avoid naming conflict
#ifdef CreateFont
#undef CreateFont
#endif

namespace TextLib {

class SFMLBitmapFont : public IBitmapFont
{
public:
    SFMLBitmapFont(SpriteLib::ISprite* sprite, char firstChar, char lastChar,
                   int frameOffset, const FontSpacing& spacing);
    ~SFMLBitmapFont() override = default;

    // Text measurement
    int MeasureText(const char* text) const override;
    int GetCharWidth(char c) const override;

    // Drawing
    void DrawText(int x, int y, const char* text, const BitmapTextParams& params) override;
    void DrawTextCentered(int x1, int x2, int y, const char* text, const BitmapTextParams& params) override;

private:
    int GetFrameForChar(char c) const;

    SpriteLib::ISprite* m_pSprite;
    char m_firstChar;
    char m_lastChar;
    int m_frameOffset;
    std::vector<int> m_charWidths;
    int m_defaultWidth;
    bool m_useDynamicSpacing;
};

class SFMLBitmapFontFactory : public BitmapFontFactory
{
public:
    ~SFMLBitmapFontFactory() override = default;

    IBitmapFont* CreateFont(SpriteLib::ISprite* sprite, char firstChar, char lastChar,
                            int frameOffset, const FontSpacing& spacing) override;
    IBitmapFont* CreateFontDynamic(SpriteLib::ISprite* sprite, char firstChar, char lastChar,
                                   int frameOffset) override;
};

} // namespace TextLib
