// --------------------------------------------------------------
//                      Helbreath Client
//
//                      1998.10 by Soph
//
// --------------------------------------------------------------

#include "NativeTypes.h"
#include "CommonTypes.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <atomic>
#include <thread>
#include <chrono>

#include "Game.h"
#include "GameModeManager.h"
#include "GlobalDef.h"
#include "resource.h"
#include "FrameTiming.h"
#include "ConfigManager.h"
#include "ResolutionConfig.h"
#include "IInput.h"
#include "RendererFactory.h"
#include "GameWindowHandler.h"
#include "Benchmark.h"
#include "DevConsole.h"

// --------------------------------------------------------------
// Global state
// G_hWnd/G_hEditWnd kept for DDrawEngine internals (DXC_ddraw.cpp) which reference them directly
NativeWindowHandle G_hWnd = nullptr;
NativeWindowHandle G_hEditWnd = nullptr;
class CGame* G_pGame = nullptr;

// Timer thread state (replaces Windows multimedia timer)
std::atomic<bool> G_bTimerSignal{false};
std::atomic<bool> G_bTimerRunning{false};
std::thread G_timerThread;
class XSocket* G_pCalcSocket = 0;
bool G_bIsCalcSocketConnected = true;
uint32_t G_dwCalcSocketTime = 0, G_dwCalcSocketSendTime = 0;

// Window event handler
static GameWindowHandler* g_pWindowHandler = nullptr;

// Function declarations
void EventLoop();
void StartTimerThread();
void StopTimerThread();

// Timer thread function - signals main thread every 1000ms
void TimerThreadFunc()
{
    while (G_bTimerRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if (G_bTimerRunning.load()) {
            G_bTimerSignal.store(true);
        }
    }
}

// --------------------------------------------------------------
// Platform-independent core
int GameMain(NativeInstance nativeInstance, int iconResourceId, const char* cmdLine)
{
    srand((unsigned)time(0));

    // Initialize in-game developer console (replaces DebugConsole window)
    DevConsole::Get().Initialize();

    // Load settings early so window size is available
    ConfigManager::Get().Initialize();
    ConfigManager::Get().Load();

    // Initialize resolution config from settings
    // This must happen before creating the window or any rendering
    ResolutionConfig::Initialize(
        ConfigManager::Get().GetBaseResolutionWidth(),
        ConfigManager::Get().GetBaseResolutionHeight(),
        ConfigManager::Get().GetWindowWidth(),
        ConfigManager::Get().GetWindowHeight()
    );

    // Create game instance
    G_pGame = new class CGame;

    // Create window using engine abstraction
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
        delete G_pGame;
        return 1;
    }

    // Set up event handler
    g_pWindowHandler = new GameWindowHandler(G_pGame);
    Window::Get()->SetEventHandler(g_pWindowHandler);
    Window::Get()->Show();

    // Store window handle for DDrawEngine internals (DXC_ddraw.cpp)
    G_hWnd = Window::GetHandle();

    // Initialize timing systems
    FrameTiming::Initialize();

    // Initialize Winsock
#ifdef _WIN32
    {
        WSADATA wsaData;
        uint16_t wVersionRequested = MAKEWORD(2, 2);
        int iErrCode = WSAStartup(wVersionRequested, &wsaData);
        if (iErrCode)
        {
            Window::ShowError("ERROR", "Winsock-V2.2 not found! Cannot execute program.");
            Window::Close();
            delete G_pGame;
            return 1;
        }
    }
#endif

    // Initialize game
    if (G_pGame->bInit() == false)
    {
        Window::Close();
        delete G_pGame;
        return 1;
    }

    // Start timer thread
    StartTimerThread();

    // Main loop
    EventLoop();

    // Cleanup
    StopTimerThread();

    // Clear event handler from window BEFORE deleting it
    // This prevents dangling pointer access during window destruction
    Window::Get()->SetEventHandler(nullptr);

    delete g_pWindowHandler;
    g_pWindowHandler = nullptr;

    delete G_pGame;
    G_pGame = nullptr;

    Window::Destroy();
    Renderer::Destroy();

    // Shutdown developer console
    DevConsole::Get().Shutdown();

    return 0;
}

// --------------------------------------------------------------
// Platform-specific entry points

#ifdef _WIN32
#include <windows.h>

// GPU Selection - Force discrete GPU on hybrid systems
// These exports tell NVIDIA Optimus and AMD PowerXpress to prefer
// the high-performance GPU over integrated graphics
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

// Console subsystem entry point (used by SFML build)
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

// --------------------------------------------------------------
void EventLoop()
{
    IWindow* pWindow = Window::Get();
    if (!pWindow)
        return;

    // Main event loop - BeginFrame must be called BEFORE ProcessMessages
    // so that events processed in this frame are visible to game code
    while (true)
    {
        // Reset per-frame input state from PREVIOUS frame
        Input::BeginFrame();

        // Process window messages - this sets pressed/released states for THIS frame
        if (!pWindow->ProcessMessages())
            break;

        FrameTiming::BeginFrame();
        G_pGame->RenderFrame();
        FrameTiming::EndFrame();
    }
}

void StartTimerThread()
{
    G_bTimerRunning.store(true);
    G_timerThread = std::thread(TimerThreadFunc);
}

void StopTimerThread()
{
    G_bTimerRunning.store(false);
    if (G_timerThread.joinable()) {
        G_timerThread.join();
    }
}
