// IInput.h: Raylib-style input interface
//
// Provides static global access to input state with frame-based semantics.
// Each engine (DDraw, SFML) implements its own input handling.
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>

// Mouse button constants (same values as IWindow.h - use guards to avoid redefinition)
#ifndef MOUSE_BUTTON_LEFT
#define MOUSE_BUTTON_LEFT   0
#endif
#ifndef MOUSE_BUTTON_RIGHT
#define MOUSE_BUTTON_RIGHT  1
#endif
#ifndef MOUSE_BUTTON_MIDDLE
#define MOUSE_BUTTON_MIDDLE 2
#endif

// ============== Abstract Key Codes ==============
// Engine-agnostic key codes - each engine maps its native keys to these
// This avoids Windows-specific VK_* codes in game logic
enum class KeyCode : int
{
    // Invalid/unknown key
    Unknown = 0,

    // Letters (A-Z: 65-90, matches ASCII)
    A = 'A', B = 'B', C = 'C', D = 'D', E = 'E', F = 'F', G = 'G', H = 'H',
    I = 'I', J = 'J', K = 'K', L = 'L', M = 'M', N = 'N', O = 'O', P = 'P',
    Q = 'Q', R = 'R', S = 'S', T = 'T', U = 'U', V = 'V', W = 'W', X = 'X',
    Y = 'Y', Z = 'Z',

    // Numbers (0-9: 48-57, matches ASCII)
    Num0 = '0', Num1 = '1', Num2 = '2', Num3 = '3', Num4 = '4',
    Num5 = '5', Num6 = '6', Num7 = '7', Num8 = '8', Num9 = '9',

    // Function keys (start at 112 to match VK_F1)
    F1 = 112, F2 = 113, F3 = 114, F4 = 115, F5 = 116, F6 = 117,
    F7 = 118, F8 = 119, F9 = 120, F10 = 121, F11 = 122, F12 = 123,

    // Navigation (match Windows VK codes for compatibility)
    Insert = 45,
    Delete = 46,
    Home = 36,
    End = 35,
    PageUp = 33,
    PageDown = 34,

    // Arrow keys
    Left = 37,
    Up = 38,
    Right = 39,
    Down = 40,

    // Special keys
    Escape = 27,
    Enter = 13,
    Tab = 9,
    Backspace = 8,
    Space = 32,

    // Modifiers
    LShift = 160,
    RShift = 161,
    LControl = 162,
    RControl = 163,
    LAlt = 164,
    RAlt = 165,

    // Numpad
    Numpad0 = 96, Numpad1 = 97, Numpad2 = 98, Numpad3 = 99, Numpad4 = 100,
    Numpad5 = 101, Numpad6 = 102, Numpad7 = 103, Numpad8 = 104, Numpad9 = 105,

    // Numpad operators
    NumpadMultiply = 106,  // *
    NumpadAdd = 107,       // +
    NumpadSeparator = 108,
    NumpadSubtract = 109,  // -
    NumpadDecimal = 110,   // .
    NumpadDivide = 111,    // /

    // Lock keys
    NumLock = 144,
    ScrollLock = 145,

    // Windows keys
    LWin = 91,
    RWin = 92,
};

// Abstract input interface
class IInput
{
public:
    virtual ~IInput() = default;

    // ============== Frame Management ==============
    // Called at start of frame to reset pressed/released states
    virtual void BeginFrame() = 0;

    // ============== Keyboard ==============
    virtual bool IsKeyDown(int key) const = 0;      // Currently held
    virtual bool IsKeyPressed(int key) const = 0;   // Just pressed this frame
    virtual bool IsKeyReleased(int key) const = 0;  // Just released this frame

    // ============== Mouse Buttons ==============
    virtual bool IsMouseButtonDown(int button) const = 0;
    virtual bool IsMouseButtonPressed(int button) const = 0;
    virtual bool IsMouseButtonReleased(int button) const = 0;

    // ============== Mouse Position ==============
    // Returns position in logical game coordinates (0-639, 0-479)
    virtual int GetMouseX() const = 0;
    virtual int GetMouseY() const = 0;

    // ============== Mouse Wheel ==============
    // Returns accumulated delta since last frame
    virtual int GetMouseWheelDelta() const = 0;

    // ============== Modifier Keys ==============
    virtual bool IsShiftDown() const = 0;
    virtual bool IsCtrlDown() const = 0;
    virtual bool IsAltDown() const = 0;

    // ============== Window Focus ==============
    virtual bool IsWindowActive() const = 0;
    virtual void SetWindowActive(bool active) = 0;

    // ============== Input Events ==============
    // Called by window/event handler to feed input
    virtual void OnKeyDown(int key) = 0;
    virtual void OnKeyUp(int key) = 0;
    virtual void OnMouseMove(int x, int y) = 0;
    virtual void OnMouseDown(int button) = 0;
    virtual void OnMouseUp(int button) = 0;
    virtual void OnMouseWheel(int delta) = 0;
};

// ============== Global Input System ==============
namespace Input {
    // Lifecycle - called by engine
    void Create();
    void Destroy();

    // Get the implementation
    IInput* Get();

    // ============== Static Convenience Functions (Raylib style) ==============

    // Keyboard
    inline bool IsKeyDown(int key) { return Get()->IsKeyDown(key); }
    inline bool IsKeyPressed(int key) { return Get()->IsKeyPressed(key); }
    inline bool IsKeyReleased(int key) { return Get()->IsKeyReleased(key); }

    // Mouse buttons
    inline bool IsMouseButtonDown(int btn) { return Get()->IsMouseButtonDown(btn); }
    inline bool IsMouseButtonPressed(int btn) { return Get()->IsMouseButtonPressed(btn); }
    inline bool IsMouseButtonReleased(int btn) { return Get()->IsMouseButtonReleased(btn); }

    // Mouse position
    inline int GetMouseX() { return Get()->GetMouseX(); }
    inline int GetMouseY() { return Get()->GetMouseY(); }
    inline int GetMouseWheelDelta() { return Get()->GetMouseWheelDelta(); }

    // Modifier keys
    inline bool IsShiftDown() { return Get()->IsShiftDown(); }
    inline bool IsCtrlDown() { return Get()->IsCtrlDown(); }
    inline bool IsAltDown() { return Get()->IsAltDown(); }

    // Window state
    inline bool IsWindowActive() { return Get()->IsWindowActive(); }

    // Frame management
    inline void BeginFrame() { Get()->BeginFrame(); }

    // ============== Hit-Testing Helpers (replaces CMouseInterface) ==============

    // Check if point is inside rectangle (exclusive right/bottom)
    inline bool PointInRect(int x, int y, int left, int top, int right, int bottom) {
        return x >= left && x < right && y >= top && y < bottom;
    }

    // Check if mouse is inside rectangle
    inline bool IsMouseInRect(int left, int top, int right, int bottom) {
        return PointInRect(GetMouseX(), GetMouseY(), left, top, right, bottom);
    }

    // Check if left click occurred inside rectangle this frame
    inline bool IsClickInRect(int left, int top, int right, int bottom) {
        return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && IsMouseInRect(left, top, right, bottom);
    }

    // Check if right click occurred inside rectangle this frame
    inline bool IsRightClickInRect(int left, int top, int right, int bottom) {
        return IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && IsMouseInRect(left, top, right, bottom);
    }
}
