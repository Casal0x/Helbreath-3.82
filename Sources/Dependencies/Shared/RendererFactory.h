// RendererFactory.h: Factory functions for creating renderer and window instances
//
// This header provides a way to create renderer/window instances without
// coupling the client to a specific implementation.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IRenderer.h"
#include "IWindow.h"
#include "ISpriteFactory.h"

// Available renderer backends
enum class RendererType
{
    DirectDraw,     // DirectDraw 7 renderer (DDrawEngine)
    SFML,           // SFML 3.0 renderer (SFMLEngine)
    // Future: OpenGL, Direct3D9, Direct3D11, Vulkan, etc.
};

// Creates a renderer instance
// The actual implementation is provided by the linked renderer library
IRenderer* CreateRenderer();

// Destroys a renderer instance created by CreateRenderer
void DestroyRenderer(IRenderer* renderer);

// Creates a window instance appropriate for the current renderer type
// Note: Named CreateGameWindow to avoid conflict with Win32 CreateWindow macro
IWindow* CreateGameWindow();

// Destroys a window instance
void DestroyGameWindow(IWindow* window);

// Creates a sprite factory instance for the current renderer
// The renderer must be initialized first
SpriteLib::ISpriteFactory* CreateSpriteFactory(IRenderer* renderer);

// Destroys a sprite factory instance
void DestroySpriteFactory(SpriteLib::ISpriteFactory* factory);

// Renderer management class - provides static access to the active renderer
class Renderer
{
public:
    // Set the renderer type and create the renderer
    // Returns true on success
    static bool Set(RendererType type);

    // Get the current renderer instance
    static IRenderer* Get();

    // Destroy the current renderer
    static void Destroy();

    // Get native DXC_ddraw* for legacy code (returns nullptr if not DirectDraw)
    // Cast result to DXC_ddraw* when using with CSprite
    static void* GetNative();

    // Get the current renderer type
    static RendererType GetType();

private:
    static IRenderer* s_pRenderer;
    static RendererType s_type;
};

// Window management class - provides static access to the main window
class Window
{
public:
    // Create the window with given parameters
    // Returns true on success
    static bool Create(const WindowParams& params);

    // Get the current window instance
    static IWindow* Get();

    // Destroy the current window
    static void Destroy();

    // Get the window handle (HWND on Windows)
    static HWND GetHandle();

    // Convenience: Check if window is open and active
    static bool IsActive();

    // Request window close (triggers close event, works for both DDraw and SFML)
    static void Close();

    // Resize the window
    static void SetSize(int width, int height, bool center = true);

    // Toggle borderless/bordered window mode
    static void SetBorderless(bool borderless);

private:
    static IWindow* s_pWindow;
};
