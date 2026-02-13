// SFMLInput.cpp: SFML implementation of IInput
//
// Part of SFMLEngine static library
// Note: SFMLWindow handles coordinate transformation, so mouse coordinates
// arrive already in logical game coordinates.
//////////////////////////////////////////////////////////////////////

#include "SFMLInput.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <cstring>

// Global input instance pointer (owned by RendererFactory)
static SFMLInput* s_pInput = nullptr;

// ============== Global Input Namespace Implementation ==============
namespace hb::shared::input {
    void create()
    {
        if (!s_pInput)
        {
            s_pInput = new SFMLInput();
        }
    }

    void destroy()
    {
        delete s_pInput;
        s_pInput = nullptr;
    }

    IInput* get()
    {
        return s_pInput;
    }
}

// ============== SFMLInput Implementation ==============

SFMLInput::SFMLInput()
    : m_hWnd(nullptr)
    , m_pRenderWindow(nullptr)
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

SFMLInput::~SFMLInput() = default;

void SFMLInput::Initialize(hb::shared::types::NativeWindowHandle hWnd)
{
    m_hWnd = hWnd;
    m_active = true;
}

void SFMLInput::SetRenderWindow(sf::RenderWindow* pWindow)
{
    m_pRenderWindow = pWindow;
}

void SFMLInput::begin_frame()
{
    // Reset per-frame edge states (pressed/released)
    // Note: m_wheelDelta is NOT reset here — it accumulates across skip frames
    // and is cleared by ResetMouseWheelDelta() after a rendered frame consumes it
    std::memset(m_mousePressed, 0, sizeof(m_mousePressed));
    std::memset(m_mouseReleased, 0, sizeof(m_mouseReleased));
    std::memset(m_keyPressed, 0, sizeof(m_keyPressed));
    std::memset(m_keyReleased, 0, sizeof(m_keyReleased));
}

void SFMLInput::reset_mouse_wheel_delta()
{
    m_wheelDelta = 0;
}

// ============== Keyboard ==============

bool SFMLInput::is_key_down(KeyCode key) const
{
    if (m_suppressed) return false;
    int k = static_cast<int>(key);
    if (k < 0 || k >= kKeyCount)
        return false;
    return m_keyDown[k];
}

bool SFMLInput::is_key_pressed(KeyCode key) const
{
    if (m_suppressed) return false;
    int k = static_cast<int>(key);
    if (k < 0 || k >= kKeyCount)
        return false;
    return m_keyPressed[k];
}

bool SFMLInput::is_key_released(KeyCode key) const
{
    if (m_suppressed) return false;
    int k = static_cast<int>(key);
    if (k < 0 || k >= kKeyCount)
        return false;
    return m_keyReleased[k];
}

void SFMLInput::on_key_down(KeyCode key)
{
    int k = static_cast<int>(key);
    if (k < 0 || k >= kKeyCount)
        return;

    // Only set pressed on initial key down, not on auto-repeat
    if (!m_keyDown[k])
    {
        m_keyPressed[k] = true;
    }
    m_keyDown[k] = true;
}

void SFMLInput::on_key_up(KeyCode key)
{
    int k = static_cast<int>(key);
    if (k < 0 || k >= kKeyCount)
        return;

    if (m_keyDown[k])
    {
        m_keyReleased[k] = true;
    }
    m_keyDown[k] = false;
}

// ============== Mouse Buttons ==============

bool SFMLInput::is_mouse_button_down(int button) const
{
    if (m_suppressed) return false;
    if (button < 0 || button > 2)
        return false;
    return m_mouseDown[button];
}

bool SFMLInput::is_mouse_button_pressed(int button) const
{
    if (m_suppressed) return false;
    if (button < 0 || button > 2)
        return false;
    return m_mousePressed[button];
}

bool SFMLInput::is_mouse_button_released(int button) const
{
    if (m_suppressed) return false;
    if (button < 0 || button > 2)
        return false;
    return m_mouseReleased[button];
}

void SFMLInput::on_mouse_down(int button)
{
    if (button < 0 || button > 2)
        return;

    if (!m_mouseDown[button])
    {
        m_mousePressed[button] = true;
    }
    m_mouseDown[button] = true;
}

void SFMLInput::on_mouse_up(int button)
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

int SFMLInput::get_mouse_x() const
{
    return m_mouseX;
}

int SFMLInput::get_mouse_y() const
{
    return m_mouseY;
}

void SFMLInput::on_mouse_move(int x, int y)
{
    // SFMLWindow already transforms to logical coordinates
    m_mouseX = x;
    m_mouseY = y;
}

// ============== Mouse Wheel ==============

int SFMLInput::get_mouse_wheel_delta() const
{
    if (m_suppressed) return 0;
    return m_wheelDelta;
}

void SFMLInput::on_mouse_wheel(int delta)
{
    m_wheelDelta += delta;
}

// ============== Modifier Keys ==============

bool SFMLInput::is_shift_down() const
{
    if (m_suppressed) return false;
    return is_key_down(KeyCode::Shift) || is_key_down(KeyCode::LShift) || is_key_down(KeyCode::RShift);
}

bool SFMLInput::is_ctrl_down() const
{
    if (m_suppressed) return false;
    return is_key_down(KeyCode::Control) || is_key_down(KeyCode::LControl) || is_key_down(KeyCode::RControl);
}

bool SFMLInput::is_alt_down() const
{
    if (m_suppressed) return false;
    return is_key_down(KeyCode::Alt) || is_key_down(KeyCode::LAlt) || is_key_down(KeyCode::RAlt);
}

// ============== Input Suppression ==============

void SFMLInput::set_suppressed(bool suppressed)
{
    m_suppressed = suppressed;
}

bool SFMLInput::is_suppressed() const
{
    return m_suppressed;
}

// ============== hb::shared::render::Window Focus ==============

bool SFMLInput::is_window_active() const
{
    return m_active;
}

void SFMLInput::set_window_active(bool active)
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
