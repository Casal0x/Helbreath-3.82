// SFMLInput.cpp: SFML implementation of IInput
//
// Part of SFMLEngine static library
// Note: SFMLWindow handles coordinate transformation, so mouse coordinates
// arrive already in logical game coordinates.
//////////////////////////////////////////////////////////////////////

#include "SFMLInput.h"
#include <cstring>

// Global input instance pointer (owned by RendererFactory)
static SFMLInput* s_pInput = nullptr;

// ============== Global Input Namespace Implementation ==============
namespace Input {
    void Create()
    {
        if (!s_pInput)
        {
            s_pInput = new SFMLInput();
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

// ============== SFMLInput Implementation ==============

SFMLInput::SFMLInput()
    : m_hWnd(nullptr)
    , m_active(false)
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

SFMLInput::~SFMLInput() = default;

void SFMLInput::Initialize(HWND hWnd)
{
    m_hWnd = hWnd;
    m_active = true;
}

void SFMLInput::BeginFrame()
{
    // Reset per-frame state
    std::memset(m_mousePressed, 0, sizeof(m_mousePressed));
    std::memset(m_mouseReleased, 0, sizeof(m_mouseReleased));
    std::memset(m_keyPressed, 0, sizeof(m_keyPressed));
    std::memset(m_keyReleased, 0, sizeof(m_keyReleased));
    m_wheelDelta = 0;
}

// ============== Keyboard ==============

bool SFMLInput::IsKeyDown(int key) const
{
    if (key < 0 || key >= kKeyCount)
        return false;
    return m_keyDown[key];
}

bool SFMLInput::IsKeyPressed(int key) const
{
    if (key < 0 || key >= kKeyCount)
        return false;
    return m_keyPressed[key];
}

bool SFMLInput::IsKeyReleased(int key) const
{
    if (key < 0 || key >= kKeyCount)
        return false;
    return m_keyReleased[key];
}

void SFMLInput::OnKeyDown(int key)
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

void SFMLInput::OnKeyUp(int key)
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

bool SFMLInput::IsMouseButtonDown(int button) const
{
    if (button < 0 || button > 2)
        return false;
    return m_mouseDown[button];
}

bool SFMLInput::IsMouseButtonPressed(int button) const
{
    if (button < 0 || button > 2)
        return false;
    return m_mousePressed[button];
}

bool SFMLInput::IsMouseButtonReleased(int button) const
{
    if (button < 0 || button > 2)
        return false;
    return m_mouseReleased[button];
}

void SFMLInput::OnMouseDown(int button)
{
    if (button < 0 || button > 2)
        return;

    if (!m_mouseDown[button])
    {
        m_mousePressed[button] = true;
    }
    m_mouseDown[button] = true;
}

void SFMLInput::OnMouseUp(int button)
{
    if (button < 0 || button > 2)
        return;

    if (m_mouseDown[button])
    {
        m_mouseReleased[button] = true;
    }
    m_mouseDown[button] = false;
}

// ============== Mouse Position ==============

int SFMLInput::GetMouseX() const
{
    return m_mouseX;
}

int SFMLInput::GetMouseY() const
{
    return m_mouseY;
}

void SFMLInput::OnMouseMove(int x, int y)
{
    // SFMLWindow already transforms to logical coordinates
    m_mouseX = x;
    m_mouseY = y;
}

// ============== Mouse Wheel ==============

int SFMLInput::GetMouseWheelDelta() const
{
    return m_wheelDelta;
}

void SFMLInput::OnMouseWheel(int delta)
{
    m_wheelDelta += delta;
}

// ============== Modifier Keys ==============

bool SFMLInput::IsShiftDown() const
{
    return IsKeyDown(VK_SHIFT) || IsKeyDown(VK_LSHIFT) || IsKeyDown(VK_RSHIFT);
}

bool SFMLInput::IsCtrlDown() const
{
    return IsKeyDown(VK_CONTROL) || IsKeyDown(VK_LCONTROL) || IsKeyDown(VK_RCONTROL);
}

bool SFMLInput::IsAltDown() const
{
    return IsKeyDown(VK_MENU) || IsKeyDown(VK_LMENU) || IsKeyDown(VK_RMENU);
}

// ============== Window Focus ==============

bool SFMLInput::IsWindowActive() const
{
    return m_active;
}

void SFMLInput::SetWindowActive(bool active)
{
    m_active = active;

    if (!active)
    {
        ClearAllKeys();
    }
}

void SFMLInput::ClearAllKeys()
{
    std::memset(m_keyDown, 0, sizeof(m_keyDown));
    std::memset(m_keyPressed, 0, sizeof(m_keyPressed));
    std::memset(m_keyReleased, 0, sizeof(m_keyReleased));
    std::memset(m_mouseDown, 0, sizeof(m_mouseDown));
    std::memset(m_mousePressed, 0, sizeof(m_mousePressed));
    std::memset(m_mouseReleased, 0, sizeof(m_mouseReleased));
}
