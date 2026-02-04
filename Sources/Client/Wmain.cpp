// --------------------------------------------------------------
//                      Helbreath Client
//
//                      1998.10 by Soph
//
// --------------------------------------------------------------

#include "NativeTypes.h"
#include "Application.h"
#include "resource.h"

// --------------------------------------------------------------
// Platform-independent core
int GameMain(NativeInstance nativeInstance, int iconResourceId, const char* cmdLine)
{
    Application app;
    if (!app.initialize(nativeInstance, iconResourceId, cmdLine))
        return 1;
    return app.run();
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
