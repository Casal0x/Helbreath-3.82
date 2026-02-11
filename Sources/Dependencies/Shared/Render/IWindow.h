// IWindow.h: Abstract interface for window management
//
// This interface allows different renderers to create windows appropriate
// for their backend (DirectDraw, OpenGL, Vulkan, etc.)
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NativeTypes.h"
#include <cstdint>

// Forward declare KeyCode enum
enum class KeyCode : int;

namespace hb::shared::render {

// Forward declaration
class IWindowEventHandler;

// Window creation parameters
struct WindowParams
{
    const char* title;
    int width;
    int height;
    bool fullscreen;
    bool borderless;
    bool centered;
    bool mouseCaptureEnabled;
    hb::shared::types::NativeInstance nativeInstance;
    int iconResourceId;  // 0 for default
};

// Abstract window interface
class IWindow
{
public:
    virtual ~IWindow() = default;

    // ============== Lifecycle ==============
    virtual bool Create(const WindowParams& params) = 0;
    virtual void Destroy() = 0;
    virtual bool IsOpen() const = 0;
    virtual void Close() = 0;  // Request window close (triggers close event)

    // ============== Properties ==============
    virtual hb::shared::types::NativeWindowHandle GetHandle() const = 0;
    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
    virtual bool IsFullscreen() const = 0;
    virtual bool IsActive() const = 0;

    // ============== Display ==============
    virtual void SetFullscreen(bool fullscreen) = 0;
    virtual void SetBorderless(bool borderless) = 0;
    virtual bool IsBorderless() const = 0;
    virtual void SetSize(int width, int height, bool center = true) = 0;  // Resize window
    virtual void Show() = 0;
    virtual void Hide() = 0;
    virtual void SetTitle(const char* title) = 0;

    // ============== Frame Rate ==============
    virtual void SetFramerateLimit(int limit) = 0;  // 0 = unlimited
    virtual int GetFramerateLimit() const = 0;
    virtual void SetVSyncEnabled(bool enabled) = 0;
    virtual bool IsVSyncEnabled() const = 0;

    // ============== Scaling ==============
    virtual void SetFullscreenStretch(bool stretch) = 0;
    virtual bool IsFullscreenStretch() const = 0;

    // ============== Cursor ==============
    virtual void SetMouseCursorVisible(bool visible) = 0;
    virtual void SetMouseCaptureEnabled(bool enabled) = 0;

    // ============== Dialogs ==============
    virtual void ShowMessageBox(const char* title, const char* message) = 0;

    // ============== Message Processing ==============
    // Process pending messages, returns false if WM_QUIT received
    virtual bool ProcessMessages() = 0;

    // Wait for a message (used when inactive)
    virtual void WaitForMessage() = 0;

    // ============== Event Handler ==============
    virtual void SetEventHandler(IWindowEventHandler* handler) = 0;
    virtual IWindowEventHandler* GetEventHandler() const = 0;
};

// Window event callback interface
// Implement this to receive window events
class IWindowEventHandler
{
public:
    virtual ~IWindowEventHandler() = default;

    // ============== Window Events ==============
    virtual void OnClose() = 0;
    virtual void OnDestroy() = 0;
    virtual void OnActivate(bool active) = 0;
    virtual void OnResize(int width, int height) = 0;

    // ============== Input Events ==============
    virtual void OnKeyDown(KeyCode keyCode) = 0;
    virtual void OnKeyUp(KeyCode keyCode) = 0;
    virtual void OnChar(char character) = 0;
    virtual void OnMouseMove(int x, int y) = 0;
    virtual void OnMouseButtonDown(int button, int x, int y) = 0;
    virtual void OnMouseButtonUp(int button, int x, int y) = 0;
    virtual void OnMouseWheel(int delta, int x, int y) = 0;

    // ============== Custom Messages ==============
    // For game-specific messages (sockets, timers, etc.)
    // Return true if handled, false to pass to DefWindowProc
    virtual bool OnCustomMessage(uint32_t message, uintptr_t wParam, intptr_t lParam) = 0;

    // ============== Text Input ==============
    // For IME and text composition
    virtual bool OnTextInput(hb::shared::types::NativeWindowHandle hWnd, uint32_t message, uintptr_t wParam, intptr_t lParam) = 0;
};

// Mouse button constants: see hb::shared::input::MouseButton in IInput.h

} // namespace hb::shared::render
