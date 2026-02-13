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

Overlay_ChangePassword::Overlay_ChangePassword(CGame* pGame)
    : IGameScreen(pGame)
    , m_iCurFocus(2)
    , m_iPrevFocus(2)
    , m_iMaxFocus(6)
{
}

void Overlay_ChangePassword::on_initialize()
{
    m_dwStartTime = GameClock::GetTimeMS();
    m_dwAnimTime = m_dwStartTime;

    // End any existing input string
    EndInputString();

    // Initialize focus
    m_iPrevFocus = 2;
    m_iCurFocus = 2;
    m_iMaxFocus = 6;
    m_pGame->m_cArrowPressed = 0;

    // Clear input buffers
    m_cAccountName.clear();
    m_cOldPassword.clear();
    m_cNewPassword.clear();
    m_cConfirmPassword.clear();

    // Copy account name from player
    m_cAccountName = m_pGame->m_pPlayer->m_cAccountName;

    // Start input on old password field
    int dlgX, dlgY;
    GetCenteredDialogPos(InterfaceNdGame4, 0, dlgX, dlgY);
    StartInputString(dlgX + 161, dlgY + 67, 11, m_cOldPassword, true);
    ClearInputString();
}

void Overlay_ChangePassword::on_uninitialize()
{
    EndInputString();
}

void Overlay_ChangePassword::UpdateFocusedInput()
{
    if (m_iPrevFocus != m_iCurFocus)
    {
        int dlgX, dlgY;
        GetCenteredDialogPos(InterfaceNdGame4, 0, dlgX, dlgY);

        EndInputString();
        switch (m_iCurFocus)
        {
        case 1:
            StartInputString(dlgX + 161, dlgY + 43, 11, m_cAccountName);
            break;
        case 2:
            StartInputString(dlgX + 161, dlgY + 67, 11, m_cOldPassword, true);
            break;
        case 3:
            StartInputString(dlgX + 161, dlgY + 91, 11, m_cNewPassword, true);
            break;
        case 4:
            StartInputString(dlgX + 161, dlgY + 115, 11, m_cConfirmPassword, true);
            break;
        }
        m_iPrevFocus = m_iCurFocus;
    }
}

bool Overlay_ChangePassword::ValidateInputs()
{
    // Check account name
    if (!CMisc::bCheckValidString(m_cAccountName.data()) || m_cAccountName.empty())
        return false;

    // Check old password
    if (!CMisc::bCheckValidString(m_cOldPassword.data()) || m_cOldPassword.empty())
        return false;

    // Check new password
    if (!CMisc::bCheckValidString(m_cNewPassword.data()) || m_cNewPassword.size() < 8)
        return false;

    // Check confirm password matches
    if (!CMisc::bCheckValidString(m_cConfirmPassword.data()))
        return false;

    if (m_cNewPassword != m_cConfirmPassword)
        return false;

    // New password must be different from old
    if (m_cOldPassword == m_cNewPassword)
        return false;

    return true;
}

void Overlay_ChangePassword::HandleSubmit()
{
    if (!ValidateInputs())
        return;

    EndInputString();

    // Copy account name/password to player session
    m_pGame->m_pPlayer->m_cAccountName = m_cAccountName;
    m_pGame->m_pPlayer->m_cAccountPassword = m_cOldPassword;

    // Build ChangePasswordRequest packet
    hb::net::ChangePasswordRequest req{};
    req.header.msg_id = MsgId::RequestChangePassword;
    req.header.msg_type = 0;
    std::snprintf(req.account_name, sizeof(req.account_name), "%s", m_cAccountName.c_str());
    std::snprintf(req.password, sizeof(req.password), "%s", m_cOldPassword.c_str());
    std::snprintf(req.new_password, sizeof(req.new_password), "%s", m_cNewPassword.c_str());
    std::snprintf(req.new_password_confirm, sizeof(req.new_password_confirm), "%s", m_cConfirmPassword.c_str());

    // Store packet for sending after connection completes
    auto* p = reinterpret_cast<char*>(&req);
    m_pGame->m_pendingLoginPacket.assign(p, p + sizeof(req));

    // Create connection
    m_pGame->m_pLSock = std::make_unique<hb::shared::net::ASIOSocket>(m_pGame->m_pIOPool->GetContext(), game_limits::socket_block_limit);
    m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr.c_str(), m_pGame->m_iLogServerPort);
    m_pGame->m_pLSock->bInitBufferSize(hb::shared::limits::MsgBufferSize);

    m_pGame->m_dwConnectMode = MsgId::RequestChangePassword;
    std::snprintf(m_pGame->m_cMsg, sizeof(m_pGame->m_cMsg), "%s", "41");

    // set_overlay will clear this overlay automatically
    m_pGame->ChangeGameMode(GameMode::Connecting);
}

void Overlay_ChangePassword::on_update()
{
    uint32_t dwTime = GameClock::GetTimeMS();

    // Animation frame updates
    if ((dwTime - m_dwAnimTime) > 100)
    {
        m_pGame->m_cMenuFrame++;
        m_dwAnimTime = dwTime;
    }
    if (m_pGame->m_cMenuFrame >= 8)
    {
        m_pGame->m_cMenuDirCnt++;
        if (m_pGame->m_cMenuDirCnt > 8)
        {
            m_pGame->m_cMenuDir++;
            m_pGame->m_cMenuDirCnt = 1;
        }
        if (m_pGame->m_cMenuDir > 8) m_pGame->m_cMenuDir = 1;
        m_pGame->m_cMenuFrame = 0;
    }

    // Tab key navigation (consistent with Login and CreateAccount screens)
    if (hb::shared::input::is_key_pressed(KeyCode::Tab))
    {
        PlayGameSound('E', 14, 5);
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
    if (m_pGame->m_cArrowPressed != 0)
    {
        switch (m_pGame->m_cArrowPressed)
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
        m_pGame->m_cArrowPressed = 0;
    }

    // Enter key
    if (hb::shared::input::is_key_pressed(KeyCode::Enter))
    {
        PlayGameSound('E', 14, 5);
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
            HandleSubmit();
            return;
        case 6: // Cancel button - return to base screen
            clear_overlay();
            return;
        }
    }

    // ESC key - return to main menu (set_screen will clear overlay automatically)
    if (hb::shared::input::is_key_pressed(KeyCode::Escape))
    {
        m_pGame->ChangeGameMode(GameMode::MainMenu);
        return;
    }

    // Mouse click detection
    int dlgX, dlgY;
    GetCenteredDialogPos(InterfaceNdGame4, 0, dlgX, dlgY);

    if (hb::shared::input::is_mouse_button_pressed(MouseButton::Left))
    {
        PlayGameSound('E', 14, 5);

        int iClickedField = 0;
        if (hb::shared::input::is_mouse_in_rect(dlgX + 147, dlgY + 36, 125, 22)) iClickedField = 1;
        else if (hb::shared::input::is_mouse_in_rect(dlgX + 147, dlgY + 60, 125, 22)) iClickedField = 2;
        else if (hb::shared::input::is_mouse_in_rect(dlgX + 147, dlgY + 84, 125, 22)) iClickedField = 3;
        else if (hb::shared::input::is_mouse_in_rect(dlgX + 147, dlgY + 108, 125, 22)) iClickedField = 4;
        else if (hb::shared::input::is_mouse_in_rect(dlgX + 44, dlgY + 208, ui_layout::btn_size_x, ui_layout::btn_size_y)) iClickedField = 5;
        else if (hb::shared::input::is_mouse_in_rect(dlgX + 217, dlgY + 208, ui_layout::btn_size_x, ui_layout::btn_size_y)) iClickedField = 6;

        switch (iClickedField)
        {
        case 1:
        case 2:
        case 3:
        case 4:
            m_iCurFocus = iClickedField;
            break;
        case 5: // OK
            HandleSubmit();
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

    // Update input field focus
    UpdateFocusedInput();
}

void Overlay_ChangePassword::on_render()
{
    bool bValidInputs = ValidateInputs();

    int dlgX, dlgY;
    GetCenteredDialogPos(InterfaceNdGame4, 0, dlgX, dlgY);

    // Draw dialog boxes
    DrawNewDialogBox(InterfaceNdGame4, dlgX, dlgY, 0);
    DrawNewDialogBox(InterfaceNdText, dlgX, dlgY, 13);
    DrawNewDialogBox(InterfaceNdGame4, dlgX + 157, dlgY + 109, 7);

    // Draw labels
    PutString(dlgX + 53, dlgY + 43, UPDATE_SCREEN_ON_CHANGE_PASSWORD1, GameColors::UILabel);
    PutString(dlgX + 53, dlgY + 67, UPDATE_SCREEN_ON_CHANGE_PASSWORD2, GameColors::UILabel);
    PutString(dlgX + 53, dlgY + 91, UPDATE_SCREEN_ON_CHANGE_PASSWORD3, GameColors::UILabel);
    PutString(dlgX + 53, dlgY + 115, UPDATE_SCREEN_ON_CHANGE_PASSWORD4, GameColors::UILabel);

    // Draw input field values (when not focused)
    static constexpr hb::shared::render::Color kInvalidInput{55, 18, 13};

    if (m_iCurFocus != 1)
    {
        const hb::shared::render::Color& color = CMisc::bCheckValidString(m_cAccountName.data()) ? GameColors::UILabel : kInvalidInput;
        PutString(dlgX + 161, dlgY + 43, m_cAccountName.c_str(), color);
    }

    if (m_iCurFocus != 2)
    {
        const hb::shared::render::Color& color = CMisc::bCheckValidString(m_cOldPassword.data()) ? GameColors::UILabel : kInvalidInput;
        std::string maskedOld(m_cOldPassword.size(), '*');
        hb::shared::text::DrawText(GameFont::Default, dlgX + 161, dlgY + 67, maskedOld.c_str(), hb::shared::text::TextStyle::Color(color));
    }

    if (m_iCurFocus != 3)
    {
        const hb::shared::render::Color& color = CMisc::bCheckValidName(m_cNewPassword.data()) ? GameColors::UILabel : kInvalidInput;
        std::string maskedNew(m_cNewPassword.size(), '*');
        hb::shared::text::DrawText(GameFont::Default, dlgX + 161, dlgY + 91, maskedNew.c_str(), hb::shared::text::TextStyle::Color(color));
    }

    if (m_iCurFocus != 4)
    {
        const hb::shared::render::Color& color = CMisc::bCheckValidName(m_cConfirmPassword.data()) ? GameColors::UILabel : kInvalidInput;
        std::string maskedConfirm(m_cConfirmPassword.size(), '*');
        hb::shared::text::DrawText(GameFont::Default, dlgX + 161, dlgY + 115, maskedConfirm.c_str(), hb::shared::text::TextStyle::Color(color));
    }

    // Show active input string (with masking for password fields)
    if (m_iCurFocus == 1)
        ShowReceivedString();
    else if (m_iCurFocus >= 2 && m_iCurFocus <= 4)
        ShowReceivedString();  // Hide (mask) password

    // Help text
    PutAlignedString(dlgX, dlgX + 334, dlgY + 146, UPDATE_SCREEN_ON_CHANGE_PASSWORD5);
    PutAlignedString(dlgX, dlgX + 334, dlgY + 161, UPDATE_SCREEN_ON_CHANGE_PASSWORD6);
    PutAlignedString(dlgX, dlgX + 334, dlgY + 176, UPDATE_SCREEN_ON_CHANGE_PASSWORD7);

    // OK button (enabled only when inputs are valid)
    int okFrame = (bValidInputs && m_iCurFocus == 5) ? 21 : 20;
    m_pGame->m_pSprite[InterfaceNdButton]->Draw(dlgX + 44, dlgY + 208, okFrame);

    // Cancel button
    int cancelFrame = (m_iCurFocus == 6) ? 17 : 16;
    m_pGame->m_pSprite[InterfaceNdButton]->Draw(dlgX + 217, dlgY + 208, cancelFrame);

    DrawVersion();
}
