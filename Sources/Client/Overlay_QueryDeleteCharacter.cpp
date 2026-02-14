// Overlay_QueryDeleteCharacter.cpp: "Delete Character" confirmation overlay
//
//////////////////////////////////////////////////////////////////////

#include "Overlay_QueryDeleteCharacter.h"
#include "Game.h"
#include "GameModeManager.h"
#include "CommonTypes.h"
#include "lan_eng.h"
#include "IInput.h"
#include "ASIOSocket.h"
#include "TextLibExt.h"
#include "GameFonts.h"


using namespace hb::shared::net;
using namespace hb::client::sprite_id;
namespace MouseButton = hb::shared::input::MouseButton;

Overlay_QueryDeleteCharacter::Overlay_QueryDeleteCharacter(CGame* game)
    : IGameScreen(game)
{
}

void Overlay_QueryDeleteCharacter::on_initialize()
{
    m_dwStartTime = GameClock::get_time_ms();
    m_dwAnimTime = m_dwStartTime;

    // Play warning sound
    play_game_sound('E', 25, 0);
}

void Overlay_QueryDeleteCharacter::on_uninitialize()
{
    // Nothing to clean up
}

void Overlay_QueryDeleteCharacter::on_update()
{
    uint32_t time = GameClock::get_time_ms();

    int dlgX, dlgY;
    get_centered_dialog_pos(InterfaceNdGame4, 2, dlgX, dlgY);

    // ESC cancels - base screen (SelectCharacter) will be revealed
    if (hb::shared::input::is_key_pressed(KeyCode::Escape))
    {
        clear_overlay();
        return;
    }

    // Mouse click detection
    if (hb::shared::input::is_mouse_button_pressed(MouseButton::Left))
    {
        play_game_sound('E', 14, 5);

        // Yes button - confirm deletion
        if (hb::shared::input::is_mouse_in_rect(dlgX + 38, dlgY + 119, ui_layout::btn_size_x, ui_layout::btn_size_y))
        {
            // Create login socket and initiate delete request
            m_game->m_l_sock = std::make_unique<hb::shared::net::ASIOSocket>(m_game->m_io_pool->get_context(), game_limits::socket_block_limit);
            m_game->m_l_sock->connect(m_game->m_log_server_addr.c_str(), m_game->m_log_server_port);
            m_game->m_l_sock->init_buffer_size(hb::shared::limits::MsgBufferSize);

            m_game->m_connect_mode = MsgId::RequestDeleteCharacter;
            std::snprintf(m_game->m_msg, sizeof(m_game->m_msg), "%s", "33");

            // set_overlay will clear this overlay automatically
            m_game->change_game_mode(GameMode::Connecting);
            return;
        }

        // No button - cancel, base screen (SelectCharacter) will be revealed
        if (hb::shared::input::is_mouse_in_rect(dlgX + 208, dlgY + 119, ui_layout::btn_size_x, ui_layout::btn_size_y))
        {
            clear_overlay();
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

void Overlay_QueryDeleteCharacter::on_render()
{
    int mouse_x = hb::shared::input::get_mouse_x();
    int mouse_y = hb::shared::input::get_mouse_y();
    uint32_t elapsed = GameClock::get_time_ms() - m_dwStartTime;

    int dlgX, dlgY;
    get_centered_dialog_pos(InterfaceNdGame4, 2, dlgX, dlgY);

    // Double shadow effect after initial animation period (600ms)
    if (elapsed >= 600)
    {
        m_game->m_Renderer->draw_rect_filled(0, 0, LOGICAL_MAX_X(), LOGICAL_MAX_Y(), hb::shared::render::Color::Black(128));
    }

    // draw dialog box
    draw_new_dialog_box(InterfaceNdGame4, dlgX, dlgY, 2);

    // Title
    hb::shared::text::draw_text(GameFont::Bitmap1, dlgX + 96, dlgY + 35, "Delete Character", hb::shared::text::TextStyle::with_highlight(GameColors::UIDarkRed));

    // Character name display
    put_string(dlgX + 53, dlgY + 70, UPDATE_SCREEN_ON_QUERY_DELETE_CHARACTER1, GameColors::UIBlack);
    put_string(dlgX + 173, dlgY + 74, "__________", GameColors::UIBlack);

    // get character name from the selected character slot
    if (m_game->m_enter_game_type > 0 && m_game->m_char_list[m_game->m_enter_game_type - 1] != nullptr)
    {
        put_string(dlgX + 173, dlgY + 70, m_game->m_char_list[m_game->m_enter_game_type - 1]->m_name.c_str(), GameColors::UILabel);
    }

    // Confirmation text
    put_aligned_string(dlgX + 16, dlgX + 291, dlgY + 95, UPDATE_SCREEN_ON_QUERY_DELETE_CHARACTER2);

    // Yes button with hover effect
    bool yes_hover = (mouse_x >= dlgX + 38) && (mouse_x <= dlgX + 38 + ui_layout::btn_size_x) &&
                     (mouse_y >= dlgY + 119) && (mouse_y <= dlgY + 119 + ui_layout::btn_size_y);
    draw_new_dialog_box(InterfaceNdButton, dlgX + 38, dlgY + 119, yes_hover ? 19 : 18);

    // No button with hover effect
    bool no_hover = (mouse_x >= dlgX + 208) && (mouse_x <= dlgX + 208 + ui_layout::btn_size_x) &&
                    (mouse_y >= dlgY + 119) && (mouse_y <= dlgY + 119 + ui_layout::btn_size_y);
    draw_new_dialog_box(InterfaceNdButton, dlgX + 208, dlgY + 119, no_hover ? 3 : 2);

    draw_version();
}
