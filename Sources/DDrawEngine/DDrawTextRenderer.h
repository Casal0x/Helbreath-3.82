// DDrawTextRenderer.h: DirectDraw implementation of ITextRenderer
//
// Part of DDrawEngine static library
//////////////////////////////////////////////////////////////////////

#pragma once

#include "ITextRenderer.h"
#include <windows.h>

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
    void DrawTextCentered(int x1, int x2, int y, const char* text, uint32_t color) override;

    // Batching
    void BeginBatch() override;
    void EndBatch() override;

private:
    // Create GDI font with current settings
    void CreateGDIFont();
    void DestroyGDIFont();

    DXC_ddraw* m_pDDraw;
    HFONT m_hFont;           // Client-created font
    HFONT m_hOldFont;        // Previous font to restore
    char m_fontName[64];     // Current font name
    int m_fontSize;
    bool m_batchActive;
    bool m_fontLoaded;
};

} // namespace TextLib
