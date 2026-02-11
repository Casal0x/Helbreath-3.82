// Screen_Login.cpp: Login Screen Implementation
//
//////////////////////////////////////////////////////////////////////

#include "Screen_Login.h"
#include "Game.h"
#include "GameModeManager.h"
#include "IInput.h"
#include "GlobalDef.h"
#include "ASIOSocket.h" // For XSocket
#include "Misc.h"    // For CMisc
#include "GameFonts.h"
#include "TextLibExt.h"


using namespace hb::shared::net;
namespace MouseButton = hb::shared::input::MouseButton;

Screen_Login::Screen_Login(CGame* pGame)
    : IGameScreen(pGame), m_cPrevFocus(0)
{
    std::memset(m_cLoginName, 0, sizeof(m_cLoginName));
    std::memset(m_cLoginPassword, 0, sizeof(m_cLoginPassword));
}

void Screen_Login::on_initialize()
{
    // Set current mode for code that checks GameModeManager::GetMode()
    GameModeManager::SetCurrentMode(GameMode::Login);

    m_pGame->EndInputString();
    m_cPrevFocus = 1;
    m_cCurFocus = 1;
    m_cMaxFocus = 4;
    m_pGame->m_cArrowPressed = 0;
    
    std::memset(m_cLoginName, 0, sizeof(m_cLoginName));
    std::memset(m_cLoginPassword, 0, sizeof(m_cLoginPassword));
    
    m_pGame->StartInputString(234, 222, 11, m_cLoginName);
    m_pGame->ClearInputString();
}

void Screen_Login::on_uninitialize()
{
    m_pGame->EndInputString();
}

void Screen_Login::on_update()
{
// Polling logic migrated from CGame::UpdateScreen_Login
    uint32_t dwTime = GameClock::GetTimeMS();
    m_pGame->m_dwCurTime = dwTime;

    // Explicit TAB handling since legacy OnKeyDown ignores it
    if (hb::shared::input::IsKeyPressed(KeyCode::Tab))
    {
        if (hb::shared::input::IsShiftDown())
        {
            m_pGame->PlayGameSound('E', 14, 5);
             m_cCurFocus--;
             if (m_cCurFocus <= 0) m_cCurFocus = m_cMaxFocus;
        }
        else
        {
            m_pGame->PlayGameSound('E', 14, 5);
             m_cCurFocus++;
             if (m_cCurFocus > m_cMaxFocus) m_cCurFocus = 1;
        }
    }

    if (m_pGame->m_cArrowPressed != 0)
    {
        switch (m_pGame->m_cArrowPressed) {
        case 1:
            m_cCurFocus--;
            if (m_cCurFocus <= 0) m_cCurFocus = m_cMaxFocus;
            break;
        case 2:
            if (m_cCurFocus == 3) m_cCurFocus = 4;
            else if (m_cCurFocus == 4) m_cCurFocus = 3;
            break;
        case 3:
            m_cCurFocus++;
            if (m_cCurFocus > m_cMaxFocus) m_cCurFocus = 1;
            break;
        case 4:
            if (m_cCurFocus == 3) m_cCurFocus = 4;
            else if (m_cCurFocus == 4) m_cCurFocus = 3;
            break;
        }
         m_pGame->m_cArrowPressed = 0;
    }

    if (hb::shared::input::IsKeyPressed(KeyCode::Enter) == true)
    {
        switch (m_cCurFocus) {
        case 1:
            m_pGame->PlayGameSound('E', 14, 5);
            m_cCurFocus++;
            if (m_cCurFocus > m_cMaxFocus) m_cCurFocus = 1;
            break;
        case 2:
        case 3:
            if ((strlen(m_cLoginName) == 0) || (strlen(m_cLoginPassword) == 0)) break;
            m_pGame->PlayGameSound('E', 14, 5);
            
            // Set Player Account Credentials
            std::memset(m_pGame->m_pPlayer->m_cAccountName, 0, sizeof(m_pGame->m_pPlayer->m_cAccountName));
            std::memset(m_pGame->m_pPlayer->m_cAccountPassword, 0, sizeof(m_pGame->m_pPlayer->m_cAccountPassword));
            std::snprintf(m_pGame->m_pPlayer->m_cAccountName, sizeof(m_pGame->m_pPlayer->m_cAccountName), "%s", m_cLoginName);
            std::snprintf(m_pGame->m_pPlayer->m_cAccountPassword, sizeof(m_pGame->m_pPlayer->m_cAccountPassword), "%s", m_cLoginPassword);

            // Connect
            m_pGame->m_pLSock = std::make_unique<hb::shared::net::ASIOSocket>(m_pGame->m_pIOPool->GetContext(), game_limits::socket_block_limit);
            m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort + (rand() % 1));
            m_pGame->m_pLSock->bInitBufferSize(hb::shared::limits::MsgBufferSize);

            m_pGame->ChangeGameMode(GameMode::Connecting);
            m_pGame->m_dwConnectMode = MsgId::RequestLogin;
            std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
            std::snprintf(m_pGame->m_cMsg, sizeof(m_pGame->m_cMsg), "%s", "11");
            return;
        case 4:
            m_pGame->ChangeGameMode(GameMode::MainMenu);
            return;
        }
    }

    if (hb::shared::input::IsKeyPressed(KeyCode::Escape) == true)
    {
        m_pGame->PlayGameSound('E', 14, 5);
        m_pGame->EndInputString();
        m_pGame->ChangeGameMode(GameMode::MainMenu);
        return;
    }

    if (m_cPrevFocus != m_cCurFocus)
    {
         m_pGame->EndInputString();
        switch (m_cCurFocus) {
        case 1:
            m_pGame->StartInputString(234, 222, 11, m_cLoginName);
            break;
        case 2:
            m_pGame->StartInputString(234, 245, 11, m_cLoginPassword, true);
            break;
        case 3:
        case 4:
            break;
        }
        m_cPrevFocus = m_cCurFocus;
    }

    // Mouse click detection
    if (hb::shared::input::IsMouseButtonPressed(MouseButton::Left))
    {
        // Name field click
        if (hb::shared::input::IsMouseInRect(234, 221, 147, 17)) {
            m_pGame->PlayGameSound('E', 14, 5);
            m_cCurFocus = 1;
        }
        // Password field click
        else if (hb::shared::input::IsMouseInRect(234, 244, 147, 17)) {
            m_pGame->PlayGameSound('E', 14, 5);
            m_cCurFocus = 2;
        }
        // Login button click
        else if (hb::shared::input::IsMouseInRect(140, 343, 84, 20)) {
            m_pGame->PlayGameSound('E', 14, 5);
            if ((strlen(m_cLoginName) != 0) && (strlen(m_cLoginPassword) != 0)) {
                m_pGame->EndInputString();
                std::memset(m_pGame->m_pPlayer->m_cAccountName, 0, sizeof(m_pGame->m_pPlayer->m_cAccountName));
                std::memset(m_pGame->m_pPlayer->m_cAccountPassword, 0, sizeof(m_pGame->m_pPlayer->m_cAccountPassword));
                std::snprintf(m_pGame->m_pPlayer->m_cAccountName, sizeof(m_pGame->m_pPlayer->m_cAccountName), "%s", m_cLoginName);
                std::snprintf(m_pGame->m_pPlayer->m_cAccountPassword, sizeof(m_pGame->m_pPlayer->m_cAccountPassword), "%s", m_cLoginPassword);

                m_pGame->m_pLSock = std::make_unique<hb::shared::net::ASIOSocket>(m_pGame->m_pIOPool->GetContext(), game_limits::socket_block_limit);
                m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort + (rand() % 1));
                m_pGame->m_pLSock->bInitBufferSize(hb::shared::limits::MsgBufferSize);

                m_pGame->ChangeGameMode(GameMode::Connecting);
                m_pGame->m_dwConnectMode = MsgId::RequestLogin;
                std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
                std::snprintf(m_pGame->m_cMsg, sizeof(m_pGame->m_cMsg), "%s", "11");
                return;
            }
        }
        // Cancel button click
        else if (hb::shared::input::IsMouseInRect(316, 343, 76, 20)) {
            m_pGame->PlayGameSound('E', 14, 5);
            m_pGame->ChangeGameMode(GameMode::MainMenu);
            return;
        }
    }

    if (hb::shared::input::IsMouseInRect(140, 343, 84, 20)) m_cCurFocus = 3;
    if (hb::shared::input::IsMouseInRect(316, 343, 76, 20)) m_cCurFocus = 4;
}

void Screen_Login::on_render()
{
    DrawLoginWindow(m_cLoginName, m_cLoginPassword, hb::shared::input::GetMouseX(), hb::shared::input::GetMouseY());
}

// Logic migrated from CGame::_Draw_OnLogin
void Screen_Login::DrawLoginWindow(char* pAccount, char* pPassword, int msX, int msY)
{
    bool bFlag = true;
    m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_LOGIN, 0, 0, 0, true);
    m_pGame->DrawVersion();

    // Smooth alpha fade-in for login box: 0-500ms delay, then 500-700ms fade from 0 to 1
    static constexpr uint32_t FADE_DELAY_MS = 500;
    static constexpr uint32_t FADE_DURATION_MS = 200;

    uint32_t elapsedMs = get_elapsed_ms();
    if (elapsedMs > FADE_DELAY_MS) {
        float fadeProgress = static_cast<float>(elapsedMs - FADE_DELAY_MS) / FADE_DURATION_MS;
        float alpha = fadeProgress > 1.0f ? 1.0f : fadeProgress;
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_LOGIN]->Draw(99, 182, 2, hb::shared::sprite::DrawParams::Alpha(alpha));
    }

    if (m_cCurFocus != 1) {
        if (CMisc::bCheckValidName(pAccount) != false)
            hb::shared::text::DrawText(GameFont::Default, 234, 222, pAccount, hb::shared::text::TextStyle::WithShadow(GameColors::InputValid));
        else hb::shared::text::DrawText(GameFont::Default, 234, 222, pAccount, hb::shared::text::TextStyle::WithShadow(GameColors::InputInvalid));
    }
    if ((CMisc::bCheckValidName(pAccount) == false) || (strlen(pAccount) == 0)) bFlag = false;

    if (m_cCurFocus != 2) {
        // Mask password with asterisks
        std::string masked(strlen(pPassword), '*');
        if ((CMisc::bCheckValidString(pPassword) != false))
            hb::shared::text::DrawText(GameFont::Default, 234, 245, masked.c_str(),
                              hb::shared::text::TextStyle::WithShadow(GameColors::InputValid));
        else
            hb::shared::text::DrawText(GameFont::Default, 234, 245, masked.c_str(),
                              hb::shared::text::TextStyle::WithShadow(GameColors::InputInvalid));
    }
    if ((CMisc::bCheckValidString(pPassword) == false) || (strlen(pPassword) == 0)) bFlag = false;

    if (m_cCurFocus == 1)
        m_pGame->ShowReceivedString();
    else
        if (m_cCurFocus == 2)
            m_pGame->ShowReceivedString(true);

    if (bFlag == true)
    {
        if (m_cCurFocus == 3) m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_LOGIN, 140, 343, 3, true);
    }
    if (m_cCurFocus == 4) m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_LOGIN, 316, 343, 4, true);
}
