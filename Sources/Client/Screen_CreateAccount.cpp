#include "Screen_CreateAccount.h"
#include "Game.h"
#include "GameModeManager.h"
#include "IInput.h"
#include "GlobalDef.h"
#include "ASIOSocket.h"
#include "Misc.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include "Packet/SharedPackets.h"

#ifdef DEF_MAKE_ACCOUNT

Screen_CreateAccount::Screen_CreateAccount(CGame* pGame)
    : IGameScreen(pGame), m_cCurFocus(1), m_cMaxFocus(7)
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
    std::memset(m_cNewAcctName, 0, sizeof(m_cNewAcctName));
    std::memset(m_cNewAcctPassword, 0, sizeof(m_cNewAcctPassword));
    std::memset(m_cNewAcctConfirm, 0, sizeof(m_cNewAcctConfirm));
    std::memset(m_cEmail, 0, sizeof(m_cEmail));
}

void Screen_CreateAccount::on_initialize()
{
    // Set current mode for code that checks GameModeManager::GetMode()
    GameModeManager::SetCurrentMode(GameMode::CreateNewAccount);

    m_pGame->EndInputString();

    m_cNewAcctPrevFocus = 1;
    m_cNewAcctPrevLB = 0;
    m_cCurFocus = 1;
    m_cMaxFocus = 7;
    m_pGame->m_cArrowPressed = 0;

    _clear_fields();

    m_pGame->StartInputString(427, 84, 11, m_cNewAcctName);
    m_pGame->ClearInputString();
}

void Screen_CreateAccount::on_uninitialize()
{
}

void Screen_CreateAccount::on_update()
{
    uint32_t dwTime = GameClock::GetTimeMS();
    m_pGame->m_dwCurTime = dwTime;

    m_sNewAcctMsX = static_cast<short>(Input::GetMouseX());
    m_sNewAcctMsY = static_cast<short>(Input::GetMouseY());
    char cLB = Input::IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? 1 : 0;

    // Handle arrow key navigation
    if (m_pGame->m_cArrowPressed != 0)
    {
        switch (m_pGame->m_cArrowPressed) {
        case 1: // Up
            m_cCurFocus--;
            if (m_cCurFocus <= 0) m_cCurFocus = m_cMaxFocus;
            break;
        case 3: // Down
            m_cCurFocus++;
            if (m_cCurFocus > m_cMaxFocus) m_cCurFocus = 1;
            break;
        }
        m_pGame->m_cArrowPressed = 0;
    }

    // Handle focus change - switch input field
    if (m_cNewAcctPrevFocus != m_cCurFocus)
    {
        m_pGame->EndInputString();
        switch (m_cCurFocus) {
        case 1: m_pGame->StartInputString(427, 84, 11, m_cNewAcctName); break;
        case 2: m_pGame->StartInputString(427, 106, 11, m_cNewAcctPassword, true); break;
        case 3: m_pGame->StartInputString(427, 129, 11, m_cNewAcctConfirm, true); break;
        case 4: m_pGame->StartInputString(311, 48 + 190 - 25 + 2, 49, m_cEmail); break;
        }
        m_pGame->EndInputString();
        m_cNewAcctPrevFocus = m_cCurFocus;
    }

    // Direct mouse click focus selection
    if (cLB != 0 && m_cNewAcctPrevLB == 0)
    {
        if (Input::IsMouseInRect(427, 84, 100, 18)) m_cCurFocus = 1;
        if (Input::IsMouseInRect(427, 106, 100, 18)) m_cCurFocus = 2;
        if (Input::IsMouseInRect(427, 129, 100, 18)) m_cCurFocus = 3;
        if (Input::IsMouseInRect(311, 215, 250, 18)) m_cCurFocus = 4;

        // Button 5: Create
        if (Input::IsMouseInRect(297, 398, 72, 20))
        {
            m_cCurFocus = 5;
            m_pGame->PlayGameSound('E', 14, 5);
            _submit_create_account();
        }
        // Button 6: Clear
        if (Input::IsMouseInRect(392, 398, 72, 20))
        {
            m_cCurFocus = 6;
            m_pGame->PlayGameSound('E', 14, 5);
            _clear_fields();
            m_cCurFocus = 1;
            m_cNewAcctPrevFocus = 0; // Trigger reset
        }
        // Button 7: Cancel
        if (Input::IsMouseInRect(488, 398, 72, 20))
        {
            m_cCurFocus = 7;
            m_pGame->PlayGameSound('E', 14, 5);
            m_pGame->ChangeGameMode(GameMode::MainMenu);
        }
    }
    m_cNewAcctPrevLB = cLB;

    if (Input::IsKeyPressed(KeyCode::Escape) == true)
    {
        m_pGame->ChangeGameMode(GameMode::MainMenu);
        return;
    }

    // Handle Tab key
    if (Input::IsKeyPressed(KeyCode::Tab))
    {
        m_pGame->PlayGameSound('E', 14, 5);
        if (Input::IsShiftDown())
        {
            m_cCurFocus--;
            if (m_cCurFocus < 1) m_cCurFocus = 4;
        }
        else
        {
            m_cCurFocus++;
            if (m_cCurFocus > 4) m_cCurFocus = 1;
        }
    }

    // Handle Enter key
    if (Input::IsKeyPressed(KeyCode::Enter))
    {
        m_pGame->PlayGameSound('E', 14, 5);

        if (m_cCurFocus <= 4)
        {
            m_cCurFocus++;
            if (m_cCurFocus > 4) m_cCurFocus = 5; // Move to Create button
        }
        else if (m_cCurFocus == 5)
        {
            // Trigger Create action
            _submit_create_account();
        }
    }
}

void Screen_CreateAccount::_submit_create_account()
{
    bool bReady = (!name().empty() && !password().empty() &&
        !confirm().empty() && CMisc::bIsValidEmail(m_cEmail) &&
        CMisc::bCheckValidName(m_cNewAcctName) && CMisc::bCheckValidName(m_cNewAcctPassword) &&
        password() == confirm());

    if (bReady)
    {
        m_pGame->m_cArrowPressed = 0;

        // Copy account name/password to player session
        std::memset(m_pGame->m_pPlayer->m_cAccountName, 0, sizeof(m_pGame->m_pPlayer->m_cAccountName));
        std::snprintf(m_pGame->m_pPlayer->m_cAccountName, sizeof(m_pGame->m_pPlayer->m_cAccountName), "%s", m_cNewAcctName);
        std::memset(m_pGame->m_pPlayer->m_cAccountPassword, 0, sizeof(m_pGame->m_pPlayer->m_cAccountPassword));
        std::snprintf(m_pGame->m_pPlayer->m_cAccountPassword, sizeof(m_pGame->m_pPlayer->m_cAccountPassword), "%s", m_cNewAcctPassword);

        // Build CreateAccountRequest packet
        hb::net::CreateAccountRequest req{};
        req.header.msg_id = MSGID_REQUEST_CREATENEWACCOUNT;
        req.header.msg_type = 0;
        std::snprintf(req.account_name, sizeof(req.account_name), "%s", m_cNewAcctName);
        std::snprintf(req.password, sizeof(req.password), "%s", m_cNewAcctPassword);
        std::snprintf(req.email, sizeof(req.email), "%s", m_cEmail);

        // Store packet for sending after connection completes
        auto* p = reinterpret_cast<char*>(&req);
        m_pGame->m_pendingLoginPacket.assign(p, p + sizeof(req));

        // Connection logic
        m_pGame->m_pLSock = std::make_unique<ASIOSocket>(m_pGame->m_pIOPool->GetContext(), game_limits::socket_block_limit);
        m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort);
        m_pGame->m_pLSock->bInitBufferSize(DEF_MSGBUFFERSIZE);

        m_pGame->ChangeGameMode(GameMode::Connecting);
        m_pGame->m_dwConnectMode = MSGID_REQUEST_CREATENEWACCOUNT;
        std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
        std::snprintf(m_pGame->m_cMsg, sizeof(m_pGame->m_cMsg), "%s", "01");
    }
}

bool Screen_CreateAccount::on_net_response(uint16_t wResponseType, char* pData)
{
	switch (wResponseType) {
	case DEF_LOGRESMSGTYPE_NEWACCOUNTCREATED:
		m_pGame->m_pLSock.reset();
		std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
		std::snprintf(m_pGame->m_cMsg, sizeof(m_pGame->m_cMsg), "%s", "54");
		m_pGame->ChangeGameMode(GameMode::LogResMsg);
		return true;

	case DEF_LOGRESMSGTYPE_NEWACCOUNTFAILED:
		m_pGame->m_pLSock.reset();
		std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
		std::snprintf(m_pGame->m_cMsg, sizeof(m_pGame->m_cMsg), "%s", "05");
		m_pGame->ChangeGameMode(GameMode::LogResMsg);
		return true;

	case DEF_LOGRESMSGTYPE_ALREADYEXISTINGACCOUNT:
		m_pGame->m_pLSock.reset();
		std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
		std::snprintf(m_pGame->m_cMsg, sizeof(m_pGame->m_cMsg), "%s", "06");
		m_pGame->ChangeGameMode(GameMode::LogResMsg);
		return true;
	}
	return false;
}

void Screen_CreateAccount::on_render()
{
    int iFlag = 0;

    // Compute validation flags for display
    if (password() != confirm())                                 iFlag = 9;
    if (CMisc::bCheckValidName(m_cNewAcctPassword) == false)     iFlag = 7;
    if (CMisc::bCheckValidName(m_cNewAcctName) == false)         iFlag = 6;
    if (CMisc::bIsValidEmail(m_cEmail) == false)                 iFlag = 5;
    if (confirm().empty())                                       iFlag = 3;
    if (password().empty())                                      iFlag = 2;
    if (name().empty())                                          iFlag = 1;

    auto labelStyle = TextLib::TextStyle::Color(GameColors::UIFormLabel);
    auto blackStyle = TextLib::TextStyle::Color(GameColors::UIBlack);

    // Draw background
    m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_NEWACCOUNT, 0, 0, 0, true);

    // Draw labels
    TextLib::DrawText(GameFont::Default, 377, 84, "Account:", labelStyle);
    TextLib::DrawText(GameFont::Default, 372, 106, "Password:", labelStyle);
    TextLib::DrawText(GameFont::Default, 372, 129, "(confirm)", labelStyle);
    TextLib::DrawText(GameFont::Default, 271, 215, "eMail:", labelStyle);

    // Show active input string
    if ((m_cCurFocus == 2) || (m_cCurFocus == 3))
        m_pGame->ShowReceivedString(true);
    else if ((m_cCurFocus == 1) || (m_cCurFocus == 4))
        m_pGame->ShowReceivedString();

    // Draw input field values with validation coloring
    auto validStyle = TextLib::TextStyle::WithShadow(GameColors::InputValid);
    auto invalidStyle = TextLib::TextStyle::WithShadow(GameColors::InputInvalid);

    if (m_cCurFocus != 1) {
        bool bValid = CMisc::bCheckValidName(m_cNewAcctName) != false;
        TextLib::DrawText(GameFont::Default, 427, 84, m_cNewAcctName, bValid ? validStyle : invalidStyle);
    }
    if (m_cCurFocus != 2) {
        std::string masked2(password().size(), '*');
        bool bValid = CMisc::bCheckValidName(m_cNewAcctPassword) != false;
        TextLib::DrawText(GameFont::Default, 427, 106, masked2.c_str(), bValid ? validStyle : invalidStyle);
    }
    if (m_cCurFocus != 3) {
        std::string masked3(confirm().size(), '*');
        bool bValid = (password() == confirm());
        TextLib::DrawText(GameFont::Default, 427, 129, masked3.c_str(), bValid ? validStyle : invalidStyle);
    }
    if (m_cCurFocus != 4) {
        bool bValid = CMisc::bIsValidEmail(m_cEmail);
        TextLib::DrawText(GameFont::Default, 311, 48 + 190 - 25 + 2, m_cEmail, bValid ? validStyle : invalidStyle);
    }

    // Draw help text based on focus
    switch (m_cCurFocus) {
    case 1:
        TextLib::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT1, blackStyle, TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 290, 345, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT2, blackStyle, TextLib::Align::TopCenter);
        break;
    case 2:
        TextLib::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT4, blackStyle, TextLib::Align::TopCenter);
        break;
    case 3:
        TextLib::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT8, blackStyle, TextLib::Align::TopCenter);
        break;
    case 4:
        TextLib::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT21, blackStyle, TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 290, 345, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT22, blackStyle, TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 290, 360, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT23, blackStyle, TextLib::Align::TopCenter);
        break;
    case 5:
        switch (iFlag) {
        case 0:
            TextLib::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT33, blackStyle, TextLib::Align::TopCenter);
            break;
        case 1:
            TextLib::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT35, blackStyle, TextLib::Align::TopCenter);
            break;
        case 2:
            TextLib::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT38, blackStyle, TextLib::Align::TopCenter);
            break;
        case 3:
            TextLib::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT42, blackStyle, TextLib::Align::TopCenter);
            break;
        case 5:
            TextLib::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT50, blackStyle, TextLib::Align::TopCenter);
            break;
        case 6:
            TextLib::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT52, blackStyle, TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 290, 345, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT53, blackStyle, TextLib::Align::TopCenter);
            break;
        case 7:
            TextLib::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT56, blackStyle, TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 290, 345, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT57, blackStyle, TextLib::Align::TopCenter);
            break;
        case 9:
            TextLib::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT63, blackStyle, TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 290, 345, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT64, blackStyle, TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 290, 360, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT65, blackStyle, TextLib::Align::TopCenter);
            break;
        }
        break;
    case 6:
        TextLib::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT80, blackStyle, TextLib::Align::TopCenter);
        break;
    case 7:
        TextLib::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT81, blackStyle, TextLib::Align::TopCenter);
        break;
    }

    // Draw buttons - highlight on focus OR mouse hover
    // Button 5: Create (at 297, 398 - size 72x20) - LEFT
    bool bHoverCreate = (m_sNewAcctMsX >= 297 && m_sNewAcctMsX <= 297 + 72 &&
        m_sNewAcctMsY >= 398 && m_sNewAcctMsY <= 398 + 20);
    if ((iFlag == 0) && (m_cCurFocus == 5 || bHoverCreate))
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(199 + 98, 398, 25);
    else m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(199 + 98, 398, 24);

    // Button 6: Clear (at 392, 398 - size 72x20) - CENTER
    bool bHoverClear = (m_sNewAcctMsX >= 392 && m_sNewAcctMsX <= 392 + 72 &&
        m_sNewAcctMsY >= 398 && m_sNewAcctMsY <= 398 + 20);
    if (m_cCurFocus == 6 || bHoverClear)
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(294 + 98, 398, 27);
    else m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(294 + 98, 398, 26);

    // Button 7: Cancel (at 488, 398 - size 72x20) - RIGHT
    bool bHoverCancel = (m_sNewAcctMsX >= 488 && m_sNewAcctMsX <= 488 + 72 &&
        m_sNewAcctMsY >= 398 && m_sNewAcctMsY <= 398 + 20);
    if (m_cCurFocus == 7 || bHoverCancel)
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(390 + 98, 398, 17);
    else m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(390 + 98, 398, 16);

    m_pGame->DrawVersion();
}

#endif // DEF_MAKE_ACCOUNT
