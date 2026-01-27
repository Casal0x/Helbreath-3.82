// DDrawBitmapFont.cpp: DirectDraw implementation of IBitmapFont
//
// Part of DDrawEngine static library
//////////////////////////////////////////////////////////////////////

#include "DDrawBitmapFont.h"
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

// DDrawBitmapFont implementation

DDrawBitmapFont::DDrawBitmapFont(SpriteLib::ISprite* sprite, char firstChar, char lastChar,
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

int DDrawBitmapFont::GetFrameForChar(char c) const
{
    if (c < m_firstChar || c > m_lastChar)
        return -1;

    return m_frameOffset + (c - m_firstChar);
}

int DDrawBitmapFont::GetCharWidth(char c) const
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

int DDrawBitmapFont::MeasureText(const char* text) const
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

void DDrawBitmapFont::DrawText(int x, int y, const char* text, const BitmapTextParams& params)
{
    if (!text || !m_pSprite)
        return;

    // Convert 8-bit RGB to DDraw's 5-6-5 bit format
    // This matches the conversion done by ColorTransferRGB for RGB565 mode
    SpriteLib::DrawParams drawParams;
    drawParams.tintR = static_cast<int16_t>(params.tintR >> 3);   // 8-bit to 5-bit
    drawParams.tintG = static_cast<int16_t>(params.tintG >> 2);   // 8-bit to 6-bit
    drawParams.tintB = static_cast<int16_t>(params.tintB >> 3);   // 8-bit to 5-bit
    drawParams.alpha = params.alpha;
    drawParams.isColorReplace = params.isColorReplace;

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
                    // Draw shadow first using transparency blend (like original PutTransSprite)
                    // This darkens the destination rather than replacing with black
                    SpriteLib::DrawParams shadowParams;
                    shadowParams.alpha = 0.5f;  // Semi-transparent for shadow effect
                    m_pSprite->Draw(currentX + 1, y + 1, frame, shadowParams);
                }

                m_pSprite->Draw(currentX, y, frame, drawParams);
            }
        }

        currentX += GetCharWidth(c);
        text++;
    }
}

void DDrawBitmapFont::DrawTextCentered(int x1, int x2, int y, const char* text, const BitmapTextParams& params)
{
    if (!text)
        return;

    int textWidth = MeasureText(text);
    int centerX = (x1 + x2) / 2;
    int drawX = centerX - (textWidth / 2);

    DrawText(drawX, y, text, params);
}

// DDrawBitmapFontFactory implementation

std::unique_ptr<IBitmapFont> DDrawBitmapFontFactory::CreateFont(SpriteLib::ISprite* sprite, char firstChar, char lastChar,
                                                                 int frameOffset, const FontSpacing& spacing)
{
    if (!sprite)
        return nullptr;

    return std::make_unique<DDrawBitmapFont>(sprite, firstChar, lastChar, frameOffset, spacing);
}

std::unique_ptr<IBitmapFont> DDrawBitmapFontFactory::CreateFontDynamic(SpriteLib::ISprite* sprite, char firstChar, char lastChar,
                                                                        int frameOffset)
{
    if (!sprite)
        return nullptr;

    FontSpacing spacing;
    spacing.useDynamicSpacing = true;
    spacing.defaultWidth = 5;

    return std::make_unique<DDrawBitmapFont>(sprite, firstChar, lastChar, frameOffset, spacing);
}

} // namespace TextLib
