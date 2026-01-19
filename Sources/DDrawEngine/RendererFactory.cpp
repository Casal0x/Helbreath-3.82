// RendererFactory.cpp: DirectDraw implementation of renderer/window factory
//
// Part of DDrawEngine static library
//////////////////////////////////////////////////////////////////////

#include "RendererFactory.h"
#include "DDrawRenderer.h"
#include "DDrawSpriteFactory.h"
#include "DDrawTextRenderer.h"
#include "DDrawBitmapFont.h"
#include "Win32Window.h"
#include "Win32Input.h"
#include "IInput.h"

// Static member initialization
IRenderer* Renderer::s_pRenderer = nullptr;
RendererType Renderer::s_type = RendererType::DirectDraw;
IWindow* Window::s_pWindow = nullptr;

// Local statics for text rendering
static TextLib::DDrawTextRenderer* s_pTextRenderer = nullptr;
static TextLib::DDrawBitmapFontFactory* s_pBitmapFontFactory = nullptr;

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

// ============== Sprite Factory Functions ==============

SpriteLib::ISpriteFactory* CreateSpriteFactory(IRenderer* renderer)
{
    if (!renderer)
        return nullptr;

    // Get the native DDrawRenderer to access the DXC_ddraw implementation
    DDrawRenderer* pDDrawRenderer = static_cast<DDrawRenderer*>(renderer);
    // DDraw uses BMP sprites from SPRITES_BMP folder
    return new DDrawSpriteFactory(pDDrawRenderer->GetDDrawImpl(), "SPRITES_BMP");
}

void DestroySpriteFactory(SpriteLib::ISpriteFactory* factory)
{
    delete factory;
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
        {
            DDrawRenderer* pDDrawRenderer = new DDrawRenderer();
            s_pRenderer = pDDrawRenderer;

            if (s_pRenderer)
            {
                // Create text renderer (will be fully initialized after Init() with DC)
                s_pTextRenderer = new TextLib::DDrawTextRenderer(pDDrawRenderer->GetDDrawImpl());
                TextLib::SetTextRenderer(s_pTextRenderer);

                // Create bitmap font factory
                s_pBitmapFontFactory = new TextLib::DDrawBitmapFontFactory();
                TextLib::SetBitmapFontFactory(s_pBitmapFontFactory);
            }

            return s_pRenderer != nullptr;
        }

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
    // Destroy text renderer
    if (s_pTextRenderer)
    {
        TextLib::SetTextRenderer(nullptr);
        delete s_pTextRenderer;
        s_pTextRenderer = nullptr;
    }

    // Destroy bitmap font factory
    if (s_pBitmapFontFactory)
    {
        TextLib::SetBitmapFontFactory(nullptr);
        delete s_pBitmapFontFactory;
        s_pBitmapFontFactory = nullptr;
    }

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

    // Create input system and initialize with window handle
    Input::Create();
    if (Input::Get())
    {
        static_cast<Win32Input*>(Input::Get())->Initialize(s_pWindow->GetHandle());
    }

    return true;
}

IWindow* Window::Get()
{
    return s_pWindow;
}

void Window::Destroy()
{
    // Destroy input system first
    Input::Destroy();

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

void Window::Close()
{
    if (s_pWindow)
    {
        s_pWindow->Close();
    }
}

void Window::SetSize(int width, int height, bool center)
{
    if (s_pWindow)
    {
        s_pWindow->SetSize(width, height, center);
    }
}
