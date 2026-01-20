// SFMLRenderer.h: SFML renderer implementing IRenderer interface
//
// Part of SFMLEngine static library
// Uses sf::RenderTexture as back buffer and renders to sf::RenderWindow
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IRenderer.h"
#include "SFMLTexture.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <vector>
#include <string>

class SFMLRenderer : public IRenderer
{
public:
    SFMLRenderer();
    virtual ~SFMLRenderer();

    // ============== IRenderer Implementation ==============

    // Initialization
    bool Init(HWND hWnd) override;
    void Shutdown() override;

    // Display Modes
    void SetFullscreen(bool fullscreen) override;
    bool IsFullscreen() const override;
    void ChangeDisplayMode(HWND hWnd) override;

    // Frame Management
    void BeginFrame() override;
    void EndFrame() override;
    bool EndFrameCheckLostSurface() override;

    // Primitive Rendering
    void PutPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) override;
    void DrawShadowBox(int x1, int y1, int x2, int y2, int type = 0) override;
    void DrawItemShadowBox(int x1, int y1, int x2, int y2, int type = 0) override;

    // Text Rendering
    void BeginTextBatch() override;
    void EndTextBatch() override;
    void DrawText(int x, int y, const char* text, uint32_t color) override;
    void DrawTextRect(RECT* rect, const char* text, uint32_t color) override;

    // Surface/Texture Management
    ITexture* CreateTexture(uint16_t width, uint16_t height) override;
    void DestroyTexture(ITexture* texture) override;

    // Direct Buffer Access (emulated via sf::Image)
    uint16_t* LockBackBuffer(int* pitch) override;
    void UnlockBackBuffer() override;

    // Clip Area
    void SetClipArea(int x, int y, int w, int h) override;
    RECT GetClipArea() const override;

    // Pixel Format Info
    int GetPixelFormat() const override;

    // Screenshot
    bool Screenshot(const char* filename) override;

    // Resolution
    int GetWidth() const override;
    int GetHeight() const override;
    int GetWidthMid() const override;
    int GetHeightMid() const override;
    void ResizeBackBuffer(int width, int height) override;

    // Pre-Draw Background Surface
    ITexture* GetBackgroundSurface() override;

    // Color Key Support
    uint32_t GetColorKey(ITexture* texture, uint16_t colorKey) override;
    uint32_t GetColorKeyRGB(ITexture* texture, uint8_t r, uint8_t g, uint8_t b) override;
    void SetColorKey(ITexture* texture, uint16_t colorKey) override;
    void SetColorKeyRGB(ITexture* texture, uint8_t r, uint8_t g, uint8_t b) override;

    // Transparency Tables (dummy - SFML uses GPU blending)
    const long (*GetTransTableRB100() const)[64] override;
    const long (*GetTransTableG100() const)[64] override;
    const long (*GetTransTableRB70() const)[64] override;
    const long (*GetTransTableG70() const)[64] override;
    const long (*GetTransTableRB50() const)[64] override;
    const long (*GetTransTableG50() const)[64] override;
    const long (*GetTransTableRB25() const)[64] override;
    const long (*GetTransTableG25() const)[64] override;
    const long (*GetTransTableRB2() const)[64] override;
    const long (*GetTransTableG2() const)[64] override;
    const long (*GetTransTableFadeRB() const)[64] override;
    const long (*GetTransTableFadeG() const)[64] override;

    // Add Tables (dummy)
    const int (*GetAddTable31() const)[510] override;
    const int (*GetAddTable63() const)[510] override;
    const int (*GetAddTransTable31() const)[64] override;
    const int (*GetAddTransTable63() const)[64] override;

    // Sprite Alpha
    char GetSpriteAlphaDegree() const override;
    void SetSpriteAlphaDegree(char degree) override;

    // Color Utilities
    void ColorTransferRGB(uint32_t rgb, int* outR, int* outG, int* outB) override;

    // Text Measurement
    int GetTextLength(const char* text, int maxWidth) override;
    int GetTextWidth(const char* text) override;

    // Surface Operations
    void BltBackBufferFromPDBGS(RECT* srcRect) override;
    void* GetBackBufferNative() override;
    void* GetPDBGSNative() override;

    // Native Access
    void* GetNativeRenderer() override;

    // ============== SFML-Specific Access ==============

    // Get the back buffer render texture for sprite drawing
    sf::RenderTexture* GetBackBuffer() { return &m_backBuffer; }

    // Get the font for text rendering
    sf::Font* GetFont() { return m_fontLoaded ? &m_font : nullptr; }

    // Set the window to render to (called from SFMLWindow)
    // This also creates the render textures since they need an OpenGL context
    void SetRenderWindow(sf::RenderWindow* window);

private:
    // Initialize transparency tables with dummy values
    void InitDummyTables();

    // Load the font for text rendering
    bool LoadFont();

    // Create render textures (called when OpenGL context is available)
    bool CreateRenderTextures();

    sf::RenderWindow* m_pRenderWindow;  // Not owned, set by SFMLWindow
    bool m_texturesCreated;
    sf::RenderTexture m_backBuffer;
    sf::RenderTexture m_pdbgs;  // Pre-Draw Background Surface
    SFMLTexture* m_pdbgsWrapper;

    sf::Font m_font;
    bool m_fontLoaded;

    // For LockBackBuffer emulation
    sf::Image m_lockedImage;
    std::vector<uint16_t> m_lockedBuffer;
    bool m_backBufferLocked;

    // Display properties
    int m_width;
    int m_height;
    bool m_fullscreen;
    RECT m_clipArea;

    // Sprite alpha degree
    char m_spriteAlphaDegree;

    // Dummy transparency tables (SFML uses GPU blending)
    static long s_dummyTransTable[64][64];
    static int s_dummyAddTable[64][510];
    static int s_dummyAddTransTable[510][64];
};
