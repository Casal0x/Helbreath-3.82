// Screen_CreateNewCharacter.cpp: Create New Character Screen Implementation
//
//////////////////////////////////////////////////////////////////////

#include "Screen_CreateNewCharacter.h"
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
#include <format>



using namespace hb::shared::net;
namespace MouseButton = hb::shared::input::MouseButton;

using namespace hb::shared::action;
using namespace hb::client::sprite_id;

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
    m_cNewCharName.clear();
    TextInputManager::Get().StartInput(193 + 4 + OX, 65 + 45 + OY, 11, m_cNewCharName);
    TextInputManager::Get().ClearInput();
}

void Screen_CreateNewCharacter::on_uninitialize()
{
    TextInputManager::Get().EndInput();
}

void Screen_CreateNewCharacter::on_update()
{
    uint32_t dwTime = GameClock::GetTimeMS();

    // Handle arrow key navigation
    if (hb::shared::input::IsKeyPressed(KeyCode::Up)) {
        m_cCurFocus--;
        if (m_cCurFocus <= 0) m_cCurFocus = m_cMaxFocus;
    }
    else if (hb::shared::input::IsKeyPressed(KeyCode::Down)) {
        m_cCurFocus++;
        if (m_cCurFocus > m_cMaxFocus) m_cCurFocus = 1;
    }

    // Handle focus change for input string
    if (m_cNewCharPrevFocus != m_cCurFocus) {
        TextInputManager::Get().EndInput();
        switch (m_cCurFocus) {
        case 1:
            TextInputManager::Get().StartInput(193 + 4 + OX, 65 + 45 + OY, 11, m_cNewCharName);
            break;
        }
        m_cNewCharPrevFocus = m_cCurFocus;
    }

    // ESC returns to character select
    if (hb::shared::input::IsKeyPressed(KeyCode::Escape)) {
        m_pGame->ChangeGameMode(GameMode::SelectCharacter);
        return;
    }

    // Capture mouse position
    short msX = static_cast<short>(hb::shared::input::GetMouseX());
    short msY = static_cast<short>(hb::shared::input::GetMouseY());
    m_sNewCharMsX = msX;
    m_sNewCharMsY = msY;

    // Compute whether character creation is valid
    m_bNewCharFlag = true;
    if (m_cNewCharName.empty()) m_bNewCharFlag = false;
    if (m_iNewCharPoint > 0) m_bNewCharFlag = false;
    if (CMisc::bCheckValidName(m_cNewCharName.data()) == false) m_bNewCharFlag = false;

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

    // Handle button hover focus
    if ((msX >= 384 + OX) && (msX <= 384 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        m_cCurFocus = 2;
    }
    else if ((msX >= 500 + OX) && (msX <= 500 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        m_cCurFocus = 3;
    }
    else if ((msX >= 60 + OX) && (msX <= 60 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        m_cCurFocus = 4;
    }
    else if ((msX >= 145 + OX) && (msX <= 145 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        m_cCurFocus = 5;
    }
    else if ((msX >= 230 + OX) && (msX <= 230 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        m_cCurFocus = 6;
    }

    // Handle mouse clicks
    if (hb::shared::input::IsMouseButtonPressed(MouseButton::Left))
    {
        m_pGame->PlayGameSound('E', 14, 5);

        // Determine which button was clicked
        int iMIbuttonNum = 0;
        if (hb::shared::input::IsMouseInRect(69 + OX, 110 + OY, 210, 17)) iMIbuttonNum = 1;  // Name
        else if (hb::shared::input::IsMouseInRect(236 + OX, 156 + OY, 21, 13)) iMIbuttonNum = 2;  // Gender -
        else if (hb::shared::input::IsMouseInRect(259 + OX, 156 + OY, 21, 13)) iMIbuttonNum = 3;  // Gender +
        else if (hb::shared::input::IsMouseInRect(236 + OX, 171 + OY, 21, 13)) iMIbuttonNum = 4;  // Skin -
        else if (hb::shared::input::IsMouseInRect(259 + OX, 171 + OY, 21, 13)) iMIbuttonNum = 5;  // Skin +
        else if (hb::shared::input::IsMouseInRect(236 + OX, 186 + OY, 21, 13)) iMIbuttonNum = 6;  // Hair style -
        else if (hb::shared::input::IsMouseInRect(259 + OX, 186 + OY, 21, 13)) iMIbuttonNum = 7;  // Hair style +
        else if (hb::shared::input::IsMouseInRect(236 + OX, 201 + OY, 21, 13)) iMIbuttonNum = 8;  // Hair color -
        else if (hb::shared::input::IsMouseInRect(259 + OX, 201 + OY, 21, 13)) iMIbuttonNum = 9;  // Hair color +
        else if (hb::shared::input::IsMouseInRect(236 + OX, 216 + OY, 21, 13)) iMIbuttonNum = 10; // Underwear -
        else if (hb::shared::input::IsMouseInRect(259 + OX, 216 + OY, 21, 13)) iMIbuttonNum = 11; // Underwear +
        else if (hb::shared::input::IsMouseInRect(236 + OX, 276 + OY, 21, 13)) iMIbuttonNum = 12; // Str +
        else if (hb::shared::input::IsMouseInRect(259 + OX, 276 + OY, 21, 13)) iMIbuttonNum = 13; // Str -
        else if (hb::shared::input::IsMouseInRect(236 + OX, 291 + OY, 21, 13)) iMIbuttonNum = 14; // Vit +
        else if (hb::shared::input::IsMouseInRect(259 + OX, 291 + OY, 21, 13)) iMIbuttonNum = 15; // Vit -
        else if (hb::shared::input::IsMouseInRect(236 + OX, 306 + OY, 21, 13)) iMIbuttonNum = 16; // Dex +
        else if (hb::shared::input::IsMouseInRect(259 + OX, 306 + OY, 21, 13)) iMIbuttonNum = 17; // Dex -
        else if (hb::shared::input::IsMouseInRect(236 + OX, 321 + OY, 21, 13)) iMIbuttonNum = 18; // Int +
        else if (hb::shared::input::IsMouseInRect(259 + OX, 321 + OY, 21, 13)) iMIbuttonNum = 19; // Int -
        else if (hb::shared::input::IsMouseInRect(236 + OX, 336 + OY, 21, 13)) iMIbuttonNum = 20; // Mag +
        else if (hb::shared::input::IsMouseInRect(259 + OX, 336 + OY, 21, 13)) iMIbuttonNum = 21; // Mag -
        else if (hb::shared::input::IsMouseInRect(236 + OX, 351 + OY, 21, 13)) iMIbuttonNum = 22; // Chr +
        else if (hb::shared::input::IsMouseInRect(259 + OX, 351 + OY, 21, 13)) iMIbuttonNum = 23; // Chr -
        else if (hb::shared::input::IsMouseInRect(384 + OX, 445 + OY, 72, 15)) iMIbuttonNum = 24; // Create
        else if (hb::shared::input::IsMouseInRect(500 + OX, 445 + OY, 72, 15)) iMIbuttonNum = 25; // Cancel
        else if (hb::shared::input::IsMouseInRect(60 + OX, 445 + OY, 72, 15)) iMIbuttonNum = 26;  // Aresden
        else if (hb::shared::input::IsMouseInRect(145 + OX, 445 + OY, 72, 15)) iMIbuttonNum = 27; // Elvine
        else if (hb::shared::input::IsMouseInRect(230 + OX, 445 + OY, 72, 15)) iMIbuttonNum = 28; // Traveler

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
            if (CMisc::bCheckValidName(m_cNewCharName.data()) == false) break;
            m_pGame->m_pPlayer->m_cPlayerName = m_cNewCharName.c_str();
            m_pGame->m_pLSock = std::make_unique<hb::shared::net::ASIOSocket>(m_pGame->m_pIOPool->GetContext(), game_limits::socket_block_limit);
            m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr.c_str(), m_pGame->m_iLogServerPort);
            m_pGame->m_pLSock->bInitBufferSize(hb::shared::limits::MsgBufferSize);
            m_pGame->ChangeGameMode(GameMode::Connecting);
            m_pGame->m_dwConnectMode = MsgId::RequestCreateNewCharacter;
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
    std::string G_cTxt;
    int i = 0;
    short msX = m_sNewCharMsX;
    short msY = m_sNewCharMsY;
    uint32_t dwTime = GameClock::GetTimeMS();

    // ======== Draw character creation UI (inlined from _bDraw_OnCreateNewCharacter) ========
    m_pGame->DrawNewDialogBox(InterfaceNdNewChar, 0, 0, 0, true);
    m_pGame->DrawNewDialogBox(InterfaceNdButton, OX, OY, 69, true);
    hb::shared::text::DrawTextAligned(GameFont::Default, 64 + OX, 90 + OY, (282) - (64), 15, _BDRAW_ON_CREATE_NEW_CHARACTER1, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::DrawTextAligned(GameFont::Default, 57 + OX, 110 + OY, (191) - (57), 15, DEF_MSG_CHARACTERNAME, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    if (m_cCurFocus != 1) hb::shared::text::DrawText(GameFont::Default, 197 + OX, 112 + OY, m_cNewCharName.c_str(), hb::shared::text::TextStyle::Color(GameColors::UILabel));
    hb::shared::text::DrawTextAligned(GameFont::Default, 64 + OX, 140 + OY, (282) - (64), 15, _BDRAW_ON_CREATE_NEW_CHARACTER2, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::DrawText(GameFont::Default, 100 + OX, 160 + OY, DEF_MSG_GENDER, hb::shared::text::TextStyle::Color(GameColors::UIBlack));
    hb::shared::text::DrawText(GameFont::Default, 100 + OX, 175 + OY, DEF_MSG_SKINCOLOR, hb::shared::text::TextStyle::Color(GameColors::UIBlack));
    hb::shared::text::DrawText(GameFont::Default, 100 + OX, 190 + OY, DEF_MSG_HAIRSTYLE, hb::shared::text::TextStyle::Color(GameColors::UIBlack));
    hb::shared::text::DrawText(GameFont::Default, 100 + OX, 205 + OY, DEF_MSG_HAIRCOLOR, hb::shared::text::TextStyle::Color(GameColors::UIBlack));
    hb::shared::text::DrawText(GameFont::Default, 100 + OX, 220 + OY, DEF_MSG_UNDERWEARCOLOR, hb::shared::text::TextStyle::Color(GameColors::UIBlack));
    hb::shared::text::DrawText(GameFont::Default, 100 + OX, 275 + OY, DEF_MSG_STRENGTH, hb::shared::text::TextStyle::Color(GameColors::UIBlack));
    hb::shared::text::DrawText(GameFont::Default, 100 + OX, 292 + OY, DEF_MSG_VITALITY, hb::shared::text::TextStyle::Color(GameColors::UIBlack));
    hb::shared::text::DrawText(GameFont::Default, 100 + OX, 309 + OY, DEF_MSG_DEXTERITY, hb::shared::text::TextStyle::Color(GameColors::UIBlack));
    hb::shared::text::DrawText(GameFont::Default, 100 + OX, 326 + OY, DEF_MSG_INTELLIGENCE, hb::shared::text::TextStyle::Color(GameColors::UIBlack));
    hb::shared::text::DrawText(GameFont::Default, 100 + OX, 343 + OY, DEF_MSG_MAGIC, hb::shared::text::TextStyle::Color(GameColors::UIBlack));
    hb::shared::text::DrawText(GameFont::Default, 100 + OX, 360 + OY, DEF_MSG_CHARISMA, hb::shared::text::TextStyle::Color(GameColors::UIBlack));

    // Stat values
    i = 0;
    G_cTxt = std::format("{}", m_pGame->m_pPlayer->m_iStatModStr);
    hb::shared::text::DrawText(GameFont::Default, 204 + OX, 277 + OY + 16 * i++, G_cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UILabel));
    G_cTxt = std::format("{}", m_pGame->m_pPlayer->m_iStatModVit);
    hb::shared::text::DrawText(GameFont::Default, 204 + OX, 277 + OY + 16 * i++, G_cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UILabel));
    G_cTxt = std::format("{}", m_pGame->m_pPlayer->m_iStatModDex);
    hb::shared::text::DrawText(GameFont::Default, 204 + OX, 277 + OY + 16 * i++, G_cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UILabel));
    G_cTxt = std::format("{}", m_pGame->m_pPlayer->m_iStatModInt);
    hb::shared::text::DrawText(GameFont::Default, 204 + OX, 277 + OY + 16 * i++, G_cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UILabel));
    G_cTxt = std::format("{}", m_pGame->m_pPlayer->m_iStatModMag);
    hb::shared::text::DrawText(GameFont::Default, 204 + OX, 277 + OY + 16 * i++, G_cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UILabel));
    G_cTxt = std::format("{}", m_pGame->m_pPlayer->m_iStatModChr);
    hb::shared::text::DrawText(GameFont::Default, 204 + OX, 277 + OY + 16 * i++, G_cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UILabel));

    // Button states
    if ((m_bNewCharFlag == true) && (m_cCurFocus == 2))
        m_pGame->m_pSprite[InterfaceNdButton]->Draw(384 + OX, 445 + OY, 25);
    else
        m_pGame->m_pSprite[InterfaceNdButton]->Draw(384 + OX, 445 + OY, 24);
    if (m_cCurFocus == 3)
        m_pGame->m_pSprite[InterfaceNdButton]->Draw(500 + OX, 445 + OY, 17);
    else
        m_pGame->m_pSprite[InterfaceNdButton]->Draw(500 + OX, 445 + OY, 16);
    if (m_cCurFocus == 4)
        m_pGame->m_pSprite[InterfaceNdButton]->Draw(60 + OX, 445 + OY, 68);
    else
        m_pGame->m_pSprite[InterfaceNdButton]->Draw(60 + OX, 445 + OY, 67);
    if (m_cCurFocus == 5)
        m_pGame->m_pSprite[InterfaceNdButton]->Draw(145 + OX, 445 + OY, 66);
    else
        m_pGame->m_pSprite[InterfaceNdButton]->Draw(145 + OX, 445 + OY, 65);
    if (m_cCurFocus == 6)
        m_pGame->m_pSprite[InterfaceNdButton]->Draw(230 + OX, 445 + OY, 64);
    else
        m_pGame->m_pSprite[InterfaceNdButton]->Draw(230 + OX, 445 + OY, 63);

    TextInputManager::Get().ShowInput();

    // Character preview
    switch (m_pGame->m_pPlayer->m_iGender) {
    case 1: m_pGame->m_entityState.m_sOwnerType = hb::shared::owner::MaleFirst; break;
    case 2: m_pGame->m_entityState.m_sOwnerType = hb::shared::owner::FemaleFirst; break;
    }
    m_pGame->m_entityState.m_sOwnerType += m_pGame->m_pPlayer->m_iSkinCol - 1;
    m_pGame->m_entityState.m_iDir = m_pGame->m_cMenuDir;
    m_pGame->m_entityState.m_appearance.Clear();
    m_pGame->m_entityState.m_appearance.iUnderwearType = m_pGame->m_pPlayer->m_iUnderCol;
    m_pGame->m_entityState.m_appearance.iHairStyle = m_pGame->m_pPlayer->m_iHairStyle;
    m_pGame->m_entityState.m_appearance.iHairColor = m_pGame->m_pPlayer->m_iHairCol;
    m_pGame->m_entityState.m_cName.fill('\0');
    std::snprintf(m_pGame->m_entityState.m_cName.data(), m_pGame->m_entityState.m_cName.size(), "%s", m_pGame->m_pPlayer->m_cPlayerName.c_str());
    m_pGame->m_entityState.m_iAction = Type::Move;
    m_pGame->m_entityState.m_iFrame = m_pGame->m_cMenuFrame;

    m_pGame->_Draw_CharacterBody(507 + OX, 267 + OY, m_pGame->m_entityState.m_sOwnerType);
    m_pGame->DrawObject_OnMove_ForMenu(0, 0, 500 + OX, 174 + OY, false, dwTime);

    // Derived stats
    i = 0;
    hb::shared::text::DrawText(GameFont::Default, 445 + OX, 192 + OY, DEF_MSG_HITPOINT, hb::shared::text::TextStyle::Color(GameColors::UIBlack));
    G_cTxt = std::format("{}", m_pGame->m_pPlayer->m_iStatModVit * 3 + 2 + m_pGame->m_pPlayer->m_iStatModStr / 2);
    hb::shared::text::DrawText(GameFont::Default, 550 + OX, 192 + OY + 16 * i++, G_cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UILabel));
    hb::shared::text::DrawText(GameFont::Default, 445 + OX, 208 + OY, DEF_MSG_MANAPOINT, hb::shared::text::TextStyle::Color(GameColors::UIBlack));
    G_cTxt = std::format("{}", m_pGame->m_pPlayer->m_iStatModMag * 2 + 2 + m_pGame->m_pPlayer->m_iStatModInt / 2);
    hb::shared::text::DrawText(GameFont::Default, 550 + OX, 192 + OY + 16 * i++, G_cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UILabel));
    hb::shared::text::DrawText(GameFont::Default, 445 + OX, 224 + OY, DEF_MSG_STAMINARPOINT, hb::shared::text::TextStyle::Color(GameColors::UIBlack));
    G_cTxt = std::format("{}", m_pGame->m_pPlayer->m_iStatModStr * 2 + 2);
    hb::shared::text::DrawText(GameFont::Default, 550 + OX, 192 + OY + 16 * i++, G_cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UILabel));

    // ======== End inlined drawing ========

    m_pGame->DrawVersion();

    // Tooltip drawing based on mouse position
    if ((msX >= 65 + 4 - 127 + OX) && (msX <= 275 + 4 + OX) && (msY >= 65 + 45 + OY) && (msY <= 82 + 45 + OY)) {
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER1, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((msX >= 261 + 4 - 212 + OX) && (msX <= 289 + 4 + OX) && (msY >= 111 + 45 + OY) && (msY <= 124 + 45 + OY)) {
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER2, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((msX >= 261 + 4 - 212 + OX) && (msX <= 289 + 4 + OX) && (msY >= 126 + 45 + OY) && (msY <= 139 + 45 + OY)) {
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER3, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((msX >= 261 + 4 - 212 + OX) && (msX <= 289 + 4 + OX) && (msY >= 141 + 45 + OY) && (msY <= 154 + 45 + OY)) {
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER4, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((msX >= 261 + 4 - 212 + OX) && (msX <= 289 + 4 + OX) && (msY >= 156 + 45 + OY) && (msY <= 169 + 45 + OY)) {
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER5, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((msX >= 261 + 4 - 212 + OX) && (msX <= 289 + 4 + OX) && (msY >= 171 + 45 + OY) && (msY <= 184 + 45 + OY)) {
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER6, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((msX >= 240 + 4 - 175 + OX) && (msX <= 268 + 4 + OX) && (msY >= 231 + 45 + OY) && (msY <= 244 + 45 + OY)) {
        // Str tooltip
        i = 0;
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER7, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER8, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER9, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER10, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER11, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((msX >= 240 + 4 - 175 + OX) && (msX <= 268 + 4 + OX) && (msY >= 246 + 45 + OY) && (msY <= 259 + 45 + OY)) {
        // Vit tooltip
        i = 0;
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER12, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER13, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER14, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER15, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER16, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((msX >= 240 + 4 - 175 + OX) && (msX <= 268 + 4 + OX) && (msY >= 261 + 45 + OY) && (msY <= 274 + 45 + OY)) {
        // Dex tooltip
        i = 0;
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER17, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER18, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER19, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER20, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((msX >= 240 + 4 - 175 + OX) && (msX <= 268 + 4 + OX) && (msY >= 276 + 45 + OY) && (msY <= 289 + 45 + OY)) {
        // Int tooltip
        i = 0;
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER21, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER22, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER23, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER24, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((msX >= 240 + 4 - 175 + OX) && (msX <= 268 + 4 + OX) && (msY >= 291 + 45 + OY) && (msY <= 304 + 45 + OY)) {
        // Mag tooltip
        i = 0;
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER25, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER26, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER27, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER28, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((msX >= 240 + 4 - 175 + OX) && (msX <= 268 + 4 + OX) && (msY >= 306 + 45 + OY) && (msY <= 319 + 45 + OY)) {
        // Charisma tooltip
        i = 0;
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER29, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER30, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER31, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER32, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((msX >= 384 + OX) && (msX <= 384 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        if (m_cNewCharName.empty()) {
            i = 0;
            hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER35, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        }
        else if (m_iNewCharPoint > 0) {
            i = 0;
            hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER36, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        }
        else if (CMisc::bCheckValidName(m_cNewCharName.data()) == false) {
            i = 0;
            hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER39, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER40, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER41, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        }
        else {
            i = 0;
            hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER44, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER45, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER46, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER47, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY + 16 * i++, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER48, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        }
    }
    else if ((msX >= 500 + OX) && (msX <= 500 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER49, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((msX >= 60 + OX) && (msX <= 60 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER50, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((msX >= 145 + OX) && (msX <= 145 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER51, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else if ((msX >= 230 + OX) && (msX <= 230 + 72 + OX) && (msY >= 445 + OY) && (msY <= 445 + 15 + OY)) {
        hb::shared::text::DrawTextAligned(GameFont::Default, 370 + OX, 345 + OY, (580) - (370), 15, UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER52, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
}
