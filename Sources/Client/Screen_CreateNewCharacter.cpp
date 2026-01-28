// Screen_CreateNewCharacter.cpp: Create New Character Screen Implementation
//
//////////////////////////////////////////////////////////////////////

#include "Screen_CreateNewCharacter.h"
#include "Game.h"
#include "GameModeManager.h"
#include "IInput.h"
#include "GlobalDef.h"
#include "XSocket.h"
#include "Misc.h"
#include "lan_eng.h"

Screen_CreateNewCharacter::Screen_CreateNewCharacter(CGame* pGame)
    : IGameScreen(pGame)
    , m_iNewCharPoint(10)
    , m_cNewCharPrevFocus(1)
    , m_dwNewCharMTime(0)
    , m_sNewCharMsX(0)
    , m_sNewCharMsY(0)
    , m_bNewCharFlag(false)
    , m_cCurFocus(1)
    , m_cMaxFocus(6)
{
    std::memset(m_cNewCharName, 0, sizeof(m_cNewCharName));
}

void Screen_CreateNewCharacter::on_initialize()
{
    // Set current mode for code that checks GameModeManager::GetMode()
    GameModeManager::SetCurrentMode(GameMode::CreateNewCharacter);

    // Initialize character creation defaults
    m_pGame->m_pPlayer->m_iGender = rand() % 2 + 1;
    m_pGame->m_pPlayer->m_iSkinCol = rand() % 3 + 1;
    m_pGame->m_pPlayer->m_iHairStyle = rand() % 8;
    m_pGame->m_pPlayer->m_iHairCol = rand() % 16;
    m_pGame->m_pPlayer->m_iUnderCol = rand() % 8;
    m_pGame->m_pPlayer->m_iStatModStr = 10;
    m_pGame->m_pPlayer->m_iStatModVit = 10;
    m_pGame->m_pPlayer->m_iStatModDex = 10;
    m_pGame->m_pPlayer->m_iStatModInt = 10;
    m_pGame->m_pPlayer->m_iStatModMag = 10;
    m_pGame->m_pPlayer->m_iStatModChr = 10;

    m_iNewCharPoint = 10; // 70 - (6 stats * 10) = 10 bonus points
    m_cNewCharPrevFocus = 1;
    m_cCurFocus = 1;
    m_cMaxFocus = 6;
    m_pGame->m_cArrowPressed = 0;
    m_dwNewCharMTime = GameClock::GetTimeMS();
    std::memset(m_cNewCharName, 0, sizeof(m_cNewCharName));
    m_pGame->StartInputString(193 + 4 + SCREENX, 65 + 45 + SCREENY, 11, m_cNewCharName);
    m_pGame->ClearInputString();
}

void Screen_CreateNewCharacter::on_uninitialize()
{
    m_pGame->EndInputString();
}

void Screen_CreateNewCharacter::on_update()
{
    uint32_t dwTime = GameClock::GetTimeMS();

    // Handle arrow key navigation
    if (Input::IsKeyPressed(VK_UP)) {
        m_cCurFocus--;
        if (m_cCurFocus <= 0) m_cCurFocus = m_cMaxFocus;
    }
    else if (Input::IsKeyPressed(VK_DOWN)) {
        m_cCurFocus++;
        if (m_cCurFocus > m_cMaxFocus) m_cCurFocus = 1;
    }

    // Handle focus change for input string
    if (m_cNewCharPrevFocus != m_cCurFocus) {
        m_pGame->EndInputString();
        switch (m_cCurFocus) {
        case 1:
            m_pGame->StartInputString(193 + 4 + SCREENX, 65 + 45 + SCREENY, 11, m_cNewCharName);
            break;
        }
        m_cNewCharPrevFocus = m_cCurFocus;
    }

    // ESC returns to character select
    if (Input::IsKeyPressed(VK_ESCAPE)) {
        m_pGame->ChangeGameMode(GameMode::SelectCharacter);
        return;
    }

    // Capture mouse position
    short msX = static_cast<short>(Input::GetMouseX());
    short msY = static_cast<short>(Input::GetMouseY());
    m_sNewCharMsX = msX;
    m_sNewCharMsY = msY;

    // Compute whether character creation is valid
    m_bNewCharFlag = true;
    if (strlen(m_cNewCharName) <= 0) m_bNewCharFlag = false;
    if (m_iNewCharPoint > 0) m_bNewCharFlag = false;
    if (CMisc::bCheckValidName(m_cNewCharName) == false) m_bNewCharFlag = false;

    // Animation frame updates
    if ((dwTime - m_dwNewCharMTime) > 100)
    {
        m_pGame->m_cMenuFrame++;
        m_dwNewCharMTime = dwTime;
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

    // Handle mouse clicks
    if (Input::IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        m_pGame->PlaySound('E', 14, 5);

        // Determine which button was clicked
        int iMIbuttonNum = 0;
        if (Input::IsMouseInRect(69 + SCREENX, 110 + SCREENY, 279 + SCREENX, 127 + SCREENY)) iMIbuttonNum = 1;  // Name
        else if (Input::IsMouseInRect(236 + SCREENX, 156 + SCREENY, 257 + SCREENX, 169 + SCREENY)) iMIbuttonNum = 2;  // Gender -
        else if (Input::IsMouseInRect(259 + SCREENX, 156 + SCREENY, 280 + SCREENX, 169 + SCREENY)) iMIbuttonNum = 3;  // Gender +
        else if (Input::IsMouseInRect(236 + SCREENX, 171 + SCREENY, 257 + SCREENX, 184 + SCREENY)) iMIbuttonNum = 4;  // Skin -
        else if (Input::IsMouseInRect(259 + SCREENX, 171 + SCREENY, 280 + SCREENX, 184 + SCREENY)) iMIbuttonNum = 5;  // Skin +
        else if (Input::IsMouseInRect(236 + SCREENX, 186 + SCREENY, 257 + SCREENX, 199 + SCREENY)) iMIbuttonNum = 6;  // Hair style -
        else if (Input::IsMouseInRect(259 + SCREENX, 186 + SCREENY, 280 + SCREENX, 199 + SCREENY)) iMIbuttonNum = 7;  // Hair style +
        else if (Input::IsMouseInRect(236 + SCREENX, 201 + SCREENY, 257 + SCREENX, 214 + SCREENY)) iMIbuttonNum = 8;  // Hair color -
        else if (Input::IsMouseInRect(259 + SCREENX, 201 + SCREENY, 280 + SCREENX, 214 + SCREENY)) iMIbuttonNum = 9;  // Hair color +
        else if (Input::IsMouseInRect(236 + SCREENX, 216 + SCREENY, 257 + SCREENX, 229 + SCREENY)) iMIbuttonNum = 10; // Underwear -
        else if (Input::IsMouseInRect(259 + SCREENX, 216 + SCREENY, 280 + SCREENX, 229 + SCREENY)) iMIbuttonNum = 11; // Underwear +
        else if (Input::IsMouseInRect(236 + SCREENX, 276 + SCREENY, 257 + SCREENX, 289 + SCREENY)) iMIbuttonNum = 12; // Str +
        else if (Input::IsMouseInRect(259 + SCREENX, 276 + SCREENY, 280 + SCREENX, 289 + SCREENY)) iMIbuttonNum = 13; // Str -
        else if (Input::IsMouseInRect(236 + SCREENX, 291 + SCREENY, 257 + SCREENX, 304 + SCREENY)) iMIbuttonNum = 14; // Vit +
        else if (Input::IsMouseInRect(259 + SCREENX, 291 + SCREENY, 280 + SCREENX, 304 + SCREENY)) iMIbuttonNum = 15; // Vit -
        else if (Input::IsMouseInRect(236 + SCREENX, 306 + SCREENY, 257 + SCREENX, 319 + SCREENY)) iMIbuttonNum = 16; // Dex +
        else if (Input::IsMouseInRect(259 + SCREENX, 306 + SCREENY, 280 + SCREENX, 319 + SCREENY)) iMIbuttonNum = 17; // Dex -
        else if (Input::IsMouseInRect(236 + SCREENX, 321 + SCREENY, 257 + SCREENX, 334 + SCREENY)) iMIbuttonNum = 18; // Int +
        else if (Input::IsMouseInRect(259 + SCREENX, 321 + SCREENY, 280 + SCREENX, 334 + SCREENY)) iMIbuttonNum = 19; // Int -
        else if (Input::IsMouseInRect(236 + SCREENX, 336 + SCREENY, 257 + SCREENX, 349 + SCREENY)) iMIbuttonNum = 20; // Mag +
        else if (Input::IsMouseInRect(259 + SCREENX, 336 + SCREENY, 280 + SCREENX, 349 + SCREENY)) iMIbuttonNum = 21; // Mag -
        else if (Input::IsMouseInRect(236 + SCREENX, 351 + SCREENY, 257 + SCREENX, 364 + SCREENY)) iMIbuttonNum = 22; // Chr +
        else if (Input::IsMouseInRect(259 + SCREENX, 351 + SCREENY, 280 + SCREENX, 364 + SCREENY)) iMIbuttonNum = 23; // Chr -
        else if (Input::IsMouseInRect(384 + SCREENX, 445 + SCREENY, 456 + SCREENX, 460 + SCREENY)) iMIbuttonNum = 24; // Create
        else if (Input::IsMouseInRect(500 + SCREENX, 445 + SCREENY, 572 + SCREENX, 460 + SCREENY)) iMIbuttonNum = 25; // Cancel
        else if (Input::IsMouseInRect(60 + SCREENX, 445 + SCREENY, 132 + SCREENX, 460 + SCREENY)) iMIbuttonNum = 26;  // Aresden
        else if (Input::IsMouseInRect(145 + SCREENX, 445 + SCREENY, 217 + SCREENX, 460 + SCREENY)) iMIbuttonNum = 27; // Elvine
        else if (Input::IsMouseInRect(230 + SCREENX, 445 + SCREENY, 302 + SCREENX, 460 + SCREENY)) iMIbuttonNum = 28; // Traveler

        switch (iMIbuttonNum) {
        case 1:
            m_cCurFocus = 1;
            break;
        case 2:
            m_pGame->m_pPlayer->m_iGender--;
            if (m_pGame->m_pPlayer->m_iGender < 1) m_pGame->m_pPlayer->m_iGender = 2;
            break;
        case 3:
            m_pGame->m_pPlayer->m_iGender++;
            if (m_pGame->m_pPlayer->m_iGender > 2) m_pGame->m_pPlayer->m_iGender = 1;
            break;
        case 4:
            m_pGame->m_pPlayer->m_iSkinCol--;
            if (m_pGame->m_pPlayer->m_iSkinCol < 1) m_pGame->m_pPlayer->m_iSkinCol = 3;
            break;
        case 5:
            m_pGame->m_pPlayer->m_iSkinCol++;
            if (m_pGame->m_pPlayer->m_iSkinCol > 3) m_pGame->m_pPlayer->m_iSkinCol = 1;
            break;
        case 6:
            m_pGame->m_pPlayer->m_iHairStyle--;
            if (m_pGame->m_pPlayer->m_iHairStyle < 0) m_pGame->m_pPlayer->m_iHairStyle = 7;
            break;
        case 7:
            m_pGame->m_pPlayer->m_iHairStyle++;
            if (m_pGame->m_pPlayer->m_iHairStyle > 7) m_pGame->m_pPlayer->m_iHairStyle = 0;
            break;
        case 8:
            m_pGame->m_pPlayer->m_iHairCol--;
            if (m_pGame->m_pPlayer->m_iHairCol < 0) m_pGame->m_pPlayer->m_iHairCol = 15;
            break;
        case 9:
            m_pGame->m_pPlayer->m_iHairCol++;
            if (m_pGame->m_pPlayer->m_iHairCol > 15) m_pGame->m_pPlayer->m_iHairCol = 0;
            break;
        case 10:
            m_pGame->m_pPlayer->m_iUnderCol--;
            if (m_pGame->m_pPlayer->m_iUnderCol < 0) m_pGame->m_pPlayer->m_iUnderCol = 7;
            break;
        case 11:
            m_pGame->m_pPlayer->m_iUnderCol++;
            if (m_pGame->m_pPlayer->m_iUnderCol > 7) m_pGame->m_pPlayer->m_iUnderCol = 0;
            break;
        case 12:
            if (m_iNewCharPoint > 0) {
                if (m_pGame->m_pPlayer->m_iStatModStr < 14) {
                    m_pGame->m_pPlayer->m_iStatModStr++;
                    m_iNewCharPoint--;
                }
            }
            break;
        case 13:
            if (m_pGame->m_pPlayer->m_iStatModStr > 10) {
                m_pGame->m_pPlayer->m_iStatModStr--;
                m_iNewCharPoint++;
            }
            break;
        case 14:
            if (m_iNewCharPoint > 0) {
                if (m_pGame->m_pPlayer->m_iStatModVit < 14) {
                    m_pGame->m_pPlayer->m_iStatModVit++;
                    m_iNewCharPoint--;
                }
            }
            break;
        case 15:
            if (m_pGame->m_pPlayer->m_iStatModVit > 10) {
                m_pGame->m_pPlayer->m_iStatModVit--;
                m_iNewCharPoint++;
            }
            break;
        case 16:
            if (m_iNewCharPoint > 0) {
                if (m_pGame->m_pPlayer->m_iStatModDex < 14) {
                    m_pGame->m_pPlayer->m_iStatModDex++;
                    m_iNewCharPoint--;
                }
            }
            break;
        case 17:
            if (m_pGame->m_pPlayer->m_iStatModDex > 10) {
                m_pGame->m_pPlayer->m_iStatModDex--;
                m_iNewCharPoint++;
            }
            break;
        case 18:
            if (m_iNewCharPoint > 0) {
                if (m_pGame->m_pPlayer->m_iStatModInt < 14) {
                    m_pGame->m_pPlayer->m_iStatModInt++;
                    m_iNewCharPoint--;
                }
            }
            break;
        case 19:
            if (m_pGame->m_pPlayer->m_iStatModInt > 10) {
                m_pGame->m_pPlayer->m_iStatModInt--;
                m_iNewCharPoint++;
            }
            break;
        case 20:
            if (m_iNewCharPoint > 0) {
                if (m_pGame->m_pPlayer->m_iStatModMag < 14) {
                    m_pGame->m_pPlayer->m_iStatModMag++;
                    m_iNewCharPoint--;
                }
            }
            break;
        case 21:
            if (m_pGame->m_pPlayer->m_iStatModMag > 10) {
                m_pGame->m_pPlayer->m_iStatModMag--;
                m_iNewCharPoint++;
            }
            break;
        case 22:
            if (m_iNewCharPoint > 0) {
                if (m_pGame->m_pPlayer->m_iStatModChr < 14) {
                    m_pGame->m_pPlayer->m_iStatModChr++;
                    m_iNewCharPoint--;
                }
            }
            break;
        case 23:
            if (m_pGame->m_pPlayer->m_iStatModChr > 10)
            {
                m_pGame->m_pPlayer->m_iStatModChr--;
                m_iNewCharPoint++;
            }
            break;

        case 24: // Create button
            if (m_cCurFocus != 2)
            {
                m_cCurFocus = 2;
                return;
            }
            if (m_bNewCharFlag == false) return;
            if (CMisc::bCheckValidName(m_cNewCharName) == false) break;
            std::memset(m_pGame->m_pPlayer->m_cPlayerName, 0, sizeof(m_pGame->m_pPlayer->m_cPlayerName));
            strcpy(m_pGame->m_pPlayer->m_cPlayerName, m_cNewCharName);
            m_pGame->m_pLSock = std::make_unique<XSocket>(DEF_SOCKETBLOCKLIMIT);
            m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort + (rand() % 1));
            m_pGame->m_pLSock->bInitBufferSize(30000);
            m_pGame->ChangeGameMode(GameMode::Connecting);
            m_pGame->m_dwConnectMode = MSGID_REQUEST_CREATENEWCHARACTER;
            std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
            strcpy(m_pGame->m_cMsg, "22");
            return;

        case 25: // Cancel button
            if (m_cCurFocus != 3)
            {
                m_cCurFocus = 3;
                return;
            }
            m_pGame->ChangeGameMode(GameMode::SelectCharacter);
            return;

        case 26: // WARRIOR preset
            if (m_cCurFocus != 4)
            {
                m_cCurFocus = 4;
                return;
            }
            m_pGame->m_pPlayer->m_iStatModMag = 10;
            m_pGame->m_pPlayer->m_iStatModInt = 10;
            m_pGame->m_pPlayer->m_iStatModChr = 10;
            m_pGame->m_pPlayer->m_iStatModStr = 14;
            m_pGame->m_pPlayer->m_iStatModVit = 12;
            m_pGame->m_pPlayer->m_iStatModDex = 14;
            m_iNewCharPoint = m_pGame->m_pPlayer->m_iStatModStr + m_pGame->m_pPlayer->m_iStatModVit + m_pGame->m_pPlayer->m_iStatModDex + m_pGame->m_pPlayer->m_iStatModInt + m_pGame->m_pPlayer->m_iStatModMag + m_pGame->m_pPlayer->m_iStatModChr;
            m_iNewCharPoint = 70 - m_iNewCharPoint;
            break;

        case 27: // MAGE preset
            if (m_cCurFocus != 5) {
                m_cCurFocus = 5;
                return;
            }
            m_pGame->m_pPlayer->m_iStatModMag = 14;
            m_pGame->m_pPlayer->m_iStatModInt = 14;
            m_pGame->m_pPlayer->m_iStatModChr = 10;
            m_pGame->m_pPlayer->m_iStatModStr = 10;
            m_pGame->m_pPlayer->m_iStatModVit = 12;
            m_pGame->m_pPlayer->m_iStatModDex = 10;
            m_iNewCharPoint = m_pGame->m_pPlayer->m_iStatModStr + m_pGame->m_pPlayer->m_iStatModVit + m_pGame->m_pPlayer->m_iStatModDex + m_pGame->m_pPlayer->m_iStatModInt + m_pGame->m_pPlayer->m_iStatModMag + m_pGame->m_pPlayer->m_iStatModChr;
            m_iNewCharPoint = 70 - m_iNewCharPoint;
            break;

        case 28: // PRIEST preset
            if (m_cCurFocus != 6) {
                m_cCurFocus = 6;
                return;
            }
            m_pGame->m_pPlayer->m_iStatModMag = 12;
            m_pGame->m_pPlayer->m_iStatModInt = 10;
            m_pGame->m_pPlayer->m_iStatModChr = 14;
            m_pGame->m_pPlayer->m_iStatModStr = 14;
            m_pGame->m_pPlayer->m_iStatModVit = 10;
            m_pGame->m_pPlayer->m_iStatModDex = 10;
            m_iNewCharPoint = m_pGame->m_pPlayer->m_iStatModStr + m_pGame->m_pPlayer->m_iStatModVit + m_pGame->m_pPlayer->m_iStatModDex + m_pGame->m_pPlayer->m_iStatModInt + m_pGame->m_pPlayer->m_iStatModMag + m_pGame->m_pPlayer->m_iStatModChr;
            m_iNewCharPoint = 70 - m_iNewCharPoint;
            break;
        }
    }
}

void Screen_CreateNewCharacter::on_render()
{
    int i = 0;
    short msX = m_sNewCharMsX;
    short msY = m_sNewCharMsY;
    uint32_t dwTime = GameClock::GetTimeMS();

    // Handle button hover focus FIRST (before drawing)
    if ((msX >= 384 + SCREENX) && (msX <= 384 + 72 + SCREENX) && (msY >= 445 + SCREENY) && (msY <= 445 + 15 + SCREENY)) {
        m_cCurFocus = 2;
    }
    else if ((msX >= 500 + SCREENX) && (msX <= 500 + 72 + SCREENX) && (msY >= 445 + SCREENY) && (msY <= 445 + 15 + SCREENY)) {
        m_cCurFocus = 3;
    }
    if ((msX >= 60 + SCREENX) && (msX <= 60 + 72 + SCREENX) && (msY >= 445 + SCREENY) && (msY <= 445 + 15 + SCREENY)) {
        m_cCurFocus = 4;
    }
    if ((msX >= 145 + SCREENX) && (msX <= 145 + 72 + SCREENX) && (msY >= 445 + SCREENY) && (msY <= 445 + 15 + SCREENY)) {
        m_cCurFocus = 5;
    }
    if ((msX >= 230 + SCREENX) && (msX <= 230 + 72 + SCREENX) && (msY >= 445 + SCREENY) && (msY <= 445 + 15 + SCREENY)) {
        m_cCurFocus = 6;
    }

    // ======== Draw character creation UI (inlined from _bDraw_OnCreateNewCharacter) ========
    m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_NEWCHAR, 0 + SCREENX, 0 + SCREENY, 0, true);
    m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, 0 + SCREENX, 0 + SCREENY, 69, true);
    m_pGame->PutAlignedString(64 + SCREENX, 282 + SCREENX, 90 + SCREENY, _BDRAW_ON_CREATE_NEW_CHARACTER1, 5, 5, 5);
    m_pGame->PutAlignedString(57 + SCREENX, 191 + SCREENX, 110 + SCREENY, DEF_MSG_CHARACTERNAME, 5, 5, 5);
    if (m_cCurFocus != 1) m_pGame->PutString(197 + SCREENX, 112 + SCREENY, m_cNewCharName, RGB(25, 35, 25));
    m_pGame->PutAlignedString(64 + SCREENX, 282 + SCREENX, 140 + SCREENY, _BDRAW_ON_CREATE_NEW_CHARACTER2, 5, 5, 5);
    m_pGame->PutString(100 + SCREENX, 160 + SCREENY, DEF_MSG_GENDER, RGB(5, 5, 5));
    m_pGame->PutString(100 + SCREENX, 175 + SCREENY, DEF_MSG_SKINCOLOR, RGB(5, 5, 5));
    m_pGame->PutString(100 + SCREENX, 190 + SCREENY, DEF_MSG_HAIRSTYLE, RGB(5, 5, 5));
    m_pGame->PutString(100 + SCREENX, 205 + SCREENY, DEF_MSG_HAIRCOLOR, RGB(5, 5, 5));
    m_pGame->PutString(100 + SCREENX, 220 + SCREENY, DEF_MSG_UNDERWEARCOLOR, RGB(5, 5, 5));
    m_pGame->PutString(100 + SCREENX, 275 + SCREENY, DEF_MSG_STRENGTH, RGB(5, 5, 5));
    m_pGame->PutString(100 + SCREENX, 292 + SCREENY, DEF_MSG_VITALITY, RGB(5, 5, 5));
    m_pGame->PutString(100 + SCREENX, 309 + SCREENY, DEF_MSG_DEXTERITY, RGB(5, 5, 5));
    m_pGame->PutString(100 + SCREENX, 326 + SCREENY, DEF_MSG_INTELLIGENCE, RGB(5, 5, 5));
    m_pGame->PutString(100 + SCREENX, 343 + SCREENY, DEF_MSG_MAGIC, RGB(5, 5, 5));
    m_pGame->PutString(100 + SCREENX, 360 + SCREENY, DEF_MSG_CHARISMA, RGB(5, 5, 5));

    // Stat values
    i = 0;
    wsprintf(m_pGame->G_cTxt, "%d", m_pGame->m_pPlayer->m_iStatModStr);
    m_pGame->PutString(204 + SCREENX, 277 + 16 * i++ + SCREENY, m_pGame->G_cTxt, RGB(25, 35, 25));
    wsprintf(m_pGame->G_cTxt, "%d", m_pGame->m_pPlayer->m_iStatModVit);
    m_pGame->PutString(204 + SCREENX, 277 + 16 * i++ + SCREENY, m_pGame->G_cTxt, RGB(25, 35, 25));
    wsprintf(m_pGame->G_cTxt, "%d", m_pGame->m_pPlayer->m_iStatModDex);
    m_pGame->PutString(204 + SCREENX, 277 + 16 * i++ + SCREENY, m_pGame->G_cTxt, RGB(25, 35, 25));
    wsprintf(m_pGame->G_cTxt, "%d", m_pGame->m_pPlayer->m_iStatModInt);
    m_pGame->PutString(204 + SCREENX, 277 + 16 * i++ + SCREENY, m_pGame->G_cTxt, RGB(25, 35, 25));
    wsprintf(m_pGame->G_cTxt, "%d", m_pGame->m_pPlayer->m_iStatModMag);
    m_pGame->PutString(204 + SCREENX, 277 + 16 * i++ + SCREENY, m_pGame->G_cTxt, RGB(25, 35, 25));
    wsprintf(m_pGame->G_cTxt, "%d", m_pGame->m_pPlayer->m_iStatModChr);
    m_pGame->PutString(204 + SCREENX, 277 + 16 * i++ + SCREENY, m_pGame->G_cTxt, RGB(25, 35, 25));

    // Button states
    if ((m_bNewCharFlag == true) && (m_cCurFocus == 2))
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(384 + SCREENX, 445 + SCREENY, 25);
    else
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(384 + SCREENX, 445 + SCREENY, 24);
    if (m_cCurFocus == 3)
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(500 + SCREENX, 445 + SCREENY, 17);
    else
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(500 + SCREENX, 445 + SCREENY, 16);
    if (m_cCurFocus == 4)
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(60 + SCREENX, 445 + SCREENY, 68);
    else
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(60 + SCREENX, 445 + SCREENY, 67);
    if (m_cCurFocus == 5)
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(145 + SCREENX, 445 + SCREENY, 66);
    else
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(145 + SCREENX, 445 + SCREENY, 65);
    if (m_cCurFocus == 6)
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(230 + SCREENX, 445 + SCREENY, 64);
    else
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(230 + SCREENX, 445 + SCREENY, 63);

    m_pGame->ShowReceivedString();

    // Character preview
    switch (m_pGame->m_pPlayer->m_iGender) {
    case 1: m_pGame->m_entityState.m_sOwnerType = 1; break;
    case 2: m_pGame->m_entityState.m_sOwnerType = 4; break;
    }
    m_pGame->m_entityState.m_sOwnerType += m_pGame->m_pPlayer->m_iSkinCol - 1;
    m_pGame->m_entityState.m_iDir = m_pGame->m_cMenuDir;
    m_pGame->m_entityState.m_sAppr1 = 0;
    m_pGame->m_entityState.m_sAppr1 = m_pGame->m_entityState.m_sAppr1 | (m_pGame->m_pPlayer->m_iUnderCol);
    m_pGame->m_entityState.m_sAppr1 = m_pGame->m_entityState.m_sAppr1 | (m_pGame->m_pPlayer->m_iHairStyle << 8);
    m_pGame->m_entityState.m_sAppr1 = m_pGame->m_entityState.m_sAppr1 | (m_pGame->m_pPlayer->m_iHairCol << 4);
    m_pGame->m_entityState.m_sAppr2 = 0;
    m_pGame->m_entityState.m_sAppr3 = 0;
    m_pGame->m_entityState.m_sAppr4 = 0;
    std::memset(m_pGame->m_entityState.m_cName.data(), 0, m_pGame->m_entityState.m_cName.size());
    memcpy(m_pGame->m_entityState.m_cName.data(), m_pGame->m_pPlayer->m_cPlayerName, 10);
    m_pGame->m_entityState.m_iAction = DEF_OBJECTMOVE;
    m_pGame->m_entityState.m_iFrame = m_pGame->m_cMenuFrame;

    m_pGame->_Draw_CharacterBody(507 + SCREENX, 267 + SCREENY, m_pGame->m_entityState.m_sOwnerType);
    m_pGame->DrawObject_OnMove_ForMenu(0 + SCREENX, 0 + SCREENY, 500 + SCREENX, 174 + SCREENY, false, dwTime);

    // Derived stats
    i = 0;
    m_pGame->PutString(445 + SCREENX, 192 + SCREENY, DEF_MSG_HITPOINT, RGB(5, 5, 5));
    wsprintf(m_pGame->G_cTxt, "%d", m_pGame->m_pPlayer->m_iStatModVit * 3 + 2 + m_pGame->m_pPlayer->m_iStatModStr / 2);
    m_pGame->PutString(550 + SCREENX, 192 + 16 * i++ + SCREENY, m_pGame->G_cTxt, RGB(25, 35, 25));
    m_pGame->PutString(445 + SCREENX, 208 + SCREENY, DEF_MSG_MANAPOINT, RGB(5, 5, 5));
    wsprintf(m_pGame->G_cTxt, "%d", m_pGame->m_pPlayer->m_iStatModMag * 2 + 2 + m_pGame->m_pPlayer->m_iStatModInt / 2);
    m_pGame->PutString(550 + SCREENX, 192 + 16 * i++ + SCREENY, m_pGame->G_cTxt, RGB(25, 35, 25));
    m_pGame->PutString(445 + SCREENX, 224 + SCREENY, DEF_MSG_STAMINARPOINT, RGB(5, 5, 5));
    wsprintf(m_pGame->G_cTxt, "%d", m_pGame->m_pPlayer->m_iStatModStr * 2 + 2);
    m_pGame->PutString(550 + SCREENX, 192 + 16 * i++ + SCREENY, m_pGame->G_cTxt, RGB(25, 35, 25));

    // ======== End inlined drawing ========

    m_pGame->DrawVersion();

    // Tooltip drawing based on mouse position
    if ((msX >= 65 + 4 - 127 + SCREENX) && (msX <= 275 + 4 + SCREENX) && (msY >= 65 + 45 + SCREENY) && (msY <= 82 + 45 + SCREENY)) {
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER1);
    }
    else if ((msX >= 261 + 4 - 212 + SCREENX) && (msX <= 289 + 4 + SCREENX) && (msY >= 111 + 45 + SCREENY) && (msY <= 124 + 45 + SCREENY)) {
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER2);
    }
    else if ((msX >= 261 + 4 - 212 + SCREENX) && (msX <= 289 + 4 + SCREENX) && (msY >= 126 + 45 + SCREENY) && (msY <= 139 + 45 + SCREENY)) {
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER3);
    }
    else if ((msX >= 261 + 4 - 212 + SCREENX) && (msX <= 289 + 4 + SCREENX) && (msY >= 141 + 45 + SCREENY) && (msY <= 154 + 45 + SCREENY)) {
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER4);
    }
    else if ((msX >= 261 + 4 - 212 + SCREENX) && (msX <= 289 + 4 + SCREENX) && (msY >= 156 + 45 + SCREENY) && (msY <= 169 + 45 + SCREENY)) {
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER5);
    }
    else if ((msX >= 261 + 4 - 212 + SCREENX) && (msX <= 289 + 4 + SCREENX) && (msY >= 171 + 45 + SCREENY) && (msY <= 184 + 45 + SCREENY)) {
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER6);
    }
    else if ((msX >= 240 + 4 - 175 + SCREENX) && (msX <= 268 + 4 + SCREENX) && (msY >= 231 + 45 + SCREENY) && (msY <= 244 + 45 + SCREENY)) {
        // Str tooltip
        i = 0;
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER7);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER8);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER9);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER10);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER11);
    }
    else if ((msX >= 240 + 4 - 175 + SCREENX) && (msX <= 268 + 4 + SCREENX) && (msY >= 246 + 45 + SCREENY) && (msY <= 259 + 45 + SCREENY)) {
        // Vit tooltip
        i = 0;
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER12);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER13);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER14);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER15);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER16);
    }
    else if ((msX >= 240 + 4 - 175 + SCREENX) && (msX <= 268 + 4 + SCREENX) && (msY >= 261 + 45 + SCREENY) && (msY <= 274 + 45 + SCREENY)) {
        // Dex tooltip
        i = 0;
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER17);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER18);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER19);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER20);
    }
    else if ((msX >= 240 + 4 - 175 + SCREENX) && (msX <= 268 + 4 + SCREENX) && (msY >= 276 + 45 + SCREENY) && (msY <= 289 + 45 + SCREENY)) {
        // Int tooltip
        i = 0;
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER21);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER22);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER23);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER24);
    }
    else if ((msX >= 240 + 4 - 175 + SCREENX) && (msX <= 268 + 4 + SCREENX) && (msY >= 291 + 45 + SCREENY) && (msY <= 304 + 45 + SCREENY)) {
        // Mag tooltip
        i = 0;
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER25);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER26);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER27);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER28);
    }
    else if ((msX >= 240 + 4 - 175 + SCREENX) && (msX <= 268 + 4 + SCREENX) && (msY >= 306 + 45 + SCREENY) && (msY <= 319 + 45 + SCREENY)) {
        // Charisma tooltip
        i = 0;
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER29);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER30);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER31);
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER32);
    }
    else if ((msX >= 384 + SCREENX) && (msX <= 384 + 72 + SCREENX) && (msY >= 445 + SCREENY) && (msY <= 445 + 15 + SCREENY)) {
        if (strlen(m_cNewCharName) <= 0) {
            i = 0;
            m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER35);
        }
        else if (m_iNewCharPoint > 0) {
            i = 0;
            m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER36);
        }
        else if (CMisc::bCheckValidName(m_cNewCharName) == false) {
            i = 0;
            m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER39);
            m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER40);
            m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER41);
        }
        else {
            i = 0;
            m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER44);
            m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER45);
            m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER46);
            m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER47);
            m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + 16 * i++ + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER48);
        }
    }
    else if ((msX >= 500 + SCREENX) && (msX <= 500 + 72 + SCREENX) && (msY >= 445 + SCREENY) && (msY <= 445 + 15 + SCREENY)) {
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER49);
    }
    else if ((msX >= 60 + SCREENX) && (msX <= 60 + 72 + SCREENX) && (msY >= 445 + SCREENY) && (msY <= 445 + 15 + SCREENY)) {
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER50);
    }
    else if ((msX >= 145 + SCREENX) && (msX <= 145 + 72 + SCREENX) && (msY >= 445 + SCREENY) && (msY <= 445 + 15 + SCREENY)) {
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER51);
    }
    else if ((msX >= 230 + SCREENX) && (msX <= 230 + 72 + SCREENX) && (msY >= 445 + SCREENY) && (msY <= 445 + 15 + SCREENY)) {
        m_pGame->PutAlignedString(370 + SCREENX, 580 + SCREENX, 345 + SCREENY, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER52);
    }
}
