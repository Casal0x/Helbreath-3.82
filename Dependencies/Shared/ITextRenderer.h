// ITextRenderer.h: Abstract interface for system font text rendering
//
// Part of the shared interface layer between client and renderers
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>

// Undefine Windows DrawText macro to avoid naming conflict
#ifdef DrawText
#undef DrawText
#endif

namespace TextLib {

struct TextMetrics
{
    int width;
    int height;
};

class ITextRenderer
{
public:
    virtual ~ITextRenderer() = default;

    // Font loading - client calls these to set up fonts
    // Returns true on success. Engine has default fallback if these fail.
    virtual bool LoadFontFromFile(const char* fontPath) = 0;
    virtual bool LoadFontByName(const char* fontName) = 0;  // System font by name (e.g., "Arial")

    // Font configuration
    virtual void SetFontSize(int size) = 0;

    // Check if a font is loaded (either client-provided or default)
    virtual bool IsFontLoaded() const = 0;

    // Text measurement
    virtual TextMetrics MeasureText(const char* text) const = 0;
    virtual int GetFittingCharCount(const char* text, int maxWidth) const = 0;

    // Drawing
    virtual void DrawText(int x, int y, const char* text, uint32_t color) = 0;
    virtual void DrawTextCentered(int x1, int x2, int y, const char* text, uint32_t color) = 0;

    // Batching for performance (DDraw needs DC acquisition)
    virtual void BeginBatch() = 0;
    virtual void EndBatch() = 0;
};

// Global accessor - set by RendererFactory during initialization
ITextRenderer* GetTextRenderer();
void SetTextRenderer(ITextRenderer* renderer);

} // namespace TextLib
