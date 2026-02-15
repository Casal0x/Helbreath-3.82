// Screen_CreateNewCharacter.cpp: Create New Character Screen Implementation
//
//////////////////////////////////////////////////////////////////////

#include "Screen_CreateNewCharacter.h"
#include "Game.h"
#include "TextInputManager.h"
#include "GameModeManager.h"
#include "IInput.h"
#include "GlobalDef.h"
#include "ASIOSocket.h"
#include "Misc.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include <format>



using namespace hb::shared::net;
namespace MouseButton = hb::shared::input::MouseButton;

using namespace hb::shared::action;
using namespace hb::client::sprite_id;

Screen_CreateNewCharacter::Screen_CreateNewCharacter(CGame* game)
    : IGameScreen(game)
    , m_iNewCharPoint(10)
    , m_cNewCharPrevFocus(1)
    , m_dwNewCharMTime(0)
    , m_sNewCharMsX(0)
    , m_sNewCharMsY(0)
    , m_bNewCharFlag(false)
    , m_cur_focus(1)
    , m_max_focus(6)
{
}

void Screen_CreateNewCharacter::on_initialize()
{
    // Set current mode for code that checks GameModeManager::get_mode()
    GameModeManager::set_current_mode(GameMode::CreateNewCharacter);

    // initialize character creation defaults
    m_game->m_player->m_gender = rand() % 2 + 1;
    m_game->m_player->m_skin_col = rand() % 3 + 1;
    m_game->m_player->m_hair_style = rand() % 8;
    m_game->m_player->m_hair_col = rand() % 16;
    m_game->m_player->m_under_col = rand() % 8;
    m_game->m_player->m_stat_mod_str = 10;
    m_game->m_player->m_stat_mod_vit = 10;
    m_game->m_player->m_stat_mod_dex = 10;
    m_game->m_player->m_stat_mod_int = 10;
    m_game->m_player->m_stat_mod_mag = 10;
    m_game->m_player->m_stat_mod_chr = 10;

    m_iNewCharPoint = 10; // 70 - (6 stats * 10) = 10 bonus points
    m_cNewCharPrevFocus = 1;
    m_cur_focus = 1;
    m_max_focus = 6;
    m_game->m_arrow_pressed = 0;
    m_dwNewCharMTime = GameClock::get_time_ms();
    m_cNewCharName.clear();
    text_input_manager::get().start_input(193 + 4 + OX, 65 + 45 + OY, 11, m_cNewCharName);
    text_input_manager::get().clear_input();
}

void Screen_CreateNewCharacter::on_uninitialize()
{
    text_input_manager::get().end_input();
}

void Screen_CreateNewCharacter::on_update()
{
    uint32_t time = GameClock::get_time_ms();

    // Handle arrow key navigation
    if (hb::shared::input::is_key_pressed(KeyCode::Up)) {
        m_cur_focus--;
        if (m_cur_focus <= 0) m_cur_focus = m_max_focus;
    }
    else if (hb::shared::input::is_key_pressed(KeyCode::Down)) {
        m_cur_focus++;
        if (m_cur_focus > m_max_focus) m_cur_focus = 1;
    }

    // Handle focus change for input string
    if (m_cNewCharPrevFocus != m_cur_focus) {
        text_input_manager::get().end_input();
        switch (m_cur_focus) {
        case 1:
            text_input_manager::get().start_input(193 + 4 + OX, 65 + 45 + OY, 11, m_cNewCharName);
            break;
        }
        m_cNewCharPrevFocus = m_cur_focus;
    }

    // ESC returns to character select
    if (hb::shared::input::is_key_pressed(KeyCode::Escape)) {
        m_game->change_game_mode(GameMode::SelectCharacter);
        return;
    }

    // Capture mouse position
    short mouse_x = static_cast<short>(hb::shared::input::get_mouse_x());
    short mouse_y = static_cast<short>(hb::shared::input::get_mouse_y());
    m_sNewCharMsX = mouse_x;
    m_sNewCharMsY = mouse_y;

    // Compute whether character creation is valid
    m_bNewCharFlag = true;
    if (m_cNewCharName.empty()) m_bNewCharFlag = false;
    if (m_iNewCharPoint > 0) m_bNewCharFlag = false;
    if (CMisc::check_valid_name(m_cNewCharName.data()) == false) m_bNewCharFlag = false;

    // Animation frame updates
    if ((time - m_dwNewCharMTime) > 100)
    {
        m_game->m_menu_frame++;
        m_dwNewCharMTime = time;
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

    // Handle button hover focus
    if ((mouse_x >= 384 + OX) && (mouse_x <= 384 + 72 + OX) && (mouse_y >= 445 + OY) && (mouse_y <= 445 + 15 + OY)) {
        m_cur_focus = 2;
    }
    else if ((mouse_x >= 500 + OX) && (mouse_x <= 500 + 72 + OX) && (mouse_y >= 445 + OY) && (mouse_y <= 445 + 15 + OY)) {
        m_cur_focus = 3;
    }
    else if ((mouse_x >= 60 + OX) && (mouse_x <= 60 + 72 + OX) && (mouse_y >= 445 + OY) && (mouse_y <= 445 + 15 + OY)) {
        m_cur_focus = 4;
    }
    else if ((mouse_x >= 145 + OX) && (mouse_x <= 145 + 72 + OX) && (mouse_y >= 445 + OY) && (mouse_y <= 445 + 15 + OY)) {
        m_cur_focus = 5;
    }
    else if ((mouse_x >= 230 + OX) && (mouse_x <= 230 + 72 + OX) && (mouse_y >= 445 + OY) && (mouse_y <= 445 + 15 + OY)) {
        m_cur_focus = 6;
    }

    // Handle mouse clicks
    if (hb::shared::input::is_mouse_button_pressed(MouseButton::Left))
    {
        m_game->play_game_sound('E', 14, 5);

        // Determine which button was clicked
        int m_ibutton_num = 0;
        if (hb::shared::input::is_mouse_in_rect(69 + OX, 110 + OY, 210, 17)) m_ibutton_num = 1;  // Name
        else if (hb::shared::input::is_mouse_in_rect(236 + OX, 156 + OY, 21, 13)) m_ibutton_num = 2;  // Gender -
        else if (hb::shared::input::is_mouse_in_rect(259 + OX, 156 + OY, 21, 13)) m_ibutton_num = 3;  // Gender +
        else if (hb::shared::input::is_mouse_in_rect(236 + OX, 171 + OY, 21, 13)) m_ibutton_num = 4;  // Skin -
        else if (hb::shared::input::is_mouse_in_rect(259 + OX, 171 + OY, 21, 13)) m_ibutton_num = 5;  // Skin +
        else if (hb::shared::input::is_mouse_in_rect(236 + OX, 186 + OY, 21, 13)) m_ibutton_num = 6;  // Hair style -
        else if (hb::shared::input::is_mouse_in_rect(259 + OX, 186 + OY, 21, 13)) m_ibutton_num = 7;  // Hair style +
        else if (hb::shared::input::is_mouse_in_rect(236 + OX, 201 + OY, 21, 13)) m_ibutton_num = 8;  // Hair color -
        else if (hb::shared::input::is_mouse_in_rect(259 + OX, 201 + OY, 21, 13)) m_ibutton_num = 9;  // Hair color +
        else if (hb::shared::input::is_mouse_in_rect(236 + OX, 216 + OY, 21, 13)) m_ibutton_num = 10; // Underwear -
        else if (hb::shared::input::is_mouse_in_rect(259 + OX, 216 + OY, 21, 13)) m_ibutton_num = 11; // Underwear +
        else if (hb::shared::input::is_mouse_in_rect(236 + OX, 276 + OY, 21, 13)) m_ibutton_num = 12; // Str +
        else if (hb::shared::input::is_mouse_in_rect(259 + OX, 276 + OY, 21, 13)) m_ibutton_num = 13; // Str -
        else if (hb::shared::input::is_mouse_in_rect(236 + OX, 291 + OY, 21, 13)) m_ibutton_num = 14; // Vit +
        else if (hb::shared::input::is_mouse_in_rect(259 + OX, 291 + OY, 21, 13)) m_ibutton_num = 15; // Vit -
        else if (hb::shared::input::is_mouse_in_rect(236 + OX, 306 + OY, 21, 13)) m_ibutton_num = 16; // Dex +
        else if (hb::shared::input::is_mouse_in_rect(259 + OX, 306 + OY, 21, 13)) m_ibutton_num = 17; // Dex -
        else if (hb::shared::input::is_mouse_in_rect(236 + OX, 321 + OY, 21, 13)) m_ibutton_num = 18; // Int +
        else if (hb::shared::input::is_mouse_in_rect(259 + OX, 321 + OY, 21, 13)) m_ibutton_num = 19; // Int -
        else if (hb::shared::input::is_mouse_in_rect(236 + OX, 336 + OY, 21, 13)) m_ibutton_num = 20; // Mag +
        else if (hb::shared::input::is_mouse_in_rect(259 + OX, 336 + OY, 21, 13)) m_ibutton_num = 21; // Mag -
        else if (hb::shared::input::is_mouse_in_rect(236 + OX, 351 + OY, 21, 13)) m_ibutton_num = 22; // Chr +
        else if (hb::shared::input::is_mouse_in_rect(259 + OX, 351 + OY, 21, 13)) m_ibutton_num = 23; // Chr -
        else if (hb::shared::input::is_mouse_in_rect(384 + OX, 445 + OY, 72, 15)) m_ibutton_num = 24; // Create
        else if (hb::shared::input::is_mouse_in_rect(500 + OX, 445 + OY, 72, 15)) m_ibutton_num = 25; // Cancel
        else if (hb::shared::input::is_mouse_in_rect(60 + OX, 445 + OY, 72, 15)) m_ibutton_num = 26;  // Aresden
        else if (hb::shared::input::is_mouse_in_rect(145 + OX, 445 + OY, 72, 15)) m_ibutton_num = 27; // Elvine
        else if (hb::shared::input::is_mouse_in_rect(230 + OX, 445 + OY, 72, 15)) m_ibutton_num = 28; // Traveler

        switch (m_ibutton_num) {
        case 1:
            m_cur_focus = 1;
            break;
        case 2:
            m_game->m_player->m_gender--;
            if (m_game->m_player->m_gender < 1) m_game->m_player->m_gender = 2;
            break;
        case 3:
            m_game->m_player->m_gender++;
            if (m_game->m_player->m_gender > 2) m_game->m_player->m_gender = 1;
            break;
        case 4:
            m_game->m_player->m_skin_col--;
            if (m_game->m_player->m_skin_col < 1) m_game->m_player->m_skin_col = 3;
            break;
        case 5:
            m_game->m_player->m_skin_col++;
            if (m_game->m_player->m_skin_col > 3) m_game->m_player->m_skin_col = 1;
            break;
        case 6:
            m_game->m_player->m_hair_style--;
            if (m_game->m_player->m_hair_style < 0) m_game->m_player->m_hair_style = 7;
            break;
        case 7:
            m_game->m_player->m_hair_style++;
            if (m_game->m_player->m_hair_style > 7) m_game->m_player->m_hair_style = 0;
            break;
        case 8:
            m_game->m_player->m_hair_col--;
            if (m_game->m_player->m_hair_col < 0) m_game->m_player->m_hair_col = 15;
            break;
        case 9:
            m_game->m_player->m_hair_col++;
            if (m_game->m_player->m_hair_col > 15) m_game->m_player->m_hair_col = 0;
            break;
        case 10:
            m_game->m_player->m_under_col--;
            if (m_game->m_player->m_under_col < 0) m_game->m_player->m_under_col = 7;
            break;
        case 11:
            m_game->m_player->m_under_col++;
            if (m_game->m_player->m_under_col > 7) m_game->m_player->m_under_col = 0;
            break;
        case 12:
            if (m_iNewCharPoint > 0) {
                if (m_game->m_player->m_stat_mod_str < 14) {
                    m_game->m_player->m_stat_mod_str++;
                    m_iNewCharPoint--;
                }
            }
            break;
        case 13:
            if (m_game->m_player->m_stat_mod_str > 10) {
                m_game->m_player->m_stat_mod_str--;
                m_iNewCharPoint++;
            }
            break;
        case 14:
            if (m_iNewCharPoint > 0) {
                if (m_game->m_player->m_stat_mod_vit < 14) {
                    m_game->m_player->m_stat_mod_vit++;
                    m_iNewCharPoint--;
                }
            }
            break;
        case 15:
            if (m_game->m_player->m_stat_mod_vit > 10) {
                m_game->m_player->m_stat_mod_vit--;
                m_iNewCharPoint++;
            }
            break;
        case 16:
            if (m_iNewCharPoint > 0) {
                if (m_game->m_player->m_stat_mod_dex < 14) {
                    m_game->m_player->m_stat_mod_dex++;
                    m_iNewCharPoint--;
                }
            }
            break;
        case 17:
            if (m_game->m_player->m_stat_mod_dex > 10) {
                m_game->m_player->m_stat_mod_dex--;
                m_iNewCharPoint++;
            }
            break;
        case 18:
            if (m_iNewCharPoint > 0) {
                if (m_game->m_player->m_stat_mod_int < 14) {
                    m_game->m_player->m_stat_mod_int++;
                    m_iNewCharPoint--;
                }
            }
            break;
        case 19:
            if (m_game->m_player->m_stat_mod_int > 10) {
                m_game->m_player->m_stat_mod_int--;
                m_iNewCharPoint++;
            }
            break;
        case 20:
            if (m_iNewCharPoint > 0) {
                if (m_game->m_player->m_stat_mod_mag < 14) {
                    m_game->m_player->m_stat_mod_mag++;
                    m_iNewCharPoint--;
                }
            }
            break;
        case 21:
            if (m_game->m_player->m_stat_mod_mag > 10) {
                m_game->m_player->m_stat_mod_mag--;
                m_iNewCharPoint++;
            }
            break;
        case 22:
            if (m_iNewCharPoint > 0) {
                if (m_game->m_player->m_stat_mod_chr < 14) {
                    m_game->m_player->m_stat_mod_chr++;
                    m_iNewCharPoint--;
                }
            }
            break;
        case 23:
            if (m_game->m_player->m_stat_mod_chr > 10)
            {
                m_game->m_player->m_stat_mod_chr--;
                m_iNewCharPoint++;
            }
            break;

        case 24: // Create button
            if (m_cur_focus != 2)
            {
                m_cur_focus = 2;
                return;
            }
            if (m_bNewCharFlag == false) return;
            if (CMisc::check_valid_name(m_cNewCharName.data()) == false) break;
            m_game->m_player->m_player_name = m_cNewCharName.c_str();
            m_game->m_l_sock = std::make_unique<hb::shared::net::ASIOSocket>(m_game->m_io_pool->get_context(), game_limits::socket_block_limit);
            m_game->m_l_sock->connect(m_game->m_log_server_addr.c_str(), m_game->m_log_server_port);
            m_game->m_l_sock->init_buffer_size(hb::shared::limits::MsgBufferSize);
            m_game->change_game_mode(GameMode::Connecting);
            m_game->m_connect_mode = MsgId::RequestCreateNewCharacter;
            std::snprintf(m_game->m_msg, sizeof(m_game->m_msg), "%s", "22");
            return;

        case 25: // Cancel button
            if (m_cur_focus != 3)
            {
                m_cur_focus = 3;
                return;
            }
            m_game->change_game_mode(GameMode::SelectCharacter);
            return;

        case 26: // WARRIOR preset
            if (m_cur_focus != 4)
            {
                m_cur_focus = 4;
                return;
            }
            m_game->m_player->m_stat_mod_mag = 10;
            m_game->m_player->m_stat_mod_int = 10;
            m_game->m_player->m_stat_mod_chr = 10;
            m_game->m_player->m_stat_mod_str = 14;
            m_game->m_player->m_stat_mod_vit = 12;
            m_game->m_player->m_stat_mod_dex = 14;
            m_iNewCharPoint = m_game->m_player->m_stat_mod_str + m_game->m_player->m_stat_mod_vit + m_game->m_player->m_stat_mod_dex + m_game->m_player->m_stat_mod_int + m_game->m_player->m_stat_mod_mag + m_game->m_player->m_stat_mod_chr;
            m_iNewCharPoint = 70 - m_iNewCharPoint;
            break;

        case 27: // MAGE preset
            if (m_cur_focus != 5) {
                m_cur_focus = 5;
                return;
            }
            m_game->m_player->m_stat_mod_mag = 14;
            m_game->m_player->m_stat_mod_int = 14;
            m_game->m_player->m_stat_mod_chr = 10;
            m_game->m_player->m_stat_mod_str = 10;
            m_game->m_player->m_stat_mod_vit = 12;
            m_game->m_player->m_stat_mod_dex = 10;
            m_iNewCharPoint = m_game->m_player->m_stat_mod_str + m_game->m_player->m_stat_mod_vit + m_game->m_player->m_stat_mod_dex + m_game->m_player->m_stat_mod_int + m_game->m_player->m_stat_mod_mag + m_game->m_player->m_stat_mod_chr;
            m_iNewCharPoint = 70 - m_iNewCharPoint;
            break;

        case 28: // PRIEST preset
            if (m_cur_focus != 6) {
                m_cur_focus = 6;
                return;
            }
            m_game->m_player->m_stat_mod_mag = 12;
            m_game->m_player->m_stat_mod_int = 10;
            m_game->m_player->m_stat_mod_chr = 14;
            m_game->m_player->m_stat_mod_str = 14;
            m_game->m_player->m_stat_mod_vit = 10;
            m_game->m_player->m_stat_mod_dex = 10;
            m_iNewCharPoint = m_game->m_player->m_stat_mod_str + m_game->m_player->m_stat_mod_vit + m_game->m_player->m_stat_mod_dex + m_game->m_player->m_stat_mod_int + m_game->m_player->m_stat_mod_mag + m_game->m_player->m_stat_mod_chr;
            m_iNewCharPoint = 70 - m_iNewCharPoint;
            break;
        }
    }
}

void Screen_CreateNewCharacter::on_render()
{
    std::string G_cTxt;
    int i = 0;
    short mouse_x = m_sNewCharMsX;
    short mouse_y = m_sNewCharMsY;
    uint32_t time = GameClock::get_time_ms();

    // ======== draw character creation UI (inlined from _bDraw_OnCreateNewCharacter) ========
    m_game->draw_new_dialog_box(InterfaceNdNewChar, 0, 0, 0, true);
    m_game->draw_new_dialog_box(InterfaceNdButton, OX, OY, 69, true);
    hb::shared::text::draw_text_aligned(GameFont::Default, 64 + OX, 90 + OY, (282) - (64), 15, _BDRAW_ON_CREATE_NEW_CHARACTER1, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::draw_text_aligned(GameFont::Default, 57 + OX, 110 + OY, (191) - (57), 15, DEF_MSG_CHARACTERNAME, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    if (m_cur_focus != 1) hb::shared::text::draw_text(GameFont::Default, 197 + OX, 112 + OY, m_cNewCharName.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UILabel));
    hb::shared::text::draw_text_aligned(GameFont::Default, 64 + OX, 140 + OY, (282) - (64), 15, _BDRAW_ON_CREATE_NEW_CHARACTER2, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::draw_text(GameFont::Default, 100 + OX, 160 + OY, DEF_MSG_GENDER, hb::shared::text::TextStyle::from_color(GameColors::UIBlack));
    hb::shared::text::draw_text(GameFont::Default, 100 + OX, 175 + OY, DEF_MSG_SKINCOLOR, hb::shared::text::TextStyle::from_color(GameColors::UIBlack));
    hb::shared::text::draw_text(GameFont::Default, 100 + OX, 190 + OY, DEF_MSG_HAIRSTYLE, hb::shared::text::TextStyle::from_color(GameColors::UIBlack));
    hb::shared::text::draw_text(GameFont::Default, 100 + OX, 205 + OY, DEF_MSG_HAIRCOLOR, hb::shared::text::TextStyle::from_color(GameColors::UIBlack));
    hb::shared::text::draw_text(GameFont::Default, 100 + OX, 220 + OY, DEF_MSG_UNDERWEARCOLOR, hb::shared::text::TextStyle::from_color(GameColors::UIBlack));
    hb::shared::text::draw_text(GameFont::Default, 100 + OX, 275 + OY, DEF_MSG_STRENGTH, hb::shared::text::TextStyle::from_color(GameColors::UIBlack));
    hb::shared::text::draw_text(GameFont::Default, 100 + OX, 292 + OY, DEF_MSG_VITALITY, hb::shared::text::TextStyle::from_color(GameColors::UIBlack));
    hb::shared::text::draw_text(GameFont::Default, 100 + OX, 309 + OY, DEF_MSG_DEXTERITY, hb::shared::text::TextStyle::from_color(GameColors::UIBlack));
    hb::shared::text::draw_text(GameFont::Default, 100 + OX, 326 + OY, DEF_MSG_INTELLIGENCE, hb::shared::text::TextStyle::from_color(GameColors::UIBlack));
    hb::shared::text::draw_text(GameFont::Default, 100 + OX, 343 + OY, DEF_MSG_MAGIC, hb::shared::text::TextStyle::from_color(GameColors::UIBlack));
    hb::shared::text::draw_text(GameFont::Default, 100 + OX, 360 + OY, DEF_MSG_CHARISMA, hb::shared::text::TextStyle::from_color(GameColors::UIBlack));

    // Stat values
    i = 0;
    G_cTxt = std::format("{}", m_game->m_player->m_stat_mod_str);
    hb::shared::text::draw_text(GameFont::Default, 204 + OX, 277 + OY + 16 * i++, G_cTxt.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UILabel));
    G_cTxt = std::format("{}", m_game->m_player->m_stat_mod_vit);
    hb::shared::text::draw_text(GameFont::Default, 204 + OX, 277 + OY + 16 * i++, G_cTxt.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UILabel));
    G_cTxt = std::format("{}", m_game->m_player->m_stat_mod_dex);
    hb::shared::text::draw_text(GameFont::Default, 204 + OX, 277 + OY + 16 * i++, G_cTxt.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UILabel));
    G_cTxt = std::format("{}", m_game->m_player->m_stat_mod_int);
    hb::shared::text::draw_text(GameFont::Default, 204 + OX, 277 + OY + 16 * i++, G_cTxt.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UILabel));
    G_cTxt = std::format("{}", m_game->m_player->m_stat_mod_mag);
    hb::shared::text::draw_text(GameFont::Default, 204 + OX, 277 + OY + 16 * i++, G_cTxt.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UILabel));
    G_cTxt = std::format("{}", m_game->m_player->m_stat_mod_chr);
    hb::shared::text::draw_text(GameFont::Default, 204 + OX, 277 + OY + 16 * i++, G_cTxt.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UILabel));

    // Button states
    if ((m_bNewCharFlag == true) && (m_cur_focus == 2))
        m_game->m_sprite[InterfaceNdButton]->draw(384 + OX, 445 + OY, 25);
    else
        m_game->m_sprite[InterfaceNdButton]->draw(384 + OX, 445 + OY, 24);
    if (m_cur_focus == 3)
        m_game->m_sprite[InterfaceNdButton]->draw(500 + OX, 445 + OY, 17);
    else
        m_game->m_sprite[InterfaceNdButton]->draw(500 + OX, 445 + OY, 16);
    if (m_cur_focus == 4)
        m_game->m_sprite[InterfaceNdButton]->draw(60 + OX, 445 + OY, 68);
    else
        m_game->m_sprite[InterfaceNdButton]->draw(60 + OX, 445 + OY, 67);
    if (m_cur_focus == 5)
        m_game->m_sprite[InterfaceNdButton]->draw(145 + OX, 445 + OY, 66);
    else
        m_game->m_sprite[InterfaceNdButton]->draw(145 + OX, 445 + OY, 65);
    if (m_cur_focus == 6)
        m_game->m_sprite[InterfaceNdButton]->draw(230 + OX, 445 + OY, 64);
    else
        m_game->m_sprite[InterfaceNdButton]->draw(230 + OX, 445 + OY, 63);

    text_input_manager::get().show_input();

    // Character preview
    switch (m_game->m_player->m_gender) {
    case 1: m_game->m_entity_state.m_owner_type = hb::shared::owner::MaleFirst; break;
    case 2: m_game->m_entity_state.m_owner_type = hb::shared::owner::FemaleFirst; break;
    }
    m_game->m_entity_state.m_owner_type += m_game->m_player->m_skin_col - 1;
    m_game->m_entity_state.m_dir = m_game->m_menu_dir;
    m_game->m_entity_state.m_appearance.clear();
    m_game->m_entity_state.m_appearance.underwear_type = m_game->m_player->m_under_col;
    m_game->m_entity_state.m_appearance.hair_style = m_game->m_player->m_hair_style;
    m_game->m_entity_state.m_appearance.hair_color = m_game->m_player->m_hair_col;
    m_game->m_entity_state.m_name.fill('\0');
    std::snprintf(m_game->m_entity_state.m_name.data(), m_game->m_entity_state.m_name.size(), "%s", m_game->m_player->m_player_name.c_str());
    m_game->m_entity_state.m_action = Type::Move;
    m_game->m_entity_state.m_frame = m_game->m_menu_frame;

    m_game->draw_character_body(507 + OX, 267 + OY, m_game->m_entity_state.m_owner_type);
    m_game->draw_object_on_move_for_menu(0, 0, 500 + OX, 174 + OY, false, time);

    // Derived stats
    i = 0;
    hb::shared::text::draw_text(GameFont::Default, 445 + OX, 192 + OY, DEF_MSG_HITPOINT, hb::shared::text::TextStyle::from_color(GameColors::UIBlack));
    G_cTxt = std::format("{}", m_game->m_player->m_stat_mod_vit * 3 + 2 + m_game->m_player->m_stat_mod_str / 2);
    hb::shared::text::draw_text(GameFont::Default, 550 + OX, 192 + OY + 16 * i++, G_cTxt.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UILabel));
    hb::shared::text::draw_text(GameFont::Default, 445 + OX, 208 + OY, DEF_MSG_MANAPOINT, hb::shared::text::TextStyle::from_color(GameColors::UIBlack));
    G_cTxt = std::format("{}", m_game->m_player->m_stat_mod_mag * 2 + 2 + m_game->m_player->m_stat_mod_int / 2);
    hb::shared::text::draw_text(GameFont::Default, 550 + OX, 192 + OY + 16 * i++, G_cTxt.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UILabel));
    hb::shared::text::draw_text(GameFont::Default, 445 + OX, 224 + OY, DEF_MSG_STAMINARPOINT, hb::shared::text::TextStyle::from_color(GameColors::UIBlack));
    G_cTxt = std::format("{}", m_game->m_player->m_stat_mod_str * 2 + 2);
    hb::shared::text::draw_text(GameFont::Default, 550 + OX, 192 + OY + 16 * i++, G_cTxt.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UILabel));

    // ======== End inlined drawing ========

    m_game->draw_version();

    // Tooltip drawing based on mouse position
    if ((mouse_x >= 65 + 4 - 127 + OX) && (mouse_x <= 275 + 4 + OX) && (mouse_y >= 65 + 45 + OY) && (mouse_y <= 82 + 45 + OY)) {
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER1, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x >= 261 + 4 - 212 + OX) && (mouse_x <= 289 + 4 + OX) && (mouse_y >= 111 + 45 + OY) && (mouse_y <= 124 + 45 + OY)) {
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER2, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x >= 261 + 4 - 212 + OX) && (mouse_x <= 289 + 4 + OX) && (mouse_y >= 126 + 45 + OY) && (mouse_y <= 139 + 45 + OY)) {
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER3, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x >= 261 + 4 - 212 + OX) && (mouse_x <= 289 + 4 + OX) && (mouse_y >= 141 + 45 + OY) && (mouse_y <= 154 + 45 + OY)) {
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER4, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x >= 261 + 4 - 212 + OX) && (mouse_x <= 289 + 4 + OX) && (mouse_y >= 156 + 45 + OY) && (mouse_y <= 169 + 45 + OY)) {
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER5, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x >= 261 + 4 - 212 + OX) && (mouse_x <= 289 + 4 + OX) && (mouse_y >= 171 + 45 + OY) && (mouse_y <= 184 + 45 + OY)) {
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER6, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x >= 240 + 4 - 175 + OX) && (mouse_x <= 268 + 4 + OX) && (mouse_y >= 231 + 45 + OY) && (mouse_y <= 244 + 45 + OY)) {
        // Str tooltip
        i = 0;
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER7, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER8, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER9, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER10, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER11, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x >= 240 + 4 - 175 + OX) && (mouse_x <= 268 + 4 + OX) && (mouse_y >= 246 + 45 + OY) && (mouse_y <= 259 + 45 + OY)) {
        // Vit tooltip
        i = 0;
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER12, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER13, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER14, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER15, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER16, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x >= 240 + 4 - 175 + OX) && (mouse_x <= 268 + 4 + OX) && (mouse_y >= 261 + 45 + OY) && (mouse_y <= 274 + 45 + OY)) {
        // Dex tooltip
        i = 0;
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER17, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER18, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER19, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER20, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x >= 240 + 4 - 175 + OX) && (mouse_x <= 268 + 4 + OX) && (mouse_y >= 276 + 45 + OY) && (mouse_y <= 289 + 45 + OY)) {
        // Int tooltip
        i = 0;
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER21, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER22, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER23, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER24, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x >= 240 + 4 - 175 + OX) && (mouse_x <= 268 + 4 + OX) && (mouse_y >= 291 + 45 + OY) && (mouse_y <= 304 + 45 + OY)) {
        // Mag tooltip
        i = 0;
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER25, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER26, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER27, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER28, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x >= 240 + 4 - 175 + OX) && (mouse_x <= 268 + 4 + OX) && (mouse_y >= 306 + 45 + OY) && (mouse_y <= 319 + 45 + OY)) {
        // Charisma tooltip
        i = 0;
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER29, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER30, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER31, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER32, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x >= 384 + OX) && (mouse_x <= 384 + 72 + OX) && (mouse_y >= 445 + OY) && (mouse_y <= 445 + 15 + OY)) {
        if (m_cNewCharName.empty()) {
            i = 0;
            hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER35, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        }
        else if (m_iNewCharPoint > 0) {
            i = 0;
            hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER36, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        }
        else if (CMisc::check_valid_name(m_cNewCharName.data()) == false) {
            i = 0;
            hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER39, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER40, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER41, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        }
        else {
            i = 0;
            hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER44, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER45, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER46, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER47, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER48, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        }
    }
    else if ((mouse_x >= 500 + OX) && (mouse_x <= 500 + 72 + OX) && (mouse_y >= 445 + OY) && (mouse_y <= 445 + 15 + OY)) {
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER49, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x >= 60 + OX) && (mouse_x <= 60 + 72 + OX) && (mouse_y >= 445 + OY) && (mouse_y <= 445 + 15 + OY)) {
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER50, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x >= 145 + OX) && (mouse_x <= 145 + 72 + OX) && (mouse_y >= 445 + OY) && (mouse_y <= 445 + 15 + OY)) {
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER51, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((mouse_x >= 230 + OX) && (mouse_x <= 230 + 72 + OX) && (mouse_y >= 445 + OY) && (mouse_y <= 445 + 15 + OY)) {
        hb::shared::text::draw_text_aligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER52, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
}
