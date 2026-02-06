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

// ============== Basic Types ==============

struct TextMetrics
{
    int width;
    int height;
};

// Text alignment flags (combine horizontal and vertical with bitwise OR)
enum Align : uint8_t
{
    // Horizontal (bits 0-1)
    Left    = 0x00,
    HCenter = 0x01,
    Right   = 0x02,

    // Vertical (bits 2-3)
    Top     = 0x00,
    VCenter = 0x04,
    Bottom  = 0x08,

    // Common combinations
    TopLeft      = Top | Left,
    TopCenter    = Top | HCenter,
    TopRight     = Top | Right,
    MiddleLeft   = VCenter | Left,
    Center       = VCenter | HCenter,
    MiddleRight  = VCenter | Right,
    BottomLeft   = Bottom | Left,
    BottomCenter = Bottom | HCenter,
    BottomRight  = Bottom | Right,

    // Masks for extracting components
    HMask = 0x03,
    VMask = 0x0C
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
    virtual void DrawText(int x, int y, const char* text, uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void DrawTextAligned(int x, int y, int width, int height, const char* text, uint8_t r, uint8_t g, uint8_t b,
                                 Align alignment = Align::TopLeft) = 0;

    // Batching for performance (DDraw needs DC acquisition)
    virtual void BeginBatch() = 0;
    virtual void EndBatch() = 0;
};

// Global accessor - set by RendererFactory during initialization
ITextRenderer* GetTextRenderer();
void SetTextRenderer(ITextRenderer* renderer);

} // namespace TextLib
