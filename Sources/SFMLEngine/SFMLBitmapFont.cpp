// SFMLBitmapFont.cpp: SFML implementation of IBitmapFont
//
// Part of SFMLEngine static library
//////////////////////////////////////////////////////////////////////

#include "SFMLBitmapFont.h"
#include "SpriteTypes.h"

namespace hb::shared::text {

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

SFMLBitmapFont::SFMLBitmapFont(hb::shared::sprite::ISprite* sprite, char firstChar, char lastChar,
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
            hb::shared::sprite::SpriteRect rect = m_pSprite->GetFrameRect(frame);
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

    // Set up draw parameters for bitmap font rendering
    // Bitmap font sprites are pure white - SFML multiply tint acts as color replacement:
    // white(255) * tint / 255 = tint. But (0,0,0) skips the tint block, so clamp to (1,1,1).
    hb::shared::sprite::DrawParams drawParams;
    drawParams.alpha = params.alpha;
    drawParams.tintR = params.tintR;
    drawParams.tintG = params.tintG;
    drawParams.tintB = params.tintB;

    if (params.isColorReplace && drawParams.tintR == 0 && drawParams.tintG == 0 && drawParams.tintB == 0)
    {
        drawParams.tintR = 1;
        drawParams.tintG = 1;
        drawParams.tintB = 1;
    }

    // Use additive blending when explicitly requested (for bright text like damage numbers)
    // This matches DDraw's additive tinting behavior for bright colors
    if (params.useAdditive)
    {
        drawParams.blendMode = hb::shared::sprite::BlendMode::Additive;
    }

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
                    // Original PutString_SprFont2 behavior:
                    // Draw raw/uncolored sprite at (+1,0) and (+1,+1) before main colored text
                    // In SFML, raw sprites appear white, so we draw with black to match DDraw
                    hb::shared::sprite::DrawParams shadowParams;
                    shadowParams.tintR = 1;
                    shadowParams.tintG = 1;
                    shadowParams.tintB = 1;
                    m_pSprite->Draw(currentX + 1, y, frame, shadowParams);
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

std::unique_ptr<IBitmapFont> SFMLBitmapFontFactory::CreateFont(hb::shared::sprite::ISprite* sprite, char firstChar, char lastChar,
                                                                int frameOffset, const FontSpacing& spacing)
{
    if (!sprite)
        return nullptr;

    return std::make_unique<SFMLBitmapFont>(sprite, firstChar, lastChar, frameOffset, spacing);
}

std::unique_ptr<IBitmapFont> SFMLBitmapFontFactory::CreateFontDynamic(hb::shared::sprite::ISprite* sprite, char firstChar, char lastChar,
                                                                       int frameOffset)
{
    if (!sprite)
        return nullptr;

    FontSpacing spacing;
    spacing.useDynamicSpacing = true;
    spacing.defaultWidth = 5;

    return std::make_unique<SFMLBitmapFont>(sprite, firstChar, lastChar, frameOffset, spacing);
}

} // namespace hb::shared::text
