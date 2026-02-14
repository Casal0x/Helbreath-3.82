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

    explicit Screen_MainMenu(CGame* game);
    ~Screen_MainMenu() override = default;

    void on_initialize() override;
    void on_uninitialize() override;
    void on_update() override;
    void on_render() override;

private:
    char m_cur_focus;
    char m_max_focus;
};
