// Screen_OnGame.cpp: Main gameplay screen implementation
//
//////////////////////////////////////////////////////////////////////

#include "Screen_OnGame.h"
#include "Game.h"
#include "GameModeManager.h"
#include "CommonTypes.h"
#include "FrameTiming.h"
#include "AudioManager.h"
#include "WeatherManager.h"
#include "ConfigManager.h"
#include "IInput.h"
#include "GlobalDef.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include <string>
#include <memory>

using namespace hb::item;

extern char G_cSpriteAlphaDegree;

Screen_OnGame::Screen_OnGame(CGame* pGame)
    : IGameScreen(pGame)
{
}

void Screen_OnGame::on_initialize()
{
    // Set current mode for code that checks GameModeManager::GetMode()
    GameModeManager::SetCurrentMode(GameMode::MainGame);

    m_dwTime = GameClock::GetTimeMS();
    m_pGame->m_dwFPStime = m_dwTime;
    m_pGame->m_dwCheckConnTime = m_dwTime;
    m_pGame->m_dwCheckSprTime = m_dwTime;
    m_pGame->m_dwCheckChatTime = m_dwTime;
    m_pGame->m_sFrameCount = 0;
    m_dwPrevChatTime = 0;

    if (AudioManager::Get().IsMusicEnabled())
        m_pGame->StartBGM();
}

void Screen_OnGame::on_uninitialize()
{
    // Nothing specific to clean up
}

void Screen_OnGame::on_update()
{
    short sVal, absX, absY, tX, tY;
    int i, iAmount;

    m_dwTime = GameClock::GetTimeMS();

    m_sMsX = static_cast<short>(Input::GetMouseX());
    m_sMsY = static_cast<short>(Input::GetMouseY());
    m_sMsZ = static_cast<short>(Input::GetMouseWheelDelta());
    m_cLB = Input::IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? 1 : 0;
    m_cRB = Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ? 1 : 0;
    m_pGame->m_dwCurTime = GameClock::GetTimeMS();

    // Sync manager singletons with game state
    AudioManager::Get().SetListenerPosition(m_pGame->m_pPlayer->m_sPlayerX, m_pGame->m_pPlayer->m_sPlayerY);

    WeatherType currentWeather = WeatherManager::FromLegacyWeather(m_pGame->m_cWhetherStatus);
    if (WeatherManager::Get().GetCurrentWeather() != currentWeather)
    {
        WeatherManager::Get().SetWeatherImmediate(currentWeather);
    }

    // Enter key handling
    if (Input::IsKeyPressed(VK_RETURN) == true)
    {
        if ((m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::GuildMenu) == true) && (m_pGame->m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode == 1) && (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::GuildMenu)) {
            m_pGame->EndInputString();
            if (strlen(m_pGame->m_pPlayer->m_cGuildName) == 0) return;
            if (strcmp(m_pGame->m_pPlayer->m_cGuildName, "NONE") != 0) {
                m_pGame->bSendCommand(MSGID_REQUEST_CREATENEWGUILD, DEF_MSGTYPE_CONFIRM, 0, 0, 0, 0, 0);
                m_pGame->m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 2;
            }
        }
        else if ((m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropExternal) == true) && (m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cMode == 1) && (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::ItemDropExternal)) {
            m_pGame->EndInputString();

            if (m_pGame->m_bSkillUsingStatus == true) {
                m_pGame->AddEventList(UPDATE_SCREEN_ONGAME1, 10);
                return;
            }

            if ((m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::NpcActionQuery) == true) && ((m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cMode == 1) || (m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cMode == 2))) {
                m_pGame->AddEventList(UPDATE_SCREEN_ONGAME1, 10);
                return;
            }

            if ((m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropConfirm) == true) || (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::SellOrRepair) == true) || (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::Manufacture) == true)) {
                m_pGame->AddEventList(UPDATE_SCREEN_ONGAME1, 10);
                return;
            }

            if (strlen(m_pGame->m_cAmountString) == 0) return;
            iAmount = atoi(m_pGame->m_cAmountString);

            if ((int)(m_pGame->m_pItemList[m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView]->m_dwCount) < iAmount) {
                iAmount = m_pGame->m_pItemList[m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView]->m_dwCount;
            }

            if (iAmount != 0) {
                if ((int)(m_pGame->m_pItemList[m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView]->m_dwCount) >= iAmount) {
                    if (m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV1 != 0) {
                        absX = abs(m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV1 - m_pGame->m_pPlayer->m_sPlayerX);
                        absY = abs(m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV2 - m_pGame->m_pPlayer->m_sPlayerY);

                        if ((absX == 0) && (absY == 0))
                            m_pGame->AddEventList(UPDATE_SCREEN_ONGAME5, 10);
                        else if ((absX <= 8) && (absY <= 8)) {
                            switch (m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV3) {
                            case 1: case 2: case 3: case 4: case 5: case 6:
                                m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcActionQuery, 1, m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView, m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV3);
                                m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV3 = iAmount;
                                m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV4 = m_pGame->m_wCommObjectID;
                                m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV5 = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV1;
                                m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV6 = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV2;
                                tX = m_sMsX - 117; tY = m_sMsY - 50;
                                if (tX < 0) tX = 0;
                                if ((tX + 235) > LOGICAL_MAX_X()) tX = LOGICAL_MAX_X() - 235;
                                if (tY < 0) tY = 0;
                                if ((tY + 100) > LOGICAL_MAX_Y()) tY = LOGICAL_MAX_Y() - 100;
                                m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sX = tX; m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sY = tY;
                                std::memset(m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr, 0, sizeof(m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr));
                                strcpy(m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr, m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cStr);
                                break;
                            case 20:
                                m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcActionQuery, 3, m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView, m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV3);
                                m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV3 = iAmount;
                                m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV4 = m_pGame->m_wCommObjectID;
                                m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV5 = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV1;
                                m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV6 = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV2;
                                tX = m_sMsX - 117; tY = m_sMsY - 50;
                                if (tX < 0) tX = 0;
                                if ((tX + 235) > LOGICAL_MAX_X()) tX = LOGICAL_MAX_X() - 235;
                                if (tY < 0) tY = 0;
                                if ((tY + 100) > LOGICAL_MAX_Y()) tY = LOGICAL_MAX_Y() - 100;
                                m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sX = tX; m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sY = tY;
                                std::memset(m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr, 0, sizeof(m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr));
                                m_pGame->GetNpcName(m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV3, m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr);
                                break;
                            case 15: case 24:
                                m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcActionQuery, 2, m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView, m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV3);
                                m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV3 = iAmount;
                                m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV4 = m_pGame->m_wCommObjectID;
                                tX = m_sMsX - 117; tY = m_sMsY - 50;
                                if (tX < 0) tX = 0;
                                if ((tX + 235) > LOGICAL_MAX_X()) tX = LOGICAL_MAX_X() - 235;
                                if (tY < 0) tY = 0;
                                if ((tY + 100) > LOGICAL_MAX_Y()) tY = LOGICAL_MAX_Y() - 100;
                                m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sX = tX; m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sY = tY;
                                std::memset(m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr, 0, sizeof(m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr));
                                m_pGame->GetNpcName(m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV3, m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr);
                                break;
                            case 1000:
                                if (m_pGame->m_stDialogBoxExchangeInfo[0].sV1 == -1) m_pGame->m_stDialogBoxExchangeInfo[0].sItemID = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV4;
                                else if (m_pGame->m_stDialogBoxExchangeInfo[1].sV1 == -1) m_pGame->m_stDialogBoxExchangeInfo[1].sItemID = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV4;
                                else if (m_pGame->m_stDialogBoxExchangeInfo[2].sV1 == -1) m_pGame->m_stDialogBoxExchangeInfo[2].sItemID = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV4;
                                else if (m_pGame->m_stDialogBoxExchangeInfo[3].sV1 == -1) m_pGame->m_stDialogBoxExchangeInfo[3].sItemID = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV4;
                                else return;
                                m_pGame->bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_SETEXCHANGEITEM, 0, m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV4, iAmount, 0, 0);
                                break;
                            case 1001:
                                for (i = 0; i < DEF_MAXSELLLIST; i++)
                                    if (m_pGame->m_stSellItemList[i].iIndex == -1) {
                                        m_pGame->m_stSellItemList[i].iIndex = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV4;
                                        m_pGame->m_stSellItemList[i].iAmount = iAmount;
                                        m_pGame->m_bIsItemDisabled[m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV4] = true;
                                        break;
                                    }
                                if (i == DEF_MAXSELLLIST) m_pGame->AddEventList(UPDATE_SCREEN_ONGAME6, 10);
                                break;
                            case 1002:
                                if (m_pGame->_iGetBankItemCount() >= (m_pGame->iMaxBankItems - 1)) m_pGame->AddEventList(DLGBOX_CLICK_NPCACTION_QUERY9, 10);
                                else {
                                    CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV1]->m_sIDnum);
                                    if (pCfg) m_pGame->bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_GIVEITEMTOCHAR, m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV1, iAmount, m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV5, m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV6, pCfg->m_cName, m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV4);
                                }
                                break;
                            default:
                            {
                                CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView]->m_sIDnum);
                                if (pCfg) m_pGame->bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_GIVEITEMTOCHAR, (char)(m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView), iAmount, m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV1, m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV2, pCfg->m_cName);
                            }
                                break;
                            }
                            m_pGame->m_bIsItemDisabled[m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView] = true;
                        }
                        else m_pGame->AddEventList(UPDATE_SCREEN_ONGAME7, 10);
                    }
                    else {
                        if (iAmount <= 0) m_pGame->AddEventList(UPDATE_SCREEN_ONGAME8, 10);
                        else {
                            CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView]->m_sIDnum);
                            if (pCfg) m_pGame->bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_ITEMDROP, 0, m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView, iAmount, 0, pCfg->m_cName);
                            m_pGame->m_bIsItemDisabled[m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView] = true;
                        }
                    }
                }
                else m_pGame->AddEventList(UPDATE_SCREEN_ONGAME9, 10);
            }
            m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::ItemDropExternal);
        }
        else
        {
            if (!m_pGame->m_bInputStatus) {
                switch (m_pGame->m_cBackupChatMsg[0]) {
                case '!': case '@': case '#': case '$': case '^':
                    std::memset(m_pGame->m_cChatMsg, 0, sizeof(m_pGame->m_cChatMsg));
                    m_pGame->m_cChatMsg[0] = m_pGame->m_cBackupChatMsg[0];
                    m_pGame->StartInputString(CHAT_INPUT_X(), CHAT_INPUT_Y(), sizeof(m_pGame->m_cChatMsg), m_pGame->m_cChatMsg);
                    break;
                default:
                    m_pGame->StartInputString(CHAT_INPUT_X(), CHAT_INPUT_Y(), sizeof(m_pGame->m_cChatMsg), m_pGame->m_cChatMsg);
                    m_pGame->ClearInputString();
                    break;
                }
            }
            else {
                m_pGame->EndInputString();
                std::memset(m_pGame->G_cTxt, 0, sizeof(m_pGame->G_cTxt));
                m_pGame->ReceiveString((char*)m_pGame->G_cTxt);
                std::memset(m_pGame->m_cBackupChatMsg, 0, sizeof(m_pGame->m_cBackupChatMsg));
                strcpy(m_pGame->m_cBackupChatMsg, m_pGame->G_cTxt);
                if ((m_pGame->m_dwCurTime - m_dwPrevChatTime) >= 700) {
                    m_dwPrevChatTime = m_pGame->m_dwCurTime;
                    if (strlen(m_pGame->G_cTxt) > 0) {
                        if (!m_pGame->m_bShout && m_pGame->G_cTxt[0] == '!') {
                            m_pGame->AddEventList(BCHECK_LOCAL_CHAT_COMMAND9, 10);
                        }
                        else {
                            m_pGame->bSendCommand(MSGID_COMMAND_CHATMSG, 0, 0, 0, 0, 0, m_pGame->G_cTxt);
                        }
                    }
                }
            }
        }
    }

    // Save viewport and apply camera shake
    m_pGame->m_Camera.SavePosition();
    m_pGame->m_Camera.ApplyShake();

    // Calculate viewport tile coordinates
    m_sPivotX = m_pGame->m_pMapData->m_sPivotX;
    m_sPivotY = m_pGame->m_pMapData->m_sPivotY;
    sVal = m_pGame->m_Camera.GetX() - (m_sPivotX * 32);
    m_sDivX = sVal / 32;
    m_sModX = sVal % 32;
    sVal = m_pGame->m_Camera.GetY() - (m_sPivotY * 32);
    m_sDivY = sVal / 32;
    m_sModY = sVal % 32;

    // Logout countdown
    if (m_pGame->m_cLogOutCount > 0) {
        if ((m_dwTime - m_pGame->m_dwLogOutCountTime) > 1000) {
            m_pGame->m_cLogOutCount--;
            m_pGame->m_dwLogOutCountTime = m_dwTime;
            wsprintf(m_pGame->G_cTxt, UPDATE_SCREEN_ONGAME13, m_pGame->m_cLogOutCount);
            m_pGame->AddEventList(m_pGame->G_cTxt, 10);
        }
    }
    if (m_pGame->m_cLogOutCount == 0) {
        m_pGame->m_cLogOutCount = -1;
        m_pGame->WriteSettings();
        m_pGame->m_pGSock.reset();
        m_pGame->m_pGSock.reset();
        PlaySound('E', 14, 5);
        AudioManager::Get().StopSound(SoundType::Effect, 38);
        AudioManager::Get().StopMusic();
        m_pGame->ChangeGameMode(GameMode::MainMenu);
        return;
    }

    // Restart countdown
    if (m_pGame->m_cRestartCount > 0) {
        if ((m_dwTime - m_pGame->m_dwRestartCountTime) > 1000) {
            m_pGame->m_cRestartCount--;
            m_pGame->m_dwRestartCountTime = m_dwTime;
            wsprintf(m_pGame->G_cTxt, UPDATE_SCREEN_ONGAME14, m_pGame->m_cRestartCount);
            m_pGame->AddEventList(m_pGame->G_cTxt, 10);
        }
    }
    if (m_pGame->m_cRestartCount == 0) {
        m_pGame->m_cRestartCount = -1;
        m_pGame->bSendCommand(MSGID_REQUEST_RESTART, 0, 0, 0, 0, 0, 0);
        return;
    }

    // Update frame counters and process commands
    int iUpdateRet = m_pGame->m_pMapData->iObjectFrameCounter(m_pGame->m_pPlayer->m_cPlayerName, m_pGame->m_Camera.GetX(), m_pGame->m_Camera.GetY());
    if (m_pGame->m_pEffectManager) m_pGame->m_pEffectManager->Update();
    // Pipeline movement: allow next move command just before animation completes
    // Uses EntityMotion progress (95%) instead of fixed time thresholds to avoid
    // visible position snapping when the next tile's motion starts
    if (!m_pGame->m_pPlayer->m_Controller.IsCommandAvailable()) {
        char cmd = m_pGame->m_pPlayer->m_Controller.GetCommand();
        if (cmd == DEF_OBJECTMOVE || cmd == DEF_OBJECTRUN ||
            cmd == DEF_OBJECTDAMAGEMOVE || cmd == DEF_OBJECTATTACKMOVE) {
            int dX = m_pGame->m_pPlayer->m_sPlayerX - m_pGame->m_pMapData->m_sPivotX;
            int dY = m_pGame->m_pPlayer->m_sPlayerY - m_pGame->m_pMapData->m_sPivotY;
            if (dX >= 0 && dX < MAPDATASIZEX && dY >= 0 && dY < MAPDATASIZEY) {
                auto& motion = m_pGame->m_pMapData->m_pData[dX][dY].m_motion;
                if (motion.bIsMoving && motion.fProgress >= 0.95f) {
                    m_pGame->m_pPlayer->m_Controller.SetCommandAvailable(true);
                    m_pGame->m_pPlayer->m_Controller.SetCommandTime(0);
                }
            }
        }
        // Fast rotation unlock: allow quick direction changes without waiting
        // for the full STOP animation (840ms). Uses a short delay to prevent spam.
        // BUT only if player is actually in STOP animation (not casting magic, attacking, etc.)
        else if (cmd == DEF_OBJECTSTOP) {
            int dX = m_pGame->m_pPlayer->m_sPlayerX - m_pGame->m_pMapData->m_sPivotX;
            int dY = m_pGame->m_pPlayer->m_sPlayerY - m_pGame->m_pMapData->m_sPivotY;
            if (dX >= 0 && dX < MAPDATASIZEX && dY >= 0 && dY < MAPDATASIZEY) {
                int8_t animAction = m_pGame->m_pMapData->m_pData[dX][dY].m_animation.cAction;
                // Only fast unlock for pure rotation (STOP animation), not after magic/attack
                if (animAction == DEF_OBJECTSTOP) {
                    uint32_t cmdTime = m_pGame->m_pPlayer->m_Controller.GetCommandTime();
                    if (cmdTime > 0 && (m_dwTime - cmdTime) >= 100) {
                        m_pGame->m_pPlayer->m_Controller.SetCommandAvailable(true);
                        m_pGame->m_pPlayer->m_Controller.SetCommandTime(0);
                    }
                }
            }
        }
    }

    // Fallback: animation-based unlock still works for non-movement actions
    // and for the final step when the player stops
    if (iUpdateRet == 2) {
        m_pGame->m_pPlayer->m_Controller.SetCommandAvailable(true);
        m_pGame->m_pPlayer->m_Controller.SetCommandTime(0);
    }
    m_pGame->CommandProcessor(m_sMsX, m_sMsY,
        ((m_sDivX + m_sPivotX) * 32 + m_sModX + m_sMsX - 17) / 32 + 1,
        ((m_sDivY + m_sPivotY) * 32 + m_sModY + m_sMsY - 17) / 32 + 1,
        m_cLB, m_cRB);

    // Restore viewport
    m_pGame->m_Camera.RestorePosition();

    m_pGame->m_Camera.Update(m_dwTime);

    // Observer mode camera (additional updates for keyboard-driven movement)
    if (m_pGame->m_bIsObserverMode) {
        if ((m_dwTime - m_pGame->m_dwObserverCamTime) > 25) {
            m_pGame->m_dwObserverCamTime = m_dwTime;
            m_pGame->m_Camera.Update(m_dwTime);
        }
    }

    // Draw flag animation (time-based triangle wave: 0..255..0 over 4 seconds, 32bpp range)
    {
        uint32_t phase = m_dwTime % 4000;
        if (phase < 2000)
            m_pGame->m_iDrawFlag = (phase * 255) / 2000;
        else
            m_pGame->m_iDrawFlag = 255 - ((phase - 2000) * 255) / 2000;
    }
}

void Screen_OnGame::on_render()
{
    char cItemColor;

    // Update all dialog boxes first (before drawing)
    m_pGame->m_dialogBoxManager.UpdateDialogBoxs();

    // Update entity motion interpolation for all tiles
    uint32_t dwTime = m_pGame->m_dwCurTime;
    for (int y = 0; y < MAPDATASIZEY; y++) {
        for (int x = 0; x < MAPDATASIZEX; x++) {
            m_pGame->m_pMapData->m_pData[x][y].m_motion.Update(dwTime);
        }
    }

    // Snap camera to player position BEFORE drawing to eliminate character vibration
    // This ensures viewport and entity position use the same motion offset
    if (!m_pGame->m_bIsObserverMode)
    {
        int playerDX = m_pGame->m_pPlayer->m_sPlayerX - m_pGame->m_pMapData->m_sPivotX;
        int playerDY = m_pGame->m_pPlayer->m_sPlayerY - m_pGame->m_pMapData->m_sPivotY;
        if (playerDX >= 0 && playerDX < MAPDATASIZEX && playerDY >= 0 && playerDY < MAPDATASIZEY)
        {
            auto& motion = m_pGame->m_pMapData->m_pData[playerDX][playerDY].m_motion;
            int camX = (m_pGame->m_pPlayer->m_sPlayerX - VIEW_CENTER_TILE_X()) * 32
                     + static_cast<int>(motion.fCurrentOffsetX) - 16;
            int camY = (m_pGame->m_pPlayer->m_sPlayerY - VIEW_CENTER_TILE_Y()) * 32
                     + static_cast<int>(motion.fCurrentOffsetY) - 16;
            m_pGame->m_Camera.SnapTo(camX, camY);

            // Recalculate viewport to match snapped camera
            int sVal = m_pGame->m_Camera.GetX() - (m_sPivotX * 32);
            m_sDivX = sVal / 32;
            m_sModX = sVal % 32;
            sVal = m_pGame->m_Camera.GetY() - (m_sPivotY * 32);
            m_sDivY = sVal / 32;
            m_sModY = sVal % 32;
        }
    }

    // Main scene rendering
    FrameTiming::BeginProfile(ProfileStage::DrawBackground);
    m_pGame->DrawBackground(m_sDivX, m_sModX, m_sDivY, m_sModY);
    FrameTiming::EndProfile(ProfileStage::DrawBackground);

    FrameTiming::BeginProfile(ProfileStage::DrawEffectLights);
    m_pGame->m_pEffectManager->DrawEffectLights();

    // Player ambient light at night
    if (G_cSpriteAlphaDegree == 2)
    {
        const int PLAYER_SCREEN_X = LOGICAL_WIDTH() / 2;
        const int PLAYER_SCREEN_Y = (LOGICAL_HEIGHT() / 2) + 16;
        constexpr int PLAYER_LIGHT_RADIUS = 2;
        constexpr float PLAYER_CENTER_INTENSITY = 0.35f;
        constexpr float PLAYER_EDGE_INTENSITY = 0.05f;
        constexpr int TILE_SIZE = 32;

        for (int ty = -PLAYER_LIGHT_RADIUS; ty <= PLAYER_LIGHT_RADIUS; ty++) {
            for (int tx = -PLAYER_LIGHT_RADIUS; tx <= PLAYER_LIGHT_RADIUS; tx++) {
                float distance = sqrtf(static_cast<float>(tx * tx + ty * ty));
                if (distance > PLAYER_LIGHT_RADIUS) continue;

                float t = distance / PLAYER_LIGHT_RADIUS;
                float intensity = PLAYER_CENTER_INTENSITY * (1.0f - t) + PLAYER_EDGE_INTENSITY * t;
                if (intensity < 0.05f) continue;

                int lightX = PLAYER_SCREEN_X + tx * TILE_SIZE;
                int lightY = PLAYER_SCREEN_Y + ty * (TILE_SIZE / 2);

                int r = 255;
                int g = 255;
                int b = static_cast<int>(200 + 40 * (1.0f - t));

                m_pGame->m_pEffectSpr[0]->Draw(lightX, lightY, 1,
                    SpriteLib::DrawParams::AdditiveColored(r, g, b, intensity));
            }
        }
    }
    FrameTiming::EndProfile(ProfileStage::DrawEffectLights);

    // Tile grid BEFORE objects (so entities draw on top)
    DrawTileGrid();

    FrameTiming::BeginProfile(ProfileStage::DrawObjects);
    m_pGame->DrawObjects(m_sPivotX, m_sPivotY, m_sDivX, m_sDivY, m_sModX, m_sModY, m_sMsX, m_sMsY);
    FrameTiming::EndProfile(ProfileStage::DrawObjects);

    FrameTiming::BeginProfile(ProfileStage::DrawEffects);
    m_pGame->m_pEffectManager->DrawEffects();
    FrameTiming::EndProfile(ProfileStage::DrawEffects);

    // Patching grid overlay (after effects, on top of everything for debug)
    DrawPatchingGrid();

    FrameTiming::BeginProfile(ProfileStage::DrawWeather);
    m_pGame->DrawWhetherEffects();
    FrameTiming::EndProfile(ProfileStage::DrawWeather);

    FrameTiming::BeginProfile(ProfileStage::DrawChat);
    m_pGame->DrawChatMsgs(-100, 0, LOGICAL_WIDTH(), LOGICAL_HEIGHT());
    FrameTiming::EndProfile(ProfileStage::DrawChat);

    m_pGame->WhetherObjectFrameCounter();

    // Apocalypse map effects
    if (m_pGame->m_cMapIndex == 26) {
        m_pGame->m_pEffectSpr[89]->Draw(1296 - m_pGame->m_Camera.GetX(), 1283 - m_pGame->m_Camera.GetY(), m_pGame->m_entityState.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.5f));
        m_pGame->m_pEffectSpr[89]->Draw(1520 - m_pGame->m_Camera.GetX(), 1123 - m_pGame->m_Camera.GetY(), m_pGame->m_entityState.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.5f));
        m_pGame->m_pEffectSpr[89]->Draw(1488 - m_pGame->m_Camera.GetX(), 3971 - m_pGame->m_Camera.GetY(), m_pGame->m_entityState.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.5f));
        m_pGame->m_pEffectSpr[93]->Draw(2574 - m_pGame->m_Camera.GetX(), 3677 - m_pGame->m_Camera.GetY(), m_pGame->m_entityState.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.5f));
        m_pGame->m_pEffectSpr[93]->Draw(3018 - m_pGame->m_Camera.GetX(), 3973 - m_pGame->m_Camera.GetY(), m_pGame->m_entityState.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.5f));
    }
    else if (m_pGame->m_cMapIndex == 27) {
        m_pGame->m_pEffectSpr[89]->Draw(1293 - m_pGame->m_Camera.GetX(), 3657 - m_pGame->m_Camera.GetY(), m_pGame->m_entityState.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.5f));
        m_pGame->m_pEffectSpr[89]->Draw(944 - m_pGame->m_Camera.GetX(), 3881 - m_pGame->m_Camera.GetY(), m_pGame->m_entityState.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.5f));
        m_pGame->m_pEffectSpr[89]->Draw(1325 - m_pGame->m_Camera.GetX(), 4137 - m_pGame->m_Camera.GetY(), m_pGame->m_entityState.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.5f));
        m_pGame->m_pEffectSpr[89]->Draw(1648 - m_pGame->m_Camera.GetX(), 3913 - m_pGame->m_Camera.GetY(), m_pGame->m_entityState.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.5f));
    }

    // Apocalypse gate
    if ((m_pGame->m_iGatePositX >= m_pGame->m_Camera.GetX() / 32) && (m_pGame->m_iGatePositX <= m_pGame->m_Camera.GetX() / 32 + VIEW_TILE_WIDTH())
        && (m_pGame->m_iGatePositY >= m_pGame->m_Camera.GetY() / 32) && (m_pGame->m_iGatePositY <= m_pGame->m_Camera.GetY() / 32 + VIEW_TILE_HEIGHT())) {
        m_pGame->m_pEffectSpr[101]->Draw(m_pGame->m_iGatePositX * 32 - m_pGame->m_Camera.GetX() - 96, m_pGame->m_iGatePositY * 32 - m_pGame->m_Camera.GetY() - 69, m_pGame->m_entityState.m_iEffectFrame % 30, SpriteLib::DrawParams::Alpha(0.5f));
    }

    // UI rendering
    FrameTiming::BeginProfile(ProfileStage::DrawDialogs);
    m_pGame->m_dialogBoxManager.DrawDialogBoxs(m_sMsX, m_sMsY, m_sMsZ, m_cLB);
    FrameTiming::EndProfile(ProfileStage::DrawDialogs);

    FrameTiming::BeginProfile(ProfileStage::DrawMisc);
    if (m_pGame->m_bInputStatus) {
        if (((m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::GuildMenu) == true) && (m_pGame->m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode == 1)) ||
            ((m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropExternal) == true) && (m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cMode == 1))) {
        }
        else m_pGame->m_Renderer->DrawShadowBox(0, LOGICAL_HEIGHT() - 69, LOGICAL_MAX_X(), LOGICAL_HEIGHT() - 51);
        m_pGame->ShowReceivedString();
    }

    m_pGame->ShowEventList(m_pGame->m_dwCurTime);

    // Item tooltip on cursor
    short iTooltipItemID = CursorTarget::GetSelectedID();
    if ((CursorTarget::GetSelectedType() == SelectedObjectType::Item) &&
        (iTooltipItemID >= 0) && (iTooltipItemID < DEF_MAXITEMS) &&
        (m_pGame->m_pItemList[iTooltipItemID] != 0))
    {
        RenderItemTooltip();
    }

    // Druncncity bubbles
    if (m_pGame->m_cMapIndex == 25)
        m_pGame->m_pEffectManager->AddEffect(EffectType::BUBBLES_DRUNK, m_pGame->m_Camera.GetX() + rand() % LOGICAL_MAX_X(), m_pGame->m_Camera.GetY() + rand() % LOGICAL_MAX_Y(), 0, 0, -1 * (rand() % 80), 1);

    // Heldenian tower count
    if ((m_pGame->m_iHeldenianAresdenLeftTower != -1) && (memcmp(m_pGame->m_cCurLocation, "BtField", 7) == 0)) {
        wsprintf(m_pGame->G_cTxt, "Aresden Flags : %d", m_pGame->m_iHeldenianAresdenFlags);
        TextLib::DrawText(GameFont::Default, 10, 140, m_pGame->G_cTxt, TextLib::TextStyle::FromColorRef(GameColors::UIWhite.ToColorRef()));
        wsprintf(m_pGame->G_cTxt, "Aresden Flags : %d", m_pGame->m_iHeldenianElvineFlags);
        TextLib::DrawText(GameFont::Default, 10, 160, m_pGame->G_cTxt, TextLib::TextStyle::FromColorRef(GameColors::UIWhite.ToColorRef()));
        wsprintf(m_pGame->G_cTxt, "Aresden's rest building number : %d", m_pGame->m_iHeldenianAresdenLeftTower);
        TextLib::DrawText(GameFont::Default, 10, 180, m_pGame->G_cTxt, TextLib::TextStyle::FromColorRef(GameColors::UIWhite.ToColorRef()));
        wsprintf(m_pGame->G_cTxt, "Elvine's rest building number : %d", m_pGame->m_iHeldenianElvineLeftTower);
        TextLib::DrawText(GameFont::Default, 10, 200, m_pGame->G_cTxt, TextLib::TextStyle::FromColorRef(GameColors::UIWhite.ToColorRef()));
    }

    m_pGame->DrawTopMsg();

    FrameTiming::EndProfile(ProfileStage::DrawMisc);

    // FPS and profiling display
    int iDisplayY = 100;
    if (ConfigManager::Get().IsShowFpsEnabled()) {
        wsprintf(m_pGame->G_cTxt, "fps : %u", FrameTiming::GetFPS());
        TextLib::DrawText(GameFont::Default, 10, iDisplayY, m_pGame->G_cTxt, TextLib::TextStyle::FromColorRef(GameColors::UIWhite.ToColorRef()));
        iDisplayY += 14;
    }

    if (ConfigManager::Get().IsShowLatencyEnabled()) {
        if (m_pGame->m_iLatencyMs >= 0)
            wsprintf(m_pGame->G_cTxt, "latency : %d ms", m_pGame->m_iLatencyMs);
        else
            wsprintf(m_pGame->G_cTxt, "latency : -- ms");
        TextLib::DrawText(GameFont::Default, 10, iDisplayY, m_pGame->G_cTxt, TextLib::TextStyle::FromColorRef(GameColors::UIWhite.ToColorRef()));
        iDisplayY += 14;
    }

    // Profiling display
    if (FrameTiming::IsProfilingEnabled()) {
        iDisplayY += 4;
        TextLib::DrawText(GameFont::Default, 10, iDisplayY, "--- Profile (avg ms) ---", TextLib::TextStyle::FromColorRef(GameColors::UIProfileYellow.ToColorRef()));
        iDisplayY += 14;

        for (int i = 0; i < static_cast<int>(ProfileStage::COUNT); i++) {
            ProfileStage stage = static_cast<ProfileStage>(i);
            double avgMs = FrameTiming::GetProfileAvgTimeMS(stage);
            int wholePart = static_cast<int>(avgMs);
            int fracPart = static_cast<int>((avgMs - wholePart) * 100);
            wsprintf(m_pGame->G_cTxt, "%-12s: %3d.%02d", FrameTiming::GetStageName(stage), wholePart, fracPart);
            TextLib::DrawText(GameFont::Default, 10, iDisplayY, m_pGame->G_cTxt, TextLib::TextStyle::FromColorRef(GameColors::UINearWhite.ToColorRef()));
            iDisplayY += 12;
        }
    }
}

void Screen_OnGame::RenderItemTooltip()
{
	short target_id = CursorTarget::GetSelectedID();
    CItem* item = m_pGame->m_pItemList[target_id].get();
    CItem* pCfg = m_pGame->GetItemConfig(item->m_sIDnum);
    if (!pCfg) return;

    char cItemColor = item->m_cItemColor;
    bool is_hand_item = pCfg->GetEquipPos() == EquipPos::LeftHand || pCfg->GetEquipPos() == EquipPos::RightHand || pCfg->GetEquipPos() == EquipPos::TwoHand;
    size_t item_sprite_index = DEF_SPRID_ITEMPACK_PIVOTPOINT + pCfg->m_sSprite;
    SpriteLib::ISprite* sprite = m_pGame->m_pSprite[item_sprite_index].get();
    bool is_equippable = pCfg->IsArmor() || pCfg->IsWeapon() || pCfg->IsAccessory();

    if (cItemColor != 0) {
        if (is_hand_item) {
            sprite->Draw(m_sMsX - CursorTarget::GetDragDistX(), m_sMsY - CursorTarget::GetDragDistY(), pCfg->m_sSpriteFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[cItemColor].r - GameColors::Base.r, GameColors::Weapons[cItemColor].g - GameColors::Base.g, GameColors::Weapons[cItemColor].b - GameColors::Base.b));
        }
        else {
            sprite->Draw(m_sMsX - CursorTarget::GetDragDistX(), m_sMsY - CursorTarget::GetDragDistY(), pCfg->m_sSpriteFrame, SpriteLib::DrawParams::Tint(GameColors::Items[cItemColor].r - GameColors::Base.r, GameColors::Items[cItemColor].g - GameColors::Base.g, GameColors::Items[cItemColor].b - GameColors::Base.b));
        }
    }
    else sprite->Draw(m_sMsX - CursorTarget::GetDragDistX(), m_sMsY - CursorTarget::GetDragDistY(), pCfg->m_sSpriteFrame);

    char cStr1[64], cStr2[64], cStr3[64];
    int iLoc;
    m_pGame->GetItemName(item, cStr1, cStr2, cStr3);
    iLoc = 0;
    if (strlen(cStr1) != 0) {
        if (m_pGame->m_bIsSpecial) TextLib::DrawText(GameFont::Default, m_sMsX, m_sMsY + 25, cStr1, TextLib::TextStyle::WithShadow(GameColors::UIItemName_Special.r, GameColors::UIItemName_Special.g, GameColors::UIItemName_Special.b));
        else TextLib::DrawText(GameFont::Default, m_sMsX, m_sMsY + 25, cStr1, TextLib::TextStyle::WithShadow(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b));
        iLoc += 15;
    }
    if (strlen(cStr2) != 0) { TextLib::DrawText(GameFont::Default, m_sMsX, m_sMsY + 25 + iLoc, cStr2, TextLib::TextStyle::WithShadow(GameColors::UIDescription.r, GameColors::UIDescription.g, GameColors::UIDescription.b)); iLoc += 15; }
    if (strlen(cStr3) != 0) { TextLib::DrawText(GameFont::Default, m_sMsX, m_sMsY + 25 + iLoc, cStr3, TextLib::TextStyle::WithShadow(GameColors::UIDescription.r, GameColors::UIDescription.g, GameColors::UIDescription.b)); iLoc += 15; }
    if ((pCfg->m_sLevelLimit != 0) && item->IsCustomMade()) {
        wsprintf(m_pGame->G_cTxt, "%s: %d", DRAW_DIALOGBOX_SHOP24, pCfg->m_sLevelLimit);
        TextLib::DrawText(GameFont::Default, m_sMsX, m_sMsY + 25 + iLoc, m_pGame->G_cTxt, TextLib::TextStyle::WithShadow(GameColors::UIDescription.r, GameColors::UIDescription.g, GameColors::UIDescription.b)); iLoc += 15;
    }
    if (is_equippable) {
        // Weight below 1100 is not displayed as a strength requirement
        if (pCfg->m_wWeight >= 1100)
        {
            // Display weight, whatever the weight calculation is, divide by 100, and round up
            int _wWeight = static_cast<int>(std::ceil(pCfg->m_wWeight / 100.0f));
            wsprintf(m_pGame->G_cTxt, DRAW_DIALOGBOX_SHOP15, _wWeight);
            TextLib::DrawText(GameFont::Default, m_sMsX, m_sMsY + 25 + iLoc, m_pGame->G_cTxt, TextLib::TextStyle::WithShadow(GameColors::UIDescription.r, GameColors::UIDescription.g, GameColors::UIDescription.b)); iLoc += 15;
        }

        // Display durability
        wsprintf(m_pGame->G_cTxt, UPDATE_SCREEN_ONGAME10, item->m_wCurLifeSpan, pCfg->m_wMaxLifeSpan);
        TextLib::DrawText(GameFont::Default, m_sMsX, m_sMsY + 25 + iLoc, m_pGame->G_cTxt, TextLib::TextStyle::WithShadow(GameColors::UIDescription.r, GameColors::UIDescription.g, GameColors::UIDescription.b)); iLoc += 15;
    }

    if (pCfg->IsStackable()) {
        auto count = std::count_if(m_pGame->m_pItemList.begin(), m_pGame->m_pItemList.end(),
            [item](const std::unique_ptr<CItem>& otherItem) {
                return otherItem != nullptr && otherItem->m_sIDnum == item->m_sIDnum;
            });

        if (count > 1) {
            wsprintf(m_pGame->G_cTxt, DEF_MSG_TOTAL_NUMBER, count);
            TextLib::DrawText(GameFont::Default, m_sMsX, m_sMsY + 40, m_pGame->G_cTxt, TextLib::TextStyle::WithShadow(GameColors::UIDescription.r, GameColors::UIDescription.g, GameColors::UIDescription.b));
        }
    }
}

//=============================================================================
// DrawTileGrid - Simple dark grid lines showing tile boundaries
// Enabled via F12 > Graphics > Tile Grid toggle
// Called BEFORE objects so it appears below entities
//=============================================================================
void Screen_OnGame::DrawTileGrid()
{
    if (!ConfigManager::Get().IsTileGridEnabled()) return;

    constexpr int TILE_SIZE = 32;
    constexpr int HALF_TILE = 16;
    constexpr float GRID_ALPHA = 0.18f;

    int screenW = LOGICAL_WIDTH();
    int screenH = LOGICAL_HEIGHT();

    // Draw dark gray grid lines (subtle)
    for (int x = -m_sModX + HALF_TILE; x <= screenW; x += TILE_SIZE) {
        m_pGame->m_Renderer->DrawLine(x, 0, x, screenH, 40, 40, 40, GRID_ALPHA);
    }
    for (int y = -m_sModY + HALF_TILE; y <= screenH; y += TILE_SIZE) {
        m_pGame->m_Renderer->DrawLine(0, y, screenW, y, 40, 40, 40, GRID_ALPHA);
    }
}

//=============================================================================
// DrawPatchingGrid - Debug grid with direction zone colors
// Enabled via F12 > Graphics > Patching Grid toggle
//=============================================================================
void Screen_OnGame::DrawPatchingGrid()
{
    if (!ConfigManager::Get().IsPatchingGridEnabled()) return;

    constexpr int TILE_SIZE = 32;
    constexpr int HALF_TILE = 16;
    constexpr float ZONE_ALPHA = 0.25f;
    constexpr float GRID_ALPHA = 0.4f;

    int screenW = LOGICAL_WIDTH();
    int screenH = LOGICAL_HEIGHT();

    short playerX = m_pGame->m_pPlayer->m_sPlayerX;
    short playerY = m_pGame->m_pPlayer->m_sPlayerY;

    int patchMode = ConfigManager::Get().GetPatchingMode();

    auto calcDir = [patchMode](short playerX, short playerY, short destX, short destY) -> int {
        short dx = destX - playerX;
        short dy = destY - playerY;
        if (dx == 0 && dy == 0) return 0;

        short absX = (dx < 0) ? -dx : dx;
        short absY = (dy < 0) ? -dy : dy;

        if (patchMode == 1) {
            // New: asymmetric zones (N/S 3:1, E/W 4:1)
            if (absY == 0) return (dx > 0) ? 3 : 7;
            if (absX == 0) return (dy < 0) ? 1 : 5;
            if (absY >= absX * 3) return (dy < 0) ? 1 : 5;
            if (absX >= absY * 4) return (dx > 0) ? 3 : 7;
        } else if (patchMode == 2) {
            // Shadow: symmetric 3:1, checks dominant axis first
            if (absX > absY) {
                if (dx > 0) {
                    if (dy > absX / 3) return 4;   // SE
                    if (dy < -absX / 3) return 2;  // NE
                    return 3;  // E
                } else {
                    if (dy > absX / 3) return 6;   // SW
                    if (dy < -absX / 3) return 8;  // NW
                    return 7;  // W
                }
            } else {
                if (dy > 0) {
                    if (dx > absY / 3) return 4;   // SE
                    if (dx < -absY / 3) return 6;  // SW
                    return 5;  // S
                } else {
                    if (dx > absY / 3) return 2;   // NE
                    if (dx < -absY / 3) return 8;  // NW
                    return 1;  // N
                }
            }
        } else {
            // Original: diagonal priority
            if (absX == 0) return (dy > 0) ? 5 : 1;
            if (absY == 0) return (dx > 0) ? 3 : 7;
        }
        // Diagonal fallback for Original and New
        if (dx > 0 && dy < 0) return 2;
        if (dx > 0 && dy > 0) return 4;
        if (dx < 0 && dy > 0) return 6;
        return 8;
    };

    auto getDirColor = [](int dir, int& r, int& g, int& b) {
        switch (dir) {
            case 1: case 3: case 5: case 7: r = 0; g = 200; b = 0; break;
            case 2: case 4: case 6: case 8: r = 200; g = 0; b = 0; break;
            default: r = 150; g = 0; b = 200; break;
        }
    };

    int startTileX = m_sDivX + m_sPivotX;
    int startTileY = m_sDivY + m_sPivotY;
    int tilesX = (screenW / TILE_SIZE) + 3;
    int tilesY = (screenH / TILE_SIZE) + 3;

    for (int ty = -1; ty < tilesY; ty++) {
        for (int tx = -1; tx < tilesX; tx++) {
            short mapX = static_cast<short>(startTileX + tx);
            short mapY = static_cast<short>(startTileY + ty);

            int dir = calcDir(playerX, playerY, mapX, mapY);
            int r, g, b;
            getDirColor(dir, r, g, b);

            int screenX = tx * TILE_SIZE - m_sModX - HALF_TILE;
            int screenY = ty * TILE_SIZE - m_sModY - HALF_TILE;

            for (int i = 0; i < TILE_SIZE; i++) {
                m_pGame->m_Renderer->DrawLine(screenX, screenY + i, screenX + TILE_SIZE, screenY + i, r, g, b, ZONE_ALPHA);
            }
        }
    }

    // Draw subtle dark grid lines over the colored zones
    for (int x = -m_sModX + HALF_TILE; x <= screenW; x += TILE_SIZE) {
        m_pGame->m_Renderer->DrawLine(x, 0, x, screenH, 20, 20, 20, 0.35f);
    }
    for (int y = -m_sModY + HALF_TILE; y <= screenH; y += TILE_SIZE) {
        m_pGame->m_Renderer->DrawLine(0, y, screenW, y, 20, 20, 20, 0.35f);
    }
}
