// DDrawTextRenderer.h: DirectDraw implementation of ITextRenderer
//
// Part of DDrawEngine static library
//////////////////////////////////////////////////////////////////////

#pragma once

#include "ITextRenderer.h"
#include <windows.h>
#include <unordered_map>

// Undefine Windows DrawText macro to avoid naming conflict
#ifdef DrawText
#undef DrawText
#endif

class DXC_ddraw;

namespace TextLib {

class DDrawTextRenderer : public ITextRenderer
{
public:
    explicit DDrawTextRenderer(DXC_ddraw* ddraw);
    ~DDrawTextRenderer() override;

    // Font loading - client calls these to set up fonts
    bool LoadFontFromFile(const char* fontPath) override;
    bool LoadFontByName(const char* fontName) override;

    // Font configuration
    void SetFontSize(int size) override;
    bool IsFontLoaded() const override;

    // Text measurement
    TextMetrics MeasureText(const char* text) const override;
    int GetFittingCharCount(const char* text, int maxWidth) const override;

    // Drawing
    void DrawText(int x, int y, const char* text, uint32_t color) override;
    void DrawTextAligned(int x, int y, int width, int height, const char* text, uint32_t color,
                         Align alignment = Align::TopLeft) override;

    // Batching
    void BeginBatch() override;
    void EndBatch() override;

private:
    // Create GDI font for a specific size (or use cached)
    HFONT GetOrCreateFont(int size);
    void ClearFontCache();

    DXC_ddraw* m_pDDraw;
    HFONT m_hOldFont;                              // Previous font to restore
    char m_fontName[64];                           // Current font name
    int m_fontSize;                                // Current active font size
    int m_batchDepth;                              // Nesting depth for batch calls (0 = no batch)
    bool m_fontLoaded;

    // Font cache: size -> HFONT (lazy loaded, cached for reuse)
    std::unordered_map<int, HFONT> m_fontCache;
};

} // namespace TextLib
