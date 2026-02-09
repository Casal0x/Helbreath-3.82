// Screen_CreateNewCharacter.cpp: Create New Character Screen Implementation
//
//////////////////////////////////////////////////////////////////////

#include "Screen_CreateNewCharacter.h"
#include "Game.h"
#include "GameModeManager.h"
#include "IInput.h"
#include "GlobalDef.h"
#include "ASIOSocket.h"
#include "Misc.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"

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
    m_pGame->StartInputString(193 + 4 + OX, 65 + 45 + OY, 11, m_cNewCharName);
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
    if (Input::IsKeyPressed(KeyCode::Up)) {
        m_cCurFocus--;
        if (m_cCurFocus <= 0) m_cCurFocus = m_cMaxFocus;
    }
    else if (Input::IsKeyPressed(KeyCode::Down)) {
        m_cCurFocus++;
        if (m_cCurFocus > m_cMaxFocus) m_cCurFocus = 1;
    }

    // Handle focus change for input string
    if (m_cNewCharPrevFocus != m_cCurFocus) {
        m_pGame->EndInputString();
        switch (m_cCurFocus) {
        case 1:
            m_pGame->StartInputString(193 + 4 + OX, 65 + 45 + OY, 11, m_cNewCharName);
            break;
        }
        m_cNewCharPrevFocus = m_cCurFocus;
    }

    // ESC returns to character select
    if (Input::IsKeyPressed(KeyCode::Escape)) {
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
        m_pGame->PlayGameSound('E', 14, 5);

        // Determine which button was clicked
        int iMIbuttonNum = 0;
        if (Input::IsMouseInRect(69 + OX, 110 + OY, 210, 17)) iMIbuttonNum = 1;  // Name
        else if (Input::IsMouseInRect(236 + OX, 156 + OY, 21, 13)) iMIbuttonNum = 2;  // Gender -
        else if (Input::IsMouseInRect(259 + OX, 156 + OY, 21, 13)) iMIbuttonNum = 3;  // Gender +
        else if (Input::IsMouseInRect(236 + OX, 171 + OY, 21, 13)) iMIbuttonNum = 4;  // Skin -
        else if (Input::IsMouseInRect(259 + OX, 171 + OY, 21, 13)) iMIbuttonNum = 5;  // Skin +
        else if (Input::IsMouseInRect(236 + OX, 186 + OY, 21, 13)) iMIbuttonNum = 6;  // Hair style -
        else if (Input::IsMouseInRect(259 + OX, 186 + OY, 21, 13)) iMIbuttonNum = 7;  // Hair style +
        else if (Input::IsMouseInRect(236 + OX, 201 + OY, 21, 13)) iMIbuttonNum = 8;  // Hair color -
        else if (Input::IsMouseInRect(259 + OX, 201 + OY, 21, 13)) iMIbuttonNum = 9;  // Hair color +
        else if (Input::IsMouseInRect(236 + OX, 216 + OY, 21, 13)) iMIbuttonNum = 10; // Underwear -
        else if (Input::IsMouseInRect(259 + OX, 216 + OY, 21, 13)) iMIbuttonNum = 11; // Underwear +
        else if (Input::IsMouseInRect(236 + OX, 276 + OY, 21, 13)) iMIbuttonNum = 12; // Str +
        else if (Input::IsMouseInRect(259 + OX, 276 + OY, 21, 13)) iMIbuttonNum = 13; // Str -
        else if (Input::IsMouseInRect(236 + OX, 291 + OY, 21, 13)) iMIbuttonNum = 14; // Vit +
        else if (Input::IsMouseInRect(259 + OX, 291 + OY, 21, 13)) iMIbuttonNum = 15; // Vit -
        else if (Input::IsMouseInRect(236 + OX, 306 + OY, 21, 13)) iMIbuttonNum = 16; // Dex +
        else if (Input::IsMouseInRect(259 + OX, 306 + OY, 21, 13)) iMIbuttonNum = 17; // Dex -
        else if (Input::IsMouseInRect(236 + OX, 321 + OY, 21, 13)) iMIbuttonNum = 18; // Int +
        else if (Input::IsMouseInRect(259 + OX, 321 + OY, 21, 13)) iMIbuttonNum = 19; // Int -
        else if (Input::IsMouseInRect(236 + OX, 336 + OY, 21, 13)) iMIbuttonNum = 20; // Mag +
        else if (Input::IsMouseInRect(259 + OX, 336 + OY, 21, 13)) iMIbuttonNum = 21; // Mag -
        else if (Input::IsMouseInRect(236 + OX, 351 + OY, 21, 13)) iMIbuttonNum = 22; // Chr +
        else if (Input::IsMouseInRect(259 + OX, 351 + OY, 21, 13)) iMIbuttonNum = 23; // Chr -
        else if (Input::IsMouseInRect(384 + OX, 445 + OY, 72, 15)) iMIbuttonNum = 24; // Create
        else if (Input::IsMouseInRect(500 + OX, 445 + OY, 72, 15)) iMIbuttonNum = 25; // Cancel
        else if (Input::IsMouseInRect(60 + OX, 445 + OY, 72, 15)) iMIbuttonNum = 26;  // Aresden
        else if (Input::IsMouseInRect(145 + OX, 445 + OY, 72, 15)) iMIbuttonNum = 27; // Elvine
        else if (Input::IsMouseInRect(230 + OX, 445 + OY, 72, 15)) iMIbuttonNum = 28; // Traveler

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
            std::snprintf(m_pGame->m_pPlayer->m_cPlayerName, sizeof(m_pGame->m_pPlayer->m_cPlayerName), "%s", m_cNewCharName);
            m_pGame->m_pLSock = std::make_unique<ASIOSocket>(m_pGame->m_pIOPool->GetContext(), game_limits::socket_block_limit);
            m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort + (rand() % 1));
            m_pGame->m_pLSock->bInitBufferSize(DEF_MSGBUFFERSIZE);
            m_pGame->ChangeGameMode(GameMode::Connecting);
            m_pGame->m_dwConnectMode = MSGID_REQUEST_CREATENEWCHARACTER;
            std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
            std::snprintf(m_pGame->m_cMsg, sizeof(m_pGame->m_cMsg), "%s", "22");
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
    if ((msX >= 384 + OX) && (msX <= 384 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        m_cCurFocus = 2;
    }
    else if ((msX >= 500 + OX) && (msX <= 500 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        m_cCurFocus = 3;
    }
    if ((msX >= 60 + OX) && (msX <= 60 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        m_cCurFocus = 4;
    }
    if ((msX >= 145 + OX) && (msX <= 145 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        m_cCurFocus = 5;
    }
    if ((msX >= 230 + OX) && (msX <= 230 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        m_cCurFocus = 6;
    }

    // ======== Draw character creation UI (inlined from _bDraw_OnCreateNewCharacter) ========
    m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_NEWCHAR, 0, 0, 0, true);
    m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, OX, OY, 69, true);
    TextLib::DrawTextAligned(GameFont::Default, 64 + OX, 90 + OY, (282) - (64), 15, _BDRAW_ON_CREATE_NEW_CHARACTER1, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    TextLib::DrawTextAligned(GameFont::Default, 57 + OX, 110 + OY, (191) - (57), 15, DEF_MSG_CHARACTERNAME, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    if (m_cCurFocus != 1) TextLib::DrawText(GameFont::Default, 197 + OX, 112 + OY, m_cNewCharName, TextLib::TextStyle::Color(GameColors::UILabel));
    TextLib::DrawTextAligned(GameFont::Default, 64 + OX, 140 + OY, (282) - (64), 15, _BDRAW_ON_CREATE_NEW_CHARACTER2, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    TextLib::DrawText(GameFont::Default, 100 + OX, 160 + OY, DEF_MSG_GENDER, TextLib::TextStyle::Color(GameColors::UIBlack));
    TextLib::DrawText(GameFont::Default, 100 + OX, 175 + OY, DEF_MSG_SKINCOLOR, TextLib::TextStyle::Color(GameColors::UIBlack));
    TextLib::DrawText(GameFont::Default, 100 + OX, 190 + OY, DEF_MSG_HAIRSTYLE, TextLib::TextStyle::Color(GameColors::UIBlack));
    TextLib::DrawText(GameFont::Default, 100 + OX, 205 + OY, DEF_MSG_HAIRCOLOR, TextLib::TextStyle::Color(GameColors::UIBlack));
    TextLib::DrawText(GameFont::Default, 100 + OX, 220 + OY, DEF_MSG_UNDERWEARCOLOR, TextLib::TextStyle::Color(GameColors::UIBlack));
    TextLib::DrawText(GameFont::Default, 100 + OX, 275 + OY, DEF_MSG_STRENGTH, TextLib::TextStyle::Color(GameColors::UIBlack));
    TextLib::DrawText(GameFont::Default, 100 + OX, 292 + OY, DEF_MSG_VITALITY, TextLib::TextStyle::Color(GameColors::UIBlack));
    TextLib::DrawText(GameFont::Default, 100 + OX, 309 + OY, DEF_MSG_DEXTERITY, TextLib::TextStyle::Color(GameColors::UIBlack));
    TextLib::DrawText(GameFont::Default, 100 + OX, 326 + OY, DEF_MSG_INTELLIGENCE, TextLib::TextStyle::Color(GameColors::UIBlack));
    TextLib::DrawText(GameFont::Default, 100 + OX, 343 + OY, DEF_MSG_MAGIC, TextLib::TextStyle::Color(GameColors::UIBlack));
    TextLib::DrawText(GameFont::Default, 100 + OX, 360 + OY, DEF_MSG_CHARISMA, TextLib::TextStyle::Color(GameColors::UIBlack));

    // Stat values
    i = 0;
    std::snprintf(m_pGame->G_cTxt, sizeof(m_pGame->G_cTxt), "%d", m_pGame->m_pPlayer->m_iStatModStr);
    TextLib::DrawText(GameFont::Default, 204 + OX, 277 + OY + 16 * i++, m_pGame->G_cTxt, TextLib::TextStyle::Color(GameColors::UILabel));
    std::snprintf(m_pGame->G_cTxt, sizeof(m_pGame->G_cTxt), "%d", m_pGame->m_pPlayer->m_iStatModVit);
    TextLib::DrawText(GameFont::Default, 204 + OX, 277 + OY + 16 * i++, m_pGame->G_cTxt, TextLib::TextStyle::Color(GameColors::UILabel));
    std::snprintf(m_pGame->G_cTxt, sizeof(m_pGame->G_cTxt), "%d", m_pGame->m_pPlayer->m_iStatModDex);
    TextLib::DrawText(GameFont::Default, 204 + OX, 277 + OY + 16 * i++, m_pGame->G_cTxt, TextLib::TextStyle::Color(GameColors::UILabel));
    std::snprintf(m_pGame->G_cTxt, sizeof(m_pGame->G_cTxt), "%d", m_pGame->m_pPlayer->m_iStatModInt);
    TextLib::DrawText(GameFont::Default, 204 + OX, 277 + OY + 16 * i++, m_pGame->G_cTxt, TextLib::TextStyle::Color(GameColors::UILabel));
    std::snprintf(m_pGame->G_cTxt, sizeof(m_pGame->G_cTxt), "%d", m_pGame->m_pPlayer->m_iStatModMag);
    TextLib::DrawText(GameFont::Default, 204 + OX, 277 + OY + 16 * i++, m_pGame->G_cTxt, TextLib::TextStyle::Color(GameColors::UILabel));
    std::snprintf(m_pGame->G_cTxt, sizeof(m_pGame->G_cTxt), "%d", m_pGame->m_pPlayer->m_iStatModChr);
    TextLib::DrawText(GameFont::Default, 204 + OX, 277 + OY + 16 * i++, m_pGame->G_cTxt, TextLib::TextStyle::Color(GameColors::UILabel));

    // Button states
    if ((m_bNewCharFlag == true) && (m_cCurFocus == 2))
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(384 + OX, 445 + OY, 25);
    else
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(384 + OX, 445 + OY, 24);
    if (m_cCurFocus == 3)
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(500 + OX, 445 + OY, 17);
    else
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(500 + OX, 445 + OY, 16);
    if (m_cCurFocus == 4)
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(60 + OX, 445 + OY, 68);
    else
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(60 + OX, 445 + OY, 67);
    if (m_cCurFocus == 5)
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(145 + OX, 445 + OY, 66);
    else
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(145 + OX, 445 + OY, 65);
    if (m_cCurFocus == 6)
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(230 + OX, 445 + OY, 64);
    else
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(230 + OX, 445 + OY, 63);

    m_pGame->ShowReceivedString();

    // Character preview
    switch (m_pGame->m_pPlayer->m_iGender) {
    case 1: m_pGame->m_entityState.m_sOwnerType = hb::owner::MaleFirst; break;
    case 2: m_pGame->m_entityState.m_sOwnerType = hb::owner::FemaleFirst; break;
    }
    m_pGame->m_entityState.m_sOwnerType += m_pGame->m_pPlayer->m_iSkinCol - 1;
    m_pGame->m_entityState.m_iDir = m_pGame->m_cMenuDir;
    m_pGame->m_entityState.m_appearance.Clear();
    m_pGame->m_entityState.m_appearance.iUnderwearType = m_pGame->m_pPlayer->m_iUnderCol;
    m_pGame->m_entityState.m_appearance.iHairStyle = m_pGame->m_pPlayer->m_iHairStyle;
    m_pGame->m_entityState.m_appearance.iHairColor = m_pGame->m_pPlayer->m_iHairCol;
    std::memset(m_pGame->m_entityState.m_cName.data(), 0, m_pGame->m_entityState.m_cName.size());
    memcpy(m_pGame->m_entityState.m_cName.data(), m_pGame->m_pPlayer->m_cPlayerName, 10);
    m_pGame->m_entityState.m_iAction = DEF_OBJECTMOVE;
    m_pGame->m_entityState.m_iFrame = m_pGame->m_cMenuFrame;

    m_pGame->_Draw_CharacterBody(507 + OX, 267 + OY, m_pGame->m_entityState.m_sOwnerType);
    m_pGame->DrawObject_OnMove_ForMenu(0, 0, 500 + OX, 174 + OY, false, dwTime);

    // Derived stats
    i = 0;
    TextLib::DrawText(GameFont::Default, 445 + OX, 192 + OY, DEF_MSG_HITPOINT, TextLib::TextStyle::Color(GameColors::UIBlack));
    std::snprintf(m_pGame->G_cTxt, sizeof(m_pGame->G_cTxt), "%d", m_pGame->m_pPlayer->m_iStatModVit * 3 + 2 + m_pGame->m_pPlayer->m_iStatModStr / 2);
    TextLib::DrawText(GameFont::Default, 550 + OX, 192 + OY + 16 * i++, m_pGame->G_cTxt, TextLib::TextStyle::Color(GameColors::UILabel));
    TextLib::DrawText(GameFont::Default, 445 + OX, 208 + OY, DEF_MSG_MANAPOINT, TextLib::TextStyle::Color(GameColors::UIBlack));
    std::snprintf(m_pGame->G_cTxt, sizeof(m_pGame->G_cTxt), "%d", m_pGame->m_pPlayer->m_iStatModMag * 2 + 2 + m_pGame->m_pPlayer->m_iStatModInt / 2);
    TextLib::DrawText(GameFont::Default, 550 + OX, 192 + OY + 16 * i++, m_pGame->G_cTxt, TextLib::TextStyle::Color(GameColors::UILabel));
    TextLib::DrawText(GameFont::Default, 445 + OX, 224 + OY, DEF_MSG_STAMINARPOINT, TextLib::TextStyle::Color(GameColors::UIBlack));
    std::snprintf(m_pGame->G_cTxt, sizeof(m_pGame->G_cTxt), "%d", m_pGame->m_pPlayer->m_iStatModStr * 2 + 2);
    TextLib::DrawText(GameFont::Default, 550 + OX, 192 + OY + 16 * i++, m_pGame->G_cTxt, TextLib::TextStyle::Color(GameColors::UILabel));

    // ======== End inlined drawing ========

    m_pGame->DrawVersion();

    // Tooltip drawing based on mouse position
    if ((msX >= 65 + 4 - 127 + OX) && (msX <= 275 + 4 + OX) && (msY >= 65 + 45 + OY) && (msY <= 82 + 45 + OY)) {
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER1, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX >= 261 + 4 - 212 + OX) && (msX <= 289 + 4 + OX) && (msY >= 111 + 45 + OY) && (msY <= 124 + 45 + OY)) {
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER2, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX >= 261 + 4 - 212 + OX) && (msX <= 289 + 4 + OX) && (msY >= 126 + 45 + OY) && (msY <= 139 + 45 + OY)) {
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER3, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX >= 261 + 4 - 212 + OX) && (msX <= 289 + 4 + OX) && (msY >= 141 + 45 + OY) && (msY <= 154 + 45 + OY)) {
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER4, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX >= 261 + 4 - 212 + OX) && (msX <= 289 + 4 + OX) && (msY >= 156 + 45 + OY) && (msY <= 169 + 45 + OY)) {
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER5, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX >= 261 + 4 - 212 + OX) && (msX <= 289 + 4 + OX) && (msY >= 171 + 45 + OY) && (msY <= 184 + 45 + OY)) {
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER6, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX >= 240 + 4 - 175 + OX) && (msX <= 268 + 4 + OX) && (msY >= 231 + 45 + OY) && (msY <= 244 + 45 + OY)) {
        // Str tooltip
        i = 0;
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER7, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER8, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER9, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER10, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER11, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX >= 240 + 4 - 175 + OX) && (msX <= 268 + 4 + OX) && (msY >= 246 + 45 + OY) && (msY <= 259 + 45 + OY)) {
        // Vit tooltip
        i = 0;
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER12, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER13, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER14, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER15, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER16, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX >= 240 + 4 - 175 + OX) && (msX <= 268 + 4 + OX) && (msY >= 261 + 45 + OY) && (msY <= 274 + 45 + OY)) {
        // Dex tooltip
        i = 0;
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER17, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER18, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER19, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER20, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX >= 240 + 4 - 175 + OX) && (msX <= 268 + 4 + OX) && (msY >= 276 + 45 + OY) && (msY <= 289 + 45 + OY)) {
        // Int tooltip
        i = 0;
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER21, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER22, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER23, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER24, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX >= 240 + 4 - 175 + OX) && (msX <= 268 + 4 + OX) && (msY >= 291 + 45 + OY) && (msY <= 304 + 45 + OY)) {
        // Mag tooltip
        i = 0;
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER25, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER26, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER27, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER28, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX >= 240 + 4 - 175 + OX) && (msX <= 268 + 4 + OX) && (msY >= 306 + 45 + OY) && (msY <= 319 + 45 + OY)) {
        // Charisma tooltip
        i = 0;
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER29, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER30, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER31, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER32, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX >= 384 + OX) && (msX <= 384 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        if (strlen(m_cNewCharName) <= 0) {
            i = 0;
            TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER35, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        }
        else if (m_iNewCharPoint > 0) {
            i = 0;
            TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER36, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        }
        else if (CMisc::bCheckValidName(m_cNewCharName) == false) {
            i = 0;
            TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER39, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER40, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER41, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        }
        else {
            i = 0;
            TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER44, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER45, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER46, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER47, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER48, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        }
    }
    else if ((msX >= 500 + OX) && (msX <= 500 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER49, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX >= 60 + OX) && (msX <= 60 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER50, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX >= 145 + OX) && (msX <= 145 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER51, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else if ((msX >= 230 + OX) && (msX <= 230 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        TextLib::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER52, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
}
