// Screen_Login.cpp: Login Screen Implementation
//
//////////////////////////////////////////////////////////////////////

#include "Screen_Login.h"
#include "Game.h"
#include "GameModeManager.h"
#include "IInput.h"
#include "GlobalDef.h"
#include "XSocket.h" // For XSocket
#include "Misc.h"    // For CMisc
#include "GameFonts.h"
#include "TextLibExt.h"

extern class XSocket* G_pCalcSocket; // Sockets are often externs in this codebase

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
    
    m_pGame->StartInputString(180 + MENUX(), 162 + MENUY(), 11, m_cLoginName);
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
    if (Input::IsKeyPressed(VK_TAB))
    {
        if (Input::IsShiftDown())
        {
            m_pGame->PlaySound('E', 14, 5);
             m_cCurFocus--;
             if (m_cCurFocus <= 0) m_cCurFocus = m_cMaxFocus;
        }
        else
        {
            m_pGame->PlaySound('E', 14, 5);
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

    if (Input::IsKeyPressed(VK_RETURN) == true)
    {
        switch (m_cCurFocus) {
        case 1:
            m_pGame->PlaySound('E', 14, 5);
            m_cCurFocus++;
            if (m_cCurFocus > m_cMaxFocus) m_cCurFocus = 1;
            break;
        case 2:
        case 3:
            if ((strlen(m_cLoginName) == 0) || (strlen(m_cLoginPassword) == 0)) break;
            m_pGame->PlaySound('E', 14, 5);
            
            // Set Player Account Credentials
            std::memset(m_pGame->m_pPlayer->m_cAccountName, 0, sizeof(m_pGame->m_pPlayer->m_cAccountName));
            std::memset(m_pGame->m_pPlayer->m_cAccountPassword, 0, sizeof(m_pGame->m_pPlayer->m_cAccountPassword));
            strcpy(m_pGame->m_pPlayer->m_cAccountName, m_cLoginName);
            strcpy(m_pGame->m_pPlayer->m_cAccountPassword, m_cLoginPassword);
            
            // Connect
            m_pGame->m_pLSock = std::make_unique<XSocket>(DEF_SOCKETBLOCKLIMIT);
            m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort + (rand() % 1));
            m_pGame->m_pLSock->bInitBufferSize(DEF_MSGBUFFERSIZE);
            
            m_pGame->ChangeGameMode(GameMode::Connecting);
            m_pGame->m_dwConnectMode = MSGID_REQUEST_LOGIN;
            std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
            strcpy(m_pGame->m_cMsg, "11");
            return;
        case 4:
            m_pGame->ChangeGameMode(GameMode::MainMenu);
            return;
        }
    }

    if (Input::IsKeyPressed(VK_ESCAPE) == true)
    {
        m_pGame->PlaySound('E', 14, 5);
        m_pGame->EndInputString();
        m_pGame->ChangeGameMode(GameMode::MainMenu);
        return;
    }

    if (m_cPrevFocus != m_cCurFocus)
    {
         m_pGame->EndInputString();
        switch (m_cCurFocus) {
        case 1:
            m_pGame->StartInputString(180 + MENUX(), 162 + MENUY(), 11, m_cLoginName);
            break;
        case 2:
            m_pGame->StartInputString(180 + MENUX(), 185 + MENUY(), 11, m_cLoginPassword, true);
            break;
        case 3:
        case 4:
            break;
        }
        m_cPrevFocus = m_cCurFocus;
    }

    // Mouse click detection
    if (Input::IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        // Name field click
        if (Input::IsMouseInRect(80 + MENUX(), 151 + MENUY(), 337 + MENUX(), 179 + MENUY())) {
            m_pGame->PlaySound('E', 14, 5);
            m_cCurFocus = 1;
        }
        // Password field click
        else if (Input::IsMouseInRect(80 + MENUX(), 180 + MENUY(), 337 + MENUX(), 205 + MENUY())) {
            m_pGame->PlaySound('E', 14, 5);
            m_cCurFocus = 2;
        }
        // Login button click
        else if (Input::IsMouseInRect(80 + MENUX(), 280 + MENUY(), 163 + MENUX(), 302 + MENUY())) {
            m_pGame->PlaySound('E', 14, 5);
            if ((strlen(m_cLoginName) != 0) && (strlen(m_cLoginPassword) != 0)) {
                m_pGame->EndInputString();
                std::memset(m_pGame->m_pPlayer->m_cAccountName, 0, sizeof(m_pGame->m_pPlayer->m_cAccountName));
                std::memset(m_pGame->m_pPlayer->m_cAccountPassword, 0, sizeof(m_pGame->m_pPlayer->m_cAccountPassword));
                strcpy(m_pGame->m_pPlayer->m_cAccountName, m_cLoginName);
                strcpy(m_pGame->m_pPlayer->m_cAccountPassword, m_cLoginPassword);
                
                m_pGame->m_pLSock = std::make_unique<XSocket>(DEF_SOCKETBLOCKLIMIT);
                m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort + (rand() % 1));
                m_pGame->m_pLSock->bInitBufferSize(DEF_MSGBUFFERSIZE);
                
                m_pGame->ChangeGameMode(GameMode::Connecting);
                m_pGame->m_dwConnectMode = MSGID_REQUEST_LOGIN;
                std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
                strcpy(m_pGame->m_cMsg, "11");
                return;
            }
        }
        // Cancel button click
        else if (Input::IsMouseInRect(258 + MENUX(), 280 + MENUY(), 327 + MENUX(), 302 + MENUY())) {
            m_pGame->PlaySound('E', 14, 5);
            m_pGame->ChangeGameMode(GameMode::MainMenu);
            return;
        }
    }

    if ((Input::GetMouseX() >= 80 + MENUX()) && (Input::GetMouseX() <= 163 + MENUX()) && (Input::GetMouseY() >= 280 + MENUY()) && (Input::GetMouseY() <= 302 + MENUY())) m_cCurFocus = 3;
    if ((Input::GetMouseX() >= 258 + MENUX()) && (Input::GetMouseX() <= 327 + MENUX()) && (Input::GetMouseY() >= 280 + MENUY()) && (Input::GetMouseY() <= 302 + MENUY())) m_cCurFocus = 4;
}

void Screen_Login::on_render()
{
    DrawLoginWindow(m_cLoginName, m_cLoginPassword, Input::GetMouseX(), Input::GetMouseY());
}

// Logic migrated from CGame::_Draw_OnLogin
void Screen_Login::DrawLoginWindow(char* pAccount, char* pPassword, int msX, int msY)
{
    bool bFlag = true;
    m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_LOGIN, 0 + MENUX(), 0 + MENUY(), 0, true);
    m_pGame->DrawVersion();

    // Smooth alpha fade-in for login box: 0-500ms delay, then 500-700ms fade from 0 to 1
    static constexpr uint32_t FADE_DELAY_MS = 500;
    static constexpr uint32_t FADE_DURATION_MS = 200;

    uint32_t elapsedMs = get_elapsed_ms();
    if (elapsedMs > FADE_DELAY_MS) {
        float fadeProgress = static_cast<float>(elapsedMs - FADE_DELAY_MS) / FADE_DURATION_MS;
        float alpha = fadeProgress > 1.0f ? 1.0f : fadeProgress;
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_LOGIN]->Draw(39 + MENUX(), 121 + MENUY(), 2, SpriteLib::DrawParams::Alpha(alpha));
    }

    if (m_cCurFocus != 1) {
        if (CMisc::bCheckValidName(pAccount) != false)
            TextLib::DrawText(GameFont::Default, 180 + MENUX(), 162 + MENUY(), pAccount, TextLib::TextStyle::WithShadow(GameColors::InputValid.r, GameColors::InputValid.g, GameColors::InputValid.b));
        else TextLib::DrawText(GameFont::Default, 180 + MENUX(), 162 + MENUY(), pAccount, TextLib::TextStyle::WithShadow(GameColors::InputInvalid.r, GameColors::InputInvalid.g, GameColors::InputInvalid.b));
    }
    if ((CMisc::bCheckValidName(pAccount) == false) || (strlen(pAccount) == 0)) bFlag = false;

    if (m_cCurFocus != 2) {
        // Mask password with asterisks
        std::string masked(strlen(pPassword), '*');
        if ((CMisc::bCheckValidString(pPassword) != false))
            TextLib::DrawText(GameFont::Default, 180 + MENUX(), 185 + MENUY(), masked.c_str(),
                              TextLib::TextStyle::WithShadow(GameColors::InputValid.r, GameColors::InputValid.g, GameColors::InputValid.b));
        else
            TextLib::DrawText(GameFont::Default, 180 + MENUX(), 185 + MENUY(), masked.c_str(),
                              TextLib::TextStyle::WithShadow(GameColors::InputInvalid.r, GameColors::InputInvalid.g, GameColors::InputInvalid.b));
    }
    if ((CMisc::bCheckValidString(pPassword) == false) || (strlen(pPassword) == 0)) bFlag = false;

    if (m_cCurFocus == 1)
        m_pGame->ShowReceivedString();
    else
        if (m_cCurFocus == 2)
            m_pGame->ShowReceivedString(true);

    if (bFlag == true)
    {
        if (m_cCurFocus == 3) m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_LOGIN, 80 + MENUX(), 282 + MENUY(), 3, true);
    }
    if (m_cCurFocus == 4) m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_LOGIN, 256 + MENUX(), 282 + MENUY(), 4, true);
}
