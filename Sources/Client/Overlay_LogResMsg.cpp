// Overlay_LogResMsg.cpp: Login/Account result message overlay
//
//////////////////////////////////////////////////////////////////////

#include "Overlay_LogResMsg.h"
#include "Game.h"
#include "GameModeManager.h"
#include "CommonTypes.h"
#include "lan_eng.h"
#include "IInput.h"
#include "AudioManager.h"
#include "TextLibExt.h"
#include "GameFonts.h"
#include <format>
#include <string>
using namespace hb::client::sprite_id;


namespace MouseButton = hb::shared::input::MouseButton;

Overlay_LogResMsg::Overlay_LogResMsg(CGame* game)
    : IGameScreen(game)
    , m_cReturnDest('1')
    , m_cMsgCode('1')
{
}

void Overlay_LogResMsg::on_initialize()
{
    m_dwStartTime = GameClock::get_time_ms();
    m_dwAnimTime = m_dwStartTime;

    // Read parameters from CGame (set before change_game_mode call)
    // m_msg[0] = return destination, m_msg[1] = message code
    m_cReturnDest = m_game->m_msg[0];
    m_cMsgCode = m_game->m_msg[1];

    // stop any playing sound
    audio_manager::get().stop_sound(sound_type::Effect, 38);
}

void Overlay_LogResMsg::on_uninitialize()
{
    // Nothing to clean up
}

void Overlay_LogResMsg::handle_dismiss()
{
    // Note: clear_overlay() is not needed here as change_game_mode will either:
    // - Call set_screen<T>() which clears overlays automatically
    // - Call set_overlay<T>() which clears existing overlays automatically

    // Transition based on return destination
    switch (m_cReturnDest)
    {
    case '0':
        m_game->change_game_mode(GameMode::CreateNewAccount);
        break;
    case '1':
        m_game->change_game_mode(GameMode::MainMenu);
        break;
    case '2':
        m_game->change_game_mode(GameMode::CreateNewCharacter);
        break;
    case '3':
    case '4':
        m_game->change_game_mode(GameMode::SelectCharacter);
        break;
    case '5':
        m_game->change_game_mode(GameMode::MainMenu);
        break;
    case '6':
        // Context-dependent based on message code
        switch (m_cMsgCode)
        {
        case 'B':
            m_game->change_game_mode(GameMode::MainMenu);
            break;
        case 'C':
        case 'M':
            m_game->change_game_mode(GameMode::ChangePassword);
            break;
        default:
            m_game->change_game_mode(GameMode::MainMenu);
            break;
        }
        break;
    case '7':
    case '8':
    default:
        m_game->change_game_mode(GameMode::MainMenu);
        break;
    }
}

void Overlay_LogResMsg::on_update()
{
    uint32_t time = GameClock::get_time_ms();

    int dlgX, dlgY;
    get_centered_dialog_pos(InterfaceNdGame4, 2, dlgX, dlgY);

    // ESC or Enter dismisses the message
    if (hb::shared::input::is_key_pressed(KeyCode::Escape) || hb::shared::input::is_key_pressed(KeyCode::Enter))
    {
        handle_dismiss();
        return;
    }

    // Check for OK button click
    if (hb::shared::input::is_mouse_button_pressed(MouseButton::Left))
    {
        if (hb::shared::input::is_mouse_in_rect(dlgX + 208, dlgY + 119, ui_layout::btn_size_x, ui_layout::btn_size_y))
        {
            handle_dismiss();
            return;
        }
    }

    // Animation frame updates
    if ((time - m_dwAnimTime) > 100)
    {
        m_game->m_menu_frame++;
        m_dwAnimTime = time;
    }
    if (m_game->m_menu_frame >= 8)
    {
        m_game->m_menu_dir_cnt++;
        if (m_game->m_menu_dir_cnt > 8)
        {
            m_game->m_menu_dir++;
            m_game->m_menu_dir_cnt = 1;
        }
        if (m_game->m_menu_dir > 8) m_game->m_menu_dir = 1;
        m_game->m_menu_frame = 0;
    }
}

void Overlay_LogResMsg::render_message(int dlgX, int dlgY)
{
    std::string txt;

    switch (m_cMsgCode)
    {
    case '1':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 80, dlgY + 40, "Password is not correct!", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG5);
        break;

    case '2':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 80, dlgY + 40, "Not existing account!", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG6);
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 90, UPDATE_SCREEN_ON_LOG_MSG7);
        break;

    case '3':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 54, dlgY + 40, "Can not connect to game server!", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG8);
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG9);
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 100, UPDATE_SCREEN_ON_LOG_MSG10);
        break;

    case '4':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 68, dlgY + 40, "New account created.", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG11);
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG12);
        break;

    case '5':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 68, dlgY + 40, "Can not create new account!", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG13);
        break;

    case '6':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 46, dlgY + 40, "Can not create new account!", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 34, dlgY + 55, "Already existing account name.", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 80, UPDATE_SCREEN_ON_LOG_MSG14);
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 95, UPDATE_SCREEN_ON_LOG_MSG15);
        break;

    case '7':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 68, dlgY + 40, "New character created.", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG16);
        break;

    case '8':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 68, dlgY + 40, "Can not create new character!", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG17);
        break;

    case '9':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 46, dlgY + 40, "Can not create new character!", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 34, dlgY + 55, "Already existing character name.", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 80, UPDATE_SCREEN_ON_LOG_MSG18);
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 95, UPDATE_SCREEN_ON_LOG_MSG19);
        break;

    case 'A':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 91, dlgY + 40, "Character deleted.", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG20);
        break;

    case 'B':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 91, dlgY + 40, "Password changed.", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG21);
        break;

    case 'C':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 46, dlgY + 40, "Can not change password!", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG22);
        break;

    case 'D':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 54, dlgY + 40, "Can not connect to game server!", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG23);
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG24);
        break;

    case 'E':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 54, dlgY + 40, "Can not connect to game server!", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG25);
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG26);
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 100, UPDATE_SCREEN_ON_LOG_MSG27);
        break;

    case 'F':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 54, dlgY + 40, "Can not connect to game server!", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG28);
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG29);
        break;

    case 'G':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 54, dlgY + 40, "Can not connect to game server!", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG30);
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG31);
        break;

    case 'H':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 78, dlgY + 40, "Connection Rejected!", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        if (m_game->m_block_year == 0)
        {
            put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG32);
            put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG33);
        }
        else
        {
            put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG34);
            txt = std::format(UPDATE_SCREEN_ON_LOG_MSG35, m_game->m_block_year, m_game->m_block_month, m_game->m_block_day);
            put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 85, txt.c_str());
        }
        break;

    case 'I':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 78, dlgY + 40, "Not Enough Point!", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 85, "Not enough points to play.");
        break;

    case 'J':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 78, dlgY + 40, "World Server Full", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 85, "Please ! Try Other World Server");
        break;

    case 'M':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 78, dlgY + 40, "Your password expired", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 85, "Please! Change password");
        break;

    case 'U':
        hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 78, dlgY + 40, "Keycode input Success!", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 85, "Keycode Registration successed.");
        break;

    case 'X':
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG38);
        put_aligned_string(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG39);
        break;

    case 'Y':
        put_aligned_string(dlgX + 16, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG40);
        put_aligned_string(dlgX + 16, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG41);
        break;

    case 'Z':
        put_aligned_string(dlgX + 16, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG42);
        put_aligned_string(dlgX + 16, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG41);
        break;
    }
}

void Overlay_LogResMsg::on_render()
{
    int mouse_x = hb::shared::input::get_mouse_x();
    int mouse_y = hb::shared::input::get_mouse_y();

    int dlgX, dlgY;
    get_centered_dialog_pos(InterfaceNdGame4, 2, dlgX, dlgY);

    // draw dialog box
    draw_new_dialog_box(InterfaceNdGame4, dlgX, dlgY, 2);

    // draw OK button with hover effect
    bool hover = (mouse_x >= dlgX + 208) && (mouse_x <= dlgX + 208 + ui_layout::btn_size_x) &&
                  (mouse_y >= dlgY + 119) && (mouse_y <= dlgY + 119 + ui_layout::btn_size_y);
    draw_new_dialog_box(InterfaceNdButton, dlgX + 208, dlgY + 119, hover ? 1 : 0);

    // render the appropriate message
    render_message(dlgX, dlgY);
}
