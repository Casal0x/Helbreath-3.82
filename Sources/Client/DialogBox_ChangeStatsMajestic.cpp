#include "DialogBox_ChangeStatsMajestic.h"
#include "Game.h"
#include "lan_eng.h"
#include "GlobalDef.h"
#include "SpriteID.h"
#include "NetMessages.h"

static constexpr int POINTS_PER_MAJESTIC = 3;
static constexpr int MIN_STAT_VALUE = 10;

DialogBox_ChangeStatsMajestic::DialogBox_ChangeStatsMajestic(CGame* pGame)
	: IDialogBox(DialogBoxId::ChangeStatsMajestic, pGame)
{
	SetDefaultRect(0 + SCREENX(), 0 + SCREENY(), 258, 339);
}

static int GetPendingMajesticCost(CGame* pGame)
{
	int iTotalReduction = -(pGame->m_pPlayer->m_wLU_Str + pGame->m_pPlayer->m_wLU_Vit +
		pGame->m_pPlayer->m_wLU_Dex + pGame->m_pPlayer->m_wLU_Int +
		pGame->m_pPlayer->m_wLU_Mag + pGame->m_pPlayer->m_wLU_Char);
	return iTotalReduction / POINTS_PER_MAJESTIC;
}

void DialogBox_ChangeStatsMajestic::DrawStatRow(short sX, short sY, int iYOffset,
	const char* pLabel, int iCurrentStat, int16_t iPendingChange,
	short msX, short msY, int iArrowYOffset, bool bCanUndo, bool bCanReduce)
{
	char cTxt[120];

	PutString(sX + 24, sY + iYOffset, (char*)pLabel, GameColors::UIBlack.ToColorRef());

	wsprintf(cTxt, "%d", iCurrentStat);
	PutString(sX + 109, sY + iYOffset, cTxt, GameColors::UILabel.ToColorRef());

	int iNewStat = iCurrentStat + iPendingChange;
	wsprintf(cTxt, "%d", iNewStat);
	if (iNewStat != iCurrentStat)
		PutString(sX + 162, sY + iYOffset, cTxt, GameColors::UIRed.ToColorRef());
	else
		PutString(sX + 162, sY + iYOffset, cTxt, GameColors::UILabel.ToColorRef());

	// UP arrow highlight (undo reduction)
	if ((msX >= sX + 195) && (msX <= sX + 205) && (msY >= sY + iArrowYOffset) && (msY <= sY + iArrowYOffset + 6) && bCanUndo)
		m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_GAME4]->Draw(sX + 195, sY + iArrowYOffset, 5);

	// DOWN arrow highlight (reduce stat)
	if ((msX >= sX + 210) && (msX <= sX + 220) && (msY >= sY + iArrowYOffset) && (msY <= sY + iArrowYOffset + 6) && bCanReduce)
		m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_GAME4]->Draw(sX + 210, sY + iArrowYOffset, 6);
}

void DialogBox_ChangeStatsMajestic::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX = Info().sX;
	short sY = Info().sY;
	short szX = Info().sSizeX;
	char cTxt[120];

	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 0);
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_TEXT, sX, sY, 2);
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME4, sX + 16, sY + 100, 4);

	PutAlignedString(sX, sX + szX, sY + 50, DRAW_DIALOGBOX_LEVELUP_SETTING14);
	PutAlignedString(sX, sX + szX, sY + 65, DRAW_DIALOGBOX_LEVELUP_SETTING15);

	// Majestic Points Remaining (total - pending cost)
	int iPendingCost = GetPendingMajesticCost(m_pGame);
	int iRemaining = m_pGame->m_iGizonItemUpgradeLeft - iPendingCost;

	PutString(sX + 20, sY + 85, DRAW_DIALOGBOX_LEVELUP_SETTING16, GameColors::UIBlack.ToColorRef());
	wsprintf(cTxt, "%d", iRemaining);
	if (iRemaining > 0)
		PutString(sX + 73, sY + 102, cTxt, GameColors::UIGreen.ToColorRef());
	else
		PutString(sX + 73, sY + 102, cTxt, GameColors::UIBlack.ToColorRef());

	bool bCanAfford = (iRemaining > 0);

	DrawStatRow(sX, sY, 125, DRAW_DIALOGBOX_LEVELUP_SETTING4,
		m_pGame->m_pPlayer->m_iStr, m_pGame->m_pPlayer->m_wLU_Str, msX, msY, 127,
		(m_pGame->m_pPlayer->m_wLU_Str < 0),
		bCanAfford && (m_pGame->m_pPlayer->m_iStr + m_pGame->m_pPlayer->m_wLU_Str - POINTS_PER_MAJESTIC >= MIN_STAT_VALUE));

	DrawStatRow(sX, sY, 144, DRAW_DIALOGBOX_LEVELUP_SETTING5,
		m_pGame->m_pPlayer->m_iVit, m_pGame->m_pPlayer->m_wLU_Vit, msX, msY, 146,
		(m_pGame->m_pPlayer->m_wLU_Vit < 0),
		bCanAfford && (m_pGame->m_pPlayer->m_iVit + m_pGame->m_pPlayer->m_wLU_Vit - POINTS_PER_MAJESTIC >= MIN_STAT_VALUE));

	DrawStatRow(sX, sY, 163, DRAW_DIALOGBOX_LEVELUP_SETTING6,
		m_pGame->m_pPlayer->m_iDex, m_pGame->m_pPlayer->m_wLU_Dex, msX, msY, 165,
		(m_pGame->m_pPlayer->m_wLU_Dex < 0),
		bCanAfford && (m_pGame->m_pPlayer->m_iDex + m_pGame->m_pPlayer->m_wLU_Dex - POINTS_PER_MAJESTIC >= MIN_STAT_VALUE));

	DrawStatRow(sX, sY, 182, DRAW_DIALOGBOX_LEVELUP_SETTING7,
		m_pGame->m_pPlayer->m_iInt, m_pGame->m_pPlayer->m_wLU_Int, msX, msY, 184,
		(m_pGame->m_pPlayer->m_wLU_Int < 0),
		bCanAfford && (m_pGame->m_pPlayer->m_iInt + m_pGame->m_pPlayer->m_wLU_Int - POINTS_PER_MAJESTIC >= MIN_STAT_VALUE));

	DrawStatRow(sX, sY, 201, DRAW_DIALOGBOX_LEVELUP_SETTING8,
		m_pGame->m_pPlayer->m_iMag, m_pGame->m_pPlayer->m_wLU_Mag, msX, msY, 203,
		(m_pGame->m_pPlayer->m_wLU_Mag < 0),
		bCanAfford && (m_pGame->m_pPlayer->m_iMag + m_pGame->m_pPlayer->m_wLU_Mag - POINTS_PER_MAJESTIC >= MIN_STAT_VALUE));

	DrawStatRow(sX, sY, 220, DRAW_DIALOGBOX_LEVELUP_SETTING9,
		m_pGame->m_pPlayer->m_iCharisma, m_pGame->m_pPlayer->m_wLU_Char, msX, msY, 222,
		(m_pGame->m_pPlayer->m_wLU_Char < 0),
		bCanAfford && (m_pGame->m_pPlayer->m_iCharisma + m_pGame->m_pPlayer->m_wLU_Char - POINTS_PER_MAJESTIC >= MIN_STAT_VALUE));

	// Cancel button (left)
	if ((msX >= sX + DEF_LBTNPOSX) && (msX <= sX + DEF_LBTNPOSX + DEF_BTNSZX) && (msY > sY + DEF_BTNPOSY) && (msY < sY + DEF_BTNPOSY + DEF_BTNSZY))
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_LBTNPOSX, sY + DEF_BTNPOSY, 17);
	else DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_LBTNPOSX, sY + DEF_BTNPOSY, 16);

	// Confirm button (right) — show as active only when there are pending changes
	if (iPendingCost > 0)
	{
		if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX) && (msY > sY + DEF_BTNPOSY) && (msY < sY + DEF_BTNPOSY + DEF_BTNSZY))
			DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 1);
		else
			DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 0);
	}
}

bool DialogBox_ChangeStatsMajestic::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	int iPendingCost = GetPendingMajesticCost(m_pGame);
	int iRemaining = m_pGame->m_iGizonItemUpgradeLeft - iPendingCost;

	struct StatEntry {
		int16_t* pPending;
		int iCurrentStat;
		int iArrowY;
	};

	StatEntry stats[] = {
		{ &m_pGame->m_pPlayer->m_wLU_Str,  m_pGame->m_pPlayer->m_iStr,      127 },
		{ &m_pGame->m_pPlayer->m_wLU_Vit,  m_pGame->m_pPlayer->m_iVit,      146 },
		{ &m_pGame->m_pPlayer->m_wLU_Dex,  m_pGame->m_pPlayer->m_iDex,      165 },
		{ &m_pGame->m_pPlayer->m_wLU_Int,  m_pGame->m_pPlayer->m_iInt,      184 },
		{ &m_pGame->m_pPlayer->m_wLU_Mag,  m_pGame->m_pPlayer->m_iMag,      203 },
		{ &m_pGame->m_pPlayer->m_wLU_Char, m_pGame->m_pPlayer->m_iCharisma, 222 },
	};

	for (auto& s : stats)
	{
		// UP arrow — undo a pending reduction (restore 3 points)
		if ((msX >= sX + 195) && (msX <= sX + 205) && (msY >= sY + s.iArrowY) && (msY <= sY + s.iArrowY + 6))
		{
			if (*s.pPending < 0)
			{
				*s.pPending += POINTS_PER_MAJESTIC;
				PlaySoundEffect('E', 14, 5);
			}
		}

		// DOWN arrow — reduce stat by 3 (costs 1 majestic point)
		if ((msX >= sX + 210) && (msX <= sX + 220) && (msY >= sY + s.iArrowY) && (msY <= sY + s.iArrowY + 6))
		{
			if (iRemaining > 0 && (s.iCurrentStat + *s.pPending - POINTS_PER_MAJESTIC >= MIN_STAT_VALUE))
			{
				*s.pPending -= POINTS_PER_MAJESTIC;
				iRemaining--;
				PlaySoundEffect('E', 14, 5);
			}
		}
	}

	// Confirm button (right) — send all pending reductions to server
	// Don't close the dialog here; the success/failure handler will close it after applying changes
	if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX) &&
		(msY > sY + DEF_BTNPOSY) && (msY < sY + DEF_BTNPOSY + DEF_BTNSZY))
	{
		if (iPendingCost > 0)
		{
			bSendCommand(MSGID_STATECHANGEPOINT, 0, 0, 0, 0, 0, 0);
			PlaySoundEffect('E', 14, 5);
		}
	}

	// Cancel button (left) — discard changes and close
	if ((msX >= sX + DEF_LBTNPOSX) && (msX <= sX + DEF_LBTNPOSX + DEF_BTNSZX) &&
		(msY > sY + DEF_BTNPOSY) && (msY < sY + DEF_BTNPOSY + DEF_BTNSZY))
	{
		m_pGame->m_pPlayer->m_wLU_Str = m_pGame->m_pPlayer->m_wLU_Vit = m_pGame->m_pPlayer->m_wLU_Dex = 0;
		m_pGame->m_pPlayer->m_wLU_Int = m_pGame->m_pPlayer->m_wLU_Mag = m_pGame->m_pPlayer->m_wLU_Char = 0;
		DisableDialogBox(DialogBoxId::ChangeStatsMajestic);
		PlaySoundEffect('E', 14, 5);
	}

	return false;
}
