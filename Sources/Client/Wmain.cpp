// --------------------------------------------------------------
//                      Helbreath Client
//
//                      1998.10 by Soph
//
// --------------------------------------------------------------

// MODERNIZED: Prevent old winsock.h from loading (must be before windows.h)
#define _WINSOCKAPI_

#include <windows.h>
#include "CommonTypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <mmsystem.h>
#include <process.h>

#include "Game.h"
#include "GlobalDef.h"
#include "resource.h"
#include "FrameTiming.h"
#include "ConfigManager.h"
#include "IInput.h"
#include "RendererFactory.h"
#include "GameWindowHandler.h"

// --------------------------------------------------------------
#define WM_USER_TIMERSIGNAL		WM_USER + 500

// Global state
HWND G_hWnd = 0;
HWND G_hEditWnd = 0;
HINSTANCE G_hInstance = 0;
MMRESULT G_mmTimer;
class CGame* G_pGame = nullptr;
class XSocket* G_pCalcSocket = 0;
bool G_bIsCalcSocketConnected = true;
uint32_t G_dwCalcSocketTime = 0, G_dwCalcSocketSendTime = 0;

char G_cCmdLine[256], G_cCmdLineTokenA[120], G_cCmdLineTokenA_Lowercase[120];
char G_cCmdLineTokenB[120], G_cCmdLineTokenC[120], G_cCmdLineTokenD[120], G_cCmdLineTokenE[120];

// Window event handler
static GameWindowHandler* g_pWindowHandler = nullptr;

// Function declarations
void EventLoop();
void Initialize(char* pCmdLine);
void CALLBACK _TimerFunc(UINT wID, UINT wUser, DWORD dwUSer, DWORD dw1, DWORD dw2);
MMRESULT _StartTimer(DWORD dwTime);
void _StopTimer(MMRESULT timerid);

// --------------------------------------------------------------
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    srand((unsigned)time(0));

    // Ensure consistent pixel coordinates under RDP and high-DPI setups
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        SetProcessDPIAware();
    }

    // Load settings early so window size is available
    ConfigManager::Get().Initialize();
    ConfigManager::Get().Load();

    // Create game instance
    G_pGame = new class CGame;
    G_hInstance = hInstance;

    // Create window using engine abstraction
    WindowParams params = {};
    params.title = "Helbreath";
    params.width = ConfigManager::Get().GetWindowWidth();
    params.height = ConfigManager::Get().GetWindowHeight();
    params.fullscreen = false;
    params.centered = true;
    params.hInstance = hInstance;
    params.iconResourceId = IDI_ICON1;

    if (!Window::Create(params))
    {
        MessageBox(nullptr, "Failed to create window!", "ERROR", MB_ICONEXCLAMATION | MB_OK);
        delete G_pGame;
        return 1;
    }

    // Set up event handler
    g_pWindowHandler = new GameWindowHandler(G_pGame);
    Window::Get()->SetEventHandler(g_pWindowHandler);
    Window::Get()->Show();

    // Store window handle globally for legacy code
    G_hWnd = Window::GetHandle();

    // Initialize game systems
    Initialize((char*)lpCmdLine);

    // Main loop
    EventLoop();

    // Cleanup
    _StopTimer(G_mmTimer);

    // Clear event handler from window BEFORE deleting it
    // This prevents dangling pointer access during window destruction
    Window::Get()->SetEventHandler(nullptr);

    delete g_pWindowHandler;
    g_pWindowHandler = nullptr;

    delete G_pGame;
    G_pGame = nullptr;

    Window::Destroy();
    Renderer::Destroy();

    return 0;
}

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

        if (G_pGame->m_bIsProgramActive || G_pGame->m_cGameMode == DEF_GAMEMODE_ONLOADING)
        {
            FrameTiming::BeginFrame();
            G_pGame->RenderFrame();
            FrameTiming::EndFrame();
        }
        else
        {
            pWindow->WaitForMessage();
        }
    }
}

void CALLBACK _TimerFunc(UINT wID, UINT wUser, DWORD dwUSer, DWORD dw1, DWORD dw2)
{
    PostMessage(G_hWnd, WM_USER_TIMERSIGNAL, wID, 0);
}

MMRESULT _StartTimer(DWORD dwTime)
{
    TIMECAPS caps;
    timeGetDevCaps(&caps, sizeof(caps));
    timeBeginPeriod(caps.wPeriodMin);
    return timeSetEvent(dwTime, 0, _TimerFunc, 0, (UINT)TIME_PERIODIC);
}

void _StopTimer(MMRESULT timerid)
{
    TIMECAPS caps;
    if (timerid != 0)
    {
        timeKillEvent(timerid);
        timerid = 0;
        timeGetDevCaps(&caps, sizeof(caps));
        timeEndPeriod(caps.wPeriodMin);
    }
}

void Initialize(char* pCmdLine)
{
    int iErrCode;
    uint16_t wVersionRequested;
    WSADATA wsaData;

    // Initialize timing systems
    FrameTiming::Initialize();

    // Initialize Winsock
    wVersionRequested = MAKEWORD(2, 2);
    iErrCode = WSAStartup(wVersionRequested, &wsaData);
    if (iErrCode)
    {
        MessageBox(G_hWnd, "Winsock-V2.2 not found! Cannot execute program.", "ERROR", MB_ICONEXCLAMATION | MB_OK);
        PostQuitMessage(0);
        return;
    }

    // Initialize game
    if (G_pGame->bInit(G_hWnd, G_hInstance, pCmdLine) == false)
    {
        PostQuitMessage(0);
        return;
    }

    // Start game timer
    G_mmTimer = _StartTimer(1000);
}
