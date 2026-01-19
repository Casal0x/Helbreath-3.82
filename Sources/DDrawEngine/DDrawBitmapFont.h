// DDrawBitmapFont.h: DirectDraw implementation of IBitmapFont
//
// Part of DDrawEngine static library
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

class DDrawBitmapFont : public IBitmapFont
{
public:
    DDrawBitmapFont(SpriteLib::ISprite* sprite, char firstChar, char lastChar,
                    int frameOffset, const FontSpacing& spacing);
    ~DDrawBitmapFont() override = default;

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

class DDrawBitmapFontFactory : public BitmapFontFactory
{
public:
    ~DDrawBitmapFontFactory() override = default;

    IBitmapFont* CreateFont(SpriteLib::ISprite* sprite, char firstChar, char lastChar,
                            int frameOffset, const FontSpacing& spacing) override;
    IBitmapFont* CreateFontDynamic(SpriteLib::ISprite* sprite, char firstChar, char lastChar,
                                   int frameOffset) override;
};

} // namespace TextLib
