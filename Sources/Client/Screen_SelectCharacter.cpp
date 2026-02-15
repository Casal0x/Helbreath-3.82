// Screen_SelectCharacter.cpp: Select Character Screen Implementation
//
//////////////////////////////////////////////////////////////////////

#include "Screen_SelectCharacter.h"
#include "Game.h"
#include "TextInputManager.h"
#include "WeatherManager.h"
#include "GameModeManager.h"
#include "IInput.h"
#include "GlobalDef.h"
#include "ASIOSocket.h"
#include "Misc.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include <format>
#include <string>



using namespace hb::shared::net;
namespace MouseButton = hb::shared::input::MouseButton;

using namespace hb::shared::action;
using namespace hb::client::sprite_id;


Screen_SelectCharacter::Screen_SelectCharacter(CGame* game)
    : IGameScreen(game)
    , m_dwSelCharCTime(0)
    , m_sSelCharMsX(0)
    , m_sSelCharMsY(0)
    , m_cur_focus(1)
    , m_max_focus(4)
{
}

void Screen_SelectCharacter::on_initialize()
{
    // Set current mode for code that checks GameModeManager::get_mode()
    GameModeManager::set_current_mode(GameMode::SelectCharacter);

    // initialize logic (migrated from CGame::UpdateScreen_SelectCharacter m_cGameModeCount == 0 block)
    weather_manager::get().set_ambient_light(1);
    m_game->init_game_settings();
    
    m_cur_focus = 1;
    m_max_focus = 4;
    
    m_game->m_arrow_pressed = 0;
    m_dwSelCharCTime = GameClock::get_time_ms();
}

void Screen_SelectCharacter::on_uninitialize()
{
    text_input_manager::get().end_input();
}

void Screen_SelectCharacter::on_update()
{
    // Logic migrated from CGame::UpdateScreen_SelectCharacter
    short mouse_x, mouse_y, z;
    char lb, rb;
    uint32_t time = GameClock::get_time_ms();
    m_game->m_cur_time = time;

    // Handle legacy arrow input (if set by on_key_down) or direct input
    // NOTE: Preferring direct input for robustness
    if (hb::shared::input::is_key_pressed(KeyCode::Right)) {
        m_cur_focus++;
        if (m_cur_focus > m_max_focus) m_cur_focus = 1;
    }
    else if (hb::shared::input::is_key_pressed(KeyCode::Left)) {
        m_cur_focus--;
        if (m_cur_focus <= 0) m_cur_focus = m_max_focus;
    }
    

    if (hb::shared::input::is_key_pressed(KeyCode::Escape) == true)
    {
        m_game->change_game_mode(GameMode::MainMenu);
        return;
    }

    if (hb::shared::input::is_key_pressed(KeyCode::Enter) == true)
    {
        m_game->play_game_sound('E', 14, 5);

        if (m_game->m_char_list[m_cur_focus - 1] != nullptr)
        {
            if (enter_game()) return;
        }
        else
        {
            m_game->change_game_mode(GameMode::CreateNewCharacter);
            return;
        }
    }

    mouse_x = static_cast<short>(hb::shared::input::get_mouse_x());
    mouse_y = static_cast<short>(hb::shared::input::get_mouse_y());
    z = static_cast<short>(hb::shared::input::get_mouse_wheel_delta());
    lb = hb::shared::input::is_mouse_button_down(MouseButton::Left) ? 1 : 0;
    rb = hb::shared::input::is_mouse_button_down(MouseButton::Right) ? 1 : 0;
    
    m_sSelCharMsX = mouse_x;
    m_sSelCharMsY = mouse_y;

    if ((time - m_dwSelCharCTime) > 100)
    {
        m_game->m_menu_frame++;
        m_dwSelCharCTime = time;
    }
    if (m_game->m_menu_frame >= 8)
    {
        m_game->m_menu_dir_cnt++;
        if (m_game->m_menu_dir_cnt > 8)
        {
            m_game->m_menu_dir++;
            m_game->m_menu_dir_cnt = 1;
        }
        m_game->m_menu_frame = 0;
    }
    if (m_game->m_menu_dir > direction::northwest) m_game->m_menu_dir = direction::north;

    if (hb::shared::input::is_mouse_button_pressed(MouseButton::Left)) {

        // Determine which button was clicked
        int m_ibutton_num = 0;
        if (hb::shared::input::is_mouse_in_rect(100 + OX, 50 + OY, 110, 200))
        {
            m_game->play_game_sound('E', 14, 5);
            m_ibutton_num = 1;
        }
        else if (hb::shared::input::is_mouse_in_rect(211 + OX, 50 + OY, 110, 200))
        {
            m_game->play_game_sound('E', 14, 5);
            m_ibutton_num = 2;
        }
        else if (hb::shared::input::is_mouse_in_rect(322 + OX, 50 + OY, 109, 200))
        {
            m_game->play_game_sound('E', 14, 5);
            m_ibutton_num = 3;
        }
        else if (hb::shared::input::is_mouse_in_rect(432 + OX, 50 + OY, 110, 200))
        {
            m_game->play_game_sound('E', 14, 5);
            m_ibutton_num = 4;
        }
        else if (hb::shared::input::is_mouse_in_rect(360 + OX, 283 + OY, 185, 32))
        {
            m_game->play_game_sound('E', 14, 5);
            m_ibutton_num = 5;
        }
        else if (hb::shared::input::is_mouse_in_rect(360 + OX, 316 + OY, 185, 29))
        {
            m_game->play_game_sound('E', 14, 5);
            m_ibutton_num = 6;
        }
        else if (hb::shared::input::is_mouse_in_rect(360 + OX, 346 + OY, 185, 29))
        {
            m_game->play_game_sound('E', 14, 5);
            m_ibutton_num = 7;
        }
        else if (hb::shared::input::is_mouse_in_rect(360 + OX, 376 + OY, 185, 29))
        {
            m_game->play_game_sound('E', 14, 5);
            m_ibutton_num = 8;
        }
        else if (hb::shared::input::is_mouse_in_rect(360 + OX, 406 + OY, 185, 29))
        {
            m_game->play_game_sound('E', 14, 5);
            m_ibutton_num = 9;
        }

        switch (m_ibutton_num) {
        case 1:
        case 2:
        case 3:
        case 4:
            if (m_cur_focus != m_ibutton_num)
                m_cur_focus = m_ibutton_num;
            else
            {
                if (m_game->m_char_list[m_cur_focus - 1] != nullptr)
                {
                    if (enter_game()) return;
                }
                else
                {
                    m_game->change_game_mode(GameMode::CreateNewCharacter);
                    return;
                }
            }
            break;

        case 5:
            if (enter_game()) return;
            break;

        case 6:
            if (m_game->m_total_char < 4)
            {
                m_game->change_game_mode(GameMode::CreateNewCharacter);
                return;
            }
            break;

        case 7:
            if (m_game->m_char_list[m_cur_focus - 1] != nullptr)
            {
                m_game->change_game_mode(GameMode::QueryDeleteCharacter);
                m_game->m_enter_game_type = m_cur_focus;
                return;
            }
            break;

        case 8:
            m_game->change_game_mode(GameMode::ChangePassword);
            return;

        case 9:
            m_game->change_game_mode(GameMode::MainMenu);
            return;
        }
    }
}

bool Screen_SelectCharacter::enter_game()
{
    if (!m_game->m_char_list[m_cur_focus - 1]) return false;
    if (m_game->m_char_list[m_cur_focus - 1]->m_sex == 0) return false;

    m_game->m_player->m_player_name = m_game->m_char_list[m_cur_focus - 1]->m_name.c_str();
    m_game->m_player->m_level = static_cast<int>(m_game->m_char_list[m_cur_focus - 1]->m_level);
    if (!CMisc::check_valid_string(m_game->m_player->m_player_name.c_str())) return false;

    m_game->m_sprite[InterfaceNdLogin]->Unload();
    m_game->m_sprite[InterfaceNdMainMenu]->Unload();
    m_game->m_l_sock = std::make_unique<hb::shared::net::ASIOSocket>(m_game->m_io_pool->get_context(), game_limits::socket_block_limit);
    m_game->m_l_sock->connect(m_game->m_log_server_addr.c_str(), m_game->m_log_server_port);
    m_game->m_l_sock->init_buffer_size(hb::shared::limits::MsgBufferSize);
    m_game->change_game_mode(GameMode::Connecting);
    m_game->m_connect_mode = MsgId::request_enter_game;
    m_game->m_enter_game_type = EnterGameMsg::New;
    std::snprintf(m_game->m_msg, sizeof(m_game->m_msg), "%s", "33");
    m_game->m_map_name = m_game->m_char_list[m_cur_focus - 1]->m_map_name;
    return true;
}

void Screen_SelectCharacter::on_render()
{
    // Sync local focus to global focus to maintain compatibility with the static helper
    m_game->m_cur_focus = m_cur_focus;
    
    // Pass local state to static drawing method
    draw_background(m_game, 0, 10, m_sSelCharMsX, m_sSelCharMsY, false);
    
    m_game->draw_version();
}

// Static helper implementation
void Screen_SelectCharacter::draw_background(CGame* game, short sX, short sY, short mouse_x, short mouse_y, bool ignore_focus)
{
    // Logic migrated from CGame::UpdateScreen_OnSelectCharacter
    int i;
    int year, month, day, hour, minute;
    int64_t temp1, temp2;
    char total_char = 0;
    uint32_t time = GameClock::get_time_ms();
    sX = OX;
    sY = 10 + OY;
    game->draw_new_dialog_box(InterfaceNdSelectChar, 0, 0, 0);
    game->draw_new_dialog_box(InterfaceNdButton, OX, OY, 50);

    temp1 = 0;
    temp2 = 0;
    year = month = day = hour = minute = 0;
    
    // Use game->m_cur_focus as the source of truth for focus since this is shared
    int cur_focus = game->m_cur_focus;

    for (i = 0; i < 4; i++)
    {
        if ((cur_focus - 1 == i) && (ignore_focus == false))
            game->m_sprite[InterfaceNdButton]->draw(sX + 110 + i * 109 - 7, 63 - 9 + OY, 62);
        else game->m_sprite[InterfaceNdButton]->draw(sX + 110 + i * 109 - 7, 63 - 9 + OY, 61);

        if (game->m_char_list[i] != nullptr)
        {
            total_char++;
            switch (game->m_char_list[i]->m_sex) {
            case 1:	game->m_entity_state.m_owner_type = hb::shared::owner::MaleFirst; break;
            case 2:	game->m_entity_state.m_owner_type = hb::shared::owner::FemaleFirst; break;
            }
            game->m_entity_state.m_owner_type += game->m_char_list[i]->m_skin_color - 1;
            game->m_entity_state.m_dir = game->m_menu_dir;
            game->m_entity_state.m_appearance = game->m_char_list[i]->m_appearance;

            game->m_entity_state.m_name.fill('\0');
            std::snprintf(game->m_entity_state.m_name.data(), game->m_entity_state.m_name.size(), "%s", game->m_char_list[i]->m_name.c_str());
            
            game->m_entity_state.m_action = Type::Move;
            game->m_entity_state.m_frame = game->m_menu_frame;

            if (game->m_char_list[i]->m_sex != 0)
            {
                if (CMisc::check_valid_string(game->m_char_list[i]->m_name.data()) == true)
                {
                    game->m_effect_sprites[0]->draw(sX + 157 + i * 109, sY + 138, 1, hb::shared::sprite::DrawParams::additive_no_color_key(0.25f));
                    game->draw_object_on_move_for_menu(0, 0, sX + 157 + i * 109, sY + 138, false, time);
                    hb::shared::text::draw_text(GameFont::Default, sX + 112 + i * 109, sY + 179 - 9, game->m_char_list[i]->m_name.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UISelectPurple));
                    int	_sLevel = game->m_char_list[i]->m_level;
                    std::string charInfoBuf;
                    charInfoBuf = std::format("{}", _sLevel);
                    hb::shared::text::draw_text(GameFont::Default, sX + 138 + i * 109, sY + 196 - 10, charInfoBuf.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UISelectPurple));

                    charInfoBuf = game->format_comma_number(game->m_char_list[i]->m_exp);
                    hb::shared::text::draw_text(GameFont::Default, sX + 138 + i * 109, sY + 211 - 10, charInfoBuf.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UISelectPurple));
                }
                temp2 = (int64_t)game->m_char_list[i]->m_year * 1000000 + (int64_t)game->m_char_list[i]->m_month * 60000 + (int64_t)game->m_char_list[i]->m_day * 1700 + (int64_t)game->m_char_list[i]->m_hour * 70 + (int64_t)game->m_char_list[i]->m_minute;
                if (temp1 < temp2)
                {
                    year = game->m_char_list[i]->m_year;
                    month = game->m_char_list[i]->m_month;
                    day = game->m_char_list[i]->m_day;
                    hour = game->m_char_list[i]->m_hour;
                    minute = game->m_char_list[i]->m_minute;
                    temp1 = temp2;
                }
            }
        }
    }
    i = 0;

    game->draw_new_dialog_box(InterfaceNdButton, OX, OY, 51);
    game->draw_new_dialog_box(InterfaceNdButton, OX, OY, 52);
    game->draw_new_dialog_box(InterfaceNdButton, OX, OY, 53);
    game->draw_new_dialog_box(InterfaceNdButton, OX, OY, 54);
    game->draw_new_dialog_box(InterfaceNdButton, OX, OY, 55);

    if ((mouse_x > 360 + OX) && (mouse_y >= 283 + OY) && (mouse_x < 545 + OX) && (mouse_y <= 315 + OY)) {
        game->draw_new_dialog_box(InterfaceNdButton, OX, OY, 56);
        hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 290 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER1, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 305 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER2, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 320 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER3, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 335 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER4, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x > 360 + OX) && (mouse_y >= 316 + OY) && (mouse_x < 545 + OX) && (mouse_y <= 345 + OY)) {
        game->draw_new_dialog_box(InterfaceNdButton, OX, OY, 57);
        hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 305 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER5, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x > 360 + OX) && (mouse_y >= 346 + OY) && (mouse_x < 545 + OX) && (mouse_y <= 375 + OY)) {
        game->draw_new_dialog_box(InterfaceNdButton, OX, OY, 58);
        hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 275 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER6, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 290 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER7, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x > 360 + OX) && (mouse_y >= 376 + OY) && (mouse_x < 545 + OX) && (mouse_y <= 405 + OY)) {
        game->draw_new_dialog_box(InterfaceNdButton, OX, OY, 59);
        hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 305 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER12, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x > 360 + OX) && (mouse_y >= 406 + OY) && (mouse_x < 545 + OX) && (mouse_y <= 435 + OY)) {
        game->draw_new_dialog_box(InterfaceNdButton, OX, OY, 60);
        hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 305 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER13, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else {
        if (total_char == 0) {
            hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 275 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER14, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 290 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER15, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 305 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER16, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 320 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER17, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 335 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER18, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        }
        else if (total_char < 4) {
            hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 275 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER19, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 290 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER20, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 305 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER21, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 320 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER22, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 335 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER23, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 350 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER24, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        }
        if (total_char == 4) {
            hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 290 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER25, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 305 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER26, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 320 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER27, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 335 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER28, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        }
    }
    
    int temp_mon, temp_day, temp_hour, temp_min;
    std::string infoBuf;
    temp_mon = temp_day = temp_hour = temp_min = 0;

    if (game->m_accnt_year != 0)
    {
        temp_min = (game->m_time_left_sec_account / 60);
        infoBuf = std::format(UPDATE_SCREEN_ON_SELECT_CHARACTER37, game->m_accnt_year, game->m_accnt_month, game->m_accnt_day);
    }
    else
    {
        if (game->m_time_left_sec_account > 0)
        {
            temp_day = (game->m_time_left_sec_account / (60 * 60 * 24));
            temp_hour = (game->m_time_left_sec_account / (60 * 60)) % 24;
            temp_min = (game->m_time_left_sec_account / 60) % 60;
            infoBuf = std::format(UPDATE_SCREEN_ON_SELECT_CHARACTER38, temp_day, temp_hour, temp_min);
        }
        else infoBuf = UPDATE_SCREEN_ON_SELECT_CHARACTER39;
    }
    hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 385 + 10 + OY, (357) - (98), 15, infoBuf.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    if (game->m_ip_year != 0)
    {
        temp_hour = (game->m_time_left_sec_ip / (60 * 60));
        temp_min = (game->m_time_left_sec_ip / 60) % 60;
        infoBuf = std::format(UPDATE_SCREEN_ON_SELECT_CHARACTER40, temp_hour, temp_min);
    }
    else
    {
        if (game->m_time_left_sec_ip > 0)
        {
            temp_day = (game->m_time_left_sec_ip / (60 * 60 * 24));
            temp_hour = (game->m_time_left_sec_ip / (60 * 60)) % 24;
            temp_min = (game->m_time_left_sec_ip / 60) % 60;
            infoBuf = std::format(UPDATE_SCREEN_ON_SELECT_CHARACTER41, temp_day, temp_hour, temp_min);
        }
        else infoBuf = UPDATE_SCREEN_ON_SELECT_CHARACTER42;
    }
    hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 400 + 10 + OY, (357) - (98), 15, infoBuf.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    if (year != 0)
    {
        infoBuf = std::format(UPDATE_SCREEN_ON_SELECT_CHARACTER43, year, month, day, hour, minute);
        hb::shared::text::draw_text_aligned(GameFont::Default, 98 + OX, 415 + 10 + OY, (357) - (98), 15, infoBuf.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }

    hb::shared::text::draw_text_aligned(GameFont::Default, 122 + OX, 456 + OY, (315) - (122), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER36, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
}
