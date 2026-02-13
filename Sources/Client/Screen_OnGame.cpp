// Screen_OnGame.cpp: Main gameplay screen implementation
//
//////////////////////////////////////////////////////////////////////

#include "Screen_OnGame.h"
#include "Game.h"
#include "EventListManager.h"
#include "TextInputManager.h"
#include "InventoryManager.h"
#include "GameModeManager.h"
#include "CommonTypes.h"
#include "FrameTiming.h"
#include "AudioManager.h"
#include "WeatherManager.h"
#include "ChatManager.h"
#include "ItemNameFormatter.h"
#include "ConfigManager.h"
#include "IInput.h"
#include "GlobalDef.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include "SpellAoE.h"
#include "Magic.h"
#include <string>
#include <memory>
#include <format>
#include <charconv>


using namespace hb::shared::net;
namespace MouseButton = hb::shared::input::MouseButton;

using namespace hb::shared::action;

using namespace hb::shared::item;
using namespace hb::client::config;
using namespace hb::client::sprite_id;

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
    m_dwPrevChatTime = 0;

    if (AudioManager::Get().IsMusicEnabled())
        m_pGame->StartBGM();
}

void Screen_OnGame::on_uninitialize()
{
    TextInputManager::Get().EndInput();
    AudioManager::Get().StopMusic();
}

void Screen_OnGame::on_update()
{
    std::string G_cTxt;
    short sVal, absX, absY, tX, tY;
    int i, iAmount;

    m_dwTime = GameClock::GetTimeMS();

    m_sMsX = static_cast<short>(hb::shared::input::get_mouse_x());
    m_sMsY = static_cast<short>(hb::shared::input::get_mouse_y());
    m_sMsZ = static_cast<short>(hb::shared::input::get_mouse_wheel_delta());
    m_cLB = hb::shared::input::is_mouse_button_down(MouseButton::Left) ? 1 : 0;
    m_cRB = hb::shared::input::is_mouse_button_down(MouseButton::Right) ? 1 : 0;
    m_pGame->m_dwCurTime = GameClock::GetTimeMS();

    // Sync manager singletons with game state
    AudioManager::Get().SetListenerPosition(m_pGame->m_pPlayer->m_sPlayerX, m_pGame->m_pPlayer->m_sPlayerY);

    // Enter key handling
    if (hb::shared::input::is_key_pressed(KeyCode::Enter) == true)
    {
        if ((m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::GuildMenu) == true) && (m_pGame->m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode == 1) && (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::GuildMenu)) {
            TextInputManager::Get().EndInput();
            if (m_pGame->m_pPlayer->m_cGuildName.empty()) return;
            if (m_pGame->m_pPlayer->m_cGuildName != "NONE") {
                m_pGame->bSendCommand(MsgId::RequestCreateNewGuild, MsgType::Confirm, 0, 0, 0, 0, 0);
                m_pGame->m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 2;
            }
        }
        else if ((m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropExternal) == true) && (m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cMode == 1) && (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::ItemDropExternal)) {
            TextInputManager::Get().EndInput();

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

            if (m_pGame->m_cAmountString.empty()) return;
            iAmount = 0;
            auto [ptr, ec] = std::from_chars(m_pGame->m_cAmountString.data(), m_pGame->m_cAmountString.data() + m_pGame->m_cAmountString.size(), iAmount);
            if (ec != std::errc{}) return;

            if (static_cast<int>(m_pGame->m_pItemList[m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView]->m_dwCount) < iAmount) {
                iAmount = m_pGame->m_pItemList[m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView]->m_dwCount;
            }

            if (iAmount != 0) {
                if (static_cast<int>(m_pGame->m_pItemList[m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView]->m_dwCount) >= iAmount) {
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
                                std::snprintf(m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr, sizeof(m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr), "%s", m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cStr);
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
                                std::snprintf(m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr, hb::shared::limits::NpcNameLen, "%s", m_pGame->GetNpcConfigName(m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV3));
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
                                std::snprintf(m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr, hb::shared::limits::NpcNameLen, "%s", m_pGame->GetNpcConfigName(m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV3));
                                break;
                            case 1000:
                                if (m_pGame->m_stDialogBoxExchangeInfo[0].sV1 == -1) m_pGame->m_stDialogBoxExchangeInfo[0].sItemID = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV4;
                                else if (m_pGame->m_stDialogBoxExchangeInfo[1].sV1 == -1) m_pGame->m_stDialogBoxExchangeInfo[1].sItemID = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV4;
                                else if (m_pGame->m_stDialogBoxExchangeInfo[2].sV1 == -1) m_pGame->m_stDialogBoxExchangeInfo[2].sItemID = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV4;
                                else if (m_pGame->m_stDialogBoxExchangeInfo[3].sV1 == -1) m_pGame->m_stDialogBoxExchangeInfo[3].sItemID = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV4;
                                else return;
                                m_pGame->bSendCommand(MsgId::CommandCommon, CommonType::SetExchangeItem, 0, m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV4, iAmount, 0, 0);
                                break;
                            case 1001:
                                for (i = 0; i < game_limits::max_sell_list; i++)
                                    if (m_pGame->m_stSellItemList[i].iIndex == -1) {
                                        m_pGame->m_stSellItemList[i].iIndex = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV4;
                                        m_pGame->m_stSellItemList[i].iAmount = iAmount;
                                        m_pGame->m_bIsItemDisabled[m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV4] = true;
                                        break;
                                    }
                                if (i == game_limits::max_sell_list) m_pGame->AddEventList(UPDATE_SCREEN_ONGAME6, 10);
                                break;
                            case 1002:
                                if (InventoryManager::Get().GetBankItemCount() >= (m_pGame->iMaxBankItems - 1)) m_pGame->AddEventList(DLGBOX_CLICK_NPCACTION_QUERY9, 10);
                                else {
                                    CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV1]->m_sIDnum);
                                    if (pCfg) m_pGame->bSendCommand(MsgId::CommandCommon, CommonType::GiveItemToChar, m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV1, iAmount, m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV5, m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV6, pCfg->m_cName, m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV4);
                                }
                                break;
                            default:
                            {
                                CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView]->m_sIDnum);
                                if (pCfg) m_pGame->bSendCommand(MsgId::CommandCommon, CommonType::GiveItemToChar, static_cast<char>(m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView), iAmount, m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV1, m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV2, pCfg->m_cName);
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
                            if (pCfg) m_pGame->bSendCommand(MsgId::CommandCommon, CommonType::ItemDrop, 0, m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView, iAmount, 0, pCfg->m_cName);
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
            if (!TextInputManager::Get().IsActive()) {
                switch (m_pGame->m_cBackupChatMsg[0]) {
                case '!': case '@': case '#': case '$': case '^':
                    m_pGame->m_cChatMsg.clear();
                    m_pGame->m_cChatMsg += m_pGame->m_cBackupChatMsg[0];
                    TextInputManager::Get().StartInput(CHAT_INPUT_X(), CHAT_INPUT_Y(), CGame::ChatMsgMaxLen, m_pGame->m_cChatMsg);
                    break;
                default:
                    TextInputManager::Get().StartInput(CHAT_INPUT_X(), CHAT_INPUT_Y(), CGame::ChatMsgMaxLen, m_pGame->m_cChatMsg);
                    TextInputManager::Get().ClearInput();
                    break;
                }
            }
            else {
                TextInputManager::Get().EndInput();
                G_cTxt = TextInputManager::Get().GetInputString();
                m_pGame->m_cBackupChatMsg = G_cTxt.c_str();
                if ((m_pGame->m_dwCurTime - m_dwPrevChatTime) >= 700) {
                    m_dwPrevChatTime = m_pGame->m_dwCurTime;
                    if (!G_cTxt.empty()) {
                        if (!ChatManager::Get().IsShoutEnabled() && G_cTxt[0] == '!') {
                            m_pGame->AddEventList(BCHECK_LOCAL_CHAT_COMMAND9, 10);
                        }
                        else {
                            m_pGame->bSendCommand(MsgId::CommandChatMsg, 0, 0, 0, 0, 0, G_cTxt.c_str());
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
    if (m_pGame->m_logout_count > 0) {
        if ((m_dwTime - m_pGame->m_logout_count_time) > 1000) {
            m_pGame->m_logout_count--;
            m_pGame->m_logout_count_time = m_dwTime;
            G_cTxt = std::format(UPDATE_SCREEN_ONGAME13, m_pGame->m_logout_count);
            m_pGame->AddEventList(G_cTxt.c_str(), 10);
        }
    }
    if (m_pGame->m_logout_count == 0) {
        m_pGame->m_logout_count = -1;
        m_pGame->WriteSettings();
        m_pGame->m_pGSock.reset();
        PlayGameSound('E', 14, 5);
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
            G_cTxt = std::format(UPDATE_SCREEN_ONGAME14, m_pGame->m_cRestartCount);
            m_pGame->AddEventList(G_cTxt.c_str(), 10);
        }
    }
    if (m_pGame->m_cRestartCount == 0) {
        m_pGame->m_cRestartCount = -1;
        m_pGame->bSendCommand(MsgId::RequestRestart, 0, 0, 0, 0, 0, 0);
        return;
    }

    // Update frame counters and process commands
    int iUpdateRet = m_pGame->m_pMapData->iObjectFrameCounter(m_pGame->m_pPlayer->m_cPlayerName, m_pGame->m_Camera.GetX(), m_pGame->m_Camera.GetY());
    if (m_pGame->m_pEffectManager) m_pGame->m_pEffectManager->Update();
    // Command unlock logic — determines when the player can act again after an action.
    // Three unlock paths, checked in priority order:
    //
    // 1. Movement pipeline (Quick Actions): unlocks at 95% move progress for smooth chaining.
    //    Only applies to MOVE/RUN — attack commands always reset cmd to STOP after execution.
    //
    // 2. Rotation unlock: allows quick direction changes during STOP animation (100ms min).
    //    Also serves as the primary unlock after attack/damage animations finish and
    //    transition the tile to STOP. Respects the attack swing time floor.
    //
    // 3. Animation completion (iUpdateRet == 2): fires once when any animation finishes.
    //    Respects the attack swing time floor.
    //
    // 4. Attack cooldown expiry: continuous per-frame check that catches the case where
    //    path 3 fired before the attack time elapsed (e.g., short damage animation replaced
    //    a longer attack). Only active when the current lock is from the attack itself
    //    (cmdTime <= attackEnd), never for subsequent movement locks.
    if (!m_pGame->m_pPlayer->m_Controller.IsCommandAvailable()) {
        char cmd = m_pGame->m_pPlayer->m_Controller.GetCommand();

        // Path 1: Movement pipeline — unlock at 95% motion progress for smooth chaining.
        // cmd stays as MOVE/RUN during movement (not reset to STOP until destination reached).
        // No attack end time check needed: movement can only start after attack cooldown expires.
        if (ConfigManager::Get().IsQuickActionsEnabled() &&
            (cmd == Type::Move || cmd == Type::Run)) {
            int dX = m_pGame->m_pPlayer->m_sPlayerX - m_pGame->m_pMapData->m_sPivotX;
            int dY = m_pGame->m_pPlayer->m_sPlayerY - m_pGame->m_pMapData->m_sPivotY;
            if (dX >= 0 && dX < MapDataSizeX && dY >= 0 && dY < MapDataSizeY) {
                auto& motion = m_pGame->m_pMapData->m_pData[dX][dY].m_motion;
                if (motion.bIsMoving && motion.fProgress >= 0.95f) {
                    m_pGame->m_pPlayer->m_Controller.SetCommandAvailable(true);
                    m_pGame->m_pPlayer->m_Controller.SetCommandTime(0);
                }
            }
        }
        // Path 2: Rotation unlock + post-attack/damage unlock.
        // After attack/magic/damage execution, cmd is always STOP. The tile animation
        // transitions to STOP when the action animation finishes. Once both cmd and tile
        // are STOP, unlock — but respect the attack swing time floor so interrupted attacks
        // (damage replacing attack) don't unlock before the server's expected swing time.
        else if (cmd == Type::Stop) {
            int dX = m_pGame->m_pPlayer->m_sPlayerX - m_pGame->m_pMapData->m_sPivotX;
            int dY = m_pGame->m_pPlayer->m_sPlayerY - m_pGame->m_pMapData->m_sPivotY;
            if (dX >= 0 && dX < MapDataSizeX && dY >= 0 && dY < MapDataSizeY) {
                int8_t animAction = m_pGame->m_pMapData->m_pData[dX][dY].m_animation.cAction;
                if (animAction == Type::Stop) {
                    uint32_t cmdTime = m_pGame->m_pPlayer->m_Controller.GetCommandTime();
                    uint32_t dwAttackEnd = m_pGame->m_pPlayer->m_Controller.GetAttackEndTime();
                    if (cmdTime == 0 || ((m_dwTime - cmdTime) >= 100 &&
                        (dwAttackEnd == 0 || m_dwTime >= dwAttackEnd))) {
                        m_pGame->m_pPlayer->m_Controller.SetCommandAvailable(true);
                        m_pGame->m_pPlayer->m_Controller.SetCommandTime(0);
                    }
                }
            }
        }

        // Path 4: Attack cooldown expiry — catches the case where the damage animation
        // finished (iUpdateRet == 2 was blocked) but the tile hasn't transitioned to STOP
        // yet (so path 2 can't fire). Only active when this lock is from the attack that
        // set attackEnd (cmdTime <= attackEnd), not for subsequent movement locks.
        // Also skip if a damage animation is currently playing — the player must wait
        // for the damage stun to finish even if attackEndTime has expired.
        uint32_t dwAttackEnd = m_pGame->m_pPlayer->m_Controller.GetAttackEndTime();
        uint32_t dwCmdTime = m_pGame->m_pPlayer->m_Controller.GetCommandTime();
        if (dwAttackEnd != 0 && dwCmdTime <= dwAttackEnd && GameClock::GetTimeMS() >= dwAttackEnd) {
            int dX4 = m_pGame->m_pPlayer->m_sPlayerX - m_pGame->m_pMapData->m_sPivotX;
            int dY4 = m_pGame->m_pPlayer->m_sPlayerY - m_pGame->m_pMapData->m_sPivotY;
            bool bDamageAnimPlaying = false;
            if (dX4 >= 0 && dX4 < MapDataSizeX && dY4 >= 0 && dY4 < MapDataSizeY) {
                int8_t animAction = m_pGame->m_pMapData->m_pData[dX4][dY4].m_animation.cAction;
                bDamageAnimPlaying = (animAction == Type::Damage || animAction == Type::DamageMove);
            }
            if (!bDamageAnimPlaying) {
                m_pGame->m_pPlayer->m_Controller.SetAttackEndTime(0);
                m_pGame->m_pPlayer->m_Controller.SetCommandAvailable(true);
                m_pGame->m_pPlayer->m_Controller.SetCommandTime(0);
            }
        }
    }

    // Path 3: Animation completion — fires once when any animation finishes.
    // Respects attack swing time floor to prevent early unlock from short damage animations.
    if (iUpdateRet == 2) {
        uint32_t dwNow = GameClock::GetTimeMS();
        uint32_t dwAttackEnd = m_pGame->m_pPlayer->m_Controller.GetAttackEndTime();
        if (dwAttackEnd == 0 || dwNow >= dwAttackEnd) {
            m_pGame->m_pPlayer->m_Controller.SetCommandAvailable(true);
            m_pGame->m_pPlayer->m_Controller.SetCommandTime(0);
        }
    }
    WeatherManager::Get().Update(m_pGame->m_dwCurTime);

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
    // Update all dialog boxes first (before drawing)
    m_pGame->m_dialogBoxManager.UpdateDialogBoxs();

    // Update entity motion interpolation for all tiles
    uint32_t dwTime = m_pGame->m_dwCurTime;
    for (int y = 0; y < MapDataSizeY; y++) {
        for (int x = 0; x < MapDataSizeX; x++) {
            m_pGame->m_pMapData->m_pData[x][y].m_motion.Update(dwTime);
        }
    }

    // Snap camera to player position BEFORE drawing to eliminate character vibration
    // This ensures viewport and entity position use the same motion offset
    if (!m_pGame->m_bIsObserverMode)
    {
        int playerDX = m_pGame->m_pPlayer->m_sPlayerX - m_pGame->m_pMapData->m_sPivotX;
        int playerDY = m_pGame->m_pPlayer->m_sPlayerY - m_pGame->m_pMapData->m_sPivotY;
        if (playerDX >= 0 && playerDX < MapDataSizeX && playerDY >= 0 && playerDY < MapDataSizeY)
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
    FrameTiming::EndProfile(ProfileStage::DrawEffectLights);

    // Tile grid BEFORE objects (so entities draw on top)
    DrawTileGrid();

    FrameTiming::BeginProfile(ProfileStage::DrawObjects);
    m_pGame->DrawObjects(m_sPivotX, m_sPivotY, m_sDivX, m_sDivY, m_sModX, m_sModY, m_sMsX, m_sMsY);
    FrameTiming::EndProfile(ProfileStage::DrawObjects);

    FrameTiming::BeginProfile(ProfileStage::DrawEffects);
    m_pGame->m_pEffectManager->DrawEffects();
    FrameTiming::EndProfile(ProfileStage::DrawEffects);

#ifdef _DEBUG
    DrawSpellTargetOverlay();
#endif

    // Patching grid overlay (after effects, on top of everything for debug)
    DrawPatchingGrid();

    FrameTiming::BeginProfile(ProfileStage::DrawWeather);
    WeatherManager::Get().Draw();
    FrameTiming::EndProfile(ProfileStage::DrawWeather);

    FrameTiming::BeginProfile(ProfileStage::DrawChat);
    m_pGame->m_floatingText.DrawAll(-100, 0, LOGICAL_WIDTH(), LOGICAL_HEIGHT(), m_pGame->m_dwCurTime, m_pGame->m_Renderer);
    FrameTiming::EndProfile(ProfileStage::DrawChat);

    // Apocalypse map effects
    if (m_pGame->m_cMapIndex == 26) {
        m_pGame->m_pEffectSpr[89]->Draw(1296 - m_pGame->m_Camera.GetX(), 1283 - m_pGame->m_Camera.GetY(), m_pGame->m_entityState.m_iEffectFrame % 12, hb::shared::sprite::DrawParams::Alpha(0.5f));
        m_pGame->m_pEffectSpr[89]->Draw(1520 - m_pGame->m_Camera.GetX(), 1123 - m_pGame->m_Camera.GetY(), m_pGame->m_entityState.m_iEffectFrame % 12, hb::shared::sprite::DrawParams::Alpha(0.5f));
        m_pGame->m_pEffectSpr[89]->Draw(1488 - m_pGame->m_Camera.GetX(), 3971 - m_pGame->m_Camera.GetY(), m_pGame->m_entityState.m_iEffectFrame % 12, hb::shared::sprite::DrawParams::Alpha(0.5f));
        m_pGame->m_pEffectSpr[93]->Draw(2574 - m_pGame->m_Camera.GetX(), 3677 - m_pGame->m_Camera.GetY(), m_pGame->m_entityState.m_iEffectFrame % 12, hb::shared::sprite::DrawParams::Alpha(0.5f));
        m_pGame->m_pEffectSpr[93]->Draw(3018 - m_pGame->m_Camera.GetX(), 3973 - m_pGame->m_Camera.GetY(), m_pGame->m_entityState.m_iEffectFrame % 12, hb::shared::sprite::DrawParams::Alpha(0.5f));
    }
    else if (m_pGame->m_cMapIndex == 27) {
        m_pGame->m_pEffectSpr[89]->Draw(1293 - m_pGame->m_Camera.GetX(), 3657 - m_pGame->m_Camera.GetY(), m_pGame->m_entityState.m_iEffectFrame % 12, hb::shared::sprite::DrawParams::Alpha(0.5f));
        m_pGame->m_pEffectSpr[89]->Draw(944 - m_pGame->m_Camera.GetX(), 3881 - m_pGame->m_Camera.GetY(), m_pGame->m_entityState.m_iEffectFrame % 12, hb::shared::sprite::DrawParams::Alpha(0.5f));
        m_pGame->m_pEffectSpr[89]->Draw(1325 - m_pGame->m_Camera.GetX(), 4137 - m_pGame->m_Camera.GetY(), m_pGame->m_entityState.m_iEffectFrame % 12, hb::shared::sprite::DrawParams::Alpha(0.5f));
        m_pGame->m_pEffectSpr[89]->Draw(1648 - m_pGame->m_Camera.GetX(), 3913 - m_pGame->m_Camera.GetY(), m_pGame->m_entityState.m_iEffectFrame % 12, hb::shared::sprite::DrawParams::Alpha(0.5f));
    }

    // Apocalypse gate
    if ((m_pGame->m_iGatePositX >= m_pGame->m_Camera.GetX() / 32) && (m_pGame->m_iGatePositX <= m_pGame->m_Camera.GetX() / 32 + VIEW_TILE_WIDTH())
        && (m_pGame->m_iGatePositY >= m_pGame->m_Camera.GetY() / 32) && (m_pGame->m_iGatePositY <= m_pGame->m_Camera.GetY() / 32 + VIEW_TILE_HEIGHT())) {
        m_pGame->m_pEffectSpr[101]->Draw(m_pGame->m_iGatePositX * 32 - m_pGame->m_Camera.GetX() - 96, m_pGame->m_iGatePositY * 32 - m_pGame->m_Camera.GetY() - 69, m_pGame->m_entityState.m_iEffectFrame % 30, hb::shared::sprite::DrawParams::Alpha(0.5f));
    }

    // UI rendering
    FrameTiming::BeginProfile(ProfileStage::DrawDialogs);
    m_pGame->m_dialogBoxManager.DrawDialogBoxs(m_sMsX, m_sMsY, m_sMsZ, m_cLB);
    FrameTiming::EndProfile(ProfileStage::DrawDialogs);

    FrameTiming::BeginProfile(ProfileStage::DrawMisc);
    if (TextInputManager::Get().IsActive()) {
        if (((m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::GuildMenu) == true) && (m_pGame->m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode == 1)) ||
            ((m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropExternal) == true) && (m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cMode == 1))) {
        }
        else m_pGame->m_Renderer->DrawRectFilled(0, LOGICAL_HEIGHT() - 69, LOGICAL_MAX_X(), 18, hb::shared::render::Color::Black(128));
        TextInputManager::Get().ShowInput();
    }

    EventListManager::Get().ShowEvents(m_pGame->m_dwCurTime);

    // Item tooltip on cursor
    short iTooltipItemID = CursorTarget::GetSelectedID();
    if ((CursorTarget::GetSelectedType() == SelectedObjectType::Item) &&
        (iTooltipItemID >= 0) && (iTooltipItemID < hb::shared::limits::MaxItems) &&
        (m_pGame->m_pItemList[iTooltipItemID] != 0))
    {
        RenderItemTooltip();
    }

    // Druncncity bubbles (throttled to ~30/sec regardless of FPS)
    if (m_pGame->m_cMapIndex == 25 && (m_pGame->m_dwCurTime - m_dwLastBubbleTime) >= 33)
    {
        m_dwLastBubbleTime = m_pGame->m_dwCurTime;
        m_pGame->m_pEffectManager->AddEffect(EffectType::BUBBLES_DRUNK, m_pGame->m_Camera.GetX() + rand() % LOGICAL_MAX_X(), m_pGame->m_Camera.GetY() + rand() % LOGICAL_MAX_Y(), 0, 0, -1 * (rand() % 80), 1);
    }

    // Heldenian tower count
    if ((m_pGame->m_iHeldenianAresdenLeftTower != -1) && (m_pGame->m_cCurLocation.starts_with("BtField"))) {
        std::string G_cTxt;
        G_cTxt = std::format("Aresden Flags : {}", m_pGame->m_iHeldenianAresdenFlags);
        hb::shared::text::DrawText(GameFont::Default, 10, 140, G_cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UIWhite));
        G_cTxt = std::format("Elvine Flags : {}", m_pGame->m_iHeldenianElvineFlags);
        hb::shared::text::DrawText(GameFont::Default, 10, 160, G_cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UIWhite));
        G_cTxt = std::format("Aresden's rest building number : {}", m_pGame->m_iHeldenianAresdenLeftTower);
        hb::shared::text::DrawText(GameFont::Default, 10, 180, G_cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UIWhite));
        G_cTxt = std::format("Elvine's rest building number : {}", m_pGame->m_iHeldenianElvineLeftTower);
        hb::shared::text::DrawText(GameFont::Default, 10, 200, G_cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UIWhite));
    }

    m_pGame->DrawTopMsg();

    FrameTiming::EndProfile(ProfileStage::DrawMisc);

    // FPS, latency, and profiling display moved to RenderFrame (global, all screens)
}

void Screen_OnGame::RenderItemTooltip()
{
	std::string G_cTxt;
	short target_id = CursorTarget::GetSelectedID();
    CItem* item = m_pGame->m_pItemList[target_id].get();
    if (!item) return;
    CItem* pCfg = m_pGame->GetItemConfig(item->m_sIDnum);
    if (!pCfg) return;

    char cItemColor = item->m_cItemColor;
    bool is_hand_item = pCfg->GetEquipPos() == EquipPos::LeftHand || pCfg->GetEquipPos() == EquipPos::RightHand || pCfg->GetEquipPos() == EquipPos::TwoHand;
    size_t item_sprite_index = ItemPackPivotPoint + pCfg->m_sSprite;
    hb::shared::sprite::ISprite* sprite = m_pGame->m_pSprite[item_sprite_index].get();
    bool is_equippable = pCfg->IsArmor() || pCfg->IsWeapon() || pCfg->IsAccessory();

    if (cItemColor != 0) {
        if (is_hand_item) {
            sprite->Draw(m_sMsX - CursorTarget::GetDragDistX(), m_sMsY - CursorTarget::GetDragDistY(), pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Tint(GameColors::Weapons[cItemColor].r, GameColors::Weapons[cItemColor].g, GameColors::Weapons[cItemColor].b));
        }
        else {
            sprite->Draw(m_sMsX - CursorTarget::GetDragDistX(), m_sMsY - CursorTarget::GetDragDistY(), pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Tint(GameColors::Items[cItemColor].r, GameColors::Items[cItemColor].g, GameColors::Items[cItemColor].b));
        }
    }
    else sprite->Draw(m_sMsX - CursorTarget::GetDragDistX(), m_sMsY - CursorTarget::GetDragDistY(), pCfg->m_sSpriteFrame);

    int iLoc;
    auto itemInfo = ItemNameFormatter::Get().Format(item);
    iLoc = 0;
    if (itemInfo.name.size() != 0) {
        if (itemInfo.is_special) hb::shared::text::DrawText(GameFont::Default, m_sMsX, m_sMsY + 25, itemInfo.name.c_str(), hb::shared::text::TextStyle::WithShadow(GameColors::UIItemName_Special));
        else hb::shared::text::DrawText(GameFont::Default, m_sMsX, m_sMsY + 25, itemInfo.name.c_str(), hb::shared::text::TextStyle::WithShadow(GameColors::UIWhite));
        iLoc += 15;
    }
    if (itemInfo.effect.size() != 0) { hb::shared::text::DrawText(GameFont::Default, m_sMsX, m_sMsY + 25 + iLoc, itemInfo.effect.c_str(), hb::shared::text::TextStyle::WithShadow(GameColors::UIDescription)); iLoc += 15; }
    if (itemInfo.extra.size() != 0) { hb::shared::text::DrawText(GameFont::Default, m_sMsX, m_sMsY + 25 + iLoc, itemInfo.extra.c_str(), hb::shared::text::TextStyle::WithShadow(GameColors::UIDescription)); iLoc += 15; }
    if ((pCfg->m_sLevelLimit != 0) && item->IsCustomMade()) {
        G_cTxt = std::format("{}: {}", DRAW_DIALOGBOX_SHOP24, pCfg->m_sLevelLimit);
        hb::shared::text::DrawText(GameFont::Default, m_sMsX, m_sMsY + 25 + iLoc, G_cTxt.c_str(), hb::shared::text::TextStyle::WithShadow(GameColors::UIDescription)); iLoc += 15;
    }
    if (is_equippable) {
        // Weight below 1100 is not displayed as a strength requirement
        if (pCfg->m_wWeight >= 1100)
        {
            // Display weight, whatever the weight calculation is, divide by 100, and round up
            int _wWeight = static_cast<int>(std::ceil(pCfg->m_wWeight / 100.0f));
            G_cTxt = std::format(DRAW_DIALOGBOX_SHOP15, _wWeight);
            hb::shared::text::DrawText(GameFont::Default, m_sMsX, m_sMsY + 25 + iLoc, G_cTxt.c_str(), hb::shared::text::TextStyle::WithShadow(GameColors::UIDescription)); iLoc += 15;
        }

        // Display durability
        G_cTxt = std::format(UPDATE_SCREEN_ONGAME10, item->m_wCurLifeSpan, pCfg->m_wMaxLifeSpan);
        hb::shared::text::DrawText(GameFont::Default, m_sMsX, m_sMsY + 25 + iLoc, G_cTxt.c_str(), hb::shared::text::TextStyle::WithShadow(GameColors::UIDescription)); iLoc += 15;
    }

    if (pCfg->IsStackable()) {
        auto count = std::count_if(m_pGame->m_pItemList.begin(), m_pGame->m_pItemList.end(),
            [item](const std::unique_ptr<CItem>& otherItem) {
                return otherItem != nullptr && otherItem->m_sIDnum == item->m_sIDnum;
            });

        if (count > 1) {
            G_cTxt = std::format(DEF_MSG_TOTAL_NUMBER, static_cast<int>(count));
            hb::shared::text::DrawText(GameFont::Default, m_sMsX, m_sMsY + 40, G_cTxt.c_str(), hb::shared::text::TextStyle::WithShadow(GameColors::UIDescription));
        }
    }
}

//=============================================================================
// DrawSpellTargetOverlay - Debug overlay showing spell AoE tiles (DEBUG ONLY)
// Draws faint blue outlines on tiles that would be affected by the current spell
// while the player is in targeting mode (m_bIsGetPointingMode).
//=============================================================================
void Screen_OnGame::DrawSpellTargetOverlay()
{
#ifndef _DEBUG
    return;
#else
    if (!m_pGame->m_bIsGetPointingMode) return;
    if (m_pGame->m_iPointCommandType < 100) return;

    int magicId = m_pGame->m_iPointCommandType - 100;
    if (magicId < 0 || magicId >= hb::shared::limits::MaxMagicType) return;
    if (!m_pGame->m_pMagicCfgList[magicId]) return;

    CMagic* pMagic = m_pGame->m_pMagicCfgList[magicId].get();
    if (pMagic->m_sType == 0) return;

    int casterX = m_pGame->m_pPlayer->m_sPlayerX;
    int casterY = m_pGame->m_pPlayer->m_sPlayerY;

    // Mouse world tile (same formula as CommandProcessor call on line 383)
    int targetX = ((m_sDivX + m_sPivotX) * 32 + m_sModX + m_sMsX - 17) / 32 + 1;
    int targetY = ((m_sDivY + m_sPivotY) * 32 + m_sModY + m_sMsY - 17) / 32 + 1;

    SpellAoEParams params;
    params.magicType = pMagic->m_sType;
    params.aoeRadiusX = pMagic->m_sAoERadiusX;
    params.aoeRadiusY = pMagic->m_sAoERadiusY;
    params.dynamicPattern = pMagic->m_sDynamicPattern;
    params.dynamicRadius = pMagic->m_sDynamicRadius;

    constexpr int MAX_TILES = 512;
    SpellAoETile tiles[MAX_TILES];
    int tileCount = SpellAoE::CalculateTiles(params,
        casterX, casterY, targetX, targetY,
        tiles, MAX_TILES);

    if (tileCount <= 0) return;

    constexpr int HALF_TILE = 16;
    hb::shared::render::Color overlayColor(100, 180, 255, 60);

    for (int i = 0; i < tileCount; i++) {
        int sx = (tiles[i].x - (m_sDivX + m_sPivotX)) * 32 - m_sModX - HALF_TILE;
        int sy = (tiles[i].y - (m_sDivY + m_sPivotY)) * 32 - m_sModY - HALF_TILE;
        m_pGame->m_Renderer->DrawRectOutline(sx, sy, 32, 32, overlayColor);
    }
#endif
}

//=============================================================================
// DrawTileGrid - Simple dark grid lines showing tile boundaries (DEBUG ONLY)
// Enabled via F12 > Graphics > Tile Grid toggle
// Called BEFORE objects so it appears below entities
//=============================================================================
void Screen_OnGame::DrawTileGrid()
{
#ifndef _DEBUG
    return;
#else
    if (!ConfigManager::Get().IsTileGridEnabled()) return;

    constexpr int TILE_SIZE = 32;
    constexpr int HALF_TILE = 16;
    constexpr float GRID_ALPHA = 0.18f;

    int screenW = LOGICAL_WIDTH();
    int screenH = LOGICAL_HEIGHT();

    // Draw dark gray grid lines (subtle)
    hb::shared::render::Color gridColor(40, 40, 40, static_cast<uint8_t>(GRID_ALPHA * 255));
    for (int x = -m_sModX + HALF_TILE; x <= screenW; x += TILE_SIZE) {
        m_pGame->m_Renderer->DrawLine(x, 0, x, screenH, gridColor);
    }
    for (int y = -m_sModY + HALF_TILE; y <= screenH; y += TILE_SIZE) {
        m_pGame->m_Renderer->DrawLine(0, y, screenW, y, gridColor);
    }
#endif
}

//=============================================================================
// DrawPatchingGrid - Debug grid with direction zone colors (DEBUG ONLY)
// Enabled via F12 > Graphics > Patching Grid toggle
//=============================================================================
void Screen_OnGame::DrawPatchingGrid()
{
#ifndef _DEBUG
    return;
#else
    if (!ConfigManager::Get().IsPatchingGridEnabled()) return;

    constexpr int TILE_SIZE = 32;
    constexpr int HALF_TILE = 16;
    constexpr float ZONE_ALPHA = 0.25f;
    constexpr float GRID_ALPHA = 0.4f;

    int screenW = LOGICAL_WIDTH();
    int screenH = LOGICAL_HEIGHT();

    short playerX = m_pGame->m_pPlayer->m_sPlayerX;
    short playerY = m_pGame->m_pPlayer->m_sPlayerY;

    // Asymmetric zones algorithm (N/S 3:1, E/W 4:1)
    auto calcDir = [](short playerX, short playerY, short destX, short destY) -> int {
        short dx = destX - playerX;
        short dy = destY - playerY;
        if (dx == 0 && dy == 0) return 0;

        short absX = (dx < 0) ? -dx : dx;
        short absY = (dy < 0) ? -dy : dy;

        // Cardinal directions
        if (absY == 0) return (dx > 0) ? 3 : 7;  // E or W
        if (absX == 0) return (dy < 0) ? 1 : 5;  // N or S
        if (absY >= absX * 3) return (dy < 0) ? 1 : 5;  // N or S zone
        if (absX >= absY * 4) return (dx > 0) ? 3 : 7;  // E or W zone

        // Diagonal fallback
        if (dx > 0 && dy < 0) return 2;  // NE
        if (dx > 0 && dy > 0) return 4;  // SE
        if (dx < 0 && dy > 0) return 6;  // SW
        return 8;  // NW
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

            hb::shared::render::Color zoneColor(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), static_cast<uint8_t>(ZONE_ALPHA * 255));
            for (int i = 0; i < TILE_SIZE; i++) {
                m_pGame->m_Renderer->DrawLine(screenX, screenY + i, screenX + TILE_SIZE, screenY + i, zoneColor);
            }
        }
    }

    // Draw subtle dark grid lines over the colored zones
    hb::shared::render::Color overlayGridColor(20, 20, 20, static_cast<uint8_t>(0.35f * 255));
    for (int x = -m_sModX + HALF_TILE; x <= screenW; x += TILE_SIZE) {
        m_pGame->m_Renderer->DrawLine(x, 0, x, screenH, overlayGridColor);
    }
    for (int y = -m_sModY + HALF_TILE; y <= screenH; y += TILE_SIZE) {
        m_pGame->m_Renderer->DrawLine(0, y, screenW, y, overlayGridColor);
    }
#endif
}
