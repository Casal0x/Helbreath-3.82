#include "Screen_CreateAccount.h"
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
#include "Packet/SharedPackets.h"
#include <string>


using namespace hb::shared::net;
using namespace hb::client::sprite_id;
namespace MouseButton = hb::shared::input::MouseButton;


Screen_CreateAccount::Screen_CreateAccount(CGame* game)
    : IGameScreen(game), m_cur_focus(1), m_max_focus(7)
{
    _clear_fields();

    m_cNewAcctPrevFocus = 1;
    m_sNewAcctMsX = 0;
    m_sNewAcctMsY = 0;
    m_cNewAcctPrevLB = 0;
}

Screen_CreateAccount::~Screen_CreateAccount()
{
}

void Screen_CreateAccount::_clear_fields()
{
    m_cNewAcctName.clear();
    m_cNewAcctPassword.clear();
    m_cNewAcctConfirm.clear();
    m_cEmail.clear();
}

void Screen_CreateAccount::on_initialize()
{
    // Set current mode for code that checks GameModeManager::get_mode()
    GameModeManager::set_current_mode(GameMode::CreateNewAccount);

    text_input_manager::get().end_input();

    m_cNewAcctPrevFocus = 1;
    m_cNewAcctPrevLB = 0;
    m_cur_focus = 1;
    m_max_focus = 7;
    m_game->m_arrow_pressed = 0;

    _clear_fields();

    text_input_manager::get().start_input(340, 210, 11, m_cNewAcctName);
    text_input_manager::get().clear_input();
}

void Screen_CreateAccount::on_uninitialize()
{
    text_input_manager::get().end_input();
}

void Screen_CreateAccount::on_update()
{
    uint32_t time = GameClock::get_time_ms();
    m_game->m_cur_time = time;

    m_sNewAcctMsX = static_cast<short>(hb::shared::input::get_mouse_x());
    m_sNewAcctMsY = static_cast<short>(hb::shared::input::get_mouse_y());
    char lb = hb::shared::input::is_mouse_button_down(MouseButton::Left) ? 1 : 0;

    // Handle arrow key navigation
    if (m_game->m_arrow_pressed != 0)
    {
        switch (m_game->m_arrow_pressed) {
        case 1: // Up
            m_cur_focus--;
            if (m_cur_focus <= 0) m_cur_focus = m_max_focus;
            break;
        case 3: // Down
            m_cur_focus++;
            if (m_cur_focus > m_max_focus) m_cur_focus = 1;
            break;
        }
        m_game->m_arrow_pressed = 0;
    }

    // Handle focus change - switch input field
    if (m_cNewAcctPrevFocus != m_cur_focus)
    {
        text_input_manager::get().end_input();
        switch (m_cur_focus) {
        case 1: text_input_manager::get().start_input(340, 210, 11, m_cNewAcctName); break;
        case 2: text_input_manager::get().start_input(340, 232, 11, m_cNewAcctPassword, true); break;
        case 3: text_input_manager::get().start_input(340, 254, 11, m_cNewAcctConfirm, true); break;
        case 4: text_input_manager::get().start_input(340, 276, 49, m_cEmail); break;
        }
        m_cNewAcctPrevFocus = m_cur_focus;
    }

    // Direct mouse click focus selection
    if (lb != 0 && m_cNewAcctPrevLB == 0)
    {
        if (hb::shared::input::is_mouse_in_rect(340, 210, 250, 18)) m_cur_focus = 1;
        if (hb::shared::input::is_mouse_in_rect(340, 232, 250, 18)) m_cur_focus = 2;
        if (hb::shared::input::is_mouse_in_rect(340, 254, 250, 18)) m_cur_focus = 3;
        if (hb::shared::input::is_mouse_in_rect(340, 276, 250, 18)) m_cur_focus = 4;

        // Button 5: Create
        if (hb::shared::input::is_mouse_in_rect(297, 398, 72, 20))
        {
            m_cur_focus = 5;
            m_game->play_game_sound('E', 14, 5);
            _submit_create_account();
        }
        // Button 6: clear
        if (hb::shared::input::is_mouse_in_rect(392, 398, 72, 20))
        {
            m_cur_focus = 6;
            m_game->play_game_sound('E', 14, 5);
            _clear_fields();
            m_cur_focus = 1;
            m_cNewAcctPrevFocus = 0; // Trigger reset
        }
        // Button 7: Cancel
        if (hb::shared::input::is_mouse_in_rect(488, 398, 72, 20))
        {
            m_cur_focus = 7;
            m_game->play_game_sound('E', 14, 5);
            m_game->change_game_mode(GameMode::MainMenu);
        }
    }
    m_cNewAcctPrevLB = lb;

    if (hb::shared::input::is_key_pressed(KeyCode::Escape) == true)
    {
        m_game->change_game_mode(GameMode::MainMenu);
        return;
    }

    // Handle Tab key
    if (hb::shared::input::is_key_pressed(KeyCode::Tab))
    {
        m_game->play_game_sound('E', 14, 5);
        if (hb::shared::input::is_shift_down())
        {
            m_cur_focus--;
            if (m_cur_focus < 1) m_cur_focus = 4;
        }
        else
        {
            m_cur_focus++;
            if (m_cur_focus > 4) m_cur_focus = 1;
        }
    }

    // Handle Enter key
    if (hb::shared::input::is_key_pressed(KeyCode::Enter))
    {
        m_game->play_game_sound('E', 14, 5);

        if (m_cur_focus <= 4)
        {
            m_cur_focus++;
            if (m_cur_focus > 4) m_cur_focus = 5; // Move to Create button
        }
        else if (m_cur_focus == 5)
        {
            // Trigger Create action
            _submit_create_account();
        }
    }
}

void Screen_CreateAccount::_submit_create_account()
{
    bool ready = (!m_cNewAcctName.empty() && !m_cNewAcctPassword.empty() &&
        !m_cNewAcctConfirm.empty() && CMisc::is_valid_email(m_cEmail.data()) &&
        CMisc::check_valid_name(m_cNewAcctName.data()) && CMisc::check_valid_name(m_cNewAcctPassword.data()) &&
        m_cNewAcctPassword == m_cNewAcctConfirm);

    if (ready)
    {
        m_game->m_arrow_pressed = 0;

        // Copy account name/password to player session
        m_game->m_player->m_account_name = m_cNewAcctName.c_str();
        m_game->m_player->m_account_password = m_cNewAcctPassword.c_str();

        // Build CreateAccountRequest packet
        hb::net::CreateAccountRequest req{};
        req.header.msg_id = MsgId::RequestCreateNewAccount;
        req.header.msg_type = 0;
        std::snprintf(req.account_name, sizeof(req.account_name), "%s", m_cNewAcctName.c_str());
        std::snprintf(req.password, sizeof(req.password), "%s", m_cNewAcctPassword.c_str());
        std::snprintf(req.email, sizeof(req.email), "%s", m_cEmail.c_str());

        // Store packet for sending after connection completes
        auto* p = reinterpret_cast<char*>(&req);
        m_game->m_pending_login_packet.assign(p, p + sizeof(req));

        // Connection logic
        m_game->m_l_sock = std::make_unique<hb::shared::net::ASIOSocket>(m_game->m_io_pool->get_context(), game_limits::socket_block_limit);
        m_game->m_l_sock->connect(m_game->m_log_server_addr.c_str(), m_game->m_log_server_port);
        m_game->m_l_sock->init_buffer_size(hb::shared::limits::MsgBufferSize);

        m_game->change_game_mode(GameMode::Connecting);
        m_game->m_connect_mode = MsgId::RequestCreateNewAccount;
        std::snprintf(m_game->m_msg, sizeof(m_game->m_msg), "%s", "01");
    }
}

bool Screen_CreateAccount::on_net_response(uint16_t response_type, char* data)
{
	switch (response_type) {
	case LogResMsg::NewAccountCreated:
		m_game->m_l_sock.reset();
		std::snprintf(m_game->m_msg, sizeof(m_game->m_msg), "%s", "54");
		m_game->change_game_mode(GameMode::LogResMsg);
		return true;

	case LogResMsg::NewAccountFailed:
		m_game->m_l_sock.reset();
		std::snprintf(m_game->m_msg, sizeof(m_game->m_msg), "%s", "05");
		m_game->change_game_mode(GameMode::LogResMsg);
		return true;

	case LogResMsg::AlreadyExistingAccount:
		m_game->m_l_sock.reset();
		std::snprintf(m_game->m_msg, sizeof(m_game->m_msg), "%s", "06");
		m_game->change_game_mode(GameMode::LogResMsg);
		return true;
	}
	return false;
}

void Screen_CreateAccount::on_render()
{
    int flag = 0;

    // Compute validation flags for display
    if (m_cNewAcctPassword != m_cNewAcctConfirm)                                 flag = 9;
    if (CMisc::check_valid_name(m_cNewAcctPassword.data()) == false)     flag = 7;
    if (CMisc::check_valid_name(m_cNewAcctName.data()) == false)         flag = 6;
    if (CMisc::is_valid_email(m_cEmail.data()) == false)                 flag = 5;
    if (m_cNewAcctConfirm.empty())                                       flag = 3;
    if (m_cNewAcctPassword.empty())                                      flag = 2;
    if (m_cNewAcctName.empty())                                          flag = 1;

    auto labelStyle = hb::shared::text::TextStyle::from_color(GameColors::UINearWhite).with_bold();
    auto helpStyle = hb::shared::text::TextStyle::from_color(hb::shared::render::Color(255, 181, 0));

    // draw background
    m_game->draw_new_dialog_box(InterfaceNdNewAccount, 0, 0, 0, true);

    // draw backdrop panel behind input fields
    m_game->m_Renderer->draw_rounded_rect_filled(258, 198, 344, 108, 8, hb::shared::render::Color::Black(200));

    // draw labels
    hb::shared::text::draw_text(GameFont::Default, 270, 210, "Account:", labelStyle);
    hb::shared::text::draw_text(GameFont::Default, 270, 232, "Password:", labelStyle);
    hb::shared::text::draw_text(GameFont::Default, 270, 254, "Confirm:", labelStyle);
    hb::shared::text::draw_text(GameFont::Default, 270, 276, "Email:", labelStyle);

    // Show active input string
    if ((m_cur_focus == 2) || (m_cur_focus == 3))
        text_input_manager::get().show_input();
    else if ((m_cur_focus == 1) || (m_cur_focus == 4))
        text_input_manager::get().show_input();

    // draw input field values with validation coloring
    auto validStyle = hb::shared::text::TextStyle::with_shadow(GameColors::InputValid);
    auto invalidStyle = hb::shared::text::TextStyle::with_shadow(GameColors::InputInvalid);

    if (m_cur_focus != 1) {
        bool valid = CMisc::check_valid_name(m_cNewAcctName.data()) != false;
        hb::shared::text::draw_text(GameFont::Default, 340, 210, m_cNewAcctName.c_str(), valid ? validStyle : invalidStyle);
    }
    if (m_cur_focus != 2) {
        std::string masked2(m_cNewAcctPassword.size(), '*');
        bool valid = CMisc::check_valid_name(m_cNewAcctPassword.data()) != false;
        hb::shared::text::draw_text(GameFont::Default, 340, 232, masked2.c_str(), valid ? validStyle : invalidStyle);
    }
    if (m_cur_focus != 3) {
        std::string masked3(m_cNewAcctConfirm.size(), '*');
        bool valid = (m_cNewAcctPassword == m_cNewAcctConfirm);
        hb::shared::text::draw_text(GameFont::Default, 340, 254, masked3.c_str(), valid ? validStyle : invalidStyle);
    }
    if (m_cur_focus != 4) {
        bool valid = CMisc::is_valid_email(m_cEmail.data());
        hb::shared::text::draw_text(GameFont::Default, 340, 276, m_cEmail.c_str(), valid ? validStyle : invalidStyle);
    }

    // draw help text based on focus
    switch (m_cur_focus) {
    case 1:
        hb::shared::text::draw_text_aligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT1, helpStyle, hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 290, 345, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT2, helpStyle, hb::shared::text::Align::TopCenter);
        break;
    case 2:
        hb::shared::text::draw_text_aligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT4, helpStyle, hb::shared::text::Align::TopCenter);
        break;
    case 3:
        hb::shared::text::draw_text_aligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT8, helpStyle, hb::shared::text::Align::TopCenter);
        break;
    case 4:
        hb::shared::text::draw_text_aligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT21, helpStyle, hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 290, 345, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT22, helpStyle, hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, 290, 360, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT23, helpStyle, hb::shared::text::Align::TopCenter);
        break;
    case 5:
        switch (flag) {
        case 0:
            hb::shared::text::draw_text_aligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT33, helpStyle, hb::shared::text::Align::TopCenter);
            break;
        case 1:
            hb::shared::text::draw_text_aligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT35, helpStyle, hb::shared::text::Align::TopCenter);
            break;
        case 2:
            hb::shared::text::draw_text_aligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT38, helpStyle, hb::shared::text::Align::TopCenter);
            break;
        case 3:
            hb::shared::text::draw_text_aligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT42, helpStyle, hb::shared::text::Align::TopCenter);
            break;
        case 5:
            hb::shared::text::draw_text_aligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT50, helpStyle, hb::shared::text::Align::TopCenter);
            break;
        case 6:
            hb::shared::text::draw_text_aligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT52, helpStyle, hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 290, 345, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT53, helpStyle, hb::shared::text::Align::TopCenter);
            break;
        case 7:
            hb::shared::text::draw_text_aligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT56, helpStyle, hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 290, 345, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT57, helpStyle, hb::shared::text::Align::TopCenter);
            break;
        case 9:
            hb::shared::text::draw_text_aligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT63, helpStyle, hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 290, 345, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT64, helpStyle, hb::shared::text::Align::TopCenter);
            hb::shared::text::draw_text_aligned(GameFont::Default, 290, 360, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT65, helpStyle, hb::shared::text::Align::TopCenter);
            break;
        }
        break;
    case 6:
        hb::shared::text::draw_text_aligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT80, helpStyle, hb::shared::text::Align::TopCenter);
        break;
    case 7:
        hb::shared::text::draw_text_aligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT81, helpStyle, hb::shared::text::Align::TopCenter);
        break;
    }

    // draw buttons - highlight on focus OR mouse hover
    // Button 5: Create (at 297, 398 - size 72x20) - LEFT
    bool hover_create = (m_sNewAcctMsX >= 297 && m_sNewAcctMsX <= 297 + 72 &&
        m_sNewAcctMsY >= 398 && m_sNewAcctMsY <= 398 + 20);
    if ((flag == 0) && (m_cur_focus == 5 || hover_create))
        m_game->m_sprite[InterfaceNdButton]->draw(199 + 98, 398, 25);
    else m_game->m_sprite[InterfaceNdButton]->draw(199 + 98, 398, 24);

    // Button 6: clear (at 392, 398 - size 72x20) - CENTER
    bool hover_clear = (m_sNewAcctMsX >= 392 && m_sNewAcctMsX <= 392 + 72 &&
        m_sNewAcctMsY >= 398 && m_sNewAcctMsY <= 398 + 20);
    if (m_cur_focus == 6 || hover_clear)
        m_game->m_sprite[InterfaceNdButton]->draw(294 + 98, 398, 27);
    else m_game->m_sprite[InterfaceNdButton]->draw(294 + 98, 398, 26);

    // Button 7: Cancel (at 488, 398 - size 72x20) - RIGHT
    bool hover_cancel = (m_sNewAcctMsX >= 488 && m_sNewAcctMsX <= 488 + 72 &&
        m_sNewAcctMsY >= 398 && m_sNewAcctMsY <= 398 + 20);
    if (m_cur_focus == 7 || hover_cancel)
        m_game->m_sprite[InterfaceNdButton]->draw(390 + 98, 398, 17);
    else m_game->m_sprite[InterfaceNdButton]->draw(390 + 98, 398, 16);

    m_game->draw_version();
}

