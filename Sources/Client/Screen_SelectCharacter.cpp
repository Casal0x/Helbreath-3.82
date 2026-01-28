// Screen_SelectCharacter.cpp: Select Character Screen Implementation
//
//////////////////////////////////////////////////////////////////////

#include "Screen_SelectCharacter.h"
#include "Game.h"
#include "GameModeManager.h"
#include "IInput.h"
#include "GlobalDef.h"
#include "XSocket.h"
#include "Misc.h"
#include "lan_eng.h"

extern char G_cSpriteAlphaDegree;

Screen_SelectCharacter::Screen_SelectCharacter(CGame* pGame)
    : IGameScreen(pGame)
    , m_dwSelCharCTime(0)
    , m_sSelCharMsX(0)
    , m_sSelCharMsY(0)
    , m_cCurFocus(1)
    , m_cMaxFocus(4)
{
}

void Screen_SelectCharacter::on_initialize()
{
    // Set legacy mode
    GameModeManager::SetLegacyMode(GameMode::SelectCharacter);

    // Initialize logic (migrated from CGame::UpdateScreen_SelectCharacter m_cGameModeCount == 0 block)
    G_cSpriteAlphaDegree = 1;
    m_pGame->InitGameSettings();
    
    m_cCurFocus = 1;
    m_cMaxFocus = 4;
    
    m_pGame->m_cArrowPressed = 0;
    m_dwSelCharCTime = GameClock::GetTimeMS();
}

void Screen_SelectCharacter::on_uninitialize()
{
}

void Screen_SelectCharacter::on_update()
{
    // Logic migrated from CGame::UpdateScreen_SelectCharacter
    short msX, msY, msZ;
    char cLB, cRB;
    uint32_t dwTime = GameClock::GetTimeMS();

    // Handle legacy arrow input (if set by OnKeyDown) or direct input
    // NOTE: Preferring direct input for robustness
    if (Input::IsKeyPressed(VK_RIGHT)) {
        m_cCurFocus++;
        if (m_cCurFocus > m_cMaxFocus) m_cCurFocus = 1;
    }
    else if (Input::IsKeyPressed(VK_LEFT)) {
        m_cCurFocus--;
        if (m_cCurFocus <= 0) m_cCurFocus = m_cMaxFocus;
    }
    

    if (Input::IsKeyPressed(VK_ESCAPE) == true)
    {
        m_pGame->ChangeGameMode(GameMode::MainMenu);
        return;
    }

    if (Input::IsKeyPressed(VK_RETURN) == true)
    {
        m_pGame->PlaySound('E', 14, 5);

        if (m_pGame->m_pCharList[m_cCurFocus - 1] != nullptr)
        {
            if (m_pGame->m_pCharList[m_cCurFocus - 1]->m_sSex != 0)
            {
                std::memset(m_pGame->m_pPlayer->m_cPlayerName, 0, sizeof(m_pGame->m_pPlayer->m_cPlayerName));
                strcpy(m_pGame->m_pPlayer->m_cPlayerName, m_pGame->m_pCharList[m_cCurFocus - 1]->m_cName);
                m_pGame->m_pPlayer->m_iLevel = (int)m_pGame->m_pCharList[m_cCurFocus - 1]->m_sLevel;
                if (CMisc::bCheckValidString(m_pGame->m_pPlayer->m_cPlayerName) == true)
                {
                    m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_LOGIN]->Unload();
                    m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_MAINMENU]->Unload();
                    m_pGame->m_pLSock = std::make_unique<XSocket>(DEF_SOCKETBLOCKLIMIT);
                    m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort + (rand() % 1));
                    m_pGame->m_pLSock->bInitBufferSize(30000);
                    m_pGame->ChangeGameMode(GameMode::Connecting);
                    m_pGame->m_dwConnectMode = MSGID_REQUEST_ENTERGAME;
                    m_pGame->m_wEnterGameType = DEF_ENTERGAMEMSGTYPE_NEW;
                    std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
                    strcpy(m_pGame->m_cMsg, "33");
                    std::memset(m_pGame->m_cMapName, 0, sizeof(m_pGame->m_cMapName));
                    memcpy(m_pGame->m_cMapName, m_pGame->m_pCharList[m_cCurFocus - 1]->m_cMapName, 10);
                    return;
                }
            }
        }
        else
        {
            m_pGame->ChangeGameMode(GameMode::CreateNewCharacter);
            return;
        }
    }

    msX = static_cast<short>(Input::GetMouseX());
    msY = static_cast<short>(Input::GetMouseY());
    msZ = static_cast<short>(Input::GetMouseWheelDelta());
    cLB = Input::IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? 1 : 0;
    cRB = Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ? 1 : 0;
    
    m_sSelCharMsX = msX;
    m_sSelCharMsY = msY;

    if ((dwTime - m_dwSelCharCTime) > 100)
    {
        m_pGame->m_cMenuFrame++;
        m_dwSelCharCTime = dwTime;
    }
    if (m_pGame->m_cMenuFrame >= 8)
    {
        m_pGame->m_cMenuDirCnt++;
        if (m_pGame->m_cMenuDirCnt > 8)
        {
            m_pGame->m_cMenuDir++;
            m_pGame->m_cMenuDirCnt = 1;
        }
        m_pGame->m_cMenuFrame = 0;
    }
    if (m_pGame->m_cMenuDir > 8) m_pGame->m_cMenuDir = 1;

    if (Input::IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_pGame->PlaySound('E', 14, 5);

        // Determine which button was clicked
        int iMIbuttonNum = 0;
        if (Input::IsMouseInRect(100 + SCREENX, 50 + SCREENY, 210 + SCREENX, 250 + SCREENY)) iMIbuttonNum = 1;
        else if (Input::IsMouseInRect(211 + SCREENX, 50 + SCREENY, 321 + SCREENX, 250 + SCREENY)) iMIbuttonNum = 2;
        else if (Input::IsMouseInRect(322 + SCREENX, 50 + SCREENY, 431 + SCREENX, 250 + SCREENY)) iMIbuttonNum = 3;
        else if (Input::IsMouseInRect(432 + SCREENX, 50 + SCREENY, 542 + SCREENX, 250 + SCREENY)) iMIbuttonNum = 4;
        else if (Input::IsMouseInRect(360 + SCREENX, 283 + SCREENY, 545 + SCREENX, 315 + SCREENY)) iMIbuttonNum = 5;
        else if (Input::IsMouseInRect(360 + SCREENX, 316 + SCREENY, 545 + SCREENX, 345 + SCREENY)) iMIbuttonNum = 6;
        else if (Input::IsMouseInRect(360 + SCREENX, 346 + SCREENY, 545 + SCREENX, 375 + SCREENY)) iMIbuttonNum = 7;
        else if (Input::IsMouseInRect(360 + SCREENX, 376 + SCREENY, 545 + SCREENX, 405 + SCREENY)) iMIbuttonNum = 8;
        else if (Input::IsMouseInRect(360 + SCREENX, 406 + SCREENY, 545 + SCREENX, 435 + SCREENY)) iMIbuttonNum = 9;

        switch (iMIbuttonNum) {
        case 1:
        case 2:
        case 3:
        case 4:
            if (m_cCurFocus != iMIbuttonNum)
                m_cCurFocus = iMIbuttonNum;
            else
            {
                if (m_pGame->m_pCharList[m_cCurFocus - 1] != nullptr)
                {
                    if (m_pGame->m_pCharList[m_cCurFocus - 1]->m_sSex != 0)
                    {
                        std::memset(m_pGame->m_pPlayer->m_cPlayerName, 0, sizeof(m_pGame->m_pPlayer->m_cPlayerName));
                        strcpy(m_pGame->m_pPlayer->m_cPlayerName, m_pGame->m_pCharList[m_cCurFocus - 1]->m_cName);
                        m_pGame->m_pPlayer->m_iLevel = (int)m_pGame->m_pCharList[m_cCurFocus - 1]->m_sLevel;
                        if (CMisc::bCheckValidString(m_pGame->m_pPlayer->m_cPlayerName) == true)
                        {
                            m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_LOGIN]->Unload();
                            m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_MAINMENU]->Unload();
                            m_pGame->m_pLSock = std::make_unique<XSocket>(DEF_SOCKETBLOCKLIMIT);
                            m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort + (rand() % 1));
                            m_pGame->m_pLSock->bInitBufferSize(30000);
                            m_pGame->ChangeGameMode(GameMode::Connecting);
                            m_pGame->m_dwConnectMode = MSGID_REQUEST_ENTERGAME;
                            m_pGame->m_wEnterGameType = DEF_ENTERGAMEMSGTYPE_NEW;
                            std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
                            strcpy(m_pGame->m_cMsg, "33");
                            std::memset(m_pGame->m_cMapName, 0, sizeof(m_pGame->m_cMapName));
                            memcpy(m_pGame->m_cMapName, m_pGame->m_pCharList[m_cCurFocus - 1]->m_cMapName, 10);
                            return;
                        }
                    }
                }
                else
                {
                    m_pGame->ChangeGameMode(GameMode::CreateNewCharacter);
                    return;
                }
            }
            break;

        case 5:
            if (m_pGame->m_pCharList[m_cCurFocus - 1] != nullptr)
            {
                if (m_pGame->m_pCharList[m_cCurFocus - 1]->m_sSex != 0)
                {
                    std::memset(m_pGame->m_pPlayer->m_cPlayerName, 0, sizeof(m_pGame->m_pPlayer->m_cPlayerName));
                    strcpy(m_pGame->m_pPlayer->m_cPlayerName, m_pGame->m_pCharList[m_cCurFocus - 1]->m_cName);
                    m_pGame->m_pPlayer->m_iLevel = (int)m_pGame->m_pCharList[m_cCurFocus - 1]->m_sLevel;

                    if (CMisc::bCheckValidString(m_pGame->m_pPlayer->m_cPlayerName) == true) {
                        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_LOGIN]->Unload();
                        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_MAINMENU]->Unload();
                        m_pGame->m_pLSock = std::make_unique<XSocket>(DEF_SOCKETBLOCKLIMIT);
                        m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort + (rand() % 1));
                        m_pGame->m_pLSock->bInitBufferSize(30000);
                        m_pGame->ChangeGameMode(GameMode::Connecting);
                        m_pGame->m_dwConnectMode = MSGID_REQUEST_ENTERGAME;
                        m_pGame->m_wEnterGameType = DEF_ENTERGAMEMSGTYPE_NEW;
                        std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
                        strcpy(m_pGame->m_cMsg, "33");
                        std::memset(m_pGame->m_cMapName, 0, sizeof(m_pGame->m_cMapName));
                        memcpy(m_pGame->m_cMapName, m_pGame->m_pCharList[m_cCurFocus - 1]->m_cMapName, 10);
                        return;
                    }
                }
            }
            break;

        case 6:
            if (m_pGame->m_iTotalChar < 4)
            {
                m_pGame->ChangeGameMode(GameMode::CreateNewCharacter);
                return;
            }
            break;

        case 7:
            if (m_pGame->m_pCharList[m_cCurFocus - 1] != nullptr)
            {
                m_pGame->ChangeGameMode(GameMode::QueryDeleteCharacter);
                m_pGame->m_wEnterGameType = m_cCurFocus;
                return;
            }
            break;

        case 8:
            m_pGame->ChangeGameMode(GameMode::ChangePassword);
            return;

        case 9:
            m_pGame->ChangeGameMode(GameMode::MainMenu);
            return;
        }
    }
}

void Screen_SelectCharacter::on_render()
{
    // Sync local focus to global focus to maintain compatibility with the static helper
    m_pGame->m_cCurFocus = m_cCurFocus;
    
    // Pass local state to static drawing method
    DrawBackground(m_pGame, 0, 10, m_sSelCharMsX, m_sSelCharMsY, false);
    
    m_pGame->DrawVersion();
}

// Static helper implementation
void Screen_SelectCharacter::DrawBackground(CGame* pGame, short sX, short sY, short msX, short msY, bool bIgnoreFocus)
{
    // Logic migrated from CGame::UpdateScreen_OnSelectCharacter
    int i;
    int iYear, iMonth, iDay, iHour, iMinute;
    int64_t iTemp1, iTemp2;
    char cTotalChar = 0;
    uint32_t dwTime = GameClock::GetTimeMS();
    sY = 10;
    pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_SELECTCHAR, 0 + SCREENX, 0 + SCREENY, 0);
    pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, 0 + SCREENX, 0 + SCREENY, 50);

    iTemp1 = 0;
    iTemp2 = 0;
    iYear = iMonth = iDay = iHour = iMinute = 0;
    
    // Use pGame->m_cCurFocus as the source of truth for focus since this is shared
    int cCurFocus = pGame->m_cCurFocus;

    for (i = 0; i < 4; i++)
    {
        if ((cCurFocus - 1 == i) && (bIgnoreFocus == false))
            pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(sX + 110 + i * 109 - 7 + SCREENX, 63 - 9 + SCREENY, 62);
        else pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(sX + 110 + i * 109 - 7 + SCREENX, 63 - 9 + SCREENY, 61);

        if (pGame->m_pCharList[i] != nullptr)
        {
            cTotalChar++;
            switch (pGame->m_pCharList[i]->m_sSex) {
            case 1:	pGame->m_entityState.m_sOwnerType = 1; break;
            case 2:	pGame->m_entityState.m_sOwnerType = 4; break;
            }
            pGame->m_entityState.m_sOwnerType += pGame->m_pCharList[i]->m_sSkinCol - 1;
            pGame->m_entityState.m_iDir = pGame->m_cMenuDir;
            pGame->m_entityState.m_sAppr1 = pGame->m_pCharList[i]->m_sAppr1;
            pGame->m_entityState.m_sAppr2 = pGame->m_pCharList[i]->m_sAppr2;
            pGame->m_entityState.m_sAppr3 = pGame->m_pCharList[i]->m_sAppr3;
            pGame->m_entityState.m_sAppr4 = pGame->m_pCharList[i]->m_sAppr4;
            pGame->m_entityState.m_iApprColor = pGame->m_pCharList[i]->m_iApprColor; 

            std::memset(pGame->m_entityState.m_cName.data(), 0, sizeof(pGame->m_entityState.m_cName.data()));
            memcpy(pGame->m_entityState.m_cName.data(), pGame->m_pCharList[i]->m_cName, 10);
            
            pGame->m_entityState.m_iAction = DEF_OBJECTMOVE;
            pGame->m_entityState.m_iFrame = pGame->m_cMenuFrame;

            if (pGame->m_pCharList[i]->m_sSex != 0)
            {
                if (CMisc::bCheckValidString(pGame->m_pCharList[i]->m_cName) == true)
                {
                    pGame->m_pEffectSpr[0]->Draw(sX + 157 + i * 109 + SCREENX, sY + 138 + SCREENY, 1, SpriteLib::DrawParams::Alpha(0.5f));
                    pGame->DrawObject_OnMove_ForMenu(0, 0, sX + 157 + i * 109 + SCREENX, sY + 138 + SCREENY, false, dwTime);
                    pGame->PutString(sX + 112 + i * 109 + SCREENX, sY + 179 - 9 + SCREENY, pGame->m_pCharList[i]->m_cName, RGB(51, 0, 51));
                    int	_sLevel = pGame->m_pCharList[i]->m_sLevel;
                    char G_cTxt[256];
                    wsprintf(G_cTxt, "%d", _sLevel);
                    pGame->PutString(sX + 138 + i * 109 + SCREENX, sY + 196 - 10 + SCREENY, G_cTxt, RGB(51, 0, 51)); 

                    pGame->DisplayCommaNumber_G_cTxt(pGame->m_pCharList[i]->m_iExp);
                    pGame->PutString(sX + 138 + i * 109 + SCREENX, sY + 211 - 10 + SCREENY, pGame->G_cTxt, RGB(51, 0, 51)); 
                }
                iTemp2 = (int64_t)pGame->m_pCharList[i]->m_iYear * 1000000 + (int64_t)pGame->m_pCharList[i]->m_iMonth * 60000 + (int64_t)pGame->m_pCharList[i]->m_iDay * 1700 + (int64_t)pGame->m_pCharList[i]->m_iHour * 70 + (int64_t)pGame->m_pCharList[i]->m_iMinute;
                if (iTemp1 < iTemp2)
                {
                    iYear = pGame->m_pCharList[i]->m_iYear;
                    iMonth = pGame->m_pCharList[i]->m_iMonth;
                    iDay = pGame->m_pCharList[i]->m_iDay;
                    iHour = pGame->m_pCharList[i]->m_iHour;
                    iMinute = pGame->m_pCharList[i]->m_iMinute;
                    iTemp1 = iTemp2;
                }
            }
        }
    }
    i = 0;

    pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, 0 + SCREENX, 0 + SCREENY, 51);
    pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, 0 + SCREENX, 0 + SCREENY, 52);
    pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, 0 + SCREENX, 0 + SCREENY, 53);
    pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, 0 + SCREENX, 0 + SCREENY, 54);
    pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, 0 + SCREENX, 0 + SCREENY, 55);

    if ((msX > 360 + SCREENX) && (msY >= 283 + SCREENY) && (msX < 545 + SCREENX) & (msY <= 315 + SCREENY)) {
        pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, 0 + SCREENX, 0 + SCREENY, 56);
        pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 290 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER1);
        pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 305 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER2);
        pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 320 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER3);
        pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 335 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER4);
    }
    else if ((msX > 360 + SCREENX) && (msY >= 316 + SCREENY) && (msX < 545 + SCREENX) & (msY <= 345 + SCREENY)) {
        pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, 0 + SCREENX, 0 + SCREENY, 57);
        pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 305 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER5);
    }
    else if ((msX > 360 + SCREENX) && (msY >= 346 + SCREENY) && (msX < 545 + SCREENX) & (msY <= 375 + SCREENY)) {
        pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, 0 + SCREENX, 0 + SCREENY, 58);
        pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 275 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER6);
        pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 290 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER7);
    }
    else if ((msX > 360 + SCREENX) && (msY >= 376 + SCREENY) && (msX < 545 + SCREENX) & (msY <= 405 + SCREENY)) {
        pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, 0 + SCREENX, 0 + SCREENY, 59);
        pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 305 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER12);
    }
    else if ((msX > 360 + SCREENX) && (msY >= 406 + SCREENY) && (msX < 545 + SCREENX) & (msY <= 435 + SCREENY)) {
        pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, 0 + SCREENX, 0 + SCREENY, 60);
        pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 305 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER13);
    }
    else {
        if (cTotalChar == 0) {
            pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 275 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER14);
            pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 290 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER15);
            pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 305 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER16);
            pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 320 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER17);
            pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 335 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER18);
        }
        else if (cTotalChar < 4) {
            pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 275 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER19);
            pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 290 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER20);
            pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 305 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER21);
            pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 320 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER22);
            pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 335 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER23);
            pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 350 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER24);
        }
        if (cTotalChar == 4) {
            pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 290 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER25);
            pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 305 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER26);
            pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 320 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER27);
            pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 335 + 15 + SCREENY, UPDATE_SCREEN_ON_SELECT_CHARACTER28);
        }
    }
    
    int iTempMon, iTempDay, iTempHour, iTempMin;
    iTempMon = iTempDay = iTempHour = iTempMin = 0;
    
    if (pGame->m_iAccntYear != 0)
    {
        iTempMin = (pGame->m_iTimeLeftSecAccount / 60);
        wsprintf(pGame->G_cTxt, UPDATE_SCREEN_ON_SELECT_CHARACTER37, pGame->m_iAccntYear, pGame->m_iAccntMonth, pGame->m_iAccntDay, iTempMin);
    }
    else
    {
        if (pGame->m_iTimeLeftSecAccount > 0)
        {
            iTempDay = (pGame->m_iTimeLeftSecAccount / (60 * 60 * 24));
            iTempHour = (pGame->m_iTimeLeftSecAccount / (60 * 60)) % 24;
            iTempMin = (pGame->m_iTimeLeftSecAccount / 60) % 60;
            wsprintf(pGame->G_cTxt, UPDATE_SCREEN_ON_SELECT_CHARACTER38, iTempDay, iTempHour, iTempMin);
        }
        else strcpy(pGame->G_cTxt, UPDATE_SCREEN_ON_SELECT_CHARACTER39);
    }
    pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 385 + 10 + SCREENY, pGame->G_cTxt);

    if (pGame->m_iIpYear != 0)
    {
        iTempHour = (pGame->m_iTimeLeftSecIP / (60 * 60));
        iTempMin = (pGame->m_iTimeLeftSecIP / 60) % 60;
        wsprintf(pGame->G_cTxt, UPDATE_SCREEN_ON_SELECT_CHARACTER40, pGame->m_iIpYear, pGame->m_iIpMonth, pGame->m_iIpDay, iTempHour, iTempMin);
    }
    else
    {
        if (pGame->m_iTimeLeftSecIP > 0)
        {
            iTempDay = (pGame->m_iTimeLeftSecIP / (60 * 60 * 24));
            iTempHour = (pGame->m_iTimeLeftSecIP / (60 * 60)) % 24;
            iTempMin = (pGame->m_iTimeLeftSecIP / 60) % 60;
            wsprintf(pGame->G_cTxt, UPDATE_SCREEN_ON_SELECT_CHARACTER41, iTempDay, iTempHour, iTempMin);
        }
        else strcpy(pGame->G_cTxt, UPDATE_SCREEN_ON_SELECT_CHARACTER42);
    }
    pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 400 + 10 + SCREENY, pGame->G_cTxt);
    if (iYear != 0)
    {
        wsprintf(pGame->G_cTxt, UPDATE_SCREEN_ON_SELECT_CHARACTER43, iYear, iMonth, iDay, iHour, iMinute);
        pGame->PutAlignedString(98 + SCREENX, 357 + SCREENX, 415 + 10 + SCREENY, pGame->G_cTxt);
    }

    pGame->PutAlignedString(122, 315, 456, UPDATE_SCREEN_ON_SELECT_CHARACTER36);
}
