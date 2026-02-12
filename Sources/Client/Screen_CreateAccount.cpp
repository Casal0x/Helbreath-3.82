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
    m_cNewAcctName.clear();
    m_cNewAcctPassword.clear();
    m_cNewAcctConfirm.clear();
    m_cEmail.clear();
}

void Screen_CreateAccount::on_initialize()
{
    // Set current mode for code that checks GameModeManager::GetMode()
    GameModeManager::SetCurrentMode(GameMode::CreateNewAccount);

    TextInputManager::Get().EndInput();

    m_cNewAcctPrevFocus = 1;
    m_cNewAcctPrevLB = 0;
    m_cCurFocus = 1;
    m_cMaxFocus = 7;
    m_pGame->m_cArrowPressed = 0;

    _clear_fields();

    TextInputManager::Get().StartInput(427, 84, 11, m_cNewAcctName);
    TextInputManager::Get().ClearInput();
}

void Screen_CreateAccount::on_uninitialize()
{
}

void Screen_CreateAccount::on_update()
{
    uint32_t dwTime = GameClock::GetTimeMS();
    m_pGame->m_dwCurTime = dwTime;

    m_sNewAcctMsX = static_cast<short>(hb::shared::input::GetMouseX());
    m_sNewAcctMsY = static_cast<short>(hb::shared::input::GetMouseY());
    char cLB = hb::shared::input::IsMouseButtonDown(MouseButton::Left) ? 1 : 0;

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
        TextInputManager::Get().EndInput();
        switch (m_cCurFocus) {
        case 1: TextInputManager::Get().StartInput(427, 84, 11, m_cNewAcctName); break;
        case 2: TextInputManager::Get().StartInput(427, 106, 11, m_cNewAcctPassword, true); break;
        case 3: TextInputManager::Get().StartInput(427, 129, 11, m_cNewAcctConfirm, true); break;
        case 4: TextInputManager::Get().StartInput(311, 48 + 190 - 25 + 2, 49, m_cEmail); break;
        }
        TextInputManager::Get().EndInput();
        m_cNewAcctPrevFocus = m_cCurFocus;
    }

    // Direct mouse click focus selection
    if (cLB != 0 && m_cNewAcctPrevLB == 0)
    {
        if (hb::shared::input::IsMouseInRect(427, 84, 100, 18)) m_cCurFocus = 1;
        if (hb::shared::input::IsMouseInRect(427, 106, 100, 18)) m_cCurFocus = 2;
        if (hb::shared::input::IsMouseInRect(427, 129, 100, 18)) m_cCurFocus = 3;
        if (hb::shared::input::IsMouseInRect(311, 215, 250, 18)) m_cCurFocus = 4;

        // Button 5: Create
        if (hb::shared::input::IsMouseInRect(297, 398, 72, 20))
        {
            m_cCurFocus = 5;
            m_pGame->PlayGameSound('E', 14, 5);
            _submit_create_account();
        }
        // Button 6: Clear
        if (hb::shared::input::IsMouseInRect(392, 398, 72, 20))
        {
            m_cCurFocus = 6;
            m_pGame->PlayGameSound('E', 14, 5);
            _clear_fields();
            m_cCurFocus = 1;
            m_cNewAcctPrevFocus = 0; // Trigger reset
        }
        // Button 7: Cancel
        if (hb::shared::input::IsMouseInRect(488, 398, 72, 20))
        {
            m_cCurFocus = 7;
            m_pGame->PlayGameSound('E', 14, 5);
            m_pGame->ChangeGameMode(GameMode::MainMenu);
        }
    }
    m_cNewAcctPrevLB = cLB;

    if (hb::shared::input::IsKeyPressed(KeyCode::Escape) == true)
    {
        m_pGame->ChangeGameMode(GameMode::MainMenu);
        return;
    }

    // Handle Tab key
    if (hb::shared::input::IsKeyPressed(KeyCode::Tab))
    {
        m_pGame->PlayGameSound('E', 14, 5);
        if (hb::shared::input::IsShiftDown())
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
    if (hb::shared::input::IsKeyPressed(KeyCode::Enter))
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
    bool bReady = (!m_cNewAcctName.empty() && !m_cNewAcctPassword.empty() &&
        !m_cNewAcctConfirm.empty() && CMisc::bIsValidEmail(m_cEmail.data()) &&
        CMisc::bCheckValidName(m_cNewAcctName.data()) && CMisc::bCheckValidName(m_cNewAcctPassword.data()) &&
        m_cNewAcctPassword == m_cNewAcctConfirm);

    if (bReady)
    {
        m_pGame->m_cArrowPressed = 0;

        // Copy account name/password to player session
        m_pGame->m_pPlayer->m_cAccountName = m_cNewAcctName.c_str();
        m_pGame->m_pPlayer->m_cAccountPassword = m_cNewAcctPassword.c_str();

        // Build CreateAccountRequest packet
        hb::net::CreateAccountRequest req{};
        req.header.msg_id = MsgId::RequestCreateNewAccount;
        req.header.msg_type = 0;
        std::snprintf(req.account_name, sizeof(req.account_name), "%s", m_cNewAcctName.c_str());
        std::snprintf(req.password, sizeof(req.password), "%s", m_cNewAcctPassword.c_str());
        std::snprintf(req.email, sizeof(req.email), "%s", m_cEmail.c_str());

        // Store packet for sending after connection completes
        auto* p = reinterpret_cast<char*>(&req);
        m_pGame->m_pendingLoginPacket.assign(p, p + sizeof(req));

        // Connection logic
        m_pGame->m_pLSock = std::make_unique<hb::shared::net::ASIOSocket>(m_pGame->m_pIOPool->GetContext(), game_limits::socket_block_limit);
        m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr.c_str(), m_pGame->m_iLogServerPort);
        m_pGame->m_pLSock->bInitBufferSize(hb::shared::limits::MsgBufferSize);

        m_pGame->ChangeGameMode(GameMode::Connecting);
        m_pGame->m_dwConnectMode = MsgId::RequestCreateNewAccount;
        std::snprintf(m_pGame->m_cMsg, sizeof(m_pGame->m_cMsg), "%s", "01");
    }
}

bool Screen_CreateAccount::on_net_response(uint16_t wResponseType, char* pData)
{
	switch (wResponseType) {
	case LogResMsg::NewAccountCreated:
		m_pGame->m_pLSock.reset();
		std::snprintf(m_pGame->m_cMsg, sizeof(m_pGame->m_cMsg), "%s", "54");
		m_pGame->ChangeGameMode(GameMode::LogResMsg);
		return true;

	case LogResMsg::NewAccountFailed:
		m_pGame->m_pLSock.reset();
		std::snprintf(m_pGame->m_cMsg, sizeof(m_pGame->m_cMsg), "%s", "05");
		m_pGame->ChangeGameMode(GameMode::LogResMsg);
		return true;

	case LogResMsg::AlreadyExistingAccount:
		m_pGame->m_pLSock.reset();
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
    if (m_cNewAcctPassword != m_cNewAcctConfirm)                                 iFlag = 9;
    if (CMisc::bCheckValidName(m_cNewAcctPassword.data()) == false)     iFlag = 7;
    if (CMisc::bCheckValidName(m_cNewAcctName.data()) == false)         iFlag = 6;
    if (CMisc::bIsValidEmail(m_cEmail.data()) == false)                 iFlag = 5;
    if (m_cNewAcctConfirm.empty())                                       iFlag = 3;
    if (m_cNewAcctPassword.empty())                                      iFlag = 2;
    if (m_cNewAcctName.empty())                                          iFlag = 1;

    auto labelStyle = hb::shared::text::TextStyle::Color(GameColors::UIFormLabel);
    auto blackStyle = hb::shared::text::TextStyle::Color(GameColors::UIBlack);

    // Draw background
    m_pGame->DrawNewDialogBox(InterfaceNdNewAccount, 0, 0, 0, true);

    // Draw labels
    hb::shared::text::DrawText(GameFont::Default, 377, 84, "Account:", labelStyle);
    hb::shared::text::DrawText(GameFont::Default, 372, 106, "Password:", labelStyle);
    hb::shared::text::DrawText(GameFont::Default, 372, 129, "(confirm)", labelStyle);
    hb::shared::text::DrawText(GameFont::Default, 271, 215, "eMail:", labelStyle);

    // Show active input string
    if ((m_cCurFocus == 2) || (m_cCurFocus == 3))
        TextInputManager::Get().ShowInput();
    else if ((m_cCurFocus == 1) || (m_cCurFocus == 4))
        TextInputManager::Get().ShowInput();

    // Draw input field values with validation coloring
    auto validStyle = hb::shared::text::TextStyle::WithShadow(GameColors::InputValid);
    auto invalidStyle = hb::shared::text::TextStyle::WithShadow(GameColors::InputInvalid);

    if (m_cCurFocus != 1) {
        bool bValid = CMisc::bCheckValidName(m_cNewAcctName.data()) != false;
        hb::shared::text::DrawText(GameFont::Default, 427, 84, m_cNewAcctName.c_str(), bValid ? validStyle : invalidStyle);
    }
    if (m_cCurFocus != 2) {
        std::string masked2(m_cNewAcctPassword.size(), '*');
        bool bValid = CMisc::bCheckValidName(m_cNewAcctPassword.data()) != false;
        hb::shared::text::DrawText(GameFont::Default, 427, 106, masked2.c_str(), bValid ? validStyle : invalidStyle);
    }
    if (m_cCurFocus != 3) {
        std::string masked3(m_cNewAcctConfirm.size(), '*');
        bool bValid = (m_cNewAcctPassword == m_cNewAcctConfirm);
        hb::shared::text::DrawText(GameFont::Default, 427, 129, masked3.c_str(), bValid ? validStyle : invalidStyle);
    }
    if (m_cCurFocus != 4) {
        bool bValid = CMisc::bIsValidEmail(m_cEmail.data());
        hb::shared::text::DrawText(GameFont::Default, 311, 48 + 190 - 25 + 2, m_cEmail.c_str(), bValid ? validStyle : invalidStyle);
    }

    // Draw help text based on focus
    switch (m_cCurFocus) {
    case 1:
        hb::shared::text::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT1, blackStyle, hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 290, 345, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT2, blackStyle, hb::shared::text::Align::TopCenter);
        break;
    case 2:
        hb::shared::text::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT4, blackStyle, hb::shared::text::Align::TopCenter);
        break;
    case 3:
        hb::shared::text::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT8, blackStyle, hb::shared::text::Align::TopCenter);
        break;
    case 4:
        hb::shared::text::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT21, blackStyle, hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 290, 345, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT22, blackStyle, hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 290, 360, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT23, blackStyle, hb::shared::text::Align::TopCenter);
        break;
    case 5:
        switch (iFlag) {
        case 0:
            hb::shared::text::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT33, blackStyle, hb::shared::text::Align::TopCenter);
            break;
        case 1:
            hb::shared::text::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT35, blackStyle, hb::shared::text::Align::TopCenter);
            break;
        case 2:
            hb::shared::text::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT38, blackStyle, hb::shared::text::Align::TopCenter);
            break;
        case 3:
            hb::shared::text::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT42, blackStyle, hb::shared::text::Align::TopCenter);
            break;
        case 5:
            hb::shared::text::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT50, blackStyle, hb::shared::text::Align::TopCenter);
            break;
        case 6:
            hb::shared::text::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT52, blackStyle, hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, 290, 345, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT53, blackStyle, hb::shared::text::Align::TopCenter);
            break;
        case 7:
            hb::shared::text::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT56, blackStyle, hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, 290, 345, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT57, blackStyle, hb::shared::text::Align::TopCenter);
            break;
        case 9:
            hb::shared::text::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT63, blackStyle, hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, 290, 345, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT64, blackStyle, hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, 290, 360, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT65, blackStyle, hb::shared::text::Align::TopCenter);
            break;
        }
        break;
    case 6:
        hb::shared::text::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT80, blackStyle, hb::shared::text::Align::TopCenter);
        break;
    case 7:
        hb::shared::text::DrawTextAligned(GameFont::Default, 290, 330, (575) - (290), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT81, blackStyle, hb::shared::text::Align::TopCenter);
        break;
    }

    // Draw buttons - highlight on focus OR mouse hover
    // Button 5: Create (at 297, 398 - size 72x20) - LEFT
    bool bHoverCreate = (m_sNewAcctMsX >= 297 && m_sNewAcctMsX <= 297 + 72 &&
        m_sNewAcctMsY >= 398 && m_sNewAcctMsY <= 398 + 20);
    if ((iFlag == 0) && (m_cCurFocus == 5 || bHoverCreate))
        m_pGame->m_pSprite[InterfaceNdButton]->Draw(199 + 98, 398, 25);
    else m_pGame->m_pSprite[InterfaceNdButton]->Draw(199 + 98, 398, 24);

    // Button 6: Clear (at 392, 398 - size 72x20) - CENTER
    bool bHoverClear = (m_sNewAcctMsX >= 392 && m_sNewAcctMsX <= 392 + 72 &&
        m_sNewAcctMsY >= 398 && m_sNewAcctMsY <= 398 + 20);
    if (m_cCurFocus == 6 || bHoverClear)
        m_pGame->m_pSprite[InterfaceNdButton]->Draw(294 + 98, 398, 27);
    else m_pGame->m_pSprite[InterfaceNdButton]->Draw(294 + 98, 398, 26);

    // Button 7: Cancel (at 488, 398 - size 72x20) - RIGHT
    bool bHoverCancel = (m_sNewAcctMsX >= 488 && m_sNewAcctMsX <= 488 + 72 &&
        m_sNewAcctMsY >= 398 && m_sNewAcctMsY <= 398 + 20);
    if (m_cCurFocus == 7 || bHoverCancel)
        m_pGame->m_pSprite[InterfaceNdButton]->Draw(390 + 98, 398, 17);
    else m_pGame->m_pSprite[InterfaceNdButton]->Draw(390 + 98, 398, 16);

    m_pGame->DrawVersion();
}

