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
int GameMain(NativeInstance nativeInstance, int iconResourceId, const char* cmdLine)
{
    srand((unsigned)time(0));

    DevConsole::Get().Initialize();

    ConfigManager::Get().Initialize();
    ConfigManager::Get().Load();

    ResolutionConfig::Initialize(
        ConfigManager::Get().GetBaseResolutionWidth(),
        ConfigManager::Get().GetBaseResolutionHeight(),
        ConfigManager::Get().GetWindowWidth(),
        ConfigManager::Get().GetWindowHeight()
    );

    auto game = std::make_unique<CGame>();

    WindowParams params = {};
    params.title = "Helbreath";
    params.width = ConfigManager::Get().GetWindowWidth();
    params.height = ConfigManager::Get().GetWindowHeight();
    params.fullscreen = false;
    params.centered = true;
    params.nativeInstance = nativeInstance;
    params.iconResourceId = iconResourceId;

    if (!Window::Create(params))
    {
        Window::ShowError("ERROR", "Failed to create window!");
        game.reset();
        return 1;
    }

    auto windowHandler = std::make_unique<GameWindowHandler>(game.get());
    Window::Get()->SetEventHandler(windowHandler.get());
    Window::Get()->Show();

    FrameTiming::Initialize();

    if (game->bInit() == false)
    {
        Window::Get()->SetEventHandler(nullptr);
        windowHandler.reset();
        game.reset();
        Window::Close();
        return 1;
    }

    // Event loop
    IWindow* window = Window::Get();
    while (true)
    {
        Input::BeginFrame();

        if (!window->ProcessMessages())
            break;

        FrameTiming::BeginFrame();
        game->RenderFrame();
        FrameTiming::EndFrame();
    }

    // Shutdown
    Window::Get()->SetEventHandler(nullptr);
    windowHandler.reset();
    game.reset();

    Window::Destroy();
    Renderer::Destroy();

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
