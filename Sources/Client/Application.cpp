#include "Application.h"
#include "Game.h"
#include "GameWindowHandler.h"
#include "RendererFactory.h"
#include "FrameTiming.h"
#include "ConfigManager.h"
#include "ResolutionConfig.h"
#include "DevConsole.h"
#include "IInput.h"
#include <cstdlib>
#include <ctime>

Application::Application() = default;

Application::~Application()
{
    _shutdown();
}

bool Application::initialize(NativeInstance instance, int icon_resource_id, const char* cmd_line)
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

    game_ = std::make_unique<CGame>();

    WindowParams params = {};
    params.title = "Helbreath";
    params.width = ConfigManager::Get().GetWindowWidth();
    params.height = ConfigManager::Get().GetWindowHeight();
    params.fullscreen = false;
    params.centered = true;
    params.nativeInstance = instance;
    params.iconResourceId = icon_resource_id;

    if (!Window::Create(params))
    {
        Window::ShowError("ERROR", "Failed to create window!");
        game_.reset();
        return false;
    }

    window_handler_ = std::make_unique<GameWindowHandler>(game_.get());
    Window::Get()->SetEventHandler(window_handler_.get());
    Window::Get()->Show();

    FrameTiming::Initialize();

    if (game_->bInit() == false)
    {
        Window::Close();
        game_.reset();
        return false;
    }

    return true;
}

int Application::run()
{
    _event_loop();
    return 0;
}

void Application::_shutdown()
{
    if (Window::Get())
        Window::Get()->SetEventHandler(nullptr);

    window_handler_.reset();
    game_.reset();

    Window::Destroy();
    Renderer::Destroy();

    DevConsole::Get().Shutdown();
}

void Application::_event_loop()
{
    IWindow* window = Window::Get();
    if (!window)
        return;

    while (true)
    {
        Input::BeginFrame();

        if (!window->ProcessMessages())
            break;

        FrameTiming::BeginFrame();
        game_->RenderFrame();
        FrameTiming::EndFrame();
    }
}
