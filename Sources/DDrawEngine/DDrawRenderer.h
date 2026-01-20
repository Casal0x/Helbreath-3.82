// DDrawRenderer.h: DirectDraw renderer implementing IRenderer interface
//
// Part of DDrawEngine static library
// Wraps the existing DXC_ddraw class to provide the IRenderer interface
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IRenderer.h"
#include "DXC_ddraw.h"
#include "DDrawTexture.h"

class DDrawRenderer : public IRenderer
{
public:
    DDrawRenderer();
    virtual ~DDrawRenderer();

    // ============== IRenderer Implementation ==============

    // Initialization
    virtual bool Init(HWND hWnd) override;
    virtual void Shutdown() override;

    // Display Modes
    virtual void SetFullscreen(bool fullscreen) override;
    virtual bool IsFullscreen() const override;
    virtual void ChangeDisplayMode(HWND hWnd) override;

    // Frame Management
    virtual void BeginFrame() override;
    virtual void EndFrame() override;
    virtual bool EndFrameCheckLostSurface() override;

    // Primitive Rendering
    virtual void PutPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) override;
    virtual void DrawShadowBox(int x1, int y1, int x2, int y2, int type = 0) override;
    virtual void DrawItemShadowBox(int x1, int y1, int x2, int y2, int type = 0) override;

    // Text Rendering
    virtual void BeginTextBatch() override;
    virtual void EndTextBatch() override;
    virtual void DrawText(int x, int y, const char* text, uint32_t color) override;
    virtual void DrawTextRect(RECT* rect, const char* text, uint32_t color) override;

    // Surface/Texture Management
    virtual ITexture* CreateTexture(uint16_t width, uint16_t height) override;
    virtual void DestroyTexture(ITexture* texture) override;

    // Direct Buffer Access
    virtual uint16_t* LockBackBuffer(int* pitch) override;
    virtual void UnlockBackBuffer() override;

    // Clip Area
    virtual void SetClipArea(int x, int y, int w, int h) override;
    virtual RECT GetClipArea() const override;

    // Pixel Format Info
    virtual int GetPixelFormat() const override;

    // Screenshot
    virtual bool Screenshot(const char* filename) override;

    // Resolution
    virtual int GetWidth() const override;
    virtual int GetHeight() const override;
    virtual int GetWidthMid() const override;
    virtual int GetHeightMid() const override;
    virtual void ResizeBackBuffer(int width, int height) override;

    // Pre-Draw Background Surface
    virtual ITexture* GetBackgroundSurface() override;

    // Color Key Support
    virtual uint32_t GetColorKey(ITexture* texture, uint16_t colorKey) override;
    virtual uint32_t GetColorKeyRGB(ITexture* texture, uint8_t r, uint8_t g, uint8_t b) override;
    virtual void SetColorKey(ITexture* texture, uint16_t colorKey) override;
    virtual void SetColorKeyRGB(ITexture* texture, uint8_t r, uint8_t g, uint8_t b) override;

    // Transparency Tables
    virtual const long (*GetTransTableRB100() const)[64] override;
    virtual const long (*GetTransTableG100() const)[64] override;
    virtual const long (*GetTransTableRB70() const)[64] override;
    virtual const long (*GetTransTableG70() const)[64] override;
    virtual const long (*GetTransTableRB50() const)[64] override;
    virtual const long (*GetTransTableG50() const)[64] override;
    virtual const long (*GetTransTableRB25() const)[64] override;
    virtual const long (*GetTransTableG25() const)[64] override;
    virtual const long (*GetTransTableRB2() const)[64] override;
    virtual const long (*GetTransTableG2() const)[64] override;
    virtual const long (*GetTransTableFadeRB() const)[64] override;
    virtual const long (*GetTransTableFadeG() const)[64] override;

    // Add Tables
    virtual const int (*GetAddTable31() const)[510] override;
    virtual const int (*GetAddTable63() const)[510] override;
    virtual const int (*GetAddTransTable31() const)[64] override;
    virtual const int (*GetAddTransTable63() const)[64] override;

    // Sprite Alpha
    virtual char GetSpriteAlphaDegree() const override;
    virtual void SetSpriteAlphaDegree(char degree) override;

    // Color Utilities
    virtual void ColorTransferRGB(uint32_t rgb, int* outR, int* outG, int* outB) override;

    // Text Measurement
    virtual int GetTextLength(const char* text, int maxWidth) override;
    virtual int GetTextWidth(const char* text) override;

    // Surface Operations
    virtual void BltBackBufferFromPDBGS(RECT* srcRect) override;
    virtual void* GetBackBufferNative() override;
    virtual void* GetPDBGSNative() override;

    // Native Access
    virtual void* GetNativeRenderer() override;

    // ============== DirectDraw-Specific Access ==============
    // For legacy code that still needs direct DXC_ddraw access during transition
    DXC_ddraw* GetDDrawImpl() { return &m_ddraw; }
    const DXC_ddraw* GetDDrawImpl() const { return &m_ddraw; }

private:
    DXC_ddraw m_ddraw;
    DDrawTexture* m_pdbgsWrapper;  // Wrapper for PDBGS surface
    bool m_backBufferLocked;
};
