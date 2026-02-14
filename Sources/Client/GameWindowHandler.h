// GameWindowHandler.h: hb::shared::render::Window event handler adapter for CGame
//
// Implements hb::shared::render::IWindowEventHandler and routes events to CGame methods
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IWindow.h"

// Forward declarations
class CGame;

class GameWindowHandler : public hb::shared::render::IWindowEventHandler
{
public:
    GameWindowHandler(CGame* game);
    virtual ~GameWindowHandler() = default;

    // ============== hb::shared::render::IWindowEventHandler Implementation ==============

    // hb::shared::render::Window Events
    virtual void on_close() override;
    virtual void on_destroy() override;
    virtual void on_activate(bool active) override;
    virtual void on_resize(int width, int height) override;

    // Input Events
    virtual void on_key_down(KeyCode key) override;
    virtual void on_key_up(KeyCode key) override;
    virtual void on_char(char character) override;
    virtual void on_mouse_move(int x, int y) override;
    virtual void on_mouse_button_down(int button, int x, int y) override;
    virtual void on_mouse_button_up(int button, int x, int y) override;
    virtual void on_mouse_wheel(int delta, int x, int y) override;

    // Custom Messages
    virtual bool on_custom_message(uint32_t message, uintptr_t wparam, intptr_t lparam) override;

    // Text Input
    virtual bool on_text_input(hb::shared::types::NativeWindowHandle hwnd, uint32_t message, uintptr_t wparam, intptr_t lparam) override;

private:
    CGame* m_game;
};
