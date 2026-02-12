#include "NativeTypes.h"
#include "Game.h"
#include "GameWindowHandler.h"
#include "RendererFactory.h"
#include "FrameTiming.h"
#include "ConfigManager.h"
#include "ResolutionConfig.h"
#include "DevConsole.h"
#include "IInput.h"
#include "resource.h"
#include <memory>
#include <cstdlib>
#include <ctime>

// --------------------------------------------------------------
// Platform-independent core
int GameMain(hb::shared::types::NativeInstance nativeInstance, int iconResourceId, const char* cmdLine)
{
    srand(static_cast<unsigned>(time(0)));

    DevConsole::Get().Initialize();

    ConfigManager::Get().Initialize();
    ConfigManager::Get().Load();

    hb::shared::render::ResolutionConfig::Initialize(
        ConfigManager::Get().GetWindowWidth(),
        ConfigManager::Get().GetWindowHeight()
    );

    auto game = std::make_unique<CGame>();

    hb::shared::render::WindowParams params = {};
    params.title = "Helbreath";
    params.width = ConfigManager::Get().GetWindowWidth();
    params.height = ConfigManager::Get().GetWindowHeight();
    params.fullscreen = false;
    params.borderless = ConfigManager::Get().IsBorderlessEnabled();
    params.centered = true;
    params.mouseCaptureEnabled = ConfigManager::Get().IsMouseCaptureEnabled();
    params.nativeInstance = nativeInstance;
    params.iconResourceId = iconResourceId;

    if (!hb::shared::render::Window::Create(params))
    {
        hb::shared::render::Window::ShowError("ERROR", "Failed to create window!");
        game.reset();
        return 1;
    }

    auto windowHandler = std::make_unique<GameWindowHandler>(game.get());
    hb::shared::render::Window::Get()->SetEventHandler(windowHandler.get());
    hb::shared::render::Window::Get()->Show();

    FrameTiming::Initialize();

    if (game->bInit() == false)
    {
        hb::shared::render::Window::Get()->SetEventHandler(nullptr);
        windowHandler.reset();
        game.reset();
        hb::shared::render::Window::Close();
        return 1;
    }

    // Push display settings from ConfigManager to engine
    // Must happen after bInit() which creates the renderer via hb::shared::render::Renderer::Set()
    hb::shared::render::Window::Get()->SetVSyncEnabled(ConfigManager::Get().IsVSyncEnabled());
    hb::shared::render::Window::Get()->SetFramerateLimit(ConfigManager::Get().GetFpsLimit());
    hb::shared::render::Window::Get()->SetFullscreenStretch(ConfigManager::Get().IsFullscreenStretchEnabled());
    if (hb::shared::render::Renderer::Get())
        hb::shared::render::Renderer::Get()->SetFullscreenStretch(ConfigManager::Get().IsFullscreenStretchEnabled());

    // Event loop — UpdateFrame runs every iteration (decoupled from frame rate),
    // RenderFrame is gated by the engine's frame limiter (VSync or FPS cap)
    hb::shared::render::IWindow* window = hb::shared::render::Window::Get();
    while (true)
    {
        hb::shared::input::BeginFrame();

        if (!window->ProcessMessages())
            break;

        // Update always runs — logic, network, audio are never frame-limited
        game->UpdateFrame();

        // Render is gated by engine-owned frame limiting (BeginFrame sleeps when not time)
        FrameTiming::BeginFrame();
        game->RenderFrame();
        FrameTiming::EndFrame();
    }

    // Shutdown
    hb::shared::render::Window::Get()->SetEventHandler(nullptr);
    windowHandler.reset();
    game.reset();

    hb::shared::render::Window::Destroy();
    hb::shared::render::Renderer::Destroy();

    DevConsole::Get().Shutdown();

    return 0;
}

// --------------------------------------------------------------
// Platform-specific entry points

#ifdef _WIN32
#include <windows.h>

// GPU Selection - Force discrete GPU on hybrid systems
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

static void InitDpiAwareness()
{
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        SetProcessDPIAware();
    }
}

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    InitDpiAwareness();
    return GameMain(hInstance, IDI_ICON1, lpCmdLine);
}

int main(int argc, char* argv[])
{
    InitDpiAwareness();
    return GameMain(GetModuleHandle(nullptr), IDI_ICON1, argc > 1 ? argv[1] : "");
}
#else
int main(int argc, char* argv[])
{
    return GameMain(nullptr, 0, argc > 1 ? argv[1] : "");
}
#endif
