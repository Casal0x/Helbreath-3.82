// Screen_Login.cpp: Login Screen Implementation
//
//////////////////////////////////////////////////////////////////////

#include "Screen_Login.h"
#include "Game.h"
#include "TextInputManager.h"
#include "GameModeManager.h"
#include "IInput.h"
#include "GlobalDef.h"
#include "ASIOSocket.h" // For XSocket
#include "Misc.h"    // For CMisc
#include "GameFonts.h"
#include "TextLibExt.h"
#include <string>


using namespace hb::shared::net;
using namespace hb::client::sprite_id;
namespace MouseButton = hb::shared::input::MouseButton;

Screen_Login::Screen_Login(CGame* game)
    : IGameScreen(game), m_cPrevFocus(0)
{
}

void Screen_Login::on_initialize()
{
    // Set current mode for code that checks GameModeManager::get_mode()
    GameModeManager::set_current_mode(GameMode::Login);

    text_input_manager::get().end_input();
    m_cPrevFocus = 1;
    m_cur_focus = 1;
    m_max_focus = 4;
    m_game->m_arrow_pressed = 0;
    
    m_cLoginName.clear();
    m_cLoginPassword.clear();
    
    text_input_manager::get().start_input(234, 222, 11, m_cLoginName);
    text_input_manager::get().clear_input();
}

void Screen_Login::on_uninitialize()
{
    text_input_manager::get().end_input();
}

void Screen_Login::on_update()
{
// Polling logic migrated from CGame::UpdateScreen_Login
    uint32_t time = GameClock::get_time_ms();
    m_game->m_cur_time = time;

    // Explicit TAB handling since legacy on_key_down ignores it
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

    if (m_game->m_arrow_pressed != 0)
    {
        switch (m_game->m_arrow_pressed) {
        case 1:
            m_cur_focus--;
            if (m_cur_focus <= 0) m_cur_focus = m_max_focus;
            break;
        case 2:
            if (m_cur_focus == 3) m_cur_focus = 4;
            else if (m_cur_focus == 4) m_cur_focus = 3;
            break;
        case 3:
            m_cur_focus++;
            if (m_cur_focus > m_max_focus) m_cur_focus = 1;
            break;
        case 4:
            if (m_cur_focus == 3) m_cur_focus = 4;
            else if (m_cur_focus == 4) m_cur_focus = 3;
            break;
        }
         m_game->m_arrow_pressed = 0;
    }

    if (hb::shared::input::is_key_pressed(KeyCode::Enter) == true)
    {
        switch (m_cur_focus) {
        case 1:
            m_game->play_game_sound('E', 14, 5);
            m_cur_focus++;
            if (m_cur_focus > m_max_focus) m_cur_focus = 1;
            break;
        case 2:
        case 3:
            m_game->play_game_sound('E', 14, 5);
            if (attempt_login()) return;
            break;
        case 4:
            m_game->change_game_mode(GameMode::MainMenu);
            return;
        }
    }

    if (hb::shared::input::is_key_pressed(KeyCode::Escape) == true)
    {
        m_game->play_game_sound('E', 14, 5);
        text_input_manager::get().end_input();
        m_game->change_game_mode(GameMode::MainMenu);
        return;
    }

    if (m_cPrevFocus != m_cur_focus)
    {
         text_input_manager::get().end_input();
        switch (m_cur_focus) {
        case 1:
            text_input_manager::get().start_input(234, 222, 11, m_cLoginName);
            break;
        case 2:
            text_input_manager::get().start_input(234, 245, 11, m_cLoginPassword, true);
            break;
        case 3:
        case 4:
            break;
        }
        m_cPrevFocus = m_cur_focus;
    }

    // Mouse click detection
    if (hb::shared::input::is_mouse_button_pressed(MouseButton::Left))
    {
        // Name field click
        if (hb::shared::input::is_mouse_in_rect(234, 221, 147, 17)) {
            m_game->play_game_sound('E', 14, 5);
            m_cur_focus = 1;
        }
        // Password field click
        else if (hb::shared::input::is_mouse_in_rect(234, 244, 147, 17)) {
            m_game->play_game_sound('E', 14, 5);
            m_cur_focus = 2;
        }
        // Login button click
        else if (hb::shared::input::is_mouse_in_rect(140, 343, 84, 20)) {
            m_game->play_game_sound('E', 14, 5);
            if (attempt_login()) return;
        }
        // Cancel button click
        else if (hb::shared::input::is_mouse_in_rect(316, 343, 76, 20)) {
            m_game->play_game_sound('E', 14, 5);
            m_game->change_game_mode(GameMode::MainMenu);
            return;
        }
    }

    if (hb::shared::input::is_mouse_in_rect(140, 343, 84, 20)) m_cur_focus = 3;
    if (hb::shared::input::is_mouse_in_rect(316, 343, 76, 20)) m_cur_focus = 4;
}

bool Screen_Login::attempt_login()
{
    if (m_cLoginName.empty() || m_cLoginPassword.empty()) return false;

    text_input_manager::get().end_input();
    m_game->m_player->m_account_name = m_cLoginName.c_str();
    m_game->m_player->m_account_password = m_cLoginPassword.c_str();

    m_game->m_l_sock = std::make_unique<hb::shared::net::ASIOSocket>(m_game->m_io_pool->get_context(), game_limits::socket_block_limit);
    m_game->m_l_sock->connect(m_game->m_log_server_addr.c_str(), m_game->m_log_server_port);
    m_game->m_l_sock->init_buffer_size(hb::shared::limits::MsgBufferSize);

    m_game->change_game_mode(GameMode::Connecting);
    m_game->m_connect_mode = MsgId::request_login;
    std::snprintf(m_game->m_msg, sizeof(m_game->m_msg), "%s", "11");
    return true;
}

void Screen_Login::on_render()
{
    draw_login_window(hb::shared::input::get_mouse_x(), hb::shared::input::get_mouse_y());
}

// Logic migrated from CGame::_Draw_OnLogin
void Screen_Login::draw_login_window(int mouse_x, int mouse_y)
{
    bool flag = true;
    m_game->draw_new_dialog_box(InterfaceNdLogin, 0, 0, 0, true);
    m_game->draw_version();

    // Smooth alpha fade-in for login box: 0-500ms delay, then 500-700ms fade from 0 to 1
    static constexpr uint32_t FADE_DELAY_MS = 500;
    static constexpr uint32_t FADE_DURATION_MS = 200;

    uint32_t elapsedMs = get_elapsed_ms();
    if (elapsedMs > FADE_DELAY_MS) {
        float fadeProgress = static_cast<float>(elapsedMs - FADE_DELAY_MS) / FADE_DURATION_MS;
        float alpha = fadeProgress > 1.0f ? 1.0f : fadeProgress;
        m_game->m_sprite[InterfaceNdLogin]->draw(99, 182, 2, hb::shared::sprite::DrawParams::alpha_blend(alpha));
    }

    if (m_cur_focus != 1) {
        if (CMisc::check_valid_name(m_cLoginName.data()) != false)
            hb::shared::text::draw_text(GameFont::Default, 234, 222, m_cLoginName.c_str(), hb::shared::text::TextStyle::with_shadow(GameColors::InputValid));
        else hb::shared::text::draw_text(GameFont::Default, 234, 222, m_cLoginName.c_str(), hb::shared::text::TextStyle::with_shadow(GameColors::InputInvalid));
    }
    if ((CMisc::check_valid_name(m_cLoginName.data()) == false) || m_cLoginName.empty()) flag = false;

    if (m_cur_focus != 2) {
        // Mask password with asterisks
        std::string masked(m_cLoginPassword.size(), '*');
        if ((CMisc::check_valid_string(m_cLoginPassword.data()) != false))
            hb::shared::text::draw_text(GameFont::Default, 234, 245, masked.c_str(),
                              hb::shared::text::TextStyle::with_shadow(GameColors::InputValid));
        else
            hb::shared::text::draw_text(GameFont::Default, 234, 245, masked.c_str(),
                              hb::shared::text::TextStyle::with_shadow(GameColors::InputInvalid));
    }
    if ((CMisc::check_valid_string(m_cLoginPassword.data()) == false) || m_cLoginPassword.empty()) flag = false;

    if (m_cur_focus == 1)
        text_input_manager::get().show_input();
    else
        if (m_cur_focus == 2)
            text_input_manager::get().show_input();

    if (flag == true)
    {
        if (m_cur_focus == 3) m_game->draw_new_dialog_box(InterfaceNdLogin, 140, 343, 3, true);
    }
    if (m_cur_focus == 4) m_game->draw_new_dialog_box(InterfaceNdLogin, 316, 343, 4, true);
}
