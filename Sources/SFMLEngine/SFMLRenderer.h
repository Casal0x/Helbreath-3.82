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
#include <vector>
#include <string>
#include <chrono>

class SFMLRenderer : public IRenderer
{
public:
    SFMLRenderer();
    virtual ~SFMLRenderer();

    // ============== IRenderer Implementation ==============

    // Initialization
    bool Init(NativeWindowHandle hWnd) override;
    void Shutdown() override;

    // Display Modes
    void SetFullscreen(bool fullscreen) override;
    bool IsFullscreen() const override;
    void ChangeDisplayMode(NativeWindowHandle hWnd) override;

    // Scaling
    void SetFullscreenStretch(bool stretch) override;
    bool IsFullscreenStretch() const override;

    // Frame Management
    void BeginFrame() override;
    void EndFrame() override;
    bool EndFrameCheckLostSurface() override;
    bool WasFramePresented() const override;

    // Frame Metrics
    uint32_t GetFPS() const override;
    double GetDeltaTime() const override;
    double GetDeltaTimeMS() const override;

    // Primitive Rendering
    void DrawPixel(int x, int y, const Color& color) override;
    void DrawLine(int x0, int y0, int x1, int y1, const Color& color,
                  BlendMode blend = BlendMode::Alpha) override;
    void DrawRectFilled(int x, int y, int w, int h, const Color& color) override;
    void DrawRectOutline(int x, int y, int w, int h, const Color& color,
                         int thickness = 1) override;
    void DrawRoundedRectFilled(int x, int y, int w, int h, int radius,
                               const Color& color) override;
    void DrawRoundedRectOutline(int x, int y, int w, int h, int radius,
                                const Color& color, int thickness = 1) override;

    // Text Rendering
    void BeginTextBatch() override;
    void EndTextBatch() override;
    void DrawText(int x, int y, const char* text, const Color& color) override;
    void DrawTextRect(const GameRectangle& rect, const char* text, const Color& color) override;

    // Surface/Texture Management
    ITexture* CreateTexture(uint16_t width, uint16_t height) override;
    void DestroyTexture(ITexture* texture) override;

    // Direct Buffer Access (emulated via sf::Image)
    uint16_t* LockBackBuffer(int* pitch) override;
    void UnlockBackBuffer() override;

    // Clip Area
    void SetClipArea(int x, int y, int w, int h) override;
    GameRectangle GetClipArea() const override;

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
    void* GetBackBufferNative() override;

    // Native Access
    void* GetNativeRenderer() override;

    // ============== SFML-Specific Access ==============

    // Get the back buffer render texture for sprite drawing
    sf::RenderTexture* GetBackBuffer() { return &m_backBuffer; }

    // Set the window to render to (called from SFMLWindow)
    // This also creates the render textures since they need an OpenGL context
    void SetRenderWindow(sf::RenderWindow* window);

    // Engine-owned frame rate control (called from SFMLWindow)
    void SetFramerateLimit(int limit);
    int GetFramerateLimit() const;
    void SetVSyncMode(bool enabled);

private:
    // Initialize transparency tables with dummy values
    void InitDummyTables();

    // Create render textures (called when OpenGL context is available)
    bool CreateRenderTextures();

    sf::RenderWindow* m_pRenderWindow;  // Not owned, set by SFMLWindow
    bool m_texturesCreated;
    sf::RenderTexture m_backBuffer;

    // For LockBackBuffer emulation
    sf::Image m_lockedImage;
    std::vector<uint16_t> m_lockedBuffer;
    bool m_backBufferLocked;

    // Display properties
    int m_width;
    int m_height;
    bool m_fullscreen;
    bool m_bFullscreenStretch;
    GameRectangle m_clipArea;

    // Engine-owned frame timing
    int m_iFpsLimit;
    bool m_bVSync;
    bool m_bSkipFrame;
    std::chrono::steady_clock::time_point m_lastPresentTime;
    std::chrono::steady_clock::duration m_targetFrameDuration;

    // Frame metrics (tracked at actual present)
    uint32_t m_fps;
    uint32_t m_framesThisSecond;
    double m_deltaTime;
    double m_fpsAccumulator;
    std::chrono::steady_clock::time_point m_lastPresentedFrameTime;

    // Sprite alpha degree
    char m_spriteAlphaDegree;

    // Dummy transparency tables (SFML uses GPU blending)
    static long s_dummyTransTable[64][64];
    static int s_dummyAddTable[64][510];
    static int s_dummyAddTransTable[510][64];
};
