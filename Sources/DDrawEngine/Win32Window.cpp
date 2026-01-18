// Win32Window.cpp: Win32 implementation of IWindow
//
// Part of DDrawEngine static library
//////////////////////////////////////////////////////////////////////

#include "Win32Window.h"
#include <cstdio>
#include <windowsx.h>  // For GET_X_LPARAM, GET_Y_LPARAM

Win32Window::Win32Window()
    : m_hWnd(nullptr)
    , m_hInstance(nullptr)
    , m_pEventHandler(nullptr)
    , m_width(0)
    , m_height(0)
    , m_fullscreen(false)
    , m_active(false)
    , m_open(false)
{
    m_className[0] = '\0';
}

Win32Window::~Win32Window()
{
    Destroy();
}

bool Win32Window::Create(const WindowParams& params)
{
    if (m_open)
        return false;  // Already created

    m_hInstance = params.hInstance;
    m_width = params.width;
    m_height = params.height;
    m_fullscreen = params.fullscreen;

    // Generate unique class name
    sprintf_s(m_className, sizeof(m_className), "Win32Window-%p", this);

    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
    wc.lpfnWndProc = StaticWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(Win32Window*);
    wc.hInstance = m_hInstance;
    wc.hIcon = params.iconResourceId ? LoadIcon(m_hInstance, MAKEINTRESOURCE(params.iconResourceId)) : LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = m_className;
    wc.hIconSm = wc.hIcon;

    if (!RegisterClassEx(&wc))
        return false;

    // Calculate window position
    int posX, posY;
    DWORD style = WS_POPUP;

    if (params.centered)
    {
        int screenWidth = GetSystemMetrics(SM_CXFULLSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYFULLSCREEN);
        posX = (screenWidth - m_width) / 2;
        posY = (screenHeight - m_height) / 2;
        if (posY > 100) posY += 40;  // Adjust for taskbar
    }
    else
    {
        posX = CW_USEDEFAULT;
        posY = CW_USEDEFAULT;
    }

    // Create the window
    m_hWnd = CreateWindowEx(
        0,
        m_className,
        params.title ? params.title : "Window",
        style,
        posX, posY,
        m_width, m_height,
        nullptr,
        nullptr,
        m_hInstance,
        this  // Pass this pointer for WndProc
    );

    if (!m_hWnd)
    {
        UnregisterClass(m_className, m_hInstance);
        return false;
    }

    m_open = true;
    m_active = true;

    return true;
}

void Win32Window::Destroy()
{
    if (m_hWnd)
    {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }

    if (m_className[0] != '\0' && m_hInstance)
    {
        UnregisterClass(m_className, m_hInstance);
        m_className[0] = '\0';
    }

    m_open = false;
    m_active = false;
}

bool Win32Window::IsOpen() const
{
    return m_open;
}

HWND Win32Window::GetHandle() const
{
    return m_hWnd;
}

int Win32Window::GetWidth() const
{
    return m_width;
}

int Win32Window::GetHeight() const
{
    return m_height;
}

bool Win32Window::IsFullscreen() const
{
    return m_fullscreen;
}

bool Win32Window::IsActive() const
{
    return m_active;
}

void Win32Window::SetFullscreen(bool fullscreen)
{
    // TODO: Implement fullscreen toggle
    m_fullscreen = fullscreen;
}

void Win32Window::Show()
{
    if (m_hWnd)
    {
        ShowWindow(m_hWnd, SW_SHOWDEFAULT);
        UpdateWindow(m_hWnd);
    }
}

void Win32Window::Hide()
{
    if (m_hWnd)
    {
        ShowWindow(m_hWnd, SW_HIDE);
    }
}

void Win32Window::SetTitle(const char* title)
{
    if (m_hWnd && title)
    {
        SetWindowText(m_hWnd, title);
    }
}

bool Win32Window::ProcessMessages()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            m_open = false;
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}

void Win32Window::WaitForMessage()
{
    ::WaitMessage();
}

void Win32Window::SetEventHandler(IWindowEventHandler* handler)
{
    m_pEventHandler = handler;
}

IWindowEventHandler* Win32Window::GetEventHandler() const
{
    return m_pEventHandler;
}

// Static WndProc that routes to instance method
LRESULT CALLBACK Win32Window::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Win32Window* pWindow = nullptr;

    if (message == WM_NCCREATE)
    {
        // Get the Win32Window pointer from CreateWindowEx
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pWindow = reinterpret_cast<Win32Window*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
        pWindow->m_hWnd = hWnd;
    }
    else
    {
        pWindow = reinterpret_cast<Win32Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pWindow)
    {
        return pWindow->HandleMessage(message, wParam, lParam);
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

// Instance method to handle messages
LRESULT Win32Window::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    // First, let event handler try to handle text input
    if (m_pEventHandler)
    {
        if (m_pEventHandler->OnTextInput(m_hWnd, message, wParam, lParam))
            return 0;
    }

    switch (message)
    {
    case WM_CLOSE:
        if (m_pEventHandler)
            m_pEventHandler->OnClose();
        return 0;

    case WM_DESTROY:
        if (m_pEventHandler)
            m_pEventHandler->OnDestroy();
        m_open = false;
        PostQuitMessage(0);
        return 0;

    case WM_ACTIVATEAPP:
        m_active = (wParam != 0);
        if (m_pEventHandler)
            m_pEventHandler->OnActivate(m_active);
        return 0;

    case WM_SIZE:
        m_width = LOWORD(lParam);
        m_height = HIWORD(lParam);
        if (m_pEventHandler)
            m_pEventHandler->OnResize(m_width, m_height);
        return 0;

    case WM_KEYDOWN:
        if (m_pEventHandler)
            m_pEventHandler->OnKeyDown(static_cast<int>(wParam));
        return DefWindowProc(m_hWnd, message, wParam, lParam);

    case WM_KEYUP:
        if (m_pEventHandler)
            m_pEventHandler->OnKeyUp(static_cast<int>(wParam));
        return DefWindowProc(m_hWnd, message, wParam, lParam);

    case WM_CHAR:
        if (m_pEventHandler)
            m_pEventHandler->OnChar(static_cast<char>(wParam));
        return 0;

    case WM_MOUSEMOVE:
        if (m_pEventHandler)
            m_pEventHandler->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_LBUTTONDOWN:
        if (m_pEventHandler)
            m_pEventHandler->OnMouseButtonDown(MOUSE_BUTTON_LEFT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_LBUTTONUP:
        if (m_pEventHandler)
            m_pEventHandler->OnMouseButtonUp(MOUSE_BUTTON_LEFT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_RBUTTONDOWN:
        if (m_pEventHandler)
            m_pEventHandler->OnMouseButtonDown(MOUSE_BUTTON_RIGHT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_RBUTTONUP:
        if (m_pEventHandler)
            m_pEventHandler->OnMouseButtonUp(MOUSE_BUTTON_RIGHT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_MBUTTONDOWN:
        if (m_pEventHandler)
            m_pEventHandler->OnMouseButtonDown(MOUSE_BUTTON_MIDDLE, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_MBUTTONUP:
        if (m_pEventHandler)
            m_pEventHandler->OnMouseButtonUp(MOUSE_BUTTON_MIDDLE, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_MOUSEWHEEL:
        if (m_pEventHandler)
        {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(m_hWnd, &pt);
            m_pEventHandler->OnMouseWheel(delta, pt.x, pt.y);
        }
        return 0;

    case WM_SYSCOMMAND:
        // Prevent screensaver and monitor power-off
        if ((wParam & 0xFFF0) == SC_SCREENSAVE || (wParam & 0xFFF0) == SC_MONITORPOWER)
            return 0;
        return DefWindowProc(m_hWnd, message, wParam, lParam);

    default:
        // Let event handler try custom messages
        if (m_pEventHandler)
        {
            if (m_pEventHandler->OnCustomMessage(message, wParam, lParam))
                return 0;
        }
        return DefWindowProc(m_hWnd, message, wParam, lParam);
    }
}
