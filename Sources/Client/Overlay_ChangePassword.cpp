// Overlay_ChangePassword.cpp: Password change overlay
//
//////////////////////////////////////////////////////////////////////

#include "Overlay_ChangePassword.h"
#include "Game.h"
#include "GameModeManager.h"
#include "CommonTypes.h"
#include "lan_eng.h"
#include "IInput.h"
#include "ASIOSocket.h"
#include "Misc.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include "Packet/SharedPackets.h"
#include <string>


using namespace hb::shared::net;
using namespace hb::client::sprite_id;
namespace MouseButton = hb::shared::input::MouseButton;

Overlay_ChangePassword::Overlay_ChangePassword(CGame* game)
    : IGameScreen(game)
    , m_iCurFocus(2)
    , m_iPrevFocus(2)
    , m_iMaxFocus(6)
{
}

void Overlay_ChangePassword::on_initialize()
{
    m_dwStartTime = GameClock::get_time_ms();
    m_dwAnimTime = m_dwStartTime;

    // End any existing input string
    end_input_string();

    // initialize focus
    m_iPrevFocus = 2;
    m_iCurFocus = 2;
    m_iMaxFocus = 6;
    m_game->m_arrow_pressed = 0;

    // clear input buffers
    m_account_name.clear();
    m_cOldPassword.clear();
    m_cNewPassword.clear();
    m_cConfirmPassword.clear();

    // Copy account name from player
    m_account_name = m_game->m_player->m_account_name;

    // start input on old password field
    int dlgX, dlgY;
    get_centered_dialog_pos(InterfaceNdGame4, 0, dlgX, dlgY);
    start_input_string(dlgX + 161, dlgY + 67, 11, m_cOldPassword, true);
    clear_input_string();
}

void Overlay_ChangePassword::on_uninitialize()
{
    end_input_string();
}

void Overlay_ChangePassword::update_focused_input()
{
    if (m_iPrevFocus != m_iCurFocus)
    {
        int dlgX, dlgY;
        get_centered_dialog_pos(InterfaceNdGame4, 0, dlgX, dlgY);

        end_input_string();
        switch (m_iCurFocus)
        {
        case 1:
            start_input_string(dlgX + 161, dlgY + 43, 11, m_account_name);
            break;
        case 2:
            start_input_string(dlgX + 161, dlgY + 67, 11, m_cOldPassword, true);
            break;
        case 3:
            start_input_string(dlgX + 161, dlgY + 91, 11, m_cNewPassword, true);
            break;
        case 4:
            start_input_string(dlgX + 161, dlgY + 115, 11, m_cConfirmPassword, true);
            break;
        }
        m_iPrevFocus = m_iCurFocus;
    }
}

bool Overlay_ChangePassword::validate_inputs()
{
    // Check account name
    if (!CMisc::check_valid_string(m_account_name.data()) || m_account_name.empty())
        return false;

    // Check old password
    if (!CMisc::check_valid_string(m_cOldPassword.data()) || m_cOldPassword.empty())
        return false;

    // Check new password
    if (!CMisc::check_valid_string(m_cNewPassword.data()) || m_cNewPassword.size() < 8)
        return false;

    // Check confirm password matches
    if (!CMisc::check_valid_string(m_cConfirmPassword.data()))
        return false;

    if (m_cNewPassword != m_cConfirmPassword)
        return false;

    // New password must be different from old
    if (m_cOldPassword == m_cNewPassword)
        return false;

    return true;
}

void Overlay_ChangePassword::handle_submit()
{
    if (!validate_inputs())
        return;

    end_input_string();

    // Copy account name/password to player session
    m_game->m_player->m_account_name = m_account_name;
    m_game->m_player->m_account_password = m_cOldPassword;

    // Build ChangePasswordRequest packet
    hb::net::ChangePasswordRequest req{};
    req.header.msg_id = MsgId::RequestChangePassword;
    req.header.msg_type = 0;
    std::snprintf(req.account_name, sizeof(req.account_name), "%s", m_account_name.c_str());
    std::snprintf(req.password, sizeof(req.password), "%s", m_cOldPassword.c_str());
    std::snprintf(req.new_password, sizeof(req.new_password), "%s", m_cNewPassword.c_str());
    std::snprintf(req.new_password_confirm, sizeof(req.new_password_confirm), "%s", m_cConfirmPassword.c_str());

    // Store packet for sending after connection completes
    auto* p = reinterpret_cast<char*>(&req);
    m_game->m_pending_login_packet.assign(p, p + sizeof(req));

    // Create connection
    m_game->m_l_sock = std::make_unique<hb::shared::net::ASIOSocket>(m_game->m_io_pool->get_context(), game_limits::socket_block_limit);
    m_game->m_l_sock->connect(m_game->m_log_server_addr.c_str(), m_game->m_log_server_port);
    m_game->m_l_sock->init_buffer_size(hb::shared::limits::MsgBufferSize);

    m_game->m_connect_mode = MsgId::RequestChangePassword;
    std::snprintf(m_game->m_msg, sizeof(m_game->m_msg), "%s", "41");

    // set_overlay will clear this overlay automatically
    m_game->change_game_mode(GameMode::Connecting);
}

void Overlay_ChangePassword::on_update()
{
    uint32_t time = GameClock::get_time_ms();

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
        if (m_game->m_menu_dir > direction::northwest) m_game->m_menu_dir = direction::north;
        m_game->m_menu_frame = 0;
    }

    // Tab key navigation (consistent with Login and CreateAccount screens)
    if (hb::shared::input::is_key_pressed(KeyCode::Tab))
    {
        play_game_sound('E', 14, 5);
        if (hb::shared::input::is_shift_down())
        {
            m_iCurFocus--;
            if (m_iCurFocus <= 0) m_iCurFocus = m_iMaxFocus;
        }
        else
        {
            m_iCurFocus++;
            if (m_iCurFocus > m_iMaxFocus) m_iCurFocus = 1;
        }
    }

    // Arrow key navigation
    if (m_game->m_arrow_pressed != 0)
    {
        switch (m_game->m_arrow_pressed)
        {
        case 1: // Up
            m_iCurFocus--;
            if (m_iCurFocus <= 0) m_iCurFocus = m_iMaxFocus;
            break;
        case 2: // Left
            if (m_iCurFocus == 5) m_iCurFocus = 6;
            else if (m_iCurFocus == 6) m_iCurFocus = 5;
            break;
        case 3: // Down
            m_iCurFocus++;
            if (m_iCurFocus > m_iMaxFocus) m_iCurFocus = 1;
            break;
        case 4: // Right
            if (m_iCurFocus == 5) m_iCurFocus = 6;
            else if (m_iCurFocus == 6) m_iCurFocus = 5;
            break;
        }
        m_game->m_arrow_pressed = 0;
    }

    // Enter key
    if (hb::shared::input::is_key_pressed(KeyCode::Enter))
    {
        play_game_sound('E', 14, 5);
        switch (m_iCurFocus)
        {
        case 1:
        case 2:
        case 3:
        case 4:
            // Move to next field
            m_iCurFocus++;
            if (m_iCurFocus > m_iMaxFocus) m_iCurFocus = 1;
            break;
        case 5: // OK button
            handle_submit();
            return;
        case 6: // Cancel button - return to base screen
            clear_overlay();
            return;
        }
    }

    // ESC key - return to main menu (set_screen will clear overlay automatically)
    if (hb::shared::input::is_key_pressed(KeyCode::Escape))
    {
        m_game->change_game_mode(GameMode::MainMenu);
        return;
    }

    // Mouse click detection
    int dlgX, dlgY;
    get_centered_dialog_pos(InterfaceNdGame4, 0, dlgX, dlgY);

    if (hb::shared::input::is_mouse_button_pressed(MouseButton::Left))
    {
        play_game_sound('E', 14, 5);

        int clicked_field = 0;
        if (hb::shared::input::is_mouse_in_rect(dlgX + 147, dlgY + 36, 125, 22)) clicked_field = 1;
        else if (hb::shared::input::is_mouse_in_rect(dlgX + 147, dlgY + 60, 125, 22)) clicked_field = 2;
        else if (hb::shared::input::is_mouse_in_rect(dlgX + 147, dlgY + 84, 125, 22)) clicked_field = 3;
        else if (hb::shared::input::is_mouse_in_rect(dlgX + 147, dlgY + 108, 125, 22)) clicked_field = 4;
        else if (hb::shared::input::is_mouse_in_rect(dlgX + 44, dlgY + 208, ui_layout::btn_size_x, ui_layout::btn_size_y)) clicked_field = 5;
        else if (hb::shared::input::is_mouse_in_rect(dlgX + 217, dlgY + 208, ui_layout::btn_size_x, ui_layout::btn_size_y)) clicked_field = 6;

        switch (clicked_field)
        {
        case 1:
        case 2:
        case 3:
        case 4:
            m_iCurFocus = clicked_field;
            break;
        case 5: // OK
            handle_submit();
            return;
        case 6: // Cancel - return to base screen
            clear_overlay();
            return;
        }
    }

    // Mouse hover for buttons
    if (hb::shared::input::is_mouse_in_rect(dlgX + 44, dlgY + 208, ui_layout::btn_size_x, ui_layout::btn_size_y))
        m_iCurFocus = 5;
    if (hb::shared::input::is_mouse_in_rect(dlgX + 217, dlgY + 208, ui_layout::btn_size_x, ui_layout::btn_size_y))
        m_iCurFocus = 6;

    // update input field focus
    update_focused_input();
}

void Overlay_ChangePassword::on_render()
{
    bool valid_inputs = validate_inputs();

    int dlgX, dlgY;
    get_centered_dialog_pos(InterfaceNdGame4, 0, dlgX, dlgY);

    // draw dialog boxes
    draw_new_dialog_box(InterfaceNdGame4, dlgX, dlgY, 0);
    draw_new_dialog_box(InterfaceNdText, dlgX, dlgY, 13);
    draw_new_dialog_box(InterfaceNdGame4, dlgX + 157, dlgY + 109, 7);

    // draw labels
    put_string(dlgX + 53, dlgY + 43, UPDATE_SCREEN_ON_CHANGE_PASSWORD1, GameColors::UILabel);
    put_string(dlgX + 53, dlgY + 67, UPDATE_SCREEN_ON_CHANGE_PASSWORD2, GameColors::UILabel);
    put_string(dlgX + 53, dlgY + 91, UPDATE_SCREEN_ON_CHANGE_PASSWORD3, GameColors::UILabel);
    put_string(dlgX + 53, dlgY + 115, UPDATE_SCREEN_ON_CHANGE_PASSWORD4, GameColors::UILabel);

    // draw input field values (when not focused)
    static constexpr hb::shared::render::Color kInvalidInput{55, 18, 13};

    if (m_iCurFocus != 1)
    {
        const hb::shared::render::Color& color = CMisc::check_valid_string(m_account_name.data()) ? GameColors::UILabel : kInvalidInput;
        put_string(dlgX + 161, dlgY + 43, m_account_name.c_str(), color);
    }

    if (m_iCurFocus != 2)
    {
        const hb::shared::render::Color& color = CMisc::check_valid_string(m_cOldPassword.data()) ? GameColors::UILabel : kInvalidInput;
        std::string maskedOld(m_cOldPassword.size(), '*');
        hb::shared::text::draw_text(GameFont::Default, dlgX + 161, dlgY + 67, maskedOld.c_str(), hb::shared::text::TextStyle::from_color(color));
    }

    if (m_iCurFocus != 3)
    {
        const hb::shared::render::Color& color = CMisc::check_valid_name(m_cNewPassword.data()) ? GameColors::UILabel : kInvalidInput;
        std::string maskedNew(m_cNewPassword.size(), '*');
        hb::shared::text::draw_text(GameFont::Default, dlgX + 161, dlgY + 91, maskedNew.c_str(), hb::shared::text::TextStyle::from_color(color));
    }

    if (m_iCurFocus != 4)
    {
        const hb::shared::render::Color& color = CMisc::check_valid_name(m_cConfirmPassword.data()) ? GameColors::UILabel : kInvalidInput;
        std::string maskedConfirm(m_cConfirmPassword.size(), '*');
        hb::shared::text::draw_text(GameFont::Default, dlgX + 161, dlgY + 115, maskedConfirm.c_str(), hb::shared::text::TextStyle::from_color(color));
    }

    // Show active input string (with masking for password fields)
    if (m_iCurFocus == 1)
        show_received_string();
    else if (m_iCurFocus >= 2 && m_iCurFocus <= 4)
        show_received_string();  // Hide (mask) password

    // Help text
    put_aligned_string(dlgX, dlgX + 334, dlgY + 146, UPDATE_SCREEN_ON_CHANGE_PASSWORD5);
    put_aligned_string(dlgX, dlgX + 334, dlgY + 161, UPDATE_SCREEN_ON_CHANGE_PASSWORD6);
    put_aligned_string(dlgX, dlgX + 334, dlgY + 176, UPDATE_SCREEN_ON_CHANGE_PASSWORD7);

    // OK button (enabled only when inputs are valid)
    int okFrame = (valid_inputs && m_iCurFocus == 5) ? 21 : 20;
    m_game->m_sprite[InterfaceNdButton]->draw(dlgX + 44, dlgY + 208, okFrame);

    // Cancel button
    int cancelFrame = (m_iCurFocus == 6) ? 17 : 16;
    m_game->m_sprite[InterfaceNdButton]->draw(dlgX + 217, dlgY + 208, cancelFrame);

    draw_version();
}
