#include "DialogBox_HudPanel.h"
#include "Game.h"
#include "GlobalDef.h"
#include "SharedCalculations.h"
#include "CursorTarget.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include <cstdlib>

// Static button data shared between draw and click handling
const DialogBox_HudPanel::ToggleButtonInfo DialogBox_HudPanel::TOGGLE_BUTTONS[] = {
	{ 410, 447, 412, 6, "Character",   DialogBoxId::CharacterInfo },
	{ 447, 484, 449, 7, "Inventory",   DialogBoxId::Inventory },
	{ 484, 521, 486, 8, "Magics",      DialogBoxId::Magic },
	{ 521, 558, 523, 9, "Skills",      DialogBoxId::Skill },
	{ 558, 595, 560, 10, "Chat Log",   DialogBoxId::ChatHistory },
	{ 595, 631, 597, 11, "System Menu", DialogBoxId::SystemMenu }
};

DialogBox_HudPanel::DialogBox_HudPanel(CGame* pGame)
	: IDialogBox(DialogBoxId::HudPanel, pGame)
{
	SetDefaultRect(0, LOGICAL_HEIGHT() - ICON_PANEL_HEIGHT(), ICON_PANEL_WIDTH(), ICON_PANEL_HEIGHT());
}

bool DialogBox_HudPanel::IsInButton(short msX, short msY, int x1, int x2) const
{
	return (msX > x1) && (msX < x2) && (msY > BTN_Y1()) && (msY < BTN_Y2());
}

void DialogBox_HudPanel::ToggleDialogWithSound(DialogBoxId::Type dialogId)
{
	if (m_pGame->m_dialogBoxManager.IsEnabled(dialogId))
		m_pGame->m_dialogBoxManager.DisableDialogBox(dialogId);
	else
		EnableDialogBox(dialogId, 0, 0, 0);
	m_pGame->PlaySound('E', 14, 5);
}

void DialogBox_HudPanel::DrawGaugeBars()
{
	int iMaxPoint, iBarWidth;
	uint32_t dwTime = m_pGame->m_dwCurTime;
	auto pSprite = m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_ICONPANNEL];

	// HP bar
	iMaxPoint = CalculateMaxHP(m_pGame->m_pPlayer->m_iVit, m_pGame->m_pPlayer->m_iLevel,
	                           m_pGame->m_pPlayer->m_iStr, m_pGame->m_pPlayer->m_iAngelicStr);
	if (m_pGame->m_pPlayer->m_iHP > iMaxPoint) m_pGame->m_pPlayer->m_iHP = iMaxPoint;
	iBarWidth = HP_MP_BAR_WIDTH - (m_pGame->m_pPlayer->m_iHP * HP_MP_BAR_WIDTH) / iMaxPoint;
	if (iBarWidth < 0) iBarWidth = 0;
	if (iBarWidth > HP_MP_BAR_WIDTH) iBarWidth = HP_MP_BAR_WIDTH;
	pSprite->DrawWidth(HP_BAR_X(), HP_BAR_Y(), 12, iBarWidth);

	// HP number
	char statBuf[16];
	snprintf(statBuf, sizeof(statBuf), "%d", (short)m_pGame->m_pPlayer->m_iHP);
	if (m_pGame->m_pPlayer->m_bIsPoisoned)
	{
		TextLib::DrawText(GameFont::Numbers, 85 + HudXOffset(), HP_NUM_Y(), statBuf,
			TextLib::TextStyle::Color(GameColors::PoisonText.r, GameColors::PoisonText.g, GameColors::PoisonText.b));
		TextLib::DrawText(GameFont::SprFont3_2, 35 + HudXOffset(), HP_BAR_Y() + 2, "Poisoned",
			TextLib::TextStyle::Color(GameColors::PoisonLabel.r, GameColors::PoisonLabel.g, GameColors::PoisonLabel.b).WithAlpha(0.7f));
	}
	else
	{
		TextLib::DrawText(GameFont::Numbers, HP_NUM_X() + 1, HP_NUM_Y() + 1, statBuf, TextLib::TextStyle::Color(GameColors::UIBlack.r, GameColors::UIBlack.g, GameColors::UIBlack.b));
		TextLib::DrawText(GameFont::Numbers, HP_NUM_X(), HP_NUM_Y(), statBuf, TextLib::TextStyle::Color(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b));
	}

	// MP bar
	iMaxPoint = CalculateMaxMP(m_pGame->m_pPlayer->m_iMag, m_pGame->m_pPlayer->m_iAngelicMag,
	                           m_pGame->m_pPlayer->m_iLevel, m_pGame->m_pPlayer->m_iInt, m_pGame->m_pPlayer->m_iAngelicInt);
	if (m_pGame->m_pPlayer->m_iMP > iMaxPoint) m_pGame->m_pPlayer->m_iMP = iMaxPoint;
	iBarWidth = HP_MP_BAR_WIDTH - (m_pGame->m_pPlayer->m_iMP * HP_MP_BAR_WIDTH) / iMaxPoint;
	if (iBarWidth < 0) iBarWidth = 0;
	if (iBarWidth > HP_MP_BAR_WIDTH) iBarWidth = HP_MP_BAR_WIDTH;
	pSprite->DrawWidth(HP_BAR_X(), MP_BAR_Y(), 12, iBarWidth);

	// MP number
	snprintf(statBuf, sizeof(statBuf), "%d", (short)m_pGame->m_pPlayer->m_iMP);
	TextLib::DrawText(GameFont::Numbers, HP_NUM_X() + 1, MP_NUM_Y() + 1, statBuf, TextLib::TextStyle::Color(GameColors::UIBlack.r, GameColors::UIBlack.g, GameColors::UIBlack.b));
	TextLib::DrawText(GameFont::Numbers, HP_NUM_X(), MP_NUM_Y(), statBuf, TextLib::TextStyle::Color(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b));

	// SP bar
	iMaxPoint = CalculateMaxSP(m_pGame->m_pPlayer->m_iStr, m_pGame->m_pPlayer->m_iAngelicStr, m_pGame->m_pPlayer->m_iLevel);
	if (m_pGame->m_pPlayer->m_iSP > iMaxPoint) m_pGame->m_pPlayer->m_iSP = iMaxPoint;
	iBarWidth = SP_BAR_WIDTH - (m_pGame->m_pPlayer->m_iSP * SP_BAR_WIDTH) / iMaxPoint;
	if (iBarWidth < 0) iBarWidth = 0;
	if (iBarWidth > SP_BAR_WIDTH) iBarWidth = SP_BAR_WIDTH;
	pSprite->DrawWidth(SP_BAR_X(), SP_BAR_Y(), 13, iBarWidth);

	// SP number
	snprintf(statBuf, sizeof(statBuf), "%d", (short)m_pGame->m_pPlayer->m_iSP);
	TextLib::DrawText(GameFont::Numbers, SP_NUM_X() + 1, SP_NUM_Y() + 1, statBuf, TextLib::TextStyle::Color(GameColors::UIBlack.r, GameColors::UIBlack.g, GameColors::UIBlack.b));
	TextLib::DrawText(GameFont::Numbers, SP_NUM_X(), SP_NUM_Y(), statBuf, TextLib::TextStyle::Color(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b));

	// Experience bar
	uint32_t iCurLevelExp = m_pGame->iGetLevelExp(m_pGame->m_pPlayer->m_iLevel);
	uint32_t iNextLevelExp = m_pGame->iGetLevelExp(m_pGame->m_pPlayer->m_iLevel + 1);
	uint32_t iExpRange = iNextLevelExp - iCurLevelExp;
	uint32_t iExpProgress = m_pGame->m_pPlayer->m_iExp - iCurLevelExp;
	iBarWidth = (iExpProgress * ICON_PANEL_WIDTH()) / iExpRange;
	if (iBarWidth < 0) iBarWidth = 0;
	if (iBarWidth > ICON_PANEL_WIDTH()) iBarWidth = ICON_PANEL_WIDTH();
	pSprite->DrawWidth(HudXOffset(), EXP_BAR_Y(), 18, iBarWidth);
}

void DialogBox_HudPanel::DrawStatusIcons(short msX, short msY)
{
	uint32_t dwTime = m_pGame->m_dwCurTime;
	auto pSprite = m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_ICONPANNEL];

	// Level up / Restart text
	if (m_pGame->m_pPlayer->m_iHP > 0)
	{
		if ((m_pGame->m_pPlayer->m_iLU_Point > 0) && !m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::LevelUpSetting))
		{
			int flashColor = (GameClock::GetTimeMS() / 3) % 255;
			TextLib::DrawText(GameFont::Bitmap1, LEVELUP_TEXT_X(), LEVELUP_TEXT_Y(), "Level Up!", TextLib::TextStyle::WithIntegratedShadow(flashColor, flashColor, 0));
		}
	}
	else if (m_pGame->m_cRestartCount == -1)
	{
		int flashColor = (GameClock::GetTimeMS() / 3) % 255;
		TextLib::DrawText(GameFont::Bitmap1, LEVELUP_TEXT_X(), LEVELUP_TEXT_Y(), "Restart", TextLib::TextStyle::WithIntegratedShadow(flashColor, flashColor, 0));
	}

	// Combat mode / Safe attack icon
	if (m_pGame->m_pPlayer->m_bIsSafeAttackMode)
		pSprite->Draw(COMBAT_ICON_X(), COMBAT_ICON_Y(), 4);
	else if (m_pGame->m_pPlayer->m_bIsCombatMode)
		pSprite->Draw(COMBAT_ICON_X(), COMBAT_ICON_Y(), 5);

	// Combat mode button hover
	if (IsInButton(msX, msY, BTN_COMBAT_X1(), BTN_COMBAT_X2()))
	{
		pSprite->Draw(BTN_COMBAT_X1(), BTN_Y1(), 16);
		const char* tooltip = m_pGame->m_pPlayer->m_bIsCombatMode
			? (m_pGame->m_pPlayer->m_bIsSafeAttackMode ? "Safe Attack" : "Attack")
			: "Peace";
		PutString(msX - 10, msY - 20, tooltip, GameColors::UITooltip.ToColorRef());
	}

	// Crusade icon
	if (m_pGame->m_bIsCrusadeMode && m_pGame->m_pPlayer->m_iCrusadeDuty != 0)
	{
		bool bHover = IsInButton(msX, msY, BTN_CRUSADE_X1(), BTN_CRUSADE_X2());
		if (m_pGame->m_pPlayer->m_bAresden)
			pSprite->Draw(BTN_CRUSADE_X1() + (bHover ? 1 : 0), BTN_Y1(), bHover ? 1 : 2);
		else
			pSprite->Draw(BTN_CRUSADE_X1(), BTN_Y1(), bHover ? 0 : 15);
	}

	// Map message / coordinates (or remaining EXP when Ctrl pressed)
	char infoBuf[128];
	if (Input::IsCtrlDown())
	{
		uint32_t iCurExp = m_pGame->iGetLevelExp(m_pGame->m_pPlayer->m_iLevel);
		uint32_t iNextExp = m_pGame->iGetLevelExp(m_pGame->m_pPlayer->m_iLevel + 1);
		if (m_pGame->m_pPlayer->m_iExp < iNextExp)
		{
			uint32_t iExpRange = iNextExp - iCurExp;
			uint32_t iExpProgress = (m_pGame->m_pPlayer->m_iExp > iCurExp) ? (m_pGame->m_pPlayer->m_iExp - iCurExp) : 0;
			snprintf(infoBuf, sizeof(infoBuf), "Rest Exp: %d", iExpRange - iExpProgress);
		}
		else
		{
			infoBuf[0] = '\0';
		}
	}
	else
	{
		snprintf(infoBuf, sizeof(infoBuf), "%s (%d,%d)", m_pGame->m_cMapMessage, m_pGame->m_pPlayer->m_sPlayerX, m_pGame->m_pPlayer->m_sPlayerY);
	}
	PutAlignedString(MAP_MSG_X1() + 1, MAP_MSG_X2() + 1, MAP_MSG_Y() + 1, infoBuf, GameColors::UIBlack.r, GameColors::UIBlack.g, GameColors::UIBlack.b);
	PutAlignedString(MAP_MSG_X1(), MAP_MSG_X2(), MAP_MSG_Y(), infoBuf, GameColors::UIPaleYellow.r, GameColors::UIPaleYellow.g, GameColors::UIPaleYellow.b);
}

void DialogBox_HudPanel::DrawIconButtons(short msX, short msY)
{
	if (msY <= BTN_Y1() || msY >= BTN_Y2()) return;

	uint32_t dwTime = m_pGame->m_dwCurTime;
	auto pSprite = m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_ICONPANNEL];
	int xOffset = HudXOffset();

	for (int i = 0; i < TOGGLE_BUTTON_COUNT; i++)
	{
		const auto& btn = TOGGLE_BUTTONS[i];
		// Add X offset for wider resolutions
		if (msX > btn.x1 + xOffset && msX < btn.x2 + xOffset)
		{
			pSprite->Draw(btn.spriteX + xOffset, BTN_Y1(), btn.spriteFrame);
			int tooltipOffset = (btn.dialogId == DialogBoxId::SystemMenu) ? -20 : -10;
			PutString(msX + tooltipOffset, msY - 20, btn.tooltip, GameColors::UITooltip.ToColorRef());
			break;
		}
	}
}

void DialogBox_HudPanel::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short panelY = Info().sY;

	// Draw main HUD background centered (at xOffset, which is 0 for 640x480, 80 for 800x600)
	m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_ICONPANNEL]->Draw(0, panelY, 14);

	DrawGaugeBars();
	DrawStatusIcons(msX, msY);
	DrawIconButtons(msX, msY);
}

bool DialogBox_HudPanel::OnClick(short msX, short msY)
{
	// Crusade button
	if (IsInButton(msX, msY, BTN_CRUSADE_X1(), BTN_CRUSADE_X2()))
	{
		if (!m_pGame->m_bIsCrusadeMode) return false;
		switch (m_pGame->m_pPlayer->m_iCrusadeDuty)
		{
		case 1: EnableDialogBox(DialogBoxId::CrusadeSoldier, 0, 0, 0); break;
		case 2: EnableDialogBox(DialogBoxId::CrusadeConstructor, 0, 0, 0); break;
		case 3: EnableDialogBox(DialogBoxId::CrusadeCommander, 0, 0, 0); break;
		default: return false;
		}
		m_pGame->PlaySound('E', 14, 5);
		return true;
	}

	// Combat mode toggle
	if (IsInButton(msX, msY, BTN_COMBAT_X1(), BTN_COMBAT_X2()))
	{
		m_pGame->bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_TOGGLECOMBATMODE, 0, 0, 0, 0, 0);
		m_pGame->PlaySound('E', 14, 5);
		return true;
	}

	// Dialog toggle buttons (Character, Inventory, Magic, Skill, Chat, System)
	int xOffset = HudXOffset();
	for (int i = 0; i < TOGGLE_BUTTON_COUNT; i++)
	{
		const auto& btn = TOGGLE_BUTTONS[i];
		// Add X offset for wider resolutions
		if (IsInButton(msX, msY, btn.x1 + xOffset, btn.x2 + xOffset))
		{
			ToggleDialogWithSound(btn.dialogId);
			return true;
		}
	}

	return false;
}

bool DialogBox_HudPanel::OnItemDrop(short msX, short msY)
{
	short sItemIndex = CursorTarget::GetSelectedID();
	if (m_pGame->m_bIsItemDisabled[sItemIndex]) return true;
	if (m_pGame->m_pPlayer->m_Controller.GetCommand() < 0) return true;

	int xOffset = HudXOffset();
	int yOffset = HudYOffset();

	// Inventory icon area - drop item into inventory
	if ((453 + xOffset < msX) && (486 + xOffset > msX) && (440 + yOffset < msY) && (475 + yOffset > msY))
	{
		auto& invInfo = InfoOf(DialogBoxId::Inventory);
		m_pGame->m_dialogBoxManager.GetDialogBox(DialogBoxId::Inventory)->OnItemDrop(invInfo.sX + (rand() % 148), invInfo.sY + (rand() % 55));
		return true;
	}

	// Character icon area - equip item
	if ((425 + xOffset < msX) && (448 + xOffset > msX) && (440 + yOffset < msY) && (475 + yOffset > msY))
	{
		m_pGame->m_dialogBoxManager.GetDialogBox(DialogBoxId::CharacterInfo)->OnItemDrop(msX, msY);
		return true;
	}

	return true;
}
