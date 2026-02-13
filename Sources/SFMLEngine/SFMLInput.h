// SFMLInput.h: SFML implementation of IInput
//
// Part of SFMLEngine static library
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NativeTypes.h"
#include "IInput.h"

namespace sf { class RenderWindow; }

class SFMLInput : public IInput
{
public:
    SFMLInput();
    virtual ~SFMLInput();

    void Initialize(hb::shared::types::NativeWindowHandle hWnd);
    void SetRenderWindow(sf::RenderWindow* pWindow);

    // ============== IInput Implementation ==============

    // Frame management
    virtual void begin_frame() override;

    // Keyboard
    virtual bool is_key_down(KeyCode key) const override;
    virtual bool is_key_pressed(KeyCode key) const override;
    virtual bool is_key_released(KeyCode key) const override;

    // Mouse buttons
    virtual bool is_mouse_button_down(int button) const override;
    virtual bool is_mouse_button_pressed(int button) const override;
    virtual bool is_mouse_button_released(int button) const override;

    // Mouse position (already in logical coordinates from SFMLWindow)
    virtual int get_mouse_x() const override;
    virtual int get_mouse_y() const override;

    // Mouse wheel
    virtual int get_mouse_wheel_delta() const override;
    virtual void reset_mouse_wheel_delta() override;

    // Modifier keys
    virtual bool is_shift_down() const override;
    virtual bool is_ctrl_down() const override;
    virtual bool is_alt_down() const override;

    // hb::shared::render::Window focus
    virtual bool is_window_active() const override;
    virtual void set_window_active(bool active) override;

    // Input suppression
    virtual void set_suppressed(bool suppressed) override;
    virtual bool is_suppressed() const override;

    // Input events (called by SFMLWindow with logical coordinates)
    virtual void on_key_down(KeyCode key) override;
    virtual void on_key_up(KeyCode key) override;
    virtual void on_mouse_move(int x, int y) override;
    virtual void on_mouse_down(int button) override;
    virtual void on_mouse_up(int button) override;
    virtual void on_mouse_wheel(int delta) override;

private:
    void ClearAllKeys();

    hb::shared::types::NativeWindowHandle m_hWnd;
    sf::RenderWindow* m_pRenderWindow;
    bool m_active;
    bool m_suppressed;

    // Mouse state
    int m_mouseX;
    int m_mouseY;
    int m_wheelDelta;
    bool m_mouseDown[3];     // Left, Right, Middle
    bool m_mousePressed[3];
    bool m_mouseReleased[3];

    // Keyboard state
    static constexpr int kKeyCount = 256;
    bool m_keyDown[kKeyCount];
    bool m_keyPressed[kKeyCount];
    bool m_keyReleased[kKeyCount];
};
