// SFMLRenderer.h: SFML renderer implementing hb::shared::render::IRenderer interface
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

class SFMLRenderer : public hb::shared::render::IRenderer
{
public:
    SFMLRenderer();
    virtual ~SFMLRenderer();

    // ============== hb::shared::render::IRenderer Implementation ==============

    // Initialization
    bool Init(hb::shared::types::NativeWindowHandle hWnd) override;
    void Shutdown() override;

    // Display Modes
    void SetFullscreen(bool fullscreen) override;
    bool IsFullscreen() const override;
    void ChangeDisplayMode(hb::shared::types::NativeWindowHandle hWnd) override;

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
    void DrawPixel(int x, int y, const hb::shared::render::Color& color) override;
    void DrawLine(int x0, int y0, int x1, int y1, const hb::shared::render::Color& color,
                  hb::shared::render::BlendMode blend = hb::shared::render::BlendMode::Alpha) override;
    void DrawRectFilled(int x, int y, int w, int h, const hb::shared::render::Color& color) override;
    void DrawRectOutline(int x, int y, int w, int h, const hb::shared::render::Color& color,
                         int thickness = 1) override;
    void DrawRoundedRectFilled(int x, int y, int w, int h, int radius,
                               const hb::shared::render::Color& color) override;
    void DrawRoundedRectOutline(int x, int y, int w, int h, int radius,
                                const hb::shared::render::Color& color, int thickness = 1) override;

    // Text Rendering
    void BeginTextBatch() override;
    void EndTextBatch() override;
    void DrawText(int x, int y, const char* text, const hb::shared::render::Color& color) override;
    void DrawTextRect(const hb::shared::geometry::GameRectangle& rect, const char* text, const hb::shared::render::Color& color) override;

    // Surface/Texture Management
    hb::shared::render::ITexture* CreateTexture(uint16_t width, uint16_t height) override;
    void DestroyTexture(hb::shared::render::ITexture* texture) override;

    // Clip Area
    void SetClipArea(int x, int y, int w, int h) override;
    hb::shared::geometry::GameRectangle GetClipArea() const override;

    // Screenshot
    bool Screenshot(const char* filename) override;

    // Resolution
    int GetWidth() const override;
    int GetHeight() const override;
    int GetWidthMid() const override;
    int GetHeightMid() const override;
    void ResizeBackBuffer(int width, int height) override;

    // Ambient Light
    char GetAmbientLightLevel() const override;
    void SetAmbientLightLevel(char level) override;

    // hb::shared::render::Color Utilities
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
    // Create render textures (called when OpenGL context is available)
    bool CreateRenderTextures();

    sf::RenderWindow* m_pRenderWindow;  // Not owned, set by SFMLWindow
    bool m_texturesCreated;
    sf::RenderTexture m_backBuffer;

    // Display properties
    int m_width;
    int m_height;
    bool m_fullscreen;
    bool m_bFullscreenStretch;
    hb::shared::geometry::GameRectangle m_clipArea;

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

    // Ambient light level
    char m_ambient_light_level;

};
