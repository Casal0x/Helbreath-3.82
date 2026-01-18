// RendererFactory.h: Factory functions for creating renderer and window instances
//
// This header provides a way to create renderer/window instances without
// coupling the client to a specific implementation.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IRenderer.h"
#include "IWindow.h"

// Available renderer backends
enum class RendererType
{
    DirectDraw,     // DirectDraw 7 renderer (DDrawEngine)
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

private:
    static IWindow* s_pWindow;
};
