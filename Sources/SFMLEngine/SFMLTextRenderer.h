// SFMLTextRenderer.h: SFML implementation of ITextRenderer
//
// Part of SFMLEngine static library
//////////////////////////////////////////////////////////////////////

#pragma once

#include "ITextRenderer.h"
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTexture.hpp>

namespace TextLib {

class SFMLTextRenderer : public ITextRenderer
{
public:
    SFMLTextRenderer(sf::RenderTexture* backBuffer);
    ~SFMLTextRenderer() override = default;

    // Font loading - client calls these to set up fonts
    bool LoadFontFromFile(const char* fontPath) override;
    bool LoadFontByName(const char* fontName) override;

    // Font configuration
    void SetFontSize(int size) override;
    bool IsFontLoaded() const override;

    // Text measurement
    TextMetrics MeasureText(const char* text) const override;
    int GetFittingCharCount(const char* text, int maxWidth) const override;
    int GetLineHeight() const override;

    // Drawing
    void DrawText(int x, int y, const char* text, const ::Color& color) override;
    void DrawTextAligned(int x, int y, int width, int height, const char* text, const ::Color& color,
                         Align alignment = Align::TopLeft) override;

    // Batching (no-op for SFML, no DC acquisition needed)
    void BeginBatch() override;
    void EndBatch() override;

    // Allow updating the back buffer pointer if needed
    void SetBackBuffer(sf::RenderTexture* backBuffer) { m_pBackBuffer = backBuffer; }

private:
    // Load default system font as fallback
    bool LoadDefaultFont();

    sf::RenderTexture* m_pBackBuffer;
    sf::Font m_font;              // Owned font (client-loaded or default)
    bool m_fontLoaded;
    unsigned int m_fontSize;
};

} // namespace TextLib
