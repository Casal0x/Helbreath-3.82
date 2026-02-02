// Win32Input.cpp: Win32/DirectDraw implementation of IInput
//
// Part of DDrawEngine static library
//////////////////////////////////////////////////////////////////////

#include "Win32Input.h"
#include "RenderConstants.h"
#include "ConfigManager.h"
#include <cstring>

// Global input instance pointer (owned by RendererFactory)
static Win32Input* s_pInput = nullptr;

// ============== Global Input Namespace Implementation ==============
namespace Input {
    void Create()
    {
        if (!s_pInput)
        {
            s_pInput = new Win32Input();
        }
    }

    void Destroy()
    {
        delete s_pInput;
        s_pInput = nullptr;
    }

    IInput* Get()
    {
        return s_pInput;
    }
}

// ============== Win32Input Implementation ==============

Win32Input::Win32Input()
    : m_hWnd(nullptr)
    , m_active(false)
    , m_suppressed(false)
    , m_mouseX(0)
    , m_mouseY(0)
    , m_wheelDelta(0)
{
    std::memset(m_mouseDown, 0, sizeof(m_mouseDown));
    std::memset(m_mousePressed, 0, sizeof(m_mousePressed));
    std::memset(m_mouseReleased, 0, sizeof(m_mouseReleased));
    std::memset(m_keyDown, 0, sizeof(m_keyDown));
    std::memset(m_keyPressed, 0, sizeof(m_keyPressed));
    std::memset(m_keyReleased, 0, sizeof(m_keyReleased));
}

Win32Input::~Win32Input()
{
    UpdateCursorClip(false);
}

void Win32Input::Initialize(HWND hWnd)
{
    m_hWnd = hWnd;
    UpdateCursorClip(false);

    if (m_hWnd && GetForegroundWindow() == m_hWnd)
    {
        SetWindowActive(true);
    }
}

void Win32Input::BeginFrame()
{
    // Reset per-frame state
    std::memset(m_mousePressed, 0, sizeof(m_mousePressed));
    std::memset(m_mouseReleased, 0, sizeof(m_mouseReleased));
    std::memset(m_keyPressed, 0, sizeof(m_keyPressed));
    std::memset(m_keyReleased, 0, sizeof(m_keyReleased));
    m_wheelDelta = 0;
}

// ============== Keyboard ==============

bool Win32Input::IsKeyDown(int key) const
{
    if (m_suppressed) return false;
    if (key < 0 || key >= kKeyCount)
        return false;
    return m_keyDown[key];
}

bool Win32Input::IsKeyPressed(int key) const
{
    if (m_suppressed) return false;
    if (key < 0 || key >= kKeyCount)
        return false;
    return m_keyPressed[key];
}

bool Win32Input::IsKeyReleased(int key) const
{
    if (m_suppressed) return false;
    if (key < 0 || key >= kKeyCount)
        return false;
    return m_keyReleased[key];
}

void Win32Input::OnKeyDown(int key)
{
    if (key < 0 || key >= kKeyCount)
        return;

    // Only set pressed on initial key down, not on auto-repeat
    if (!m_keyDown[key])
    {
        m_keyPressed[key] = true;
    }
    m_keyDown[key] = true;
}

void Win32Input::OnKeyUp(int key)
{
    if (key < 0 || key >= kKeyCount)
        return;

    if (m_keyDown[key])
    {
        m_keyReleased[key] = true;
    }
    m_keyDown[key] = false;
}

// ============== Mouse Buttons ==============

bool Win32Input::IsMouseButtonDown(int button) const
{
    if (m_suppressed) return false;
    if (button < 0 || button > 2)
        return false;
    return m_mouseDown[button];
}

bool Win32Input::IsMouseButtonPressed(int button) const
{
    if (m_suppressed) return false;
    if (button < 0 || button > 2)
        return false;
    return m_mousePressed[button];
}

bool Win32Input::IsMouseButtonReleased(int button) const
{
    if (m_suppressed) return false;
    if (button < 0 || button > 2)
        return false;
    return m_mouseReleased[button];
}

void Win32Input::OnMouseDown(int button)
{
    if (button < 0 || button > 2)
        return;

    if (!m_mouseDown[button])
    {
        m_mousePressed[button] = true;
    }
    m_mouseDown[button] = true;

    // Capture mouse for drag operations
    if (m_hWnd)
    {
        SetCapture(m_hWnd);
    }
}

void Win32Input::OnMouseUp(int button)
{
    if (button < 0 || button > 2)
        return;

    if (m_mouseDown[button])
    {
        m_mouseReleased[button] = true;
    }
    m_mouseDown[button] = false;

    // Release capture when all buttons are up
    if (!m_mouseDown[0] && !m_mouseDown[1] && !m_mouseDown[2])
    {
        if (GetCapture() == m_hWnd)
        {
            ReleaseCapture();
        }
    }
}

// ============== Mouse Position ==============

int Win32Input::GetMouseX() const
{
    return m_mouseX;
}

int Win32Input::GetMouseY() const
{
    return m_mouseY;
}

void Win32Input::OnMouseMove(int x, int y)
{
    UpdateLogicalPosition(x, y);
}

void Win32Input::UpdateLogicalPosition(int clientX, int clientY)
{
    if (!m_hWnd)
    {
        m_mouseX = clientX;
        m_mouseY = clientY;
        return;
    }

    RECT rcClient{};
    GetClientRect(m_hWnd, &rcClient);
    int winW = rcClient.right - rcClient.left;
    int winH = rcClient.bottom - rcClient.top;
    if (winW <= 0 || winH <= 0)
        return;

    // Calculate scale maintaining aspect ratio
    double scale = static_cast<double>(winW) / static_cast<double>(LOGICAL_WIDTH());
    double scaleY = static_cast<double>(winH) / static_cast<double>(LOGICAL_HEIGHT());
    if (scaleY < scale)
    {
        scale = scaleY;
    }
    if (scale <= 0.0)
    {
        scale = 1.0;
    }

    // Calculate letterbox offset
    int destW = static_cast<int>(LOGICAL_WIDTH() * scale);
    int destH = static_cast<int>(LOGICAL_HEIGHT() * scale);
    int offsetX = (winW - destW) / 2;
    int offsetY = (winH - destH) / 2;

    // Transform to logical coordinates
    long scaledX = static_cast<long>((clientX - offsetX) / scale);
    long scaledY = static_cast<long>((clientY - offsetY) / scale);

    // Clamp to valid range
    if (scaledX < 0) scaledX = 0;
    if (scaledY < 0) scaledY = 0;
    if (scaledX > LOGICAL_MAX_X()) scaledX = LOGICAL_MAX_X();
    if (scaledY > LOGICAL_MAX_Y()) scaledY = LOGICAL_MAX_Y();

    m_mouseX = static_cast<int>(scaledX);
    m_mouseY = static_cast<int>(scaledY);
}

// ============== Mouse Wheel ==============

int Win32Input::GetMouseWheelDelta() const
{
    if (m_suppressed) return 0;
    return m_wheelDelta;
}

void Win32Input::OnMouseWheel(int delta)
{
    m_wheelDelta += delta;
}

// ============== Modifier Keys ==============

bool Win32Input::IsShiftDown() const
{
    if (m_suppressed) return false;
    return IsKeyDown(VK_SHIFT) || IsKeyDown(VK_LSHIFT) || IsKeyDown(VK_RSHIFT);
}

bool Win32Input::IsCtrlDown() const
{
    if (m_suppressed) return false;
    return IsKeyDown(VK_CONTROL) || IsKeyDown(VK_LCONTROL) || IsKeyDown(VK_RCONTROL);
}

bool Win32Input::IsAltDown() const
{
    if (m_suppressed) return false;
    return IsKeyDown(VK_MENU) || IsKeyDown(VK_LMENU) || IsKeyDown(VK_RMENU);
}

// ============== Input Suppression ==============

void Win32Input::SetSuppressed(bool suppressed)
{
    m_suppressed = suppressed;
}

bool Win32Input::IsSuppressed() const
{
    return m_suppressed;
}

// ============== Window Focus ==============

bool Win32Input::IsWindowActive() const
{
    return m_active;
}

void Win32Input::SetWindowActive(bool active)
{
    m_active = active;
    UpdateCursorClip(active);

    if (!active)
    {
        ClearAllKeys();
    }
}

void Win32Input::ClearAllKeys()
{
    std::memset(m_keyDown, 0, sizeof(m_keyDown));
    std::memset(m_keyPressed, 0, sizeof(m_keyPressed));
    std::memset(m_keyReleased, 0, sizeof(m_keyReleased));
    std::memset(m_mouseDown, 0, sizeof(m_mouseDown));
    std::memset(m_mousePressed, 0, sizeof(m_mousePressed));
    std::memset(m_mouseReleased, 0, sizeof(m_mouseReleased));
}

void Win32Input::UpdateCursorClip(bool active)
{
    if (!m_hWnd)
        return;

    if (active)
    {
        if (!ConfigManager::Get().IsMouseCaptureEnabled())
        {
            ClipCursor(nullptr);
            return;
        }

        RECT rcClient{};
        GetClientRect(m_hWnd, &rcClient);
        int winW = rcClient.right - rcClient.left;
        int winH = rcClient.bottom - rcClient.top;

        double scale = static_cast<double>(winW) / static_cast<double>(LOGICAL_WIDTH());
        double scaleY = static_cast<double>(winH) / static_cast<double>(LOGICAL_HEIGHT());
        if (scaleY < scale)
        {
            scale = scaleY;
        }
        if (scale <= 0.0)
        {
            scale = 1.0;
        }

        int destW = static_cast<int>(LOGICAL_WIDTH() * scale);
        int destH = static_cast<int>(LOGICAL_HEIGHT() * scale);
        int offsetX = (winW - destW) / 2;
        int offsetY = (winH - destH) / 2;

        POINT ptTopLeft{ offsetX, offsetY };
        POINT ptBottomRight{ offsetX + destW, offsetY + destH };
        ClientToScreen(m_hWnd, &ptTopLeft);
        ClientToScreen(m_hWnd, &ptBottomRight);
        RECT rcClip{ ptTopLeft.x, ptTopLeft.y, ptBottomRight.x, ptBottomRight.y };
        ClipCursor(&rcClip);
    }
    else
    {
        ClipCursor(nullptr);
    }
}
