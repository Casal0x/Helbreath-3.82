// Screen_SelectCharacter.cpp: Select Character Screen Implementation
//
//////////////////////////////////////////////////////////////////////

#include "Screen_SelectCharacter.h"
#include "Game.h"
#include "GameModeManager.h"
#include "IInput.h"
#include "GlobalDef.h"
#include "ASIOSocket.h"
#include "Misc.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"

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
    // Set current mode for code that checks GameModeManager::GetMode()
    GameModeManager::SetCurrentMode(GameMode::SelectCharacter);

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
    m_pGame->m_dwCurTime = dwTime;

    // Handle legacy arrow input (if set by OnKeyDown) or direct input
    // NOTE: Preferring direct input for robustness
    if (Input::IsKeyPressed(KeyCode::Right)) {
        m_cCurFocus++;
        if (m_cCurFocus > m_cMaxFocus) m_cCurFocus = 1;
    }
    else if (Input::IsKeyPressed(KeyCode::Left)) {
        m_cCurFocus--;
        if (m_cCurFocus <= 0) m_cCurFocus = m_cMaxFocus;
    }
    

    if (Input::IsKeyPressed(KeyCode::Escape) == true)
    {
        m_pGame->ChangeGameMode(GameMode::MainMenu);
        return;
    }

    if (Input::IsKeyPressed(KeyCode::Enter) == true)
    {
        m_pGame->PlayGameSound('E', 14, 5);

        if (m_pGame->m_pCharList[m_cCurFocus - 1] != nullptr)
        {
            if (m_pGame->m_pCharList[m_cCurFocus - 1]->m_sSex != 0)
            {
                std::memset(m_pGame->m_pPlayer->m_cPlayerName, 0, sizeof(m_pGame->m_pPlayer->m_cPlayerName));
                std::snprintf(m_pGame->m_pPlayer->m_cPlayerName, sizeof(m_pGame->m_pPlayer->m_cPlayerName), "%s", m_pGame->m_pCharList[m_cCurFocus - 1]->m_cName);
                m_pGame->m_pPlayer->m_iLevel = (int)m_pGame->m_pCharList[m_cCurFocus - 1]->m_sLevel;
                if (CMisc::bCheckValidString(m_pGame->m_pPlayer->m_cPlayerName) == true)
                {
                    m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_LOGIN]->Unload();
                    m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_MAINMENU]->Unload();
                    m_pGame->m_pLSock = std::make_unique<ASIOSocket>(m_pGame->m_pIOPool->GetContext(), DEF_SOCKETBLOCKLIMIT);
                    m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort + (rand() % 1));
                    m_pGame->m_pLSock->bInitBufferSize(DEF_MSGBUFFERSIZE);
                    m_pGame->ChangeGameMode(GameMode::Connecting);
                    m_pGame->m_dwConnectMode = MSGID_REQUEST_ENTERGAME;
                    m_pGame->m_wEnterGameType = DEF_ENTERGAMEMSGTYPE_NEW;
                    std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
                    std::snprintf(m_pGame->m_cMsg, sizeof(m_pGame->m_cMsg), "%s", "33");
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

        // Determine which button was clicked
        int iMIbuttonNum = 0;
        if (Input::IsMouseInRect(100 + OX, 50 + OY, 110, 200))
        {
            m_pGame->PlayGameSound('E', 14, 5);
            iMIbuttonNum = 1;
        }
        else if (Input::IsMouseInRect(211 + OX, 50 + OY, 110, 200))
        {
            m_pGame->PlayGameSound('E', 14, 5);
            iMIbuttonNum = 2;
        }
        else if (Input::IsMouseInRect(322 + OX, 50 + OY, 109, 200))
        {
            m_pGame->PlayGameSound('E', 14, 5);
            iMIbuttonNum = 3;
        }
        else if (Input::IsMouseInRect(432 + OX, 50 + OY, 110, 200))
        {
            m_pGame->PlayGameSound('E', 14, 5);
            iMIbuttonNum = 4;
        }
        else if (Input::IsMouseInRect(360 + OX, 283 + OY, 185, 32))
        {
            m_pGame->PlayGameSound('E', 14, 5);
            iMIbuttonNum = 5;
        }
        else if (Input::IsMouseInRect(360 + OX, 316 + OY, 185, 29))
        {
            m_pGame->PlayGameSound('E', 14, 5);
            iMIbuttonNum = 6;
        }
        else if (Input::IsMouseInRect(360 + OX, 346 + OY, 185, 29))
        {
            m_pGame->PlayGameSound('E', 14, 5);
            iMIbuttonNum = 7;
        }
        else if (Input::IsMouseInRect(360 + OX, 376 + OY, 185, 29))
        {
            m_pGame->PlayGameSound('E', 14, 5);
            iMIbuttonNum = 8;
        }
        else if (Input::IsMouseInRect(360 + OX, 406 + OY, 185, 29))
        {
            m_pGame->PlayGameSound('E', 14, 5);
            iMIbuttonNum = 9;
        }

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
                        std::snprintf(m_pGame->m_pPlayer->m_cPlayerName, sizeof(m_pGame->m_pPlayer->m_cPlayerName), "%s", m_pGame->m_pCharList[m_cCurFocus - 1]->m_cName);
                        m_pGame->m_pPlayer->m_iLevel = (int)m_pGame->m_pCharList[m_cCurFocus - 1]->m_sLevel;
                        if (CMisc::bCheckValidString(m_pGame->m_pPlayer->m_cPlayerName) == true)
                        {
                            m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_LOGIN]->Unload();
                            m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_MAINMENU]->Unload();
                            m_pGame->m_pLSock = std::make_unique<ASIOSocket>(m_pGame->m_pIOPool->GetContext(), DEF_SOCKETBLOCKLIMIT);
                            m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort + (rand() % 1));
                            m_pGame->m_pLSock->bInitBufferSize(DEF_MSGBUFFERSIZE);
                            m_pGame->ChangeGameMode(GameMode::Connecting);
                            m_pGame->m_dwConnectMode = MSGID_REQUEST_ENTERGAME;
                            m_pGame->m_wEnterGameType = DEF_ENTERGAMEMSGTYPE_NEW;
                            std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
                            std::snprintf(m_pGame->m_cMsg, sizeof(m_pGame->m_cMsg), "%s", "33");
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
                    std::snprintf(m_pGame->m_pPlayer->m_cPlayerName, sizeof(m_pGame->m_pPlayer->m_cPlayerName), "%s", m_pGame->m_pCharList[m_cCurFocus - 1]->m_cName);
                    m_pGame->m_pPlayer->m_iLevel = (int)m_pGame->m_pCharList[m_cCurFocus - 1]->m_sLevel;

                    if (CMisc::bCheckValidString(m_pGame->m_pPlayer->m_cPlayerName) == true) {
                        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_LOGIN]->Unload();
                        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_MAINMENU]->Unload();
                        m_pGame->m_pLSock = std::make_unique<ASIOSocket>(m_pGame->m_pIOPool->GetContext(), DEF_SOCKETBLOCKLIMIT);
                        m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort + (rand() % 1));
                        m_pGame->m_pLSock->bInitBufferSize(DEF_MSGBUFFERSIZE);
                        m_pGame->ChangeGameMode(GameMode::Connecting);
                        m_pGame->m_dwConnectMode = MSGID_REQUEST_ENTERGAME;
                        m_pGame->m_wEnterGameType = DEF_ENTERGAMEMSGTYPE_NEW;
                        std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
                        std::snprintf(m_pGame->m_cMsg, sizeof(m_pGame->m_cMsg), "%s", "33");
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
    sX = OX;
    sY = 10 + OY;
    pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_SELECTCHAR, 0, 0, 0);
    pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, OX, OY, 50);

    iTemp1 = 0;
    iTemp2 = 0;
    iYear = iMonth = iDay = iHour = iMinute = 0;
    
    // Use pGame->m_cCurFocus as the source of truth for focus since this is shared
    int cCurFocus = pGame->m_cCurFocus;

    for (i = 0; i < 4; i++)
    {
        if ((cCurFocus - 1 == i) && (bIgnoreFocus == false))
            pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(sX + 110 + i * 109 - 7, 63 - 9 + OY, 62);
        else pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(sX + 110 + i * 109 - 7, 63 - 9 + OY, 61);

        if (pGame->m_pCharList[i] != nullptr)
        {
            cTotalChar++;
            switch (pGame->m_pCharList[i]->m_sSex) {
            case 1:	pGame->m_entityState.m_sOwnerType = hb::owner::MaleFirst; break;
            case 2:	pGame->m_entityState.m_sOwnerType = hb::owner::FemaleFirst; break;
            }
            pGame->m_entityState.m_sOwnerType += pGame->m_pCharList[i]->m_sSkinCol - 1;
            pGame->m_entityState.m_iDir = pGame->m_cMenuDir;
            pGame->m_entityState.m_appearance = pGame->m_pCharList[i]->m_appearance;

            std::memset(pGame->m_entityState.m_cName.data(), 0, sizeof(pGame->m_entityState.m_cName.data()));
            memcpy(pGame->m_entityState.m_cName.data(), pGame->m_pCharList[i]->m_cName, 10);
            
            pGame->m_entityState.m_iAction = DEF_OBJECTMOVE;
            pGame->m_entityState.m_iFrame = pGame->m_cMenuFrame;

            if (pGame->m_pCharList[i]->m_sSex != 0)
            {
                if (CMisc::bCheckValidString(pGame->m_pCharList[i]->m_cName) == true)
                {
                    pGame->m_pEffectSpr[0]->Draw(sX + 157 + i * 109, sY + 138, 1, SpriteLib::DrawParams::AdditiveNoColorKey(0.25f));
                    pGame->DrawObject_OnMove_ForMenu(0, 0, sX + 157 + i * 109, sY + 138, false, dwTime);
                    TextLib::DrawText(GameFont::Default, sX + 112 + i * 109, sY + 179 - 9, pGame->m_pCharList[i]->m_cName, TextLib::TextStyle::Color(GameColors::UISelectPurple));
                    int	_sLevel = pGame->m_pCharList[i]->m_sLevel;
                    char charInfoBuf[32];
                    snprintf(charInfoBuf, sizeof(charInfoBuf), "%d", _sLevel);
                    TextLib::DrawText(GameFont::Default, sX + 138 + i * 109, sY + 196 - 10, charInfoBuf, TextLib::TextStyle::Color(GameColors::UISelectPurple));

                    pGame->FormatCommaNumber(pGame->m_pCharList[i]->m_iExp, charInfoBuf, sizeof(charInfoBuf));
                    TextLib::DrawText(GameFont::Default, sX + 138 + i * 109, sY + 211 - 10, charInfoBuf, TextLib::TextStyle::Color(GameColors::UISelectPurple));
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

    pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, OX, OY, 51);
    pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, OX, OY, 52);
    pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, OX, OY, 53);
    pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, OX, OY, 54);
    pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, OX, OY, 55);

    if ((msX > 360 + OX) && (msY >= 283 + OY) && (msX < 545 + OX) && (msY <= 315 + OY)) {
        pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, OX, OY, 56);
        TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 290 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER1, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 305 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER2, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 320 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER3, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 335 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER4, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX > 360 + OX) && (msY >= 316 + OY) && (msX < 545 + OX) && (msY <= 345 + OY)) {
        pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, OX, OY, 57);
        TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 305 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER5, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX > 360 + OX) && (msY >= 346 + OY) && (msX < 545 + OX) && (msY <= 375 + OY)) {
        pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, OX, OY, 58);
        TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 275 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER6, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 290 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER7, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX > 360 + OX) && (msY >= 376 + OY) && (msX < 545 + OX) && (msY <= 405 + OY)) {
        pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, OX, OY, 59);
        TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 305 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER12, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX > 360 + OX) && (msY >= 406 + OY) && (msX < 545 + OX) && (msY <= 435 + OY)) {
        pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, OX, OY, 60);
        TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 305 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER13, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else {
        if (cTotalChar == 0) {
            TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 275 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER14, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 290 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER15, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 305 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER16, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 320 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER17, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 335 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER18, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        }
        else if (cTotalChar < 4) {
            TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 275 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER19, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 290 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER20, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 305 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER21, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 320 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER22, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 335 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER23, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 350 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER24, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        }
        if (cTotalChar == 4) {
            TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 290 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER25, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 305 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER26, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 320 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER27, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 335 + 15 + OY, (357) - (98), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER28, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        }
    }
    
    int iTempMon, iTempDay, iTempHour, iTempMin;
    char infoBuf[128];
    iTempMon = iTempDay = iTempHour = iTempMin = 0;

    if (pGame->m_iAccntYear != 0)
    {
        iTempMin = (pGame->m_iTimeLeftSecAccount / 60);
        snprintf(infoBuf, sizeof(infoBuf), UPDATE_SCREEN_ON_SELECT_CHARACTER37, pGame->m_iAccntYear, pGame->m_iAccntMonth, pGame->m_iAccntDay);
    }
    else
    {
        if (pGame->m_iTimeLeftSecAccount > 0)
        {
            iTempDay = (pGame->m_iTimeLeftSecAccount / (60 * 60 * 24));
            iTempHour = (pGame->m_iTimeLeftSecAccount / (60 * 60)) % 24;
            iTempMin = (pGame->m_iTimeLeftSecAccount / 60) % 60;
            snprintf(infoBuf, sizeof(infoBuf), UPDATE_SCREEN_ON_SELECT_CHARACTER38, iTempDay, iTempHour, iTempMin);
        }
        else snprintf(infoBuf, sizeof(infoBuf), "%s", UPDATE_SCREEN_ON_SELECT_CHARACTER39);
    }
    TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 385 + 10 + OY, (357) - (98), 15, infoBuf, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);

    if (pGame->m_iIpYear != 0)
    {
        iTempHour = (pGame->m_iTimeLeftSecIP / (60 * 60));
        iTempMin = (pGame->m_iTimeLeftSecIP / 60) % 60;
        snprintf(infoBuf, sizeof(infoBuf), UPDATE_SCREEN_ON_SELECT_CHARACTER40, iTempHour, iTempMin);
    }
    else
    {
        if (pGame->m_iTimeLeftSecIP > 0)
        {
            iTempDay = (pGame->m_iTimeLeftSecIP / (60 * 60 * 24));
            iTempHour = (pGame->m_iTimeLeftSecIP / (60 * 60)) % 24;
            iTempMin = (pGame->m_iTimeLeftSecIP / 60) % 60;
            snprintf(infoBuf, sizeof(infoBuf), UPDATE_SCREEN_ON_SELECT_CHARACTER41, iTempDay, iTempHour, iTempMin);
        }
        else snprintf(infoBuf, sizeof(infoBuf), "%s", UPDATE_SCREEN_ON_SELECT_CHARACTER42);
    }
    TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 400 + 10 + OY, (357) - (98), 15, infoBuf, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    if (iYear != 0)
    {
        snprintf(infoBuf, sizeof(infoBuf), UPDATE_SCREEN_ON_SELECT_CHARACTER43, iYear, iMonth, iDay, iHour, iMinute);
        TextLib::DrawTextAligned(GameFont::Default, 98 + OX, 415 + 10 + OY, (357) - (98), 15, infoBuf, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }

    TextLib::DrawTextAligned(GameFont::Default, 122 + OX, 456 + OY, (315) - (122), 15, UPDATE_SCREEN_ON_SELECT_CHARACTER36, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
}
