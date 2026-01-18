// RendererFactory.cpp: DirectDraw implementation of renderer/window factory
//
// Part of DDrawEngine static library
//////////////////////////////////////////////////////////////////////

#include "RendererFactory.h"
#include "DDrawRenderer.h"
#include "Win32Window.h"

// Static member initialization
IRenderer* Renderer::s_pRenderer = nullptr;
RendererType Renderer::s_type = RendererType::DirectDraw;
IWindow* Window::s_pWindow = nullptr;

// ============== Renderer Factory Functions ==============

IRenderer* CreateRenderer()
{
    return new DDrawRenderer();
}

void DestroyRenderer(IRenderer* renderer)
{
    delete renderer;
}

// ============== Window Factory Functions ==============

IWindow* CreateGameWindow()
{
    // For DirectDraw, we use Win32Window
    // Other renderers might use different window types
    return new Win32Window();
}

void DestroyGameWindow(IWindow* window)
{
    delete window;
}

// ============== Renderer Class Implementation ==============

bool Renderer::Set(RendererType type)
{
    // Destroy existing renderer if any
    Destroy();

    s_type = type;

    switch (type)
    {
    case RendererType::DirectDraw:
        s_pRenderer = new DDrawRenderer();
        return s_pRenderer != nullptr;

    // Future renderer types would be handled here
    default:
        return false;
    }
}

IRenderer* Renderer::Get()
{
    return s_pRenderer;
}

void Renderer::Destroy()
{
    if (s_pRenderer)
    {
        delete s_pRenderer;
        s_pRenderer = nullptr;
    }
}

void* Renderer::GetNative()
{
    if (s_pRenderer)
    {
        return s_pRenderer->GetNativeRenderer();
    }
    return nullptr;
}

RendererType Renderer::GetType()
{
    return s_type;
}

// ============== Window Class Implementation ==============

bool Window::Create(const WindowParams& params)
{
    // Destroy existing window if any
    Destroy();

    // Create appropriate window type based on current renderer
    s_pWindow = CreateGameWindow();
    if (!s_pWindow)
        return false;

    if (!s_pWindow->Create(params))
    {
        delete s_pWindow;
        s_pWindow = nullptr;
        return false;
    }

    return true;
}

IWindow* Window::Get()
{
    return s_pWindow;
}

void Window::Destroy()
{
    if (s_pWindow)
    {
        s_pWindow->Destroy();
        delete s_pWindow;
        s_pWindow = nullptr;
    }
}

HWND Window::GetHandle()
{
    return s_pWindow ? s_pWindow->GetHandle() : nullptr;
}

bool Window::IsActive()
{
    return s_pWindow && s_pWindow->IsOpen() && s_pWindow->IsActive();
}
