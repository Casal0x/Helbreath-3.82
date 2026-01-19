// SFMLBitmapFont.cpp: SFML implementation of IBitmapFont
//
// Part of SFMLEngine static library
//////////////////////////////////////////////////////////////////////

#include "SFMLBitmapFont.h"
#include "SpriteTypes.h"

namespace TextLib {

// Global accessor implementation
static BitmapFontFactory* s_pBitmapFontFactory = nullptr;

BitmapFontFactory* GetBitmapFontFactory()
{
    return s_pBitmapFontFactory;
}

void SetBitmapFontFactory(BitmapFontFactory* factory)
{
    s_pBitmapFontFactory = factory;
}

// SFMLBitmapFont implementation

SFMLBitmapFont::SFMLBitmapFont(SpriteLib::ISprite* sprite, char firstChar, char lastChar,
                               int frameOffset, const FontSpacing& spacing)
    : m_pSprite(sprite)
    , m_firstChar(firstChar)
    , m_lastChar(lastChar)
    , m_frameOffset(frameOffset)
    , m_charWidths(spacing.charWidths)
    , m_defaultWidth(spacing.defaultWidth)
    , m_useDynamicSpacing(spacing.useDynamicSpacing)
{
}

int SFMLBitmapFont::GetFrameForChar(char c) const
{
    if (c < m_firstChar || c > m_lastChar)
        return -1;

    return m_frameOffset + (c - m_firstChar);
}

int SFMLBitmapFont::GetCharWidth(char c) const
{
    if (c == ' ')
        return m_defaultWidth;

    if (c < m_firstChar || c > m_lastChar)
        return m_defaultWidth;

    int charIndex = c - m_firstChar;

    if (m_useDynamicSpacing && m_pSprite)
    {
        int frame = GetFrameForChar(c);
        if (frame >= 0 && frame < m_pSprite->GetFrameCount())
        {
            SpriteLib::SpriteRect rect = m_pSprite->GetFrameRect(frame);
            return rect.width;
        }
    }

    if (!m_charWidths.empty() && charIndex < static_cast<int>(m_charWidths.size()))
    {
        return m_charWidths[charIndex];
    }

    return m_defaultWidth;
}

int SFMLBitmapFont::MeasureText(const char* text) const
{
    if (!text)
        return 0;

    int width = 0;
    while (*text)
    {
        width += GetCharWidth(*text);
        text++;
    }
    return width;
}

void SFMLBitmapFont::DrawText(int x, int y, const char* text, const BitmapTextParams& params)
{
    if (!text || !m_pSprite)
        return;

    // For bitmap fonts on black sprites, we use additive blending
    // This makes: black sprite + tint = tint (matches DDraw's behavior)
    // Use ColorReplace mode to treat tint values as direct RGB output
    SpriteLib::DrawParams drawParams;
    drawParams.alpha = params.alpha;
    drawParams.isAdditive = true;  // Use additive blending for black font sprites
    drawParams.isColorReplace = true;  // Tint values are direct RGB
    drawParams.tintR = params.tintR;
    drawParams.tintG = params.tintG;
    drawParams.tintB = params.tintB;

    int currentX = x;

    while (*text)
    {
        char c = *text;

        if (c != ' ')
        {
            int frame = GetFrameForChar(c);
            if (frame >= 0 && frame < m_pSprite->GetFrameCount())
            {
                if (params.shadow)
                {
                    // Draw shadow first (offset by 1,1 with black color)
                    // Shadow uses additive blend with black = no change
                    // So we use alpha blend for shadow to darken
                    SpriteLib::DrawParams shadowParams;
                    shadowParams.tintR = 0;
                    shadowParams.tintG = 0;
                    shadowParams.tintB = 0;
                    shadowParams.isColorReplace = true;
                    shadowParams.isAdditive = false;  // Use alpha blend for shadow
                    shadowParams.alpha = 0.5f;  // Semi-transparent to darken
                    m_pSprite->Draw(currentX + 1, y + 1, frame, shadowParams);
                }

                m_pSprite->Draw(currentX, y, frame, drawParams);
            }
        }

        currentX += GetCharWidth(c);
        text++;
    }
}

void SFMLBitmapFont::DrawTextCentered(int x1, int x2, int y, const char* text, const BitmapTextParams& params)
{
    if (!text)
        return;

    int textWidth = MeasureText(text);
    int centerX = (x1 + x2) / 2;
    int drawX = centerX - (textWidth / 2);

    DrawText(drawX, y, text, params);
}

// SFMLBitmapFontFactory implementation

IBitmapFont* SFMLBitmapFontFactory::CreateFont(SpriteLib::ISprite* sprite, char firstChar, char lastChar,
                                                int frameOffset, const FontSpacing& spacing)
{
    if (!sprite)
        return nullptr;

    return new SFMLBitmapFont(sprite, firstChar, lastChar, frameOffset, spacing);
}

IBitmapFont* SFMLBitmapFontFactory::CreateFontDynamic(SpriteLib::ISprite* sprite, char firstChar, char lastChar,
                                                       int frameOffset)
{
    if (!sprite)
        return nullptr;

    FontSpacing spacing;
    spacing.useDynamicSpacing = true;
    spacing.defaultWidth = 5;

    return new SFMLBitmapFont(sprite, firstChar, lastChar, frameOffset, spacing);
}

} // namespace TextLib
