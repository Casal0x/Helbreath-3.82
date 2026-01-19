// SFMLTextRenderer.cpp: SFML implementation of ITextRenderer
//
// Part of SFMLEngine static library
//////////////////////////////////////////////////////////////////////

#include "SFMLTextRenderer.h"
#include <SFML/Graphics/Text.hpp>
#include <cstring>
#include <string>

namespace TextLib {

// Global accessor implementation
static ITextRenderer* s_pTextRenderer = nullptr;

ITextRenderer* GetTextRenderer()
{
    return s_pTextRenderer;
}

void SetTextRenderer(ITextRenderer* renderer)
{
    s_pTextRenderer = renderer;
}

SFMLTextRenderer::SFMLTextRenderer(sf::RenderTexture* backBuffer)
    : m_pBackBuffer(backBuffer)
    , m_fontLoaded(false)
    , m_fontSize(12)
{
    // Try to load a default font as fallback
    LoadDefaultFont();
}

bool SFMLTextRenderer::LoadDefaultFont()
{
    // Fallback to Windows system fonts if client didn't load a font
    const char* defaultFonts[] = {
        "C:\\Windows\\Fonts\\tahoma.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\segoeui.ttf"
    };

    for (const char* path : defaultFonts)
    {
        if (m_font.openFromFile(path))
        {
            // Disable texture smoothing for pixel-perfect rendering to match DDraw
            m_font.setSmooth(false);
            m_fontLoaded = true;
            return true;
        }
    }

    return false;
}

bool SFMLTextRenderer::LoadFontFromFile(const char* fontPath)
{
    if (!fontPath)
        return false;

    if (m_font.openFromFile(fontPath))
    {
        // Disable texture smoothing for pixel-perfect rendering to match DDraw
        m_font.setSmooth(false);
        m_fontLoaded = true;
        return true;
    }

    return false;
}

bool SFMLTextRenderer::LoadFontByName(const char* fontName)
{
    if (!fontName)
        return false;

    // Try to find the font in Windows Fonts folder
    std::string fontPath = "C:\\Windows\\Fonts\\";
    fontPath += fontName;

    // Try with common extensions
    const char* extensions[] = { ".ttf", ".otf", ".TTF", ".OTF", "" };

    for (const char* ext : extensions)
    {
        std::string fullPath = fontPath + ext;
        if (m_font.openFromFile(fullPath))
        {
            // Disable texture smoothing for pixel-perfect rendering to match DDraw
            m_font.setSmooth(false);
            m_fontLoaded = true;
            return true;
        }
    }

    return false;
}

void SFMLTextRenderer::SetFontSize(int size)
{
    m_fontSize = static_cast<unsigned int>(size);
}

bool SFMLTextRenderer::IsFontLoaded() const
{
    return m_fontLoaded;
}

TextMetrics SFMLTextRenderer::MeasureText(const char* text) const
{
    TextMetrics metrics = {0, 0};

    if (!text || !m_fontLoaded)
        return metrics;

    sf::Text sfText(m_font, text, m_fontSize);
    sf::FloatRect bounds = sfText.getLocalBounds();

    metrics.width = static_cast<int>(bounds.size.x);
    metrics.height = static_cast<int>(bounds.size.y);

    return metrics;
}

int SFMLTextRenderer::GetFittingCharCount(const char* text, int maxWidth) const
{
    if (!text || !m_fontLoaded)
        return 0;

    int len = static_cast<int>(strlen(text));
    sf::Text sfText(m_font, "", m_fontSize);

    for (int i = len; i > 0; i--)
    {
        std::string substr(text, i);
        sfText.setString(substr);
        sf::FloatRect bounds = sfText.getLocalBounds();

        if (bounds.size.x <= static_cast<float>(maxWidth))
            return i;
    }

    return 0;
}

void SFMLTextRenderer::DrawText(int x, int y, const char* text, uint32_t color)
{
    if (!text || !m_fontLoaded || !m_pBackBuffer)
        return;

    // Extract RGB from COLORREF (Windows color format: 0x00BBGGRR)
    uint8_t r = static_cast<uint8_t>(color & 0xFF);
    uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
    uint8_t b = static_cast<uint8_t>((color >> 16) & 0xFF);

    sf::Text sfText(m_font, text, m_fontSize);
    sfText.setPosition({static_cast<float>(x), static_cast<float>(y)});
    sfText.setFillColor(sf::Color(r, g, b));

    m_pBackBuffer->draw(sfText);
}

void SFMLTextRenderer::DrawTextCentered(int x1, int x2, int y, const char* text, uint32_t color)
{
    if (!text || !m_fontLoaded || !m_pBackBuffer)
        return;

    // Extract RGB from COLORREF (Windows color format: 0x00BBGGRR)
    uint8_t r = static_cast<uint8_t>(color & 0xFF);
    uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
    uint8_t b = static_cast<uint8_t>((color >> 16) & 0xFF);

    sf::Text sfText(m_font, text, m_fontSize);
    sf::FloatRect bounds = sfText.getLocalBounds();

    // Calculate centered position and round to integer for pixel-perfect rendering
    float centerX = static_cast<float>(x1 + x2) / 2.0f;
    float drawX = centerX - (bounds.size.x / 2.0f) - bounds.position.x;
    int pixelX = static_cast<int>(drawX + 0.5f);  // Round to nearest pixel

    sfText.setPosition({static_cast<float>(pixelX), static_cast<float>(y)});
    sfText.setFillColor(sf::Color(r, g, b));

    m_pBackBuffer->draw(sfText);
}

void SFMLTextRenderer::BeginBatch()
{
    // No-op for SFML - we don't need DC acquisition
}

void SFMLTextRenderer::EndBatch()
{
    // No-op for SFML
}

} // namespace TextLib
