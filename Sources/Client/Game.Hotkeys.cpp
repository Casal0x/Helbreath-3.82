#include "Game.h"
#include "TextInputManager.h"
#include "MagicCastingSystem.h"
#include "ChatManager.h"
#include "AudioManager.h"
#include "ConfigManager.h"
#include "HotkeyManager.h"
#include "DevConsole.h"
#include "Overlay_DevConsole.h"
#include "LAN_ENG.H"
#include <format>
#include <string>


using namespace hb::shared::net;
namespace MouseButton = hb::shared::input::MouseButton;

void CGame::RegisterHotkeys()
{
	auto& hotkeys = HotkeyManager::Get();
	hotkeys.Clear();

	HotkeyManager::KeyCombo ctrlOnly{ KeyCode::Unknown, true, false, false };

	hotkeys.Register({ KeyCode::A, ctrlOnly.ctrl, ctrlOnly.shift, ctrlOnly.alt }, HotkeyManager::Trigger::KeyUp,
		[this]() { Hotkey_ToggleForceAttack(); });
	hotkeys.Register({ KeyCode::D, ctrlOnly.ctrl, ctrlOnly.shift, ctrlOnly.alt }, HotkeyManager::Trigger::KeyUp,
		[this]() { Hotkey_CycleDetailLevel(); });
	hotkeys.Register({ KeyCode::H, ctrlOnly.ctrl, ctrlOnly.shift, ctrlOnly.alt }, HotkeyManager::Trigger::KeyUp,
		[this]() { Hotkey_ToggleHelp(); });
	hotkeys.Register({ KeyCode::W, ctrlOnly.ctrl, ctrlOnly.shift, ctrlOnly.alt }, HotkeyManager::Trigger::KeyUp,
		[this]() { Hotkey_ToggleDialogTransparency(); });
	hotkeys.Register({ KeyCode::X, ctrlOnly.ctrl, ctrlOnly.shift, ctrlOnly.alt }, HotkeyManager::Trigger::KeyUp,
		[this]() { Hotkey_ToggleSystemMenu(); });
	hotkeys.Register({ KeyCode::M, ctrlOnly.ctrl, ctrlOnly.shift, ctrlOnly.alt }, HotkeyManager::Trigger::KeyUp,
		[this]() { Hotkey_ToggleGuideMap(); });
	hotkeys.Register({ KeyCode::R, ctrlOnly.ctrl, ctrlOnly.shift, ctrlOnly.alt }, HotkeyManager::Trigger::KeyUp,
		[this]() { Hotkey_ToggleRunningMode(); });
	hotkeys.Register({ KeyCode::S, ctrlOnly.ctrl, ctrlOnly.shift, ctrlOnly.alt }, HotkeyManager::Trigger::KeyUp,
		[this]() { Hotkey_ToggleSoundAndMusic(); });
	hotkeys.Register({ KeyCode::T, ctrlOnly.ctrl, ctrlOnly.shift, ctrlOnly.alt }, HotkeyManager::Trigger::KeyUp,
		[this]() { Hotkey_WhisperTarget(); });

	// Alt+Tilde: Toggle developer console (GM mode only)
	hotkeys.Register({ KeyCode::Grave, false, false, true }, HotkeyManager::Trigger::KeyUp,
		[this]() {
			DevConsole& console = DevConsole::Get();
			if (console.IsVisible())
			{
				console.Hide();
				GameModeManager::clear_overlay();
			}
			else
			{
				if (m_pPlayer == nullptr || !m_pPlayer->m_bIsGMMode)
					return;
				console.Show();
				GameModeManager::set_overlay<Overlay_DevConsole>();
			}
		});
}

void CGame::Hotkey_ToggleForceAttack()
{
	if (!hb::shared::input::IsCtrlDown() || GameModeManager::GetMode() != GameMode::MainGame || TextInputManager::Get().IsActive()) {
		return;
	}
	if (m_pPlayer->m_bForceAttack)
	{
		m_pPlayer->m_bForceAttack = false;
		AddEventList(DEF_MSG_FORCEATTACK_OFF, 10);
	}
	else
	{
		m_pPlayer->m_bForceAttack = true;
		AddEventList(DEF_MSG_FORCEATTACK_ON, 10);
	}
}

void CGame::Hotkey_CycleDetailLevel()
{
	if (!hb::shared::input::IsCtrlDown() || GameModeManager::GetMode() != GameMode::MainGame || TextInputManager::Get().IsActive()) {
		return;
	}
	int detailLevel = ConfigManager::Get().GetDetailLevel();
	detailLevel++;
	if (detailLevel > 2) detailLevel = 0;
	ConfigManager::Get().SetDetailLevel(detailLevel);
	switch (detailLevel) {
	case 0:
		AddEventList(NOTIFY_MSG_DETAIL_LEVEL_LOW, 10);
		break;
	case 1:
		AddEventList(NOTIFY_MSG_DETAIL_LEVEL_MEDIUM, 10);
		break;
	case 2:
		AddEventList(NOTIFY_MSG_DETAIL_LEVEL_HIGH, 10);
		break;
	}
}

void CGame::Hotkey_ToggleHelp()
{
	if (!hb::shared::input::IsCtrlDown() || GameModeManager::GetMode() != GameMode::MainGame || TextInputManager::Get().IsActive()) {
		return;
	}
	if (m_dialogBoxManager.IsEnabled(DialogBoxId::Help) == false)
		m_dialogBoxManager.EnableDialogBox(DialogBoxId::Help, 0, 0, 0);
	else
	{
		m_dialogBoxManager.DisableDialogBox(DialogBoxId::Help);
		m_dialogBoxManager.DisableDialogBox(DialogBoxId::Text);
	}
}

void CGame::Hotkey_ToggleDialogTransparency()
{
	if (!hb::shared::input::IsCtrlDown() || GameModeManager::GetMode() != GameMode::MainGame || TextInputManager::Get().IsActive()) {
		return;
	}
	bool enabled = ConfigManager::Get().IsDialogTransparencyEnabled();
	ConfigManager::Get().SetDialogTransparencyEnabled(!enabled);
}

void CGame::Hotkey_ToggleSystemMenu()
{
	if (!hb::shared::input::IsCtrlDown() || GameModeManager::GetMode() != GameMode::MainGame || TextInputManager::Get().IsActive()) {
		return;
	}
	if (m_dialogBoxManager.IsEnabled(DialogBoxId::SystemMenu) == false)
		m_dialogBoxManager.EnableDialogBox(DialogBoxId::SystemMenu, 0, 0, 0);
	else m_dialogBoxManager.DisableDialogBox(DialogBoxId::SystemMenu);
}

void CGame::Hotkey_ToggleGuideMap()
{
	if (GameModeManager::GetMode() != GameMode::MainGame || !hb::shared::input::IsCtrlDown()) {
		return;
	}
	if (m_dialogBoxManager.IsEnabled(DialogBoxId::GuideMap) == true) m_dialogBoxManager.DisableDialogBox(DialogBoxId::GuideMap);
	else m_dialogBoxManager.EnableDialogBox(DialogBoxId::GuideMap, 0, 0, 0, 0);
}

void CGame::Hotkey_ToggleRunningMode()
{
	if (!hb::shared::input::IsCtrlDown() || GameModeManager::GetMode() != GameMode::MainGame || TextInputManager::Get().IsActive()) {
		return;
	}
	bool runningMode = ConfigManager::Get().IsRunningModeEnabled();
	if (runningMode)
	{
		ConfigManager::Get().SetRunningModeEnabled(false);
		AddEventList(NOTIFY_MSG_CONVERT_WALKING_MODE, 10);
	}
	else
	{
		ConfigManager::Get().SetRunningModeEnabled(true);
		AddEventList(NOTIFY_MSG_CONVERT_RUNNING_MODE, 10);
	}
}

void CGame::Hotkey_ToggleSoundAndMusic()
{
	if (!hb::shared::input::IsCtrlDown() || GameModeManager::GetMode() != GameMode::MainGame || TextInputManager::Get().IsActive()) {
		return;
	}
	if (AudioManager::Get().IsMusicEnabled())
	{
		AudioManager::Get().SetMusicEnabled(false);
		ConfigManager::Get().SetMusicEnabled(false);
		AudioManager::Get().StopMusic();
		AddEventList(NOTIFY_MSG_MUSIC_OFF, 10);
		return;
	}
	if (AudioManager::Get().IsSoundEnabled())
	{
		AudioManager::Get().StopSound(SoundType::Effect, 38);
		AudioManager::Get().SetSoundEnabled(false);
		ConfigManager::Get().SetSoundEnabled(false);
		AddEventList(NOTIFY_MSG_SOUND_OFF, 10);
		return;
	}
	if (AudioManager::Get().IsSoundAvailable())
	{
		AudioManager::Get().SetMusicEnabled(true);
		ConfigManager::Get().SetMusicEnabled(true);
		AddEventList(NOTIFY_MSG_MUSIC_ON, 10);
	}
	if (AudioManager::Get().IsSoundAvailable())
	{
		AudioManager::Get().SetSoundEnabled(true);
		ConfigManager::Get().SetSoundEnabled(true);
		AddEventList(NOTIFY_MSG_SOUND_ON, 10);
	}
	StartBGM();
}

void CGame::Hotkey_WhisperTarget()
{
	if (!hb::shared::input::IsCtrlDown() || GameModeManager::GetMode() != GameMode::MainGame || TextInputManager::Get().IsActive()) {
		return;
	}
	std::string tempid;

	char cLB, cRB;
	short sX, sY, msX, msY, msZ;
	sX = m_dialogBoxManager.Info(DialogBoxId::ChatHistory).sX;
	sY = m_dialogBoxManager.Info(DialogBoxId::ChatHistory).sY;
	msX = static_cast<short>(hb::shared::input::GetMouseX());

	msY = static_cast<short>(hb::shared::input::GetMouseY());

	msZ = static_cast<short>(hb::shared::input::GetMouseWheelDelta());

	cLB = hb::shared::input::IsMouseButtonDown(MouseButton::Left) ? 1 : 0;

	cRB = hb::shared::input::IsMouseButtonDown(MouseButton::Right) ? 1 : 0;
	if (m_dialogBoxManager.IsEnabled(DialogBoxId::ChatHistory) == true && (msX >= sX + 20) && (msX <= sX + 360) && (msY >= sY + 35) && (msY <= sY + 139))
	{
		char cBuff[64];
		int i = (139 - msY + sY) / 13;
		int iMsgIdx = i + m_dialogBoxManager.Info(DialogBoxId::ChatHistory).sView;
		CMsg* pChatMsg = ChatManager::Get().GetMessage(iMsgIdx);
		if (pChatMsg == nullptr) return;
		if (pChatMsg->m_pMsg[0] == ' ') { i++; pChatMsg = ChatManager::Get().GetMessage(i + m_dialogBoxManager.Info(DialogBoxId::ChatHistory).sView); }
		if (pChatMsg == nullptr) return;
		std::snprintf(cBuff, sizeof(cBuff), "%s", pChatMsg->m_pMsg);
		char* sep = std::strchr(cBuff, ':');
		if (sep) *sep = '\0';
		tempid = std::format("/to {}", cBuff);
		bSendCommand(MsgId::CommandChatMsg, 0, 0, 0, 0, 0, tempid.c_str());
	}
	else if (m_entityState.IsPlayer() && (m_entityState.m_cName[0] != '\0') && (m_iIlusionOwnerH == 0)
		&& ((m_bIsCrusadeMode == false) || !IsHostile(m_entityState.m_status.iRelationship)))
	{
		tempid = std::format("/to {}", m_entityState.m_cName.data());
		bSendCommand(MsgId::CommandChatMsg, 0, 0, 0, 0, 0, tempid.c_str());
	}
	else
	{
		TextInputManager::Get().EndInput();
		m_cChatMsg = "/to ";
		TextInputManager::Get().StartInput(CHAT_INPUT_X(), CHAT_INPUT_Y(), ChatMsgMaxLen, m_cChatMsg);
	}
}

void CGame::Hotkey_Simple_UseHealthPotion()
{
	int i = 0;
	if (m_pPlayer->m_iHP <= 0) return;
	if (m_bItemUsingStatus == true)
	{
		AddEventList(USE_RED_POTION1, 10);
		return;
	}
	if (m_dialogBoxManager.IsEnabled(DialogBoxId::Exchange) == true)
	{
		AddEventList(USE_RED_POTION2, 10);
		return;
	}
	for (i = 0; i < hb::shared::limits::MaxItems; i++)
	{
		if ((m_pItemList[i] != 0) && (m_bIsItemDisabled[i] != true))
		{
			CItem* pCfg = GetItemConfig(m_pItemList[i]->m_sIDnum);
			if (pCfg && (pCfg->m_sSprite == 6) && (pCfg->m_sSpriteFrame == 1))
			{
				bSendCommand(MsgId::CommandCommon, CommonType::ReqUseItem, 0, i, 0, 0, 0);
				m_bIsItemDisabled[i] = true;
				m_bItemUsingStatus = true;
				return;
			}
		}
	}

	for (i = 0; i < hb::shared::limits::MaxItems; i++)
	{
		if ((m_pItemList[i] != 0) && (m_bIsItemDisabled[i] != true))
		{
			CItem* pCfg = GetItemConfig(m_pItemList[i]->m_sIDnum);
			if (pCfg && (pCfg->m_sSprite == 6) && (pCfg->m_sSpriteFrame == 2))
			{
				bSendCommand(MsgId::CommandCommon, CommonType::ReqUseItem, 0, i, 0, 0, 0);
				m_bIsItemDisabled[i] = true;
				m_bItemUsingStatus = true;
				return;
			}
		}
	}
}

void CGame::Hotkey_Simple_UseManaPotion()
{
	int i = 0;
	if (m_pPlayer->m_iHP <= 0) return;
	if (m_bItemUsingStatus == true)
	{
		AddEventList(USE_BLUE_POTION1, 10);
		return;
	}
	if (m_dialogBoxManager.IsEnabled(DialogBoxId::Exchange) == true)
	{
		AddEventList(USE_BLUE_POTION2, 10);
		return;
	}

	for (i = 0; i < hb::shared::limits::MaxItems; i++)
	{
		if ((m_pItemList[i] != 0) && (m_bIsItemDisabled[i] != true))
		{
			CItem* pCfg = GetItemConfig(m_pItemList[i]->m_sIDnum);
			if (pCfg && (pCfg->m_sSprite == 6) && (pCfg->m_sSpriteFrame == 3))
			{
				bSendCommand(MsgId::CommandCommon, CommonType::ReqUseItem, 0, i, 0, 0, 0);
				m_bIsItemDisabled[i] = true;
				m_bItemUsingStatus = true;
				return;
			}
		}
	}

	for (i = 0; i < hb::shared::limits::MaxItems; i++)
	{
		if ((m_pItemList[i] != 0) && (m_bIsItemDisabled[i] != true))
		{
			CItem* pCfg = GetItemConfig(m_pItemList[i]->m_sIDnum);
			if (pCfg && (pCfg->m_sSprite == 6) && (pCfg->m_sSpriteFrame == 4))
			{
				bSendCommand(MsgId::CommandCommon, CommonType::ReqUseItem, 0, i, 0, 0, 0);
				m_bIsItemDisabled[i] = true;
				m_bItemUsingStatus = true;
				return;
			}
		}
	}
}

void CGame::Hotkey_Simple_LoadBackupChat()
{
	if (((m_dialogBoxManager.IsEnabled(DialogBoxId::GuildMenu) == true) && (m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode == 1) && (m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::GuildMenu)) ||
		((m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropExternal) == true) && (m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cMode == 1) && (m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::ItemDropExternal)))
	{
		return;
	}
	if ((!TextInputManager::Get().IsActive()) && (m_cBackupChatMsg[0] != '!') && (m_cBackupChatMsg[0] != '~') && (m_cBackupChatMsg[0] != '^') &&
		(m_cBackupChatMsg[0] != '@'))
	{
		m_cChatMsg = m_cBackupChatMsg;
		TextInputManager::Get().StartInput(CHAT_INPUT_X(), CHAT_INPUT_Y(), ChatMsgMaxLen, m_cChatMsg);
	}
}

void CGame::Hotkey_Simple_UseMagicShortcut()
{
	if (GameModeManager::GetMode() != GameMode::MainGame) return;
	MagicCastingSystem::Get().BeginCast(m_sMagicShortCut);
}

void CGame::Hotkey_Simple_ToggleCharacterInfo()
{
	if (m_dialogBoxManager.IsEnabled(DialogBoxId::CharacterInfo) == false)
		m_dialogBoxManager.EnableDialogBox(DialogBoxId::CharacterInfo, 0, 0, 0);
	else m_dialogBoxManager.DisableDialogBox(DialogBoxId::CharacterInfo);
}

void CGame::Hotkey_Simple_ToggleInventory()
{
	if (m_dialogBoxManager.IsEnabled(DialogBoxId::Inventory) == false)
		m_dialogBoxManager.EnableDialogBox(DialogBoxId::Inventory, 0, 0, 0);
	else m_dialogBoxManager.DisableDialogBox(DialogBoxId::Inventory);
}

void CGame::Hotkey_Simple_ToggleMagic()
{
	if (m_dialogBoxManager.IsEnabled(DialogBoxId::Magic) == false)
		m_dialogBoxManager.EnableDialogBox(DialogBoxId::Magic, 0, 0, 0);
	else m_dialogBoxManager.DisableDialogBox(DialogBoxId::Magic);
}

void CGame::Hotkey_Simple_ToggleSkill()
{
	if (m_dialogBoxManager.IsEnabled(DialogBoxId::Skill) == false)
		m_dialogBoxManager.EnableDialogBox(DialogBoxId::Skill, 0, 0, 0);
	else m_dialogBoxManager.DisableDialogBox(DialogBoxId::Skill);
}

void CGame::Hotkey_Simple_ToggleChatHistory()
{
	if (m_dialogBoxManager.IsEnabled(DialogBoxId::ChatHistory) == false)
		m_dialogBoxManager.EnableDialogBox(DialogBoxId::ChatHistory, 0, 0, 0);
	else m_dialogBoxManager.DisableDialogBox(DialogBoxId::ChatHistory);
}

void CGame::Hotkey_Simple_ToggleSystemMenu()
{
	if (TextInputManager::Get().IsActive()) return;
	if (m_dialogBoxManager.IsEnabled(DialogBoxId::SystemMenu) == false)
		m_dialogBoxManager.EnableDialogBox(DialogBoxId::SystemMenu, 0, 0, 0);
	else m_dialogBoxManager.DisableDialogBox(DialogBoxId::SystemMenu);
}

void CGame::Hotkey_Simple_UseShortcut1()
{
	UseShortCut(1);
}

void CGame::Hotkey_Simple_UseShortcut2()
{
	UseShortCut(2);
}

void CGame::Hotkey_Simple_UseShortcut3()
{
	UseShortCut(3);
}

void CGame::Hotkey_Simple_WhisperCycleUp()
{
	m_cArrowPressed = 1;
	if (GameModeManager::GetMode() == GameMode::MainGame)
	{
		ChatManager::Get().CycleWhisperUp();
		int idx = ChatManager::Get().GetWhisperIndex();
		const char* name = ChatManager::Get().GetWhisperTargetName(idx);
		if (name != nullptr) {
			TextInputManager::Get().EndInput();
			m_cChatMsg = std::format("/to {}", name);
			TextInputManager::Get().StartInput(CHAT_INPUT_X(), CHAT_INPUT_Y(), ChatMsgMaxLen, m_cChatMsg);
		}
	}
}

void CGame::Hotkey_Simple_WhisperCycleDown()
{
	m_cArrowPressed = 3;
	if (GameModeManager::GetMode() == GameMode::MainGame)
	{
		ChatManager::Get().CycleWhisperDown();
		int idx = ChatManager::Get().GetWhisperIndex();
		const char* name = ChatManager::Get().GetWhisperTargetName(idx);
		if (name != nullptr) {
			TextInputManager::Get().EndInput();
			m_cChatMsg = std::format("/to {}", name);
			TextInputManager::Get().StartInput(CHAT_INPUT_X(), CHAT_INPUT_Y(), ChatMsgMaxLen, m_cChatMsg);
		}
	}
}

void CGame::Hotkey_Simple_ArrowLeft()
{
	m_cArrowPressed = 4;
}

void CGame::Hotkey_Simple_ArrowRight()
{
	m_cArrowPressed = 2;
}

void CGame::Hotkey_Simple_Screenshot()
{
	CreateScreenShot();
}

void CGame::Hotkey_Simple_TabToggleCombat()
{
	if (hb::shared::input::IsShiftDown())
	{
		m_cCurFocus--;
		if (m_cCurFocus < 1) m_cCurFocus = m_cMaxFocus;
	}
	else
	{
		m_cCurFocus++;
		if (m_cCurFocus > m_cMaxFocus) m_cCurFocus = 1;
	}
	if (GameModeManager::GetMode() == GameMode::MainGame)
	{
		bSendCommand(MsgId::CommandCommon, CommonType::ToggleCombatMode, 0, 0, 0, 0, 0);
	}
}

void CGame::Hotkey_Simple_ToggleSafeAttack()
{
	if (GameModeManager::GetMode() == GameMode::MainGame) {
		bSendCommand(MsgId::CommandCommon, CommonType::ToggleSafeAttackMode, 0, 0, 0, 0, 0);
	}
}

void CGame::Hotkey_Simple_Escape()
{
	// Note: Escape handling is automatic through hb::shared::input::IsKeyPressed(KeyCode::Escape)
	if (GameModeManager::GetMode() == GameMode::MainGame)
	{
		if ((m_bIsObserverMode == true) && (hb::shared::input::IsShiftDown())) {
			if (m_cLogOutCount == -1) m_cLogOutCount = 1;
			m_dialogBoxManager.DisableDialogBox(DialogBoxId::SystemMenu);
			PlayGameSound('E', 14, 5);
		}
		else if (m_cLogOutCount != -1) {
			if (m_bForceDisconn == false) {
				m_cLogOutCount = -1;
				AddEventList(DLGBOX_CLICK_SYSMENU2, 10);
			}
		}
		if (m_bIsGetPointingMode == true) {
			m_bIsGetPointingMode = false;
			AddEventList(COMMAND_PROCESSOR1, 10);
		}
		m_bIsF1HelpWindowEnabled = false;
	}
}

void CGame::Hotkey_Simple_SpecialAbility()
{
	int i = 0;
	uint32_t dwTime = GameClock::GetTimeMS();
	if (GameModeManager::GetMode() != GameMode::MainGame) return;
	if (TextInputManager::Get().IsActive()) return;
	if (m_pPlayer->m_bIsSpecialAbilityEnabled == true)
	{
		if (m_pPlayer->m_iSpecialAbilityType != 0) {
			bSendCommand(MsgId::CommandCommon, CommonType::RequestActivateSpecAbility, 0, 0, 0, 0, 0);
			m_pPlayer->m_bIsSpecialAbilityEnabled = false;
		}
		else AddEventList(ON_KEY_UP26, 10);
	}
	else {
		if (m_pPlayer->m_iSpecialAbilityType == 0) AddEventList(ON_KEY_UP26, 10);
		else {
			std::string G_cTxt;
			if (m_pPlayer->m_playerAppearance.iEffectType != 0) {
				AddEventList(ON_KEY_UP28, 10);
				return;
			}

			i = (dwTime - m_dwSpecialAbilitySettingTime) / 1000;
			i = m_pPlayer->m_iSpecialAbilityTimeLeftSec - i;
			if (i < 0) i = 0;

			if (i < 60) {
				switch (m_pPlayer->m_iSpecialAbilityType) {
				case 1: G_cTxt = std::format(ON_KEY_UP29, i); break;//"
				case 2: G_cTxt = std::format(ON_KEY_UP30, i); break;//"
				case 3: G_cTxt = std::format(ON_KEY_UP31, i); break;//"
				case 4: G_cTxt = std::format(ON_KEY_UP32, i); break;//"
				case 5: G_cTxt = std::format(ON_KEY_UP33, i); break;//"
				case 50:G_cTxt = std::format(ON_KEY_UP34, i); break;//"
				case 51:G_cTxt = std::format(ON_KEY_UP35, i); break;//"
				case 52:G_cTxt = std::format(ON_KEY_UP36, i); break;//"
				}
			}
			else {
				switch (m_pPlayer->m_iSpecialAbilityType) {
				case 1: G_cTxt = std::format(ON_KEY_UP37, i / 60); break;//"
				case 2: G_cTxt = std::format(ON_KEY_UP38, i / 60); break;//"
				case 3: G_cTxt = std::format(ON_KEY_UP39, i / 60); break;//"
				case 4: G_cTxt = std::format(ON_KEY_UP40, i / 60); break;//"
				case 5: G_cTxt = std::format(ON_KEY_UP41, i / 60); break;//"
				case 50:G_cTxt = std::format(ON_KEY_UP42, i / 60); break;//"
				case 51:G_cTxt = std::format(ON_KEY_UP43, i / 60); break;//"
				case 52:G_cTxt = std::format(ON_KEY_UP44, i / 60); break;//"
				}
			}
			AddEventList(G_cTxt.c_str(), 10);
		}
	}
}

void CGame::Hotkey_Simple_ZoomIn()
{
	if (TextInputManager::Get().IsActive() == false)
	{
		ConfigManager::Get().SetZoomMapEnabled(true);
	}
}

void CGame::Hotkey_Simple_ZoomOut()
{
	if (TextInputManager::Get().IsActive() == false)
	{
		ConfigManager::Get().SetZoomMapEnabled(false);
	}
}
