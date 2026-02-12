#include "DialogBox_Constructor.h"
#include "Game.h"
#include "TeleportManager.h"
#include "lan_eng.h"
#include "GlobalDef.h"
#include "SpriteID.h"
#include "ConfigManager.h"
#include "NetMessages.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
DialogBox_Constructor::DialogBox_Constructor(CGame* pGame)
	: IDialogBox(DialogBoxId::CrusadeConstructor, pGame)
{
	SetDefaultRect(20 , 20 , 310, 386);
}

void DialogBox_Constructor::OnUpdate()
{
	uint32_t dwTime = GameClock::GetTimeMS();
	if ((dwTime - m_pGame->m_dwCommanderCommandRequestedTime) > 1000 * 10)
	{
		m_pGame->_RequestMapStatus("middleland", 1);
		m_pGame->m_dwCommanderCommandRequestedTime = dwTime;
	}
}

void DialogBox_Constructor::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX, sY, szX, szY, MapSzX, MapSzY;
	double dV1, dV2, dV3;
	int tX, tY;
	char cMapName[12];
	sX = Info().sX;
	sY = Info().sY;
	szX = Info().sSizeX;

	DrawNewDialogBox(InterfaceNdCrusade, sX, sY - 5, 0, false, ConfigManager::Get().IsDialogTransparencyEnabled());
	DrawNewDialogBox(InterfaceNdText, sX, sY, 16, false, ConfigManager::Get().IsDialogTransparencyEnabled());

	switch (Info().cMode) {
	case 0: // Main dlg
		if (m_pGame->m_pPlayer->m_iConstructLocX != -1)
		{
			std::string locationBuf;
			std::memset(cMapName, 0, sizeof(cMapName));
			m_pGame->GetOfficialMapName(m_pGame->m_cConstructMapName.c_str(), cMapName);
			locationBuf = std::format(DRAW_DIALOGBOX_CONSTRUCTOR1, cMapName, m_pGame->m_pPlayer->m_iConstructLocX, m_pGame->m_pPlayer->m_iConstructLocY);
			PutAlignedString(sX, sX + szX, sY + 40, locationBuf.c_str());
		}
		else PutAlignedString(sX, sX + szX, sY + 40, DRAW_DIALOGBOX_CONSTRUCTOR2);

		DrawNewDialogBox(InterfaceNdCrusade, sX, sY, 21, false, ConfigManager::Get().IsDialogTransparencyEnabled());

		if ((msX >= sX + 20) && (msX <= sX + 20 + 46)
			&& (msY >= sY + 340) && (msY <= sY + 340 + 52))
		{
			m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20, sY + 340, 24);
		}
		else m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20, sY + 340, 30);

		if ((msX >= sX + 20 + 50) && (msX <= sX + 20 + 46 + 50)
			&& (msY >= sY + 340) && (msY <= sY + 340 + 52))
		{
			m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 50, sY + 340, 15);
		}
		else m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 50, sY + 340, 1);

		if ((msX >= sX + 20 + 150 + 74) && (msX <= sX + 20 + 46 + 150 + 74)
			&& (msY >= sY + 340) && (msY <= sY + 340 + 52))
		{
			m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 150 + 74, sY + 340, 18);
		}
		else m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 150 + 74, sY + 340, 4);

		if ((msX >= sX + 20) && (msX <= sX + 20 + 46)
			&& (msY >= sY + 340) && (msY <= sY + 340 + 52))
		{
			hb::shared::text::DrawText(GameFont::Default, msX + 20, msY + 35, DRAW_DIALOGBOX_CONSTRUCTOR3, hb::shared::text::TextStyle::WithShadow(GameColors::UIWhite));
		}
		else if ((msX >= sX + 20 + 50) && (msX <= sX + 20 + 46 + 50)
			&& (msY >= sY + 340) && (msY <= sY + 340 + 52))
		{
			hb::shared::text::DrawText(GameFont::Default, msX + 20, msY + 35, DRAW_DIALOGBOX_CONSTRUCTOR4, hb::shared::text::TextStyle::WithShadow(GameColors::UIWhite));
		}
		else if ((msX >= sX + 20 + 150 + 74) && (msX <= sX + 20 + 46 + 150 + 74)
			&& (msY >= sY + 340) && (msY <= sY + 340 + 52))
		{
			hb::shared::text::DrawText(GameFont::Default, msX + 20, msY + 35, DRAW_DIALOGBOX_CONSTRUCTOR5, hb::shared::text::TextStyle::WithShadow(GameColors::UIWhite));
		}
		break;

	case 1: // Select building
		PutAlignedString(sX, sX + szX, sY + 40, DRAW_DIALOGBOX_CONSTRUCTOR6);
		PutAlignedString(sX, sX + 323, sY + 80, DRAW_DIALOGBOX_CONSTRUCTOR7);
		PutAlignedString(sX, sX + 323, sY + 95, DRAW_DIALOGBOX_CONSTRUCTOR8);
		PutAlignedString(sX, sX + 323, sY + 110, DRAW_DIALOGBOX_CONSTRUCTOR9);
		PutAlignedString(sX, sX + 323, sY + 125, DRAW_DIALOGBOX_CONSTRUCTOR10);
		PutAlignedString(sX, sX + 323, sY + 140, DRAW_DIALOGBOX_CONSTRUCTOR11);
		PutAlignedString(sX, sX + 323, sY + 155, DRAW_DIALOGBOX_CONSTRUCTOR12);

		if ((msX >= sX + 20) && (msX <= sX + 20 + 46) && (msY >= sY + 220) && (msY <= sY + 220 + 50))
		{
			m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20, sY + 220, 27);
		}
		else m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20, sY + 220, 33);

		if ((msX >= sX + 20 + 50) && (msX <= sX + 20 + 50 + 45) && (msY >= sY + 220) && (msY <= sY + 220 + 50))
		{
			m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 50, sY + 220, 28);
		}
		else m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 50, sY + 220, 34);

		if ((msX >= sX + 20 + 100) && (msX <= sX + 20 + 100 + 45) && (msY >= sY + 220) && (msY <= sY + 220 + 50))
		{
			m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 100, sY + 220, 26);
		}
		else m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 100, sY + 220, 32);

		if ((msX >= sX + 20 + 150) && (msX <= sX + 20 + 150 + 45) && (msY >= sY + 220) && (msY <= sY + 220 + 50))
		{
			m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 150, sY + 220, 25);
		}
		else m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 150, sY + 220, 31);

		if ((msX >= sX + 20 + 150 + 74 - 50) && (msX <= sX + 20 + 46 + 150 + 74 - 50) && (msY >= sY + 322) && (msY <= sY + 322 + 52))
		{
			m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 100 + 74, sY + 322, 19);
		}
		else m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 100 + 74, sY + 322, 20);

		if ((msX >= sX + 20 + 150 + 74) && (msX <= sX + 20 + 46 + 150 + 74) && (msY >= sY + 322) && (msY <= sY + 322 + 52))
		{
			m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 150 + 74, sY + 322, 18);
		}
		else m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 150 + 74, sY + 322, 4);

		if ((msX >= sX + 20) && (msX <= sX + 20 + 46) && (msY >= sY + 220) && (msY <= sY + 220 + 50))
		{
			hb::shared::text::DrawText(GameFont::Default, msX + 20, msY + 35, DRAW_DIALOGBOX_CONSTRUCTOR13, hb::shared::text::TextStyle::WithShadow(GameColors::UIWhite));
		}
		else if ((msX >= sX + 20 + 50) && (msX <= sX + 20 + 50 + 45) && (msY >= sY + 220) && (msY <= sY + 220 + 50))
		{
			hb::shared::text::DrawText(GameFont::Default, msX + 20, msY + 35, DRAW_DIALOGBOX_CONSTRUCTOR14, hb::shared::text::TextStyle::WithShadow(GameColors::UIWhite));
		}
		else if ((msX >= sX + 20 + 100) && (msX <= sX + 20 + 100 + 45) && (msY >= sY + 220) && (msY <= sY + 220 + 50))
		{
			hb::shared::text::DrawText(GameFont::Default, msX + 20, msY + 35, DRAW_DIALOGBOX_CONSTRUCTOR15, hb::shared::text::TextStyle::WithShadow(GameColors::UIWhite));
		}
		else if ((msX >= sX + 20 + 150) && (msX <= sX + 20 + 150 + 45) && (msY >= sY + 220) && (msY <= sY + 220 + 50))
		{
			hb::shared::text::DrawText(GameFont::Default, msX + 20, msY + 35, DRAW_DIALOGBOX_CONSTRUCTOR16, hb::shared::text::TextStyle::WithShadow(GameColors::UIWhite));
		}
		else if ((msX >= sX + 20 + 150 + 74 - 50) && (msX <= sX + 20 + 46 + 150 + 74 - 50) && (msY >= sY + 322) && (msY <= sY + 322 + 52))
		{
			hb::shared::text::DrawText(GameFont::Default, msX + 20, msY + 35, DRAW_DIALOGBOX_CONSTRUCTOR17, hb::shared::text::TextStyle::WithShadow(GameColors::UIWhite));
		}
		else if ((msX >= sX + 20 + 150 + 74) && (msX <= sX + 20 + 46 + 150 + 74) && (msY >= sY + 322) && (msY <= sY + 322 + 52))
		{
			hb::shared::text::DrawText(GameFont::Default, msX + 20, msY + 35, DRAW_DIALOGBOX_CONSTRUCTOR18, hb::shared::text::TextStyle::WithShadow(GameColors::UIWhite));
		}
		break;

	case 2: // Teleport
		PutAlignedString(sX, sX + szX, sY + 40, DRAW_DIALOGBOX_CONSTRUCTOR19);
		DrawNewDialogBox(InterfaceNdCrusade, sX, sY, 21, false, ConfigManager::Get().IsDialogTransparencyEnabled());

		if ((msX >= sX + 20 + 50) && (msX <= sX + 20 + 46 + 50)
			&& (msY >= sY + 340) && (msY <= sY + 340 + 52))
		{
			m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 50, sY + 340, 15);
		}
		else m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 50, sY + 340, 1);

		if ((msX >= sX + 20 + 150 + 74 - 50) && (msX <= sX + 20 + 46 + 150 + 74 - 50)
			&& (msY >= sY + 340) && (msY <= sY + 340 + 52))
		{
			m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 100 + 74, sY + 340, 19);
		}
		else m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 100 + 74, sY + 340, 20);

		if ((msX >= sX + 20 + 150 + 74) && (msX <= sX + 20 + 46 + 150 + 74)
			&& (msY >= sY + 340) && (msY <= sY + 340 + 52))
		{
			m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 150 + 74, sY + 340, 18);
		}
		else m_pGame->m_pSprite[InterfaceNdCrusade]->Draw(sX + 20 + 150 + 74, sY + 340, 4);

		if ((msX >= sX + 20 + 50) && (msX <= sX + 20 + 46 + 50)
			&& (msY >= sY + 340) && (msY <= sY + 340 + 52))
		{
			hb::shared::text::DrawText(GameFont::Default, msX + 20, msY + 35, DRAW_DIALOGBOX_CONSTRUCTOR20, hb::shared::text::TextStyle::WithShadow(GameColors::UIWhite));
		}
		else if ((msX >= sX + 20 + 150 + 74 - 50) && (msX <= sX + 20 + 46 + 150 + 74 - 50)
			&& (msY >= sY + 340) && (msY <= sY + 340 + 52))
		{
			hb::shared::text::DrawText(GameFont::Default, msX + 20, msY + 35, DRAW_DIALOGBOX_CONSTRUCTOR21, hb::shared::text::TextStyle::WithShadow(GameColors::UIWhite));
		}
		else if ((msX >= sX + 20 + 150 + 74) && (msX <= sX + 20 + 46 + 150 + 74)
			&& (msY >= sY + 340) && (msY <= sY + 340 + 52))
		{
			hb::shared::text::DrawText(GameFont::Default, msX + 20, msY + 35, DRAW_DIALOGBOX_CONSTRUCTOR22, hb::shared::text::TextStyle::WithShadow(GameColors::UIWhite));
		}
		break;
	}

	// Draw map overlay
	switch (Info().cMode) {
	case 0: // Main dlg
	case 2: // TP
		szX = 0;
		szY = 0;
		MapSzX = 0;
		MapSzY = 0;
		if (m_pGame->m_cStatusMapName == "aresden")
		{
			szX = 250;
			szY = 250;
		}
		else if (m_pGame->m_cStatusMapName == "elvine")
		{
			szX = 250;
			szY = 250;
		}
		else if (m_pGame->m_cStatusMapName == "middleland")
		{
			szX = 279;
			szY = 280;
			MapSzX = 524;
			MapSzY = 524;
		}
		if (szX != 0)
		{
			for (int i = 0; i < hb::shared::limits::MaxCrusadeStructures; i++)
				if (m_pGame->m_stCrusadeStructureInfo[i].cType == 42)
				{
					dV1 = static_cast<double>(MapSzX);
					dV2 = static_cast<double>(m_pGame->m_stCrusadeStructureInfo[i].sX);
					dV3 = (dV2 * static_cast<double>(szX)) / dV1;
					tX = static_cast<int>(dV3);
					dV1 = static_cast<double>(MapSzY);
					dV2 = static_cast<double>(m_pGame->m_stCrusadeStructureInfo[i].sY);
					dV3 = (dV2 * static_cast<double>(szY)) / dV1;
					tY = static_cast<int>(dV3);
					switch (m_pGame->m_stCrusadeStructureInfo[i].cType) {
					case 42:
						DrawNewDialogBox(InterfaceNdCrusade, sX + tX + 15, sY + tY + 60, 40);
						break;
					}
				}
			if (TeleportManager::Get().GetLocX() != -1)
			{
				dV1 = static_cast<double>(MapSzX);
				dV2 = static_cast<double>(TeleportManager::Get().GetLocX());
				dV3 = (dV2 * static_cast<double>(szX)) / dV1;
				tX = static_cast<int>(dV3);
				dV1 = static_cast<double>(MapSzY);
				dV2 = static_cast<double>(TeleportManager::Get().GetLocY());
				dV3 = (dV2 * static_cast<double>(szY)) / dV1;
				tY = static_cast<int>(dV3);
				DrawNewDialogBox(InterfaceNdCrusade, sX + tX + 15, sY + tY + 60, 42, false, true);
			}
			if ((Info().cMode != 2) && (m_pGame->m_pPlayer->m_iConstructLocX != -1))
			{
				dV1 = static_cast<double>(MapSzX);
				dV2 = static_cast<double>(m_pGame->m_pPlayer->m_iConstructLocX);
				dV3 = (dV2 * static_cast<double>(szX)) / dV1;
				tX = static_cast<int>(dV3);
				dV1 = static_cast<double>(MapSzY);
				dV2 = static_cast<double>(m_pGame->m_pPlayer->m_iConstructLocY);
				dV3 = (dV2 * static_cast<double>(szY)) / dV1;
				tY = static_cast<int>(dV3);
				DrawNewDialogBox(InterfaceNdCrusade, sX + tX + 15, sY + tY + 60, 41, false, true);
			}
			if (m_pGame->m_cMapName == "middleland")
			{
				dV1 = static_cast<double>(MapSzX);
				dV2 = static_cast<double>(m_pGame->m_pPlayer->m_sPlayerX);
				dV3 = (dV2 * static_cast<double>(szX)) / dV1;
				tX = static_cast<int>(dV3);
				dV1 = static_cast<double>(MapSzY);
				dV2 = static_cast<double>(m_pGame->m_pPlayer->m_sPlayerY);
				dV3 = (dV2 * static_cast<double>(szY)) / dV1;
				tY = static_cast<int>(dV3);
				DrawNewDialogBox(InterfaceNdCrusade, sX + tX + 15, sY + tY + 60, 43);
			}
		}
		if (szX > 0 && szY > 0 && (msX >= sX + 15) && (msX <= sX + 15 + 278)
			&& (msY >= sY + 60) && (msY <= sY + 60 + 272))
		{
			dV1 = static_cast<double>(msX - (sX + 15));
			dV2 = static_cast<double>(MapSzX);
			dV3 = (dV2 * dV1) / szX;
			tX = static_cast<int>(dV3);
			dV1 = static_cast<double>(msY - (sY + 60));
			dV2 = static_cast<double>(MapSzY);
			dV3 = (dV2 * dV1) / szY;
			tY = static_cast<int>(dV3);
			if (tX < 30) tX = 30;
			if (tY < 30) tY = 30;
			if (tX > MapSzX - 30) tX = MapSzX - 30;
			if (tY > MapSzY - 30) tY = MapSzY - 30;
			std::string coordBuf;
			coordBuf = std::format("{},{}", tX, tY);
			hb::shared::text::DrawText(GameFont::SprFont3_2, msX + 10, msY - 10, coordBuf.c_str(), hb::shared::text::TextStyle::WithTwoPointShadow(GameColors::Yellow4x));
		}
		break;
	}
}

bool DialogBox_Constructor::OnClick(short msX, short msY)
{
	short sX, sY;
	if (m_pGame->m_bIsCrusadeMode == false) return false;
	sX = Info().sX;
	sY = Info().sY;

	switch (Info().cMode) {
	case 0: // Main
		if ((msX >= sX + 20) && (msX <= sX + 20 + 46) && (msY >= sY + 340) && (msY <= sY + 340 + 52))
		{
			if (m_pGame->m_pPlayer->m_iConstructLocX == -1)
			{
				m_pGame->SetTopMsg(m_pGame->m_pGameMsgList[14]->m_pMsg, 5);
			}
			else
			{
				Info().cMode = 1;
				PlaySoundEffect('E', 14, 5);
			}
		}
		else if ((msX >= sX + 20 + 50) && (msX <= sX + 20 + 46 + 50) && (msY >= sY + 340) && (msY <= sY + 340 + 52))
		{
			if (TeleportManager::Get().GetLocX() == -1)
			{
				if (m_pGame->m_pGameMsgList[15]) m_pGame->SetTopMsg(m_pGame->m_pGameMsgList[15]->m_pMsg, 5);
			}
			else if (m_pGame->m_cMapName == TeleportManager::Get().GetMapName())
			{
				if (m_pGame->m_pGameMsgList[16]) m_pGame->SetTopMsg(m_pGame->m_pGameMsgList[16]->m_pMsg, 5);
			}
			else
			{
				Info().cMode = 2;
				PlaySoundEffect('E', 14, 5);
			}
		}
		else if ((msX >= sX + 20 + 150 + 74) && (msX <= sX + 20 + 46 + 150 + 74) && (msY >= sY + 340) && (msY <= sY + 340 + 52))
		{
			DisableDialogBox(DialogBoxId::Text);
			EnableDialogBox(DialogBoxId::Text, 805, 0, 0);
			PlaySoundEffect('E', 14, 5);
		}
		break;

	case 1: // Choose building
		if ((msX >= sX + 20) && (msX <= sX + 20 + 46) && (msY >= sY + 220) && (msY <= sY + 220 + 50))
		{
			bSendCommand(MsgId::CommandCommon, CommonType::SummonWarUnit, 0, 38, 1, m_pGame->m_dialogBoxManager.Info(DialogBoxId::CrusadeCommander).sV1, 0);
			PlaySoundEffect('E', 14, 5);
			DisableDialogBox(DialogBoxId::CrusadeConstructor);
		}
		if ((msX >= sX + 20 + 50) && (msX <= sX + 20 + 50 + 45) && (msY >= sY + 220) && (msY <= sY + 220 + 50))
		{
			bSendCommand(MsgId::CommandCommon, CommonType::SummonWarUnit, 0, 39, 1, m_pGame->m_dialogBoxManager.Info(DialogBoxId::CrusadeCommander).sV1, 0);
			PlaySoundEffect('E', 14, 5);
			DisableDialogBox(DialogBoxId::CrusadeConstructor);
		}
		if ((msX >= sX + 20 + 100) && (msX <= sX + 20 + 100 + 45) && (msY >= sY + 220) && (msY <= sY + 220 + 50))
		{
			bSendCommand(MsgId::CommandCommon, CommonType::SummonWarUnit, 0, 36, 1, m_pGame->m_dialogBoxManager.Info(DialogBoxId::CrusadeCommander).sV1, 0);
			PlaySoundEffect('E', 14, 5);
			DisableDialogBox(DialogBoxId::CrusadeConstructor);
		}
		if ((msX >= sX + 20 + 150) && (msX <= sX + 20 + 150 + 45) && (msY >= sY + 220) && (msY <= sY + 220 + 50))
		{
			bSendCommand(MsgId::CommandCommon, CommonType::SummonWarUnit, 0, 37, 1, m_pGame->m_dialogBoxManager.Info(DialogBoxId::CrusadeCommander).sV1, 0);
			PlaySoundEffect('E', 14, 5);
			DisableDialogBox(DialogBoxId::CrusadeConstructor);
		}

		if ((msX >= sX + 20 + 150 + 74 - 50) && (msX <= sX + 20 + 46 + 150 + 74 - 50) && (msY >= sY + 322) && (msY <= sY + 322 + 52))
		{
			Info().cMode = 0;
			PlaySoundEffect('E', 14, 5);
		}
		if ((msX >= sX + 20 + 150 + 74) && (msX <= sX + 20 + 46 + 150 + 74) && (msY >= sY + 322) && (msY <= sY + 322 + 52))
		{
			DisableDialogBox(DialogBoxId::Text);
			EnableDialogBox(DialogBoxId::Text, 806, 0, 0);
			PlaySoundEffect('E', 14, 5);
		}
		break;

	case 2: // Use TP
		if ((msX >= sX + 20 + 50) && (msX <= sX + 20 + 46 + 50) && (msY >= sY + 340) && (msY <= sY + 340 + 52))
		{
			bSendCommand(MsgId::CommandCommon, CommonType::GuildTeleport, 0, 0, 0, 0, 0);
			DisableDialogBox(DialogBoxId::CrusadeConstructor);
			PlaySoundEffect('E', 14, 5);
		}
		if ((msX >= sX + 20 + 150 + 74 - 50) && (msX <= sX + 20 + 46 + 150 + 74 - 50) && (msY >= sY + 340) && (msY <= sY + 340 + 52))
		{
			Info().cMode = 0;
			PlaySoundEffect('E', 14, 5);
		}
		if ((msX >= sX + 20 + 150 + 74) && (msX <= sX + 20 + 46 + 150 + 74) && (msY >= sY + 340) && (msY <= sY + 340 + 52))
		{
			DisableDialogBox(DialogBoxId::Text);
			EnableDialogBox(DialogBoxId::Text, 807, 0, 0);
			PlaySoundEffect('E', 14, 5);
		}
		break;
	}
	return false;
}
