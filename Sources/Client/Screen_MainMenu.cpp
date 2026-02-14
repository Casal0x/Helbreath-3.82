// Screen_MainMenu.cpp: Main Menu Screen implementation
//
//////////////////////////////////////////////////////////////////////

#include "Screen_MainMenu.h"
#include "Game.h"
#include "TextInputManager.h"
#include "GameModeManager.h"
#include "IInput.h"
#include "GlobalDef.h"
#include "Log.h"
using namespace hb::client::sprite_id;

namespace MouseButton = hb::shared::input::MouseButton;

Screen_MainMenu::Screen_MainMenu(CGame* game)
    : IGameScreen(game), m_cur_focus(1), m_max_focus(3)
{
}

void Screen_MainMenu::on_initialize()
{
    // Set current mode for code that checks GameModeManager::get_mode()
    GameModeManager::set_current_mode(GameMode::MainMenu);

    // Note: Sprite removal logic (m_sprite.remove) is better handled by resource management,
    // but preserving legacy behavior for now.
    // InterfaceNdLoading removal was in original code.
    // m_game->m_sprite.remove(InterfaceNdLoading); // Keeping strict to original if needed? 
    // Actually, let's keep it safe.
    m_game->m_sprite.remove(InterfaceNdLoading);
    
    text_input_manager::get().end_input();

    m_cur_focus = 1;
    m_max_focus = 3;
    m_game->m_arrow_pressed = 0;
}

void Screen_MainMenu::on_uninitialize()
{
    // Nothing specific to clean up
}

void Screen_MainMenu::on_update()
{
    // Poll mouse input
    uint32_t time = GameClock::get_time_ms();
    m_game->m_cur_time = time;

    // update focus based on mouse position
    if ((hb::shared::input::get_mouse_x() >= 465) && (hb::shared::input::get_mouse_y() >= 238) && (hb::shared::input::get_mouse_x() <= 465+164) && (hb::shared::input::get_mouse_y() <= 238+22)) m_cur_focus = 1;
    if ((hb::shared::input::get_mouse_x() >= 465) && (hb::shared::input::get_mouse_y() >= 276) && (hb::shared::input::get_mouse_x() <= 465+164) && (hb::shared::input::get_mouse_y() <= 276+22)) m_cur_focus = 2;
    if ((hb::shared::input::get_mouse_x() >= 465) && (hb::shared::input::get_mouse_y() >= 315) && (hb::shared::input::get_mouse_x() <= 465+164) && (hb::shared::input::get_mouse_y() <= 315 +22)) m_cur_focus = 3;

    if (m_game->m_arrow_pressed != 0) {
        switch (m_game->m_arrow_pressed) {
        case 1:
            m_cur_focus--;
            if (m_cur_focus <= 0) m_cur_focus = m_max_focus;
            break;
        case 3:
            m_cur_focus++;
            if (m_cur_focus > m_max_focus) m_cur_focus = 1;
            break;
        }
        m_game->m_arrow_pressed = 0;
    }

    // Handle Tab key
    if (hb::shared::input::is_key_pressed(KeyCode::Tab))
    {
        if (hb::shared::input::is_shift_down())
        {
            m_game->play_game_sound('E', 14, 5);
            m_cur_focus--;
            if (m_cur_focus <= 0) m_cur_focus = m_max_focus;
        }
        else
        {
            m_game->play_game_sound('E', 14, 5);
            m_cur_focus++;
            if (m_cur_focus > m_max_focus) m_cur_focus = 1;
        }
    }

    if (hb::shared::input::is_key_pressed(KeyCode::Enter) == true) {
        switch (m_cur_focus) {
        case 1:
            m_game->play_game_sound('E', 14, 5);
            m_game->change_game_mode(GameMode::Login);
            return;
        case 2:
            m_game->play_game_sound('E', 14, 5);
            m_game->change_game_mode(GameMode::CreateNewAccount);
            return;
        case 3:
            m_game->play_game_sound('E', 14, 5);
            m_game->change_game_mode(GameMode::Quit);
            return;
        }
    }

    // Mouse click detection
    if (hb::shared::input::is_mouse_button_pressed(MouseButton::Left)) {
        // Game button
        if (hb::shared::input::is_mouse_in_rect(465, 238, 164, 22)) {
            m_game->play_game_sound('E', 14, 5);
            m_cur_focus = 1;
            m_game->change_game_mode(GameMode::Login);
            return;
        }
        // Account button
        else if (hb::shared::input::is_mouse_in_rect(465, 276, 164, 22)) {
            m_game->play_game_sound('E', 14, 5);
            m_cur_focus = 2;
            m_game->change_game_mode(GameMode::CreateNewAccount);
            return;
        }
        // Quit button
        else if (hb::shared::input::is_mouse_in_rect(465, 315, 164, 22)) {
            m_game->play_game_sound('E', 14, 5);
            m_cur_focus = 3;
            m_game->change_game_mode(GameMode::Quit);
            return;
        }
    }
}

void Screen_MainMenu::on_render()
{
    m_game->draw_new_dialog_box(InterfaceNdMainMenu, 0, 0, 0, true);

    switch (m_cur_focus) {
    case 1:
        m_game->m_sprite[InterfaceNdMainMenu]->draw(465, 238, 1);
        break;
    case 2:
        m_game->m_sprite[InterfaceNdMainMenu]->draw(465, 276, 2);
        break;
    case 3:
        m_game->m_sprite[InterfaceNdMainMenu]->draw(465, 315, 3);
        break;
    }

    m_game->draw_version();
}
