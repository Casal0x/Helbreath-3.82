#include "DialogBox_LevelUpSetting.h"
#include "Game.h"
#include "lan_eng.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
DialogBox_LevelUpSetting::DialogBox_LevelUpSetting(CGame* pGame)
	: IDialogBox(DialogBoxId::LevelUpSetting, pGame)
{
	SetDefaultRect(0 , 0 , 258, 339);
}

void DialogBox_LevelUpSetting::DrawStatRow(short sX, short sY, int iYOffset, const char* pLabel,
                                            int iCurrentStat, int iPendingChange, short msX, short msY,
                                            int iArrowYOffset, bool bCanIncrease, bool bCanDecrease)
{
	std::string cTxt;
	uint32_t dwTime = m_pGame->m_dwCurTime;

	// Stat label
	PutString(sX + 24, sY + iYOffset, (char*)pLabel, GameColors::UIBlack);

	// Current value
	cTxt = std::format("{}", iCurrentStat);
	PutString(sX + 109, sY + iYOffset, cTxt.c_str(), GameColors::UILabel);

	// New value (with pending changes)
	int iNewStat = iCurrentStat + iPendingChange;
	cTxt = std::format("{}", iNewStat);
	if (iNewStat != iCurrentStat)
		PutString(sX + 162, sY + iYOffset, cTxt.c_str(), GameColors::UIRed);
	else
		PutString(sX + 162, sY + iYOffset, cTxt.c_str(), GameColors::UILabel);

	// + arrow highlight
	if ((msX >= sX + 195) && (msX <= sX + 205) && (msY >= sY + iArrowYOffset) && (msY <= sY + iArrowYOffset + 6) && bCanIncrease)
		m_pGame->m_pSprite[InterfaceNdGame4]->Draw(sX + 195, sY + iArrowYOffset, 5);

	// - arrow highlight
	if ((msX >= sX + 210) && (msX <= sX + 220) && (msY >= sY + iArrowYOffset) && (msY <= sY + iArrowYOffset + 6) && bCanDecrease)
		m_pGame->m_pSprite[InterfaceNdGame4]->Draw(sX + 210, sY + iArrowYOffset, 6);
}

void DialogBox_LevelUpSetting::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX = Info().sX;
	short sY = Info().sY;
	short szX = Info().sSizeX;
	uint32_t dwTime = m_pGame->m_dwCurTime;
	std::string cTxt;

	DrawNewDialogBox(InterfaceNdGame2, sX, sY, 0);
	DrawNewDialogBox(InterfaceNdText, sX, sY, 2);
	DrawNewDialogBox(InterfaceNdGame4, sX + 16, sY + 100, 4);

	// Header text
	PutAlignedString(sX, sX + szX, sY + 50, DRAW_DIALOGBOX_LEVELUP_SETTING1);
	PutAlignedString(sX, sX + szX, sY + 65, DRAW_DIALOGBOX_LEVELUP_SETTING2);

	// Points Left
	PutString(sX + 20, sY + 85, DRAW_DIALOGBOX_LEVELUP_SETTING3, GameColors::UIBlack);
	cTxt = std::format("{}", m_pGame->m_pPlayer->m_iLU_Point);
	if (m_pGame->m_pPlayer->m_iLU_Point > 0)
		PutString(sX + 73, sY + 102, cTxt.c_str(), GameColors::UIGreen);
	else
		PutString(sX + 73, sY + 102, cTxt.c_str(), GameColors::UIBlack);

	// Draw stat rows
	DrawStatRow(sX, sY, 125, DRAW_DIALOGBOX_LEVELUP_SETTING4, m_pGame->m_pPlayer->m_iStr, m_pGame->m_pPlayer->m_wLU_Str,
	            msX, msY, 127, (m_pGame->m_pPlayer->m_iStr < m_pGame->iMaxStats), (m_pGame->m_pPlayer->m_wLU_Str > 0));

	DrawStatRow(sX, sY, 144, DRAW_DIALOGBOX_LEVELUP_SETTING5, m_pGame->m_pPlayer->m_iVit, m_pGame->m_pPlayer->m_wLU_Vit,
	            msX, msY, 146, (m_pGame->m_pPlayer->m_iVit < m_pGame->iMaxStats), (m_pGame->m_pPlayer->m_wLU_Vit > 0));

	DrawStatRow(sX, sY, 163, DRAW_DIALOGBOX_LEVELUP_SETTING6, m_pGame->m_pPlayer->m_iDex, m_pGame->m_pPlayer->m_wLU_Dex,
	            msX, msY, 165, (m_pGame->m_pPlayer->m_iDex < m_pGame->iMaxStats), (m_pGame->m_pPlayer->m_wLU_Dex > 0));

	DrawStatRow(sX, sY, 182, DRAW_DIALOGBOX_LEVELUP_SETTING7, m_pGame->m_pPlayer->m_iInt, m_pGame->m_pPlayer->m_wLU_Int,
	            msX, msY, 184, (m_pGame->m_pPlayer->m_iInt < m_pGame->iMaxStats), (m_pGame->m_pPlayer->m_wLU_Int > 0));

	DrawStatRow(sX, sY, 201, DRAW_DIALOGBOX_LEVELUP_SETTING8, m_pGame->m_pPlayer->m_iMag, m_pGame->m_pPlayer->m_wLU_Mag,
	            msX, msY, 203, (m_pGame->m_pPlayer->m_iMag < m_pGame->iMaxStats), (m_pGame->m_pPlayer->m_wLU_Mag > 0));

	DrawStatRow(sX, sY, 220, DRAW_DIALOGBOX_LEVELUP_SETTING9, m_pGame->m_pPlayer->m_iCharisma, m_pGame->m_pPlayer->m_wLU_Char,
	            msX, msY, 222, (m_pGame->m_pPlayer->m_iCharisma < m_pGame->iMaxStats), (m_pGame->m_pPlayer->m_wLU_Char > 0));

	// Close button
	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) &&
	    (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
		DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
	else
		DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);

	// Majestic button (only if no pending changes and no points left)
	if ((m_pGame->m_pPlayer->m_wLU_Str == 0) && (m_pGame->m_pPlayer->m_wLU_Vit == 0) && (m_pGame->m_pPlayer->m_wLU_Dex == 0) &&
	    (m_pGame->m_pPlayer->m_wLU_Int == 0) && (m_pGame->m_pPlayer->m_wLU_Mag == 0) && (m_pGame->m_pPlayer->m_wLU_Char == 0))
	{
		if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) &&
		    (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
		{
			if (m_pGame->m_pPlayer->m_iLU_Point <= 0)
				DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 21);
		}
		else
		{
			if (m_pGame->m_pPlayer->m_iLU_Point <= 0)
				DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 20);
		}
	}
}

bool DialogBox_LevelUpSetting::HandleStatClick(short msX, short msY, short sX, short sY,
                                                int iYOffset, int& iCurrentStat, int16_t& cPendingChange)
{
	bool bMajesticOpen = m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ChangeStatsMajestic);

	// + button
	if ((msX >= sX + 195) && (msX <= sX + 205) && (msY >= sY + iYOffset) && (msY <= sY + iYOffset + 6) &&
	    (iCurrentStat <= m_pGame->iMaxStats) && (m_pGame->m_pPlayer->m_iLU_Point > 0))
	{
		if (hb::shared::input::is_ctrl_down())
		{
			if ((m_pGame->m_pPlayer->m_iLU_Point >= 5) && !bMajesticOpen)
			{
				m_pGame->m_pPlayer->m_iLU_Point -= 5;
				cPendingChange += 5;
			}
		}
		else
		{
			if ((m_pGame->m_pPlayer->m_iLU_Point > 0) && !bMajesticOpen)
			{
				m_pGame->m_pPlayer->m_iLU_Point--;
				cPendingChange++;
			}
		}
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	// - button
	if ((msX >= sX + 210) && (msX <= sX + 220) && (msY >= sY + iYOffset) && (msY <= sY + iYOffset + 6) &&
	    (cPendingChange > 0))
	{
		if (hb::shared::input::is_ctrl_down())
		{
			if ((cPendingChange >= 5) && !bMajesticOpen)
			{
				cPendingChange -= 5;
				m_pGame->m_pPlayer->m_iLU_Point += 5;
			}
		}
		else
		{
			if ((cPendingChange > 0) && !bMajesticOpen)
			{
				cPendingChange--;
				m_pGame->m_pPlayer->m_iLU_Point++;
			}
		}
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	return false;
}

bool DialogBox_LevelUpSetting::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	// Strength +/-
	if (HandleStatClick(msX, msY, sX, sY, 127, m_pGame->m_pPlayer->m_iStr, m_pGame->m_pPlayer->m_wLU_Str))
		return true;

	// Vitality +/-
	if (HandleStatClick(msX, msY, sX, sY, 146, m_pGame->m_pPlayer->m_iVit, m_pGame->m_pPlayer->m_wLU_Vit))
		return true;

	// Dexterity +/-
	if (HandleStatClick(msX, msY, sX, sY, 165, m_pGame->m_pPlayer->m_iDex, m_pGame->m_pPlayer->m_wLU_Dex))
		return true;

	// Intelligence +/-
	if (HandleStatClick(msX, msY, sX, sY, 184, m_pGame->m_pPlayer->m_iInt, m_pGame->m_pPlayer->m_wLU_Int))
		return true;

	// Magic +/-
	if (HandleStatClick(msX, msY, sX, sY, 203, m_pGame->m_pPlayer->m_iMag, m_pGame->m_pPlayer->m_wLU_Mag))
		return true;

	// Charisma +/-
	if (HandleStatClick(msX, msY, sX, sY, 222, m_pGame->m_pPlayer->m_iCharisma, m_pGame->m_pPlayer->m_wLU_Char))
		return true;

	// Close/OK button
	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) &&
	    (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		if (Info().sV1 != m_pGame->m_pPlayer->m_iLU_Point)
			bSendCommand(MsgId::LevelUpSettings, 0, 0, 0, 0, 0, 0);
		DisableThisDialog();
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	// Majestic button
	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) &&
	    (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		if ((m_pGame->m_iGizonItemUpgradeLeft > 0) && (m_pGame->m_pPlayer->m_iLU_Point <= 0) &&
		    (m_pGame->m_pPlayer->m_wLU_Str == 0) && (m_pGame->m_pPlayer->m_wLU_Vit == 0) && (m_pGame->m_pPlayer->m_wLU_Dex == 0) &&
		    (m_pGame->m_pPlayer->m_wLU_Int == 0) && (m_pGame->m_pPlayer->m_wLU_Mag == 0) && (m_pGame->m_pPlayer->m_wLU_Char == 0))
		{
			DisableThisDialog();
			EnableDialogBox(DialogBoxId::ChangeStatsMajestic, 0, 0, 0);
			return true;
		}
	}

	return false;
}
