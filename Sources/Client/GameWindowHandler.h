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
    GameWindowHandler(CGame* pGame);
    virtual ~GameWindowHandler() = default;

    // ============== hb::shared::render::IWindowEventHandler Implementation ==============

    // hb::shared::render::Window Events
    virtual void OnClose() override;
    virtual void OnDestroy() override;
    virtual void OnActivate(bool active) override;
    virtual void OnResize(int width, int height) override;

    // Input Events
    virtual void OnKeyDown(KeyCode keyCode) override;
    virtual void OnKeyUp(KeyCode keyCode) override;
    virtual void OnChar(char character) override;
    virtual void OnMouseMove(int x, int y) override;
    virtual void OnMouseButtonDown(int button, int x, int y) override;
    virtual void OnMouseButtonUp(int button, int x, int y) override;
    virtual void OnMouseWheel(int delta, int x, int y) override;

    // Custom Messages
    virtual bool OnCustomMessage(uint32_t message, uintptr_t wParam, intptr_t lParam) override;

    // Text Input
    virtual bool OnTextInput(hb::shared::types::NativeWindowHandle hWnd, uint32_t message, uintptr_t wParam, intptr_t lParam) override;

private:
    CGame* m_pGame;
};
