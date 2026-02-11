// IRenderer.h: Abstract interface for renderer backends
//
// Part of DDrawEngine static library
// This interface allows swapping renderer implementations (DirectDraw, OpenGL, etc.)
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NativeTypes.h"
#include "GameGeometry.h"
#include "PrimitiveTypes.h"
#include <cstdint>

// Undefine Windows DrawText macro to avoid naming conflict with IRenderer::DrawText
#ifdef DrawText
#undef DrawText
#endif
#include "RenderConstants.h"

namespace hb::shared::render {

// Forward declarations
class ITexture;
class IRenderer
{
public:
    virtual ~IRenderer() = default;

    // ============== Initialization ==============
    virtual bool Init(hb::shared::types::NativeWindowHandle hWnd) = 0;
    virtual void Shutdown() = 0;

    // ============== Display Modes ==============
    virtual void SetFullscreen(bool fullscreen) = 0;
    virtual bool IsFullscreen() const = 0;
    virtual void ChangeDisplayMode(hb::shared::types::NativeWindowHandle hWnd) = 0;

    // ============== Scaling ==============
    virtual void SetFullscreenStretch(bool stretch) = 0;
    virtual bool IsFullscreenStretch() const = 0;

    // ============== Frame Management ==============
    virtual void BeginFrame() = 0;      // ClearBackBuffer
    virtual void EndFrame() = 0;        // Flip/Present
    virtual bool EndFrameCheckLostSurface() = 0;  // Flip and return true if surface lost
    virtual bool WasFramePresented() const { return true; }  // False if frame was skipped by limiter

    // ============== Frame Metrics ==============
    // Tracked internally by the engine at the point of actual present
    virtual uint32_t GetFPS() const { return 0; }
    virtual double GetDeltaTime() const { return 0.0; }     // Seconds between presented frames
    virtual double GetDeltaTimeMS() const { return 0.0; }   // Milliseconds between presented frames

    // ============== Primitive Rendering ==============
    // All rect-based primitives use (x, y, w, h) format, consistent with hb::shared::geometry::GameRectangle.
    // DrawLine uses (x0, y0, x1, y1) endpoint format.

    virtual void DrawPixel(int x, int y, const Color& color) = 0;
    virtual void DrawLine(int x0, int y0, int x1, int y1, const Color& color,
                          BlendMode blend = BlendMode::Alpha) = 0;
    virtual void DrawRectFilled(int x, int y, int w, int h, const Color& color) = 0;
    virtual void DrawRectOutline(int x, int y, int w, int h, const Color& color,
                                 int thickness = 1) = 0;
    virtual void DrawRoundedRectFilled(int x, int y, int w, int h, int radius,
                                       const Color& color) = 0;
    virtual void DrawRoundedRectOutline(int x, int y, int w, int h, int radius,
                                        const Color& color, int thickness = 1) = 0;

    // ============== Text Rendering ==============
    virtual void BeginTextBatch() = 0;      // GetBackBufferDC
    virtual void EndTextBatch() = 0;        // ReleaseBackBufferDC
    virtual void DrawText(int x, int y, const char* text, const Color& color) = 0;
    virtual void DrawTextRect(const hb::shared::geometry::GameRectangle& rect, const char* text, const Color& color) = 0;

    // ============== Surface/Texture Management ==============
    virtual ITexture* CreateTexture(uint16_t width, uint16_t height) = 0;
    virtual void DestroyTexture(ITexture* texture) = 0;

    // ============== Blitting ==============
    // Note: For transition, we keep direct surface access methods
    // Future implementations may need to adapt these

    // ============== Clip Area ==============
    virtual void SetClipArea(int x, int y, int w, int h) = 0;
    virtual hb::shared::geometry::GameRectangle GetClipArea() const = 0;

    // ============== Screenshot ==============
    virtual bool Screenshot(const char* filename) = 0;

    // ============== Resolution ==============
    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
    virtual int GetWidthMid() const = 0;
    virtual int GetHeightMid() const = 0;
    virtual void ResizeBackBuffer(int width, int height) = 0;  // Resize back buffer for resolution change

    // ============== Sprite Alpha Degree ==============
    virtual char GetSpriteAlphaDegree() const = 0;
    virtual void SetSpriteAlphaDegree(char degree) = 0;

    // ============== Color Utilities ==============
    virtual void ColorTransferRGB(uint32_t rgb, int* outR, int* outG, int* outB) = 0;

    // ============== Text Measurement ==============
    virtual int GetTextLength(const char* text, int maxWidth) = 0;  // Get character count that fits in width
    virtual int GetTextWidth(const char* text) = 0;  // Get pixel width of text string

    // ============== Surface Operations ==============
    virtual void* GetBackBufferNative() = 0;   // Returns native back buffer handle (LPDIRECTDRAWSURFACE7 for DDraw)

    // ============== Native Access (for legacy sprite code) ==============
    // Returns platform-specific renderer handle for code that can't use the interface yet
    // DirectDraw: returns DXC_ddraw*
    virtual void* GetNativeRenderer() = 0;
};

} // namespace hb::shared::render
