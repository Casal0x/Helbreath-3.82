// Overlay_ChangePassword.cpp: Password change overlay
//
//////////////////////////////////////////////////////////////////////

#include "Overlay_ChangePassword.h"
#include "Game.h"
#include "GameModeManager.h"
#include "CommonTypes.h"
#include "lan_eng.h"
#include "IInput.h"
#include "XSocket.h"
#include "Misc.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include "Packet/SharedPackets.h"

Overlay_ChangePassword::Overlay_ChangePassword(CGame* pGame)
    : IGameScreen(pGame)
    , m_iCurFocus(2)
    , m_iPrevFocus(2)
    , m_iMaxFocus(6)
{
    std::memset(m_cAccountName, 0, sizeof(m_cAccountName));
    std::memset(m_cOldPassword, 0, sizeof(m_cOldPassword));
    std::memset(m_cNewPassword, 0, sizeof(m_cNewPassword));
    std::memset(m_cConfirmPassword, 0, sizeof(m_cConfirmPassword));
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
    std::memset(m_cAccountName, 0, sizeof(m_cAccountName));
    std::memset(m_cOldPassword, 0, sizeof(m_cOldPassword));
    std::memset(m_cNewPassword, 0, sizeof(m_cNewPassword));
    std::memset(m_cConfirmPassword, 0, sizeof(m_cConfirmPassword));

    // Copy account name from player
    strcpy(m_cAccountName, m_pGame->m_pPlayer->m_cAccountName);

    // Start input on old password field
    int dlgX, dlgY;
    GetCenteredDialogPos(DEF_SPRID_INTERFACE_ND_GAME4, 0, dlgX, dlgY);
    StartInputString(dlgX + 161, dlgY + 67, 11, m_cOldPassword);
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
        GetCenteredDialogPos(DEF_SPRID_INTERFACE_ND_GAME4, 0, dlgX, dlgY);

        EndInputString();
        switch (m_iCurFocus)
        {
        case 1:
            StartInputString(dlgX + 161, dlgY + 43, 11, m_cAccountName);
            break;
        case 2:
            StartInputString(dlgX + 161, dlgY + 67, 11, m_cOldPassword);
            break;
        case 3:
            StartInputString(dlgX + 161, dlgY + 91, 11, m_cNewPassword);
            break;
        case 4:
            StartInputString(dlgX + 161, dlgY + 115, 11, m_cConfirmPassword);
            break;
        }
        m_iPrevFocus = m_iCurFocus;
    }
}

bool Overlay_ChangePassword::ValidateInputs()
{
    // Check account name
    if (!CMisc::bCheckValidString(m_cAccountName) || strlen(m_cAccountName) == 0)
        return false;

    // Check old password
    if (!CMisc::bCheckValidString(m_cOldPassword) || strlen(m_cOldPassword) == 0)
        return false;

    // Check new password (must be valid name format)
    if (!CMisc::bCheckValidName(m_cNewPassword) || strlen(m_cNewPassword) < 8)
        return false;

    // Check confirm password matches
    if (!CMisc::bCheckValidName(m_cConfirmPassword))
        return false;

    if (memcmp(m_cNewPassword, m_cConfirmPassword, DEF_ACCOUNT_PASS - 1) != 0)
        return false;

    // New password must be different from old
    if (memcmp(m_cOldPassword, m_cNewPassword, DEF_ACCOUNT_PASS - 1) == 0)
        return false;

    return true;
}

void Overlay_ChangePassword::HandleSubmit()
{
    if (!ValidateInputs())
        return;

    EndInputString();

    // Copy account name/password to player session
    std::memset(m_pGame->m_pPlayer->m_cAccountName, 0, sizeof(m_pGame->m_pPlayer->m_cAccountName));
    std::memset(m_pGame->m_pPlayer->m_cAccountPassword, 0, sizeof(m_pGame->m_pPlayer->m_cAccountPassword));
    strcpy(m_pGame->m_pPlayer->m_cAccountName, m_cAccountName);
    strcpy(m_pGame->m_pPlayer->m_cAccountPassword, m_cOldPassword);

    // Build ChangePasswordRequest packet
    hb::net::ChangePasswordRequest req{};
    req.header.msg_id = MSGID_REQUEST_CHANGEPASSWORD;
    req.header.msg_type = 0;
    std::memcpy(req.account_name, m_cAccountName, sizeof(req.account_name));
    std::memcpy(req.password, m_cOldPassword, sizeof(req.password));
    std::memcpy(req.new_password, m_cNewPassword, sizeof(req.new_password));
    std::memcpy(req.new_password_confirm, m_cConfirmPassword, sizeof(req.new_password_confirm));

    // Store packet for sending after connection completes
    auto* p = reinterpret_cast<char*>(&req);
    m_pGame->m_pendingLoginPacket.assign(p, p + sizeof(req));

    // Create connection
    m_pGame->m_pLSock = std::make_unique<XSocket>(DEF_SOCKETBLOCKLIMIT);
    m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort + (rand() % 1));
    m_pGame->m_pLSock->bInitBufferSize(DEF_MSGBUFFERSIZE);

    m_pGame->m_dwConnectMode = MSGID_REQUEST_CHANGEPASSWORD;
    std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
    strcpy(m_pGame->m_cMsg, "41");

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
    if (Input::IsKeyPressed(VK_RETURN))
    {
        PlaySound('E', 14, 5);
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
    if (Input::IsKeyPressed(VK_ESCAPE))
    {
        m_pGame->ChangeGameMode(GameMode::MainMenu);
        return;
    }

    // Mouse click detection
    int dlgX, dlgY;
    GetCenteredDialogPos(DEF_SPRID_INTERFACE_ND_GAME4, 0, dlgX, dlgY);

    if (Input::IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        PlaySound('E', 14, 5);

        int iClickedField = 0;
        if (Input::IsMouseInRect(dlgX + 147, dlgY + 36, dlgX + 272, dlgY + 58)) iClickedField = 1;
        else if (Input::IsMouseInRect(dlgX + 147, dlgY + 60, dlgX + 272, dlgY + 82)) iClickedField = 2;
        else if (Input::IsMouseInRect(dlgX + 147, dlgY + 84, dlgX + 272, dlgY + 106)) iClickedField = 3;
        else if (Input::IsMouseInRect(dlgX + 147, dlgY + 108, dlgX + 272, dlgY + 130)) iClickedField = 4;
        else if (Input::IsMouseInRect(dlgX + 44, dlgY + 208, dlgX + 44 + DEF_BTNSZX, dlgY + 208 + DEF_BTNSZY)) iClickedField = 5;
        else if (Input::IsMouseInRect(dlgX + 217, dlgY + 208, dlgX + 217 + DEF_BTNSZX, dlgY + 208 + DEF_BTNSZY)) iClickedField = 6;

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
    if (Input::IsMouseInRect(dlgX + 44, dlgY + 208, dlgX + 44 + DEF_BTNSZX, dlgY + 208 + DEF_BTNSZY))
        m_iCurFocus = 5;
    if (Input::IsMouseInRect(dlgX + 217, dlgY + 208, dlgX + 217 + DEF_BTNSZX, dlgY + 208 + DEF_BTNSZY))
        m_iCurFocus = 6;

    // Update input field focus
    UpdateFocusedInput();
}

void Overlay_ChangePassword::on_render()
{
    bool bValidInputs = ValidateInputs();

    int dlgX, dlgY;
    GetCenteredDialogPos(DEF_SPRID_INTERFACE_ND_GAME4, 0, dlgX, dlgY);

    // Draw dialog boxes
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME4, dlgX, dlgY, 0);
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_TEXT, dlgX, dlgY, 13);
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME4, dlgX + 157, dlgY + 109, 7);

    // Draw labels
    PutString(dlgX + 53, dlgY + 43, UPDATE_SCREEN_ON_CHANGE_PASSWORD1, GameColors::UILabel.ToColorRef());
    PutString(dlgX + 53, dlgY + 67, UPDATE_SCREEN_ON_CHANGE_PASSWORD2, GameColors::UILabel.ToColorRef());
    PutString(dlgX + 53, dlgY + 91, UPDATE_SCREEN_ON_CHANGE_PASSWORD3, GameColors::UILabel.ToColorRef());
    PutString(dlgX + 53, dlgY + 115, UPDATE_SCREEN_ON_CHANGE_PASSWORD4, GameColors::UILabel.ToColorRef());

    // Draw input field values (when not focused)
    if (m_iCurFocus != 1)
    {
        uint32_t color = CMisc::bCheckValidString(m_cAccountName) ? GameColors::UILabel.ToColorRef() : RGB(55, 18, 13);
        PutString(dlgX + 161, dlgY + 43, m_cAccountName, color);
    }

    if (m_iCurFocus != 2)
    {
        uint32_t color = CMisc::bCheckValidString(m_cOldPassword) ? GameColors::UILabel.ToColorRef() : RGB(55, 18, 13);
        std::string maskedOld(strlen(m_cOldPassword), '*');
        TextLib::DrawText(GameFont::Default, dlgX + 161, dlgY + 67, maskedOld.c_str(), TextLib::TextStyle::FromColorRef(color));
    }

    if (m_iCurFocus != 3)
    {
        uint32_t color = CMisc::bCheckValidName(m_cNewPassword) ? GameColors::UILabel.ToColorRef() : RGB(55, 18, 13);
        std::string maskedNew(strlen(m_cNewPassword), '*');
        TextLib::DrawText(GameFont::Default, dlgX + 161, dlgY + 91, maskedNew.c_str(), TextLib::TextStyle::FromColorRef(color));
    }

    if (m_iCurFocus != 4)
    {
        uint32_t color = CMisc::bCheckValidName(m_cConfirmPassword) ? GameColors::UILabel.ToColorRef() : RGB(55, 18, 13);
        std::string maskedConfirm(strlen(m_cConfirmPassword), '*');
        TextLib::DrawText(GameFont::Default, dlgX + 161, dlgY + 115, maskedConfirm.c_str(), TextLib::TextStyle::FromColorRef(color));
    }

    // Show active input string (with masking for password fields)
    if (m_iCurFocus == 1)
        ShowReceivedString();
    else if (m_iCurFocus >= 2 && m_iCurFocus <= 4)
        ShowReceivedString(true);  // Hide (mask) password

    // Help text
    PutAlignedString(dlgX, dlgX + 334, dlgY + 146, UPDATE_SCREEN_ON_CHANGE_PASSWORD5);
    PutAlignedString(dlgX, dlgX + 334, dlgY + 161, UPDATE_SCREEN_ON_CHANGE_PASSWORD6);
    PutAlignedString(dlgX, dlgX + 334, dlgY + 176, UPDATE_SCREEN_ON_CHANGE_PASSWORD7);

    // OK button (enabled only when inputs are valid)
    int okFrame = (bValidInputs && m_iCurFocus == 5) ? 21 : 20;
    m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(dlgX + 44, dlgY + 208, okFrame);

    // Cancel button
    int cancelFrame = (m_iCurFocus == 6) ? 17 : 16;
    m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(dlgX + 217, dlgY + 208, cancelFrame);

    DrawVersion();
}
