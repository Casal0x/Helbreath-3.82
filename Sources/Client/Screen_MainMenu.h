// Screen_MainMenu.h: Main Menu Screen
//
// Handles the initial menu (Login, New Account, Quit)
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IGameScreen.h"

class Screen_MainMenu : public IGameScreen
{
public:
    SCREEN_TYPE(Screen_MainMenu)

    explicit Screen_MainMenu(CGame* pGame);
    ~Screen_MainMenu() override = default;

    void on_initialize() override;
    void on_uninitialize() override;
    void on_update() override;
    void on_render() override;

private:
    char m_cCurFocus;
    char m_cMaxFocus;
};
