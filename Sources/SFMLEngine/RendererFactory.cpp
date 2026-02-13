// RendererFactory.cpp: SFML implementation of renderer factory functions
//
// Part of SFMLEngine static library
// Provides SFML-specific implementations of the factory functions
//////////////////////////////////////////////////////////////////////

#include "RendererFactory.h"
#include "ISpriteFactory.h"
#include "SFMLRenderer.h"
#include "SFMLWindow.h"
#include "SFMLSpriteFactory.h"
#include "SFMLTextRenderer.h"
#include "SFMLBitmapFont.h"
#include "SFMLInput.h"
#include "IInput.h"


namespace hb::shared::render {

// Static member initialization (matches RendererFactory.h class declarations)
IRenderer* Renderer::s_pRenderer = nullptr;
RendererType Renderer::s_type = RendererType::SFML;
IWindow* Window::s_pWindow = nullptr;

// Local static for sprite factory
static SFMLSpriteFactory* s_pSpriteFactory = nullptr;

// Local statics for text rendering
static hb::shared::text::SFMLTextRenderer* s_pTextRenderer = nullptr;
static hb::shared::text::SFMLBitmapFontFactory* s_pBitmapFontFactory = nullptr;


// Factory functions implementation
IRenderer* CreateRenderer()
{
    return new SFMLRenderer();
}

void DestroyRenderer(IRenderer* renderer)
{
    delete renderer;
}

IWindow* CreateGameWindow()
{
    return new SFMLWindow();
}

void DestroyGameWindow(IWindow* window)
{
    delete window;
}

hb::shared::sprite::ISpriteFactory* CreateSpriteFactory(IRenderer* renderer)
{
    if (!renderer)
        return nullptr;

    // Create SFML sprite factory with the renderer - uses PNG sprites
    SFMLRenderer* pSFMLRenderer = static_cast<SFMLRenderer*>(renderer);
    SFMLSpriteFactory* factory = new SFMLSpriteFactory(pSFMLRenderer);
    factory->SetSpritePath("sprites_png");
    return factory;
}

void DestroySpriteFactory(hb::shared::sprite::ISpriteFactory* factory)
{
    delete factory;
}

// Renderer class static methods
bool Renderer::Set(RendererType type)
{
    // Clean up existing renderer
    if (s_pRenderer)
    {
        Destroy();
    }

    s_type = type;

    if (type == RendererType::SFML)
    {
        s_pRenderer = CreateRenderer();
        if (s_pRenderer)
        {
            SFMLRenderer* sfmlRenderer = static_cast<SFMLRenderer*>(s_pRenderer);

            // Create and set sprite factory - SFML uses PNG sprites
            s_pSpriteFactory = new SFMLSpriteFactory(sfmlRenderer);
            s_pSpriteFactory->SetSpritePath("sprites_png");
            hb::shared::sprite::Sprites::SetFactory(s_pSpriteFactory);

            // Create bitmap font factory
            s_pBitmapFontFactory = new hb::shared::text::SFMLBitmapFontFactory();
            hb::shared::text::SetBitmapFontFactory(s_pBitmapFontFactory);

            // If window already exists (created before renderer), link them now
            IWindow* pWindow = Window::get();
            if (pWindow)
            {
                SFMLWindow* sfmlWindow = static_cast<SFMLWindow*>(pWindow);
                sfmlRenderer->SetRenderWindow(sfmlWindow->GetRenderWindow());

                // Create text renderer with back buffer (font loaded internally with fallback)
                s_pTextRenderer = new hb::shared::text::SFMLTextRenderer(sfmlRenderer->GetBackBuffer());
                hb::shared::text::SetTextRenderer(s_pTextRenderer);
            }
        }
        return s_pRenderer != nullptr;
    }

    return false;
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
        hb::shared::text::SetTextRenderer(nullptr);
        delete s_pTextRenderer;
        s_pTextRenderer = nullptr;
    }

    // Destroy bitmap font factory
    if (s_pBitmapFontFactory)
    {
        hb::shared::text::SetBitmapFontFactory(nullptr);
        delete s_pBitmapFontFactory;
        s_pBitmapFontFactory = nullptr;
    }

    // Destroy sprite factory
    if (s_pSpriteFactory)
    {
        hb::shared::sprite::Sprites::SetFactory(nullptr);
        delete s_pSpriteFactory;
        s_pSpriteFactory = nullptr;
    }

    if (s_pRenderer)
    {
        DestroyRenderer(s_pRenderer);
        s_pRenderer = nullptr;
    }
}

void* Renderer::GetNative()
{
    return s_pRenderer;
}

RendererType Renderer::GetType()
{
    return s_type;
}

// Window class static methods
IWindow* Window::create()
{
    if (s_pWindow)
    {
        destroy();
    }

    s_pWindow = CreateGameWindow();
    // Returns the allocated-but-not-realized window.
    // Caller configures via set_title/set_size/etc., then calls realize().
    return s_pWindow;
}

bool Window::realize()
{
    if (!s_pWindow)
        return false;

    // Create OS window from staged params
    if (!s_pWindow->realize())
    {
        delete s_pWindow;
        s_pWindow = nullptr;
        return false;
    }

    // Create input system (needs realized OS window)
    hb::shared::input::create();
    if (hb::shared::input::get())
    {
        SFMLInput* pInput = static_cast<SFMLInput*>(hb::shared::input::get());
        pInput->Initialize(s_pWindow->get_handle());
        pInput->SetRenderWindow(static_cast<SFMLWindow*>(s_pWindow)->GetRenderWindow());
    }

    // Link the SFML window's render window to the renderer (if renderer exists already)
    IRenderer* pRenderer = Renderer::Get();
    if (pRenderer)
    {
        SFMLWindow* sfmlWindow = static_cast<SFMLWindow*>(s_pWindow);
        SFMLRenderer* sfmlRenderer = static_cast<SFMLRenderer*>(pRenderer);
        sfmlRenderer->SetRenderWindow(sfmlWindow->GetRenderWindow());

        // Create text renderer now that we have back buffer
        if (!s_pTextRenderer)
        {
            s_pTextRenderer = new hb::shared::text::SFMLTextRenderer(sfmlRenderer->GetBackBuffer());
            hb::shared::text::SetTextRenderer(s_pTextRenderer);
        }
    }

    return true;
}

IWindow* Window::get()
{
    return s_pWindow;
}

void Window::destroy()
{
    // Destroy input system first
    hb::shared::input::destroy();

    if (s_pWindow)
    {
        // Unlink from renderer
        IRenderer* pRenderer = Renderer::Get();
        if (pRenderer)
        {
            SFMLRenderer* sfmlRenderer = static_cast<SFMLRenderer*>(pRenderer);
            sfmlRenderer->SetRenderWindow(nullptr);
        }

        DestroyGameWindow(s_pWindow);
        s_pWindow = nullptr;
    }
}

hb::shared::types::NativeWindowHandle Window::get_handle()
{
    return s_pWindow ? s_pWindow->get_handle() : nullptr;
}

bool Window::is_active()
{
    return s_pWindow ? s_pWindow->is_active() : false;
}

void Window::close()
{
    if (s_pWindow)
    {
        s_pWindow->close();
    }
}

void Window::set_size(int width, int height, bool center)
{
    if (s_pWindow)
    {
        s_pWindow->set_size(width, height, center);
    }
}

void Window::set_borderless(bool borderless)
{
    if (s_pWindow)
    {
        s_pWindow->set_borderless(borderless);
    }
}

void Window::show_error(const char* title, const char* message)
{
    if (s_pWindow)
    {
        s_pWindow->show_message_box(title, message);
    }
    else
    {
#ifdef _WIN32
        MessageBox(nullptr, message, title, MB_ICONEXCLAMATION | MB_OK);
#else
        fprintf(stderr, "%s: %s\n", title, message);
#endif
    }
}

// Sprite factory static storage (defined in ISpriteFactory.cpp in Shared)
// The implementation uses hb::shared::sprite::Sprites::SetFactory/GetFactory

} // namespace hb::shared::render
