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

extern class XSocket* G_pCalcSocket; // Sockets are often externs in this codebase

Screen_Login::Screen_Login(CGame* pGame)
    : IGameScreen(pGame), m_cPrevFocus(0)
{
    std::memset(m_cLoginName, 0, sizeof(m_cLoginName));
    std::memset(m_cLoginPassword, 0, sizeof(m_cLoginPassword));
}

void Screen_Login::on_initialize()
{
    // Set legacy mode
    GameModeManager::SetLegacyMode(GameMode::Login);

    m_pGame->EndInputString();
    m_cPrevFocus = 1;
    m_cCurFocus = 1;
    m_cMaxFocus = 4;
    m_pGame->m_cArrowPressed = 0;
    
    std::memset(m_cLoginName, 0, sizeof(m_cLoginName));
    std::memset(m_cLoginPassword, 0, sizeof(m_cLoginPassword));
    
    m_pGame->StartInputString(180 + SCREENX, 162 + SCREENY, 11, m_cLoginName);
    m_pGame->ClearInputString();
}

void Screen_Login::on_uninitialize()
{
    m_pGame->EndInputString();
}

void Screen_Login::on_update()
{
    // Polling logic migrated from CGame::UpdateScreen_Login
    
    // Explicit TAB handling since legacy OnKeyDown ignores it
    if (Input::IsKeyPressed(VK_TAB))
    {
        if (Input::IsShiftDown())
        {
             m_cCurFocus--;
             if (m_cCurFocus <= 0) m_cCurFocus = m_cMaxFocus;
        }
        else
        {
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
        m_pGame->PlaySound('E', 14, 5);

        switch (m_cCurFocus) {
        case 1:
            m_cCurFocus++;
            if (m_cCurFocus > m_cMaxFocus) m_cCurFocus = 1;
            break;
        case 2:
        case 3:
            if ((strlen(m_cLoginName) == 0) || (strlen(m_cLoginPassword) == 0)) break;
            
            // Set Player Account Credentials
            std::memset(m_pGame->m_pPlayer->m_cAccountName, 0, sizeof(m_pGame->m_pPlayer->m_cAccountName));
            std::memset(m_pGame->m_pPlayer->m_cAccountPassword, 0, sizeof(m_pGame->m_pPlayer->m_cAccountPassword));
            strcpy(m_pGame->m_pPlayer->m_cAccountName, m_cLoginName);
            strcpy(m_pGame->m_pPlayer->m_cAccountPassword, m_cLoginPassword);
            
            // Connect
            m_pGame->m_pLSock = std::make_unique<XSocket>(DEF_SOCKETBLOCKLIMIT);
            m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort + (rand() % 1));
            m_pGame->m_pLSock->bInitBufferSize(30000);
            
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
        m_pGame->EndInputString();
        m_pGame->ChangeGameMode(GameMode::MainMenu);
        return;
    }

    if (m_cPrevFocus != m_cCurFocus)
    {
         m_pGame->EndInputString();
        switch (m_cCurFocus) {
        case 1:
            m_pGame->StartInputString(180 + SCREENX, 162 + SCREENY, 11, m_cLoginName);
            break;
        case 2:
            m_pGame->StartInputString(180 + SCREENX, 185 + SCREENY, 11, m_cLoginPassword, true);
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
        m_pGame->PlaySound('E', 14, 5);
        // Name field click
        if (Input::IsMouseInRect(80 + SCREENX, 151 + SCREENY, 337 + SCREENX, 179 + SCREENY)) {
            m_cCurFocus = 1;
        }
        // Password field click
        else if (Input::IsMouseInRect(80 + SCREENX, 180 + SCREENY, 337 + SCREENX, 205 + SCREENY)) {
            m_cCurFocus = 2;
        }
        // Login button click
        else if (Input::IsMouseInRect(80 + SCREENX, 280 + SCREENY, 163 + SCREENX, 302 + SCREENY)) {
            if ((strlen(m_cLoginName) != 0) && (strlen(m_cLoginPassword) != 0)) {
                m_pGame->EndInputString();
                std::memset(m_pGame->m_pPlayer->m_cAccountName, 0, sizeof(m_pGame->m_pPlayer->m_cAccountName));
                std::memset(m_pGame->m_pPlayer->m_cAccountPassword, 0, sizeof(m_pGame->m_pPlayer->m_cAccountPassword));
                strcpy(m_pGame->m_pPlayer->m_cAccountName, m_cLoginName);
                strcpy(m_pGame->m_pPlayer->m_cAccountPassword, m_cLoginPassword);
                
                m_pGame->m_pLSock = std::make_unique<XSocket>(DEF_SOCKETBLOCKLIMIT);
                m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort + (rand() % 1));
                m_pGame->m_pLSock->bInitBufferSize(30000);
                
                m_pGame->ChangeGameMode(GameMode::Connecting);
                m_pGame->m_dwConnectMode = MSGID_REQUEST_LOGIN;
                std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
                strcpy(m_pGame->m_cMsg, "11");
                return;
            }
        }
        // Cancel button click
        else if (Input::IsMouseInRect(258 + SCREENX, 280 + SCREENY, 327 + SCREENX, 302 + SCREENY)) {
            m_pGame->ChangeGameMode(GameMode::MainMenu);
            return;
        }
    }

    if ((Input::GetMouseX() >= 80 + SCREENX) && (Input::GetMouseX() <= 163 + SCREENX) && (Input::GetMouseY() >= 280 + SCREENY) && (Input::GetMouseY() <= 302 + SCREENY)) m_cCurFocus = 3;
    if ((Input::GetMouseX() >= 258 + SCREENX) && (Input::GetMouseX() <= 327 + SCREENX) && (Input::GetMouseY() >= 280 + SCREENY) && (Input::GetMouseY() <= 302 + SCREENY)) m_cCurFocus = 4;
}

void Screen_Login::on_render()
{
    DrawLoginWindow(m_cLoginName, m_cLoginPassword, Input::GetMouseX(), Input::GetMouseY(), m_pGame->m_cGameModeCount);
}

// Logic migrated from CGame::_Draw_OnLogin
void Screen_Login::DrawLoginWindow(char* pAccount, char* pPassword, int msX, int msY, int iFrame)
{
    bool bFlag = true;
    m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_LOGIN, 0 + SCREENX, 0 + SCREENY, 0, true);
    m_pGame->DrawVersion();

    if ((iFrame >= 15) && (iFrame <= 20)) m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_LOGIN]->Draw(39 + SCREENX, 121 + SCREENY, 2, SpriteLib::DrawParams::Alpha(0.25f));
    else if (iFrame > 20) m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_LOGIN, 39 + SCREENX, 121 + SCREENY, 2, true);

    if (m_cCurFocus != 1) {
        if (CMisc::bCheckValidName(pAccount) != false)
            m_pGame->PutString2(180 + SCREENX, 162 + SCREENY, pAccount, 200, 200, 200);
        else m_pGame->PutString2(180 + SCREENX, 162 + SCREENY, pAccount, 200, 100, 100);
    }
    if ((CMisc::bCheckValidName(pAccount) == false) || (strlen(pAccount) == 0)) bFlag = false;

    if (m_cCurFocus != 2) {
        if ((CMisc::bCheckValidString(pPassword) != false))
            m_pGame->PutString(180 + SCREENX, 185 + SCREENY, pPassword, RGB(200, 200, 200), true, 1);
        else m_pGame->PutString(180 + SCREENX, 185 + SCREENY, pPassword, RGB(200, 100, 100), true, 1);
    }
    if ((CMisc::bCheckValidString(pPassword) == false) || (strlen(pPassword) == 0)) bFlag = false;

    if (m_cCurFocus == 1)
        m_pGame->ShowReceivedString();
    else
        if (m_cCurFocus == 2)
            m_pGame->ShowReceivedString(true);

    if (bFlag == true)
    {
        if (m_cCurFocus == 3) m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_LOGIN, 80 + SCREENX, 282 + SCREENY, 3, true);
    }
    if (m_cCurFocus == 4) m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_LOGIN, 256 + SCREENX, 282 + SCREENY, 4, true);
}
