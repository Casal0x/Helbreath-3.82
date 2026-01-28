// IRenderer.h: Abstract interface for renderer backends
//
// Part of DDrawEngine static library
// This interface allows swapping renderer implementations (DirectDraw, OpenGL, etc.)
//////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>
#include <cstdint>

// Undefine Windows DrawText macro to avoid naming conflict with IRenderer::DrawText
#ifdef DrawText
#undef DrawText
#endif
#include "RenderConstants.h"

// Forward declarations
class ITexture;
class CSprite;

class IRenderer
{
public:
    virtual ~IRenderer() = default;

    // ============== Initialization ==============
    virtual bool Init(HWND hWnd) = 0;
    virtual void Shutdown() = 0;

    // ============== Display Modes ==============
    virtual void SetFullscreen(bool fullscreen) = 0;
    virtual bool IsFullscreen() const = 0;
    virtual void ChangeDisplayMode(HWND hWnd) = 0;

    // ============== Frame Management ==============
    virtual void BeginFrame() = 0;      // ClearBackBuffer
    virtual void EndFrame() = 0;        // Flip/Present
    virtual bool EndFrameCheckLostSurface() = 0;  // Flip and return true if surface lost

    // ============== Primitive Rendering ==============
    virtual void PutPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void DrawShadowBox(int x1, int y1, int x2, int y2, int type = 0) = 0;
    virtual void DrawItemShadowBox(int x1, int y1, int x2, int y2, int type = 0) = 0;

    // ============== Screen Effects ==============
    // Draw a full-screen black overlay with given alpha (0.0 = transparent, 1.0 = opaque)
    // Used for fade transitions between game modes
    virtual void DrawFadeOverlay(float alpha) = 0;

    // ============== Text Rendering ==============
    virtual void BeginTextBatch() = 0;      // GetBackBufferDC
    virtual void EndTextBatch() = 0;        // ReleaseBackBufferDC
    virtual void DrawText(int x, int y, const char* text, uint32_t color) = 0;
    virtual void DrawTextRect(RECT* rect, const char* text, uint32_t color) = 0;

    // ============== Surface/Texture Management ==============
    virtual ITexture* CreateTexture(uint16_t width, uint16_t height) = 0;
    virtual void DestroyTexture(ITexture* texture) = 0;

    // ============== Blitting ==============
    // Note: For transition, we keep direct surface access methods
    // Future implementations may need to adapt these

    // ============== Direct Buffer Access (Legacy Support) ==============
    // These allow direct pixel manipulation for complex effects
    virtual uint16_t* LockBackBuffer(int* pitch) = 0;
    virtual void UnlockBackBuffer() = 0;

    // ============== Clip Area ==============
    virtual void SetClipArea(int x, int y, int w, int h) = 0;
    virtual RECT GetClipArea() const = 0;

    // ============== Pixel Format Info ==============
    virtual int GetPixelFormat() const = 0;  // PIXELFORMAT_RGB565, PIXELFORMAT_RGB555, etc.

    // ============== Screenshot ==============
    virtual bool Screenshot(const char* filename) = 0;

    // ============== Resolution ==============
    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
    virtual int GetWidthMid() const = 0;
    virtual int GetHeightMid() const = 0;
    virtual void ResizeBackBuffer(int width, int height) = 0;  // Resize back buffer for resolution change

    // ============== Pre-Draw Background Surface (PDBGS) ==============
    virtual ITexture* GetBackgroundSurface() = 0;

    // ============== Color Key Support ==============
    virtual uint32_t GetColorKey(ITexture* texture, uint16_t colorKey) = 0;
    virtual uint32_t GetColorKeyRGB(ITexture* texture, uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void SetColorKey(ITexture* texture, uint16_t colorKey) = 0;
    virtual void SetColorKeyRGB(ITexture* texture, uint8_t r, uint8_t g, uint8_t b) = 0;

    // ============== Transparency Tables (Engine Owns) ==============
    // Note: These are for legacy sprite code that does manual alpha blending
    virtual const long (*GetTransTableRB100() const)[64] = 0;
    virtual const long (*GetTransTableG100() const)[64] = 0;
    virtual const long (*GetTransTableRB70() const)[64] = 0;
    virtual const long (*GetTransTableG70() const)[64] = 0;
    virtual const long (*GetTransTableRB50() const)[64] = 0;
    virtual const long (*GetTransTableG50() const)[64] = 0;
    virtual const long (*GetTransTableRB25() const)[64] = 0;
    virtual const long (*GetTransTableG25() const)[64] = 0;
    virtual const long (*GetTransTableRB2() const)[64] = 0;
    virtual const long (*GetTransTableG2() const)[64] = 0;
    virtual const long (*GetTransTableFadeRB() const)[64] = 0;
    virtual const long (*GetTransTableFadeG() const)[64] = 0;

    // ============== Add Tables (for additive blending) ==============
    virtual const int (*GetAddTable31() const)[510] = 0;
    virtual const int (*GetAddTable63() const)[510] = 0;
    virtual const int (*GetAddTransTable31() const)[64] = 0;
    virtual const int (*GetAddTransTable63() const)[64] = 0;

    // ============== Sprite Alpha Degree ==============
    virtual char GetSpriteAlphaDegree() const = 0;
    virtual void SetSpriteAlphaDegree(char degree) = 0;

    // ============== Color Utilities ==============
    virtual void ColorTransferRGB(uint32_t rgb, int* outR, int* outG, int* outB) = 0;

    // ============== Text Measurement ==============
    virtual int GetTextLength(const char* text, int maxWidth) = 0;  // Get character count that fits in width
    virtual int GetTextWidth(const char* text) = 0;  // Get pixel width of text string

    // ============== Surface Operations ==============
    virtual void BltBackBufferFromPDBGS(RECT* srcRect) = 0;  // Copy from PDBGS to back buffer
    virtual void* GetBackBufferNative() = 0;   // Returns native back buffer handle (LPDIRECTDRAWSURFACE7 for DDraw)
    virtual void* GetPDBGSNative() = 0;        // Returns native PDBGS handle

    // ============== Native Access (for legacy sprite code) ==============
    // Returns platform-specific renderer handle for code that can't use the interface yet
    // DirectDraw: returns DXC_ddraw*
    virtual void* GetNativeRenderer() = 0;
};
