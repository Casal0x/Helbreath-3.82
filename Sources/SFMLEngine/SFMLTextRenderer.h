// SFMLTextRenderer.h: SFML implementation of ITextRenderer
//
// Part of SFMLEngine static library
//////////////////////////////////////////////////////////////////////

#pragma once

#include "ITextRenderer.h"
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTexture.hpp>

namespace hb::shared::text {

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
    TextMetrics measure_text(const char* text) const override;
    int get_fitting_char_count(const char* text, int maxWidth) const override;
    int get_line_height() const override;

    // Drawing
    void draw_text(int x, int y, const char* text, const hb::shared::render::Color& color) override;
    void draw_text_aligned(int x, int y, int width, int height, const char* text, const hb::shared::render::Color& color,
                         Align alignment = Align::TopLeft) override;

    // Batching (no-op for SFML, no DC acquisition needed)
    void begin_batch() override;
    void end_batch() override;

    // Allow updating the back buffer pointer if needed
    void SetBackBuffer(sf::RenderTexture* backBuffer) { m_back_buffer = backBuffer; }

private:
    // Load default system font as fallback
    bool LoadDefaultFont();

    sf::RenderTexture* m_back_buffer;
    sf::Font m_font;              // Owned font (client-loaded or default)
    bool m_fontLoaded;
    unsigned int m_font_size;
};

} // namespace hb::shared::text
