#include "DialogBox_GuildHallMenu.h"
#include "Game.h"
#include "lan_eng.h"
#include "GlobalDef.h"
#include "SpriteID.h"
#include "NetMessages.h"
#include "GameFonts.h"
#include "TextLibExt.h"

DialogBox_GuildHallMenu::DialogBox_GuildHallMenu(CGame* pGame)
	: IDialogBox(DialogBoxId::GuildHallMenu, pGame)
{
	SetDefaultRect(337 + SCREENX(), 57 + SCREENY(), 258, 339);
}

void DialogBox_GuildHallMenu::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX, sY, szX;
	char cTxt[120];
	sX = Info().sX;
	sY = Info().sY;
	szX = Info().sSizeX;
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 2);

	switch (Info().cMode) {
	case 0: // initial diag
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 70) && (msY < sY + 95))
			PutAlignedString(sX, sX + szX, sY + 70, "Teleport to Battle Field", GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);
		else PutAlignedString(sX, sX + szX, sY + 70, "Teleport to Battle Field", GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);

		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 95) && (msY < sY + 120))
			PutAlignedString(sX, sX + szX, sY + 95, "Hire a soldier", GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);
		else PutAlignedString(sX, sX + szX, sY + 95, "Hire a soldier", GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);

		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 120) && (msY < sY + 145))
			PutAlignedString(sX, sX + szX, sY + 120, "Taking Flags", GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);
		else PutAlignedString(sX, sX + szX, sY + 120, "Taking Flags", GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);

		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 145) && (msY < sY + 170))
			PutAlignedString(sX, sX + szX, sY + 145, "Receive a Tutelary Angel", GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);
		else PutAlignedString(sX, sX + szX, sY + 145, "Receive a Tutelary Angel", GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);
		break;

	case 1: // TP diag
		if (m_pGame->m_iTeleportMapCount > 0)
		{
			char teleportBuf[128];
			TextLib::DrawText(GameFont::Default, sX + 35, sY + 250, DRAW_DIALOGBOX_CITYHALL_MENU72_1, TextLib::TextStyle::WithShadow(GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b));
			for (int i = 0; i < m_pGame->m_iTeleportMapCount; i++)
			{
				std::memset(cTxt, 0, sizeof(cTxt));
				m_pGame->GetOfficialMapName(m_pGame->m_stTeleportList[i].mapname, cTxt);
				snprintf(teleportBuf, sizeof(teleportBuf), DRAW_DIALOGBOX_CITYHALL_MENU77, cTxt, m_pGame->m_stTeleportList[i].iCost);
				if ((msX >= sX + DEF_LBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX) && (msY >= sY + 130 + i * 15) && (msY <= sY + 144 + i * 15))
					PutAlignedString(sX, sX + szX, sY + 130 + i * 15, teleportBuf, GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);
				else PutAlignedString(sX, sX + szX, sY + 130 + i * 15, teleportBuf, GameColors::UIMenuHighlight.r, GameColors::UIMenuHighlight.g, GameColors::UIMenuHighlight.b);
			}
		}
		else if (m_pGame->m_iTeleportMapCount == -1)
		{
			PutAlignedString(sX, sX + szX, sY + 125, DRAW_DIALOGBOX_CITYHALL_MENU73, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);
			PutAlignedString(sX, sX + szX, sY + 150, DRAW_DIALOGBOX_CITYHALL_MENU74, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);
			PutAlignedString(sX, sX + szX, sY + 175, DRAW_DIALOGBOX_CITYHALL_MENU75, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);
		}
		else
		{
			PutAlignedString(sX, sX + szX, sY + 175, DRAW_DIALOGBOX_CITYHALL_MENU76, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);
		}
		break;

	case 2: // Soldier diag
		PutAlignedString(sX, sX + szX, sY + 45, "You will hire a soldier by summon points", GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);
		if ((m_pGame->m_pPlayer->m_iConstructionPoint >= 2000) && (m_pGame->m_bIsCrusadeMode == false))
		{
			if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 70) && (msY < sY + 95))
				PutAlignedString(sX, sX + szX, sY + 70, "Sorceress             2000 Point", GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);
			else PutAlignedString(sX, sX + szX, sY + 70, "Sorceress             2000 Point", GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);
		}
		else PutAlignedString(sX, sX + szX, sY + 70, "Sorceress             2000 Point", GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b);

		if ((m_pGame->m_pPlayer->m_iConstructionPoint >= 3000) && (m_pGame->m_bIsCrusadeMode == false))
		{
			if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 95) && (msY < sY + 120))
				PutAlignedString(sX, sX + szX, sY + 95, "Ancient Temple Knight 3000 Point", GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);
			else PutAlignedString(sX, sX + szX, sY + 95, "Ancient Temple Knight 3000 Point", GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);
		}
		else PutAlignedString(sX, sX + szX, sY + 95, "Ancient Temple Knight 3000 Point", GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b);

		if ((m_pGame->m_pPlayer->m_iConstructionPoint >= 1500) && (m_pGame->m_bIsCrusadeMode == false))
		{
			if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 120) && (msY < sY + 145))
				PutAlignedString(sX, sX + szX, sY + 120, "Elf Master            1500 Point", GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);
			else PutAlignedString(sX, sX + szX, sY + 120, "Elf Master            1500 Point", GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);
		}
		else PutAlignedString(sX, sX + szX, sY + 120, "Elf Master            1500 Point", GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b);

		if ((m_pGame->m_pPlayer->m_iConstructionPoint >= 3000) && (m_pGame->m_bIsCrusadeMode == false))
		{
			if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 145) && (msY < sY + 171))
				PutAlignedString(sX, sX + szX, sY + 145, "Dark Shadow Knight    3000 Point", GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);
			else PutAlignedString(sX, sX + szX, sY + 145, "Dark Shadow Knight    3000 Point", GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);
		}
		else PutAlignedString(sX, sX + szX, sY + 145, "Dark Shadow Knight    3000 Point", GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b);

		if ((m_pGame->m_pPlayer->m_iConstructionPoint >= 4000) && (m_pGame->m_bIsCrusadeMode == false))
		{
			if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 170) && (msY < sY + 195))
				PutAlignedString(sX, sX + szX, sY + 170, "Heavy Battle Tank     4000 Point", GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);
			else PutAlignedString(sX, sX + szX, sY + 170, "Heavy Battle Tank     4000 Point", GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);
		}
		else PutAlignedString(sX, sX + szX, sY + 170, "Heavy Battle Tank     4000 Point", GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b);

		if ((m_pGame->m_pPlayer->m_iConstructionPoint >= 3000) && (m_pGame->m_bIsCrusadeMode == false))
		{
			if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 195) && (msY < sY + 220))
				PutAlignedString(sX, sX + szX, sY + 195, "Barbarian             3000 Point", GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);
			else PutAlignedString(sX, sX + szX, sY + 195, "Barbarian             3000 Point", GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);
		}
		else PutAlignedString(sX, sX + szX, sY + 195, "Barbarian             3000 Point", GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b);

		PutAlignedString(sX, sX + szX, sY + 220, "You should join a guild to hire soldiers.", GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);
		char pointsBuf[64];
		snprintf(pointsBuf, sizeof(pointsBuf), "Summon points : %d", m_pGame->m_pPlayer->m_iConstructionPoint);
		PutAlignedString(sX, sX + szX, sY + 250, pointsBuf, GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);
		PutAlignedString(sX, sX + szX, sY + 280, "Maximum summon points : 12000 points.", GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);
		PutAlignedString(sX, sX + szX, sY + 300, "Maximum hiring number : 5 ", GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);
		break;

	case 3: // Hire a Flag Diag
		PutAlignedString(sX, sX + szX, sY + 45, "You may acquire Flags with EK points.", GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);
		PutAlignedString(sX, sX + szX, sY + 70, "Price is 10 EK per Flag.", GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);
		if ((msX >= sX + 35) && (msX <= sX + 220) && (msY >= sY + 140) && (msY <= sY + 165))
			PutAlignedString(sX, sX + szX, sY + 140, "Take a Flag", GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);
		else
			PutAlignedString(sX, sX + szX, sY + 140, "Take a Flag", GameColors::UIMenuHighlight.r, GameColors::UIMenuHighlight.g, GameColors::UIMenuHighlight.b);
		break;

	case 4: // Tutelar Angel Diag
		PutAlignedString(sX, sX + szX, sY + 45, "5 magesty points will be deducted", GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);
		PutAlignedString(sX, sX + szX, sY + 80, "upon receiving the Pendant of Tutelary Angel.", GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);
		PutAlignedString(sX, sX + szX, sY + 105, "Would you like to receive the Tutelary Angel?", GameColors::UIMagicBlue.r, GameColors::UIMagicBlue.g, GameColors::UIMagicBlue.b);
		char angelBuf[64];
		snprintf(angelBuf, sizeof(angelBuf), DRAW_DIALOGBOX_ITEMUPGRADE11, m_pGame->m_iGizonItemUpgradeLeft);
		PutAlignedString(sX, sX + szX, sY + 140, angelBuf, GameColors::UIBlack.r, GameColors::UIBlack.g, GameColors::UIBlack.b);

		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 175) && (msY < sY + 200)
			&& (m_pGame->m_iGizonItemUpgradeLeft > 4))
			PutAlignedString(sX, sX + szX, sY + 175, "Tutelary Angel (STR) will be handed out.", GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);
		else PutAlignedString(sX, sX + szX, sY + 175, "Tutelary Angel (STR) will be handed out.", GameColors::UIMenuHighlight.r, GameColors::UIMenuHighlight.g, GameColors::UIMenuHighlight.b);

		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 200) && (msY < sY + 225)
			&& (m_pGame->m_iGizonItemUpgradeLeft > 4))
			PutAlignedString(sX, sX + szX, sY + 200, "Tutelary Angel (DEX) will be handed out.", GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);
		else PutAlignedString(sX, sX + szX, sY + 200, "Tutelary Angel (DEX) will be handed out.", GameColors::UIMenuHighlight.r, GameColors::UIMenuHighlight.g, GameColors::UIMenuHighlight.b);

		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 225) && (msY < sY + 250)
			&& (m_pGame->m_iGizonItemUpgradeLeft > 4))
			PutAlignedString(sX, sX + szX, sY + 225, "Tutelary Angel (INT) will be handed out.", GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);
		else PutAlignedString(sX, sX + szX, sY + 225, "Tutelary Angel (INT) will be handed out.", GameColors::UIMenuHighlight.r, GameColors::UIMenuHighlight.g, GameColors::UIMenuHighlight.b);

		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 250) && (msY < sY + 275)
			&& (m_pGame->m_iGizonItemUpgradeLeft > 4))
			PutAlignedString(sX, sX + szX, sY + 250, "Tutelary Angel (MAG) will be handed out.", GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);
		else PutAlignedString(sX, sX + szX, sY + 250, "Tutelary Angel (MAG) will be handed out.", GameColors::UIMenuHighlight.r, GameColors::UIMenuHighlight.g, GameColors::UIMenuHighlight.b);
		break;
	}
}

bool DialogBox_GuildHallMenu::OnClick(short msX, short msY)
{
	short sX, sY;
	sX = Info().sX;
	sY = Info().sY;

	switch (Info().cMode) {
	case 0: // initial diag
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 70) && (msY < sY + 95))
		{
			Info().cMode = 1;
			m_pGame->m_iTeleportMapCount = -1;
			bSendCommand(MSGID_REQUEST_HELDENIAN_TP_LIST, 0, 0, 0, 0, 0, 0);
			PlaySoundEffect('E', 14, 5);
		}
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 95) && (msY < sY + 120))
		{
			Info().cMode = 2;
			PlaySoundEffect('E', 14, 5);
		}
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 120) && (msY < sY + 145))
		{
			Info().cMode = 3;
			PlaySoundEffect('E', 14, 5);
		}
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 145) && (msY < sY + 170))
		{
			Info().cMode = 4;
			PlaySoundEffect('E', 14, 5);
		}
		break;

	case 1: // TP now
		if (m_pGame->m_iTeleportMapCount > 0)
		{
			for (int i = 0; i < m_pGame->m_iTeleportMapCount; i++)
			{
				if ((msX >= sX + DEF_LBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX) && (msY >= sY + 130 + i * 15) && (msY <= sY + 144 + i * 15))
				{
					bSendCommand(MSGID_REQUEST_HELDENIAN_TP, 0, 0, m_pGame->m_stTeleportList[i].iIndex, 0, 0, 0);
					DisableDialogBox(DialogBoxId::GuildHallMenu);
					return false;
				}
			}
		}
		break;

	case 2: // Buy a soldier scroll
		if ((msX >= sX + 35) && (msX <= sX + 220) && (msY > sY + 70) && (msY < sY + 95)
			&& (m_pGame->m_pPlayer->m_iConstructionPoint >= 2000) && (m_pGame->m_bIsCrusadeMode == false))
		{
			bSendCommand(MSGID_REQUEST_HELDENIAN_SCROLL, 875, 1, 2, 3, 4, "Gail", 5);
			PlaySoundEffect('E', 14, 5);
		}
		if ((msX >= sX + 35) && (msX <= sX + 220) && (msY > sY + 95) && (msY < sY + 120)
			&& (m_pGame->m_pPlayer->m_iConstructionPoint >= 3000) && (m_pGame->m_bIsCrusadeMode == false))
		{
			bSendCommand(MSGID_REQUEST_HELDENIAN_SCROLL, 876, 0, 0, 0, 0, "Gail", 0);
			PlaySoundEffect('E', 14, 5);
		}
		if ((msX >= sX + 35) && (msX <= sX + 220) && (msY > sY + 120) && (msY < sY + 145)
			&& (m_pGame->m_pPlayer->m_iConstructionPoint >= 1500) && (m_pGame->m_bIsCrusadeMode == false))
		{
			bSendCommand(MSGID_REQUEST_HELDENIAN_SCROLL, 877, 0, 0, 0, 0, "Gail", 0);
			PlaySoundEffect('E', 14, 5);
		}
		if ((msX >= sX + 35) && (msX <= sX + 220) && (msY > sY + 145) && (msY < sY + 170)
			&& (m_pGame->m_pPlayer->m_iConstructionPoint >= 3000) && (m_pGame->m_bIsCrusadeMode == false))
		{
			bSendCommand(MSGID_REQUEST_HELDENIAN_SCROLL, 878, 0, 0, 0, 0, "Gail", 0);
			PlaySoundEffect('E', 14, 5);
		}
		if ((msX >= sX + 35) && (msX <= sX + 220) && (msY > sY + 170) && (msY < sY + 195)
			&& (m_pGame->m_pPlayer->m_iConstructionPoint >= 4000) && (m_pGame->m_bIsCrusadeMode == false))
		{
			bSendCommand(MSGID_REQUEST_HELDENIAN_SCROLL, 879, 0, 0, 0, 0, "Gail", 0);
			PlaySoundEffect('E', 14, 5);
		}
		if ((msX >= sX + 35) && (msX <= sX + 220) && (msY > sY + 195) && (msY < sY + 220)
			&& (m_pGame->m_pPlayer->m_iConstructionPoint >= 3000) && (m_pGame->m_bIsCrusadeMode == false))
		{
			bSendCommand(MSGID_REQUEST_HELDENIAN_SCROLL, 880, 0, 0, 0, 0, "Gail", 0);
			PlaySoundEffect('E', 14, 5);
		}
		break;

	case 3: // Buy a Flag
		if ((msX >= sX + 35) && (msX <= sX + 220) && (msY >= sY + 140) && (msY <= sY + 165)
			&& (m_pGame->m_pPlayer->m_iEnemyKillCount >= 3))
		{
			bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQ_GETOCCUPYFLAG, 0, 0, 0, 0, 0, 0);
			PlaySoundEffect('E', 14, 5);
		}
		break;

	case 4: // Buy an Angel
		if ((msX >= sX + 35) && (msX <= sX + 220) && (msY >= sY + 175) && (msY <= sY + 200)
			&& (m_pGame->m_iGizonItemUpgradeLeft >= 5))
		{
			bSendCommand(DEF_REQUEST_ANGEL, 0, 0, 1, 0, 0, "Gail", 0);
			PlaySoundEffect('E', 14, 5);
		}
		if ((msX >= sX + 35) && (msX <= sX + 220) && (msY >= sY + 200) && (msY <= sY + 225)
			&& (m_pGame->m_iGizonItemUpgradeLeft >= 5))
		{
			bSendCommand(DEF_REQUEST_ANGEL, 0, 0, 2, 0, 0, "Gail", 0);
			PlaySoundEffect('E', 14, 5);
		}
		if ((msX >= sX + 35) && (msX <= sX + 220) && (msY >= sY + 225) && (msY <= sY + 250)
			&& (m_pGame->m_iGizonItemUpgradeLeft >= 5))
		{
			bSendCommand(DEF_REQUEST_ANGEL, 0, 0, 3, 0, 0, "Gail", 0);
			PlaySoundEffect('E', 14, 5);
		}
		if ((msX >= sX + 35) && (msX <= sX + 220) && (msY >= sY + 250) && (msY <= sY + 275)
			&& (m_pGame->m_iGizonItemUpgradeLeft >= 5))
		{
			bSendCommand(DEF_REQUEST_ANGEL, 0, 0, 4, 0, 0, "Gail", 0);
			PlaySoundEffect('E', 14, 5);
		}
		break;
	}
	return false;
}
