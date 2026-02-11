// SFMLTextRenderer.cpp: SFML implementation of ITextRenderer
//
// Part of SFMLEngine static library
//////////////////////////////////////////////////////////////////////

#include "SFMLTextRenderer.h"
#include <SFML/Graphics/Text.hpp>
#include <cstring>
#include <string>

namespace hb::shared::text {

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
    , m_fontSize(12)  // Default size to match GDI rendering
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
    // SFML/FreeType renders ~2 points larger than GDI, so compensate
    int adjustedSize = (size > 2) ? (size - 2) : size;
    m_fontSize = static_cast<unsigned int>(adjustedSize);
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

int SFMLTextRenderer::GetLineHeight() const
{
    if (!m_fontLoaded)
        return 0;

    return static_cast<int>(m_font.getLineSpacing(m_fontSize));
}

void SFMLTextRenderer::DrawText(int x, int y, const char* text, const hb::shared::render::Color& color)
{
    if (!text || !m_fontLoaded || !m_pBackBuffer)
        return;

    sf::Text sfText(m_font, text, m_fontSize);
    sfText.setPosition({static_cast<float>(x), static_cast<float>(y)});
    sfText.setFillColor(sf::Color(color.r, color.g, color.b));

    m_pBackBuffer->draw(sfText);
}

void SFMLTextRenderer::DrawTextAligned(int x, int y, int width, int height, const char* text, const hb::shared::render::Color& color,
                                        Align alignment)
{
    if (!text || !m_fontLoaded || !m_pBackBuffer)
        return;

    sf::Text sfText(m_font, text, m_fontSize);
    sf::FloatRect bounds = sfText.getLocalBounds();

    // Extract alignment components
    uint8_t hAlign = alignment & Align::HMask;
    uint8_t vAlign = alignment & Align::VMask;

    // Calculate X position based on horizontal alignment
    float drawX = static_cast<float>(x) - bounds.position.x;
    if (hAlign == Align::HCenter)
        drawX = static_cast<float>(x) + (static_cast<float>(width) - bounds.size.x) / 2.0f - bounds.position.x;
    else if (hAlign == Align::Right)
        drawX = static_cast<float>(x + width) - bounds.size.x - bounds.position.x;

    // Calculate Y position based on vertical alignment
    float drawY = static_cast<float>(y) - bounds.position.y;
    if (vAlign == Align::VCenter)
        drawY = static_cast<float>(y) + (static_cast<float>(height) - bounds.size.y) / 2.0f - bounds.position.y;
    else if (vAlign == Align::Bottom)
        drawY = static_cast<float>(y + height) - bounds.size.y - bounds.position.y;

    // Round to nearest pixel for pixel-perfect rendering
    int pixelX = static_cast<int>(drawX + 0.5f);
    int pixelY = static_cast<int>(drawY + 0.5f);

    sfText.setPosition({static_cast<float>(pixelX), static_cast<float>(pixelY)});
    sfText.setFillColor(sf::Color(color.r, color.g, color.b));

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

} // namespace hb::shared::text
