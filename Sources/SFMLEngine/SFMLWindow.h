// SFMLWindow.h: Pure SFML window implementing hb::shared::render::IWindow interface
//
// Part of SFMLEngine static library
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IWindow.h"
#include "IInput.h"  // For KeyCode enum
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Keyboard.hpp>

class SFMLWindow : public hb::shared::render::IWindow
{
public:
    SFMLWindow();
    virtual ~SFMLWindow();

    // ============== hb::shared::render::IWindow Implementation ==============

    // Lifecycle
    bool Create(const hb::shared::render::WindowParams& params) override;
    void Destroy() override;
    bool IsOpen() const override;
    void Close() override;

    // Properties
    hb::shared::types::NativeWindowHandle GetHandle() const override;
    int GetWidth() const override;
    int GetHeight() const override;
    bool IsFullscreen() const override;
    bool IsActive() const override;

    // Display
    void SetFullscreen(bool fullscreen) override;
    void SetBorderless(bool borderless) override;
    bool IsBorderless() const override;
    void SetSize(int width, int height, bool center = true) override;
    void Show() override;
    void Hide() override;
    void SetTitle(const char* title) override;

    // Frame Rate
    void SetFramerateLimit(int limit) override;
    int GetFramerateLimit() const override;
    void SetVSyncEnabled(bool enabled) override;
    bool IsVSyncEnabled() const override;

    // Scaling
    void SetFullscreenStretch(bool stretch) override;
    bool IsFullscreenStretch() const override;

    // Cursor
    void SetMouseCursorVisible(bool visible) override;
    void SetMouseCaptureEnabled(bool enabled) override;

    // Dialogs
    void ShowMessageBox(const char* title, const char* message) override;

    // Message Processing
    bool ProcessMessages() override;
    void WaitForMessage() override;

    // Event Handler
    void SetEventHandler(hb::shared::render::IWindowEventHandler* handler) override;
    hb::shared::render::IWindowEventHandler* GetEventHandler() const override;

    // ============== SFML-Specific Access ==============

    sf::RenderWindow* GetRenderWindow() { return &m_renderWindow; }
    const sf::RenderWindow* GetRenderWindow() const { return &m_renderWindow; }

private:
    // Convert SFML key to abstract KeyCode
    static KeyCode SfmlKeyToKeyCode(sf::Keyboard::Key key);

    // Transform window coordinates to logical game coordinates (640x480)
    void TransformMouseCoords(int windowX, int windowY, int& logicalX, int& logicalY) const;

    sf::RenderWindow m_renderWindow;
    hb::shared::types::NativeWindowHandle m_hWnd;  // Native handle for compatibility
    hb::shared::render::IWindowEventHandler* m_pEventHandler;
    int m_width;
    int m_height;
    bool m_fullscreen;
    bool m_bFullscreenStretch;
    bool m_borderless;
    bool m_bMouseCaptureEnabled;
    bool m_bVSync;
    int m_iFpsLimit;
    int m_windowedWidth;
    int m_windowedHeight;
    bool m_active;
    bool m_open;
};
