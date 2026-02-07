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

    void Initialize(NativeWindowHandle hWnd);
    void SetRenderWindow(sf::RenderWindow* pWindow);

    // ============== IInput Implementation ==============

    // Frame management
    virtual void BeginFrame() override;

    // Keyboard
    virtual bool IsKeyDown(KeyCode key) const override;
    virtual bool IsKeyPressed(KeyCode key) const override;
    virtual bool IsKeyReleased(KeyCode key) const override;

    // Mouse buttons
    virtual bool IsMouseButtonDown(int button) const override;
    virtual bool IsMouseButtonPressed(int button) const override;
    virtual bool IsMouseButtonReleased(int button) const override;

    // Mouse position (already in logical coordinates from SFMLWindow)
    virtual int GetMouseX() const override;
    virtual int GetMouseY() const override;

    // Mouse wheel
    virtual int GetMouseWheelDelta() const override;

    // Modifier keys
    virtual bool IsShiftDown() const override;
    virtual bool IsCtrlDown() const override;
    virtual bool IsAltDown() const override;

    // Window focus
    virtual bool IsWindowActive() const override;
    virtual void SetWindowActive(bool active) override;

    // Input suppression
    virtual void SetSuppressed(bool suppressed) override;
    virtual bool IsSuppressed() const override;

    // Input events (called by SFMLWindow with logical coordinates)
    virtual void OnKeyDown(KeyCode key) override;
    virtual void OnKeyUp(KeyCode key) override;
    virtual void OnMouseMove(int x, int y) override;
    virtual void OnMouseDown(int button) override;
    virtual void OnMouseUp(int button) override;
    virtual void OnMouseWheel(int delta) override;

private:
    void ClearAllKeys();

    NativeWindowHandle m_hWnd;
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
