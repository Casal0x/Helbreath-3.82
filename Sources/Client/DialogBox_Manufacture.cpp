#include "DialogBox_Manufacture.h"
#include "CursorTarget.h"
#include "Game.h"
#include "BuildItemManager.h"
#include "ItemNameFormatter.h"
#include "GameFonts.h"
#include "GlobalDef.h"
#include "SpriteID.h"
#include "TextLibExt.h"
#include "lan_eng.h"
#include "NetMessages.h"
#include "IInput.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::shared::item;
using namespace hb::client::sprite_id;

DialogBox_Manufacture::DialogBox_Manufacture(CGame* pGame)
	: IDialogBox(DialogBoxId::Manufacture, pGame)
{
	SetDefaultRect(100 , 60 , 258, 339);
}

void DialogBox_Manufacture::OnDraw(short msX, short msY, short msZ, char cLB)
{
	if (!m_pGame->EnsureItemConfigsLoaded()) return;
	int iAdjX = 5;
	int iAdjY = 8;
	short sX, sY;

	switch (Info().cMode) {
	case 1: // Alchemy waiting for ingredients
		if (Info().cStr[0] != 0)
		{
			sX = Info().sX + iAdjX + (Info().cStr[0] - (rand() % (Info().cStr[0] * 2)));
			sY = Info().sY + iAdjY + (Info().cStr[0] - (rand() % (Info().cStr[0] * 2)));
		}
		else
		{
			sX = Info().sX;
			sY = Info().sY;
		}
		DrawAlchemyWaiting(sX, sY, msX, msY);
		break;

	case 2: // Alchemy creating potion
		if (Info().cStr[0] != 0)
		{
			sX = Info().sX + iAdjX + (Info().cStr[0] - (rand() % (Info().cStr[0] * 2)));
			sY = Info().sY + iAdjY + (Info().cStr[0] - (rand() % (Info().cStr[0] * 2)));
		}
		else
		{
			sX = Info().sX;
			sY = Info().sY;
		}
		DrawAlchemyCreating(sX, sY);
		break;

	case 3: // Manufacture: item list
		sX = Info().sX;
		sY = Info().sY;
		DrawManufactureList(sX, sY, msX, msY, msZ, cLB);
		break;

	case 4: // Manufacture: waiting for ingredients
		sX = Info().sX;
		sY = Info().sY;
		DrawManufactureWaiting(sX, sY, msX, msY);
		break;

	case 5: // Manufacture: in progress
		sX = Info().sX;
		sY = Info().sY;
		DrawManufactureInProgress(sX, sY);
		break;

	case 6: // Manufacture: done
		sX = Info().sX;
		sY = Info().sY;
		DrawManufactureDone(sX, sY, msX, msY);
		break;

	case 7: // Crafting: waiting for ingredients
		if (Info().cStr[0] != 0)
		{
			sX = Info().sX + iAdjX + (Info().cStr[0] - (rand() % (Info().cStr[0] * 2)));
			sY = Info().sY + iAdjY + (Info().cStr[0] - (rand() % (Info().cStr[0] * 2)));
		}
		else
		{
			sX = Info().sX;
			sY = Info().sY;
		}
		DrawCraftingWaiting(sX, sY, msX, msY);
		break;

	case 8: // Crafting: in progress
		if (Info().cStr[0] != 0)
		{
			sX = Info().sX + 5 + (Info().cStr[0] - (rand() % (Info().cStr[0] * 2)));
			sY = Info().sY + 8 + (Info().cStr[0] - (rand() % (Info().cStr[0] * 2)));
		}
		else
		{
			sX = Info().sX;
			sY = Info().sY;
		}
		DrawCraftingInProgress(sX, sY);
		break;
	}
}

void DialogBox_Manufacture::DrawAlchemyWaiting(short sX, short sY, short msX, short msY)
{
	int iAdjX = 5;
	int iAdjY = 8;
	uint32_t dwTime = m_pGame->m_dwCurTime;

	m_pGame->m_pSprite[InterfaceAddInterface]->Draw(sX, sY, 1);

	if (Info().sV1 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV1]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint +
		pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + (1 - (rand() % 3)), sY + iAdjY + 55 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Alpha(0.5f));
	}

	if (Info().sV2 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV2]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint +
		pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 45 * 1 + (1 - (rand() % 3)), sY + iAdjY + 55 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Alpha(0.5f));
	}

	if (Info().sV3 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV3]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint +
		pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 45 * 2 + (1 - (rand() % 3)), sY + iAdjY + 55 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Alpha(0.5f));
	}

	if (Info().sV4 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV4]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint +
		pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + (1 - (rand() % 3)), sY + iAdjY + 100 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Alpha(0.5f));
	}

	if (Info().sV5 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV5]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint +
		pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 45 * 1 + (1 - (rand() % 3)), sY + iAdjY + 100 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Alpha(0.5f));
	}

	if (Info().sV6 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV6]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint +
		pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 45 * 2 + (1 - (rand() % 3)), sY + iAdjY + 100 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Alpha(0.5f));
	}

	if ((msX >= sX + iAdjX + 60) && (msX <= sX + iAdjX + 153) && (msY >= sY + iAdjY + 175) && (msY <= sY + iAdjY + 195))
		hb::shared::text::DrawText(GameFont::Bitmap1, sX + iAdjX + 60, sY + iAdjY + 175, "Try Now!", hb::shared::text::TextStyle::WithHighlight(GameColors::BmpBtnBlue));
	else hb::shared::text::DrawText(GameFont::Bitmap1, sX + iAdjX + 60, sY + iAdjY + 175, "Try Now!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicBlue));
}

void DialogBox_Manufacture::DrawAlchemyCreating(short sX, short sY)
{
	int iAdjX = 5;
	int iAdjY = 8;
	uint32_t dwTime = m_pGame->m_dwCurTime;

	m_pGame->m_pSprite[InterfaceAddInterface]->Draw(sX, sY, 1);

	if (Info().sV1 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV1]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint +
		pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + (1 - (rand() % 3)), sY + iAdjY + 55 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Alpha(0.5f));
	}

	if (Info().sV2 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV2]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint +
		pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 45 * 1 + (1 - (rand() % 3)), sY + iAdjY + 55 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Alpha(0.5f));
	}

	if (Info().sV3 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV3]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint +
		pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 45 * 2 + (1 - (rand() % 3)), sY + iAdjY + 55 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Alpha(0.5f));
	}

	if (Info().sV4 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV4]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint +
		pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + (1 - (rand() % 3)), sY + iAdjY + 100 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Alpha(0.5f));
	}

	if (Info().sV5 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV5]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint +
		pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 45 * 1 + (1 - (rand() % 3)), sY + iAdjY + 100 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Alpha(0.5f));
	}

	if (Info().sV6 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV6]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint +
		pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 45 * 2 + (1 - (rand() % 3)), sY + iAdjY + 100 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Alpha(0.5f));
	}

	hb::shared::text::DrawText(GameFont::Bitmap1, sX + iAdjX + 60, sY + iAdjY + 175, "Creating...", hb::shared::text::TextStyle::WithHighlight(GameColors::BmpBtnRed));

	if ((dwTime - Info().dwT1) > 1000)
	{
		Info().dwT1 = dwTime;
		Info().cStr[0]++;
	}

	if (Info().cStr[0] >= 5)
	{
		m_pGame->bSendCommand(MsgId::CommandCommon, CommonType::ReqCreatePortion, 0, 0, 0, 0, 0);
		m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::Manufacture);
		m_pGame->PlayGameSound('E', 42, 0);
	}
}

void DialogBox_Manufacture::DrawManufactureList(short sX, short sY, short msX, short msY, short msZ, char cLB)
{
	int iAdjX = 5;
	int iAdjY = 8;
	uint32_t dwTime = m_pGame->m_dwCurTime;
	short szX = Info().sSizeX;
	std::string cTemp, cTemp2;
	int iLoc;

	DrawNewDialogBox(InterfaceNdGame3, sX, sY, 0);
	DrawNewDialogBox(InterfaceNdText, sX, sY, 8);
	PutString(sX + iAdjX + 44, sY + iAdjY + 38, "Name", GameColors::UIBlack);
	PutString(sX + iAdjX + 171, sY + iAdjY + 38, "Max.Skill", GameColors::UIBlack);

	iLoc = 0;
	for (int i = 0; i < 13; i++)
		if (BuildItemManager::Get().GetDisplayList()[i + Info().sView] != 0) {

			auto itemInfo = ItemNameFormatter::Get().Format(m_pGame->FindItemIdByName(BuildItemManager::Get().GetDisplayList()[i + Info().sView]->m_cName.c_str()),  0);
			cTemp = itemInfo.name.c_str();
			cTemp2 = std::format("{}%", BuildItemManager::Get().GetDisplayList()[i + Info().sView]->m_iMaxSkill);

			if ((msX >= sX + 30) && (msX <= sX + 180) && (msY >= sY + iAdjY + 55 + iLoc * 15) && (msY <= sY + iAdjY + 69 + iLoc * 15))
			{
				PutString(sX + 30, sY + iAdjY + 55 + iLoc * 15, cTemp.c_str(), GameColors::UIWhite);
				PutString(sX + 190, sY + iAdjY + 55 + iLoc * 15, cTemp2.c_str(), GameColors::UIWhite);
			}
			else
			{
				if (BuildItemManager::Get().GetDisplayList()[i + Info().sView]->m_bBuildEnabled == true)
				{
					PutString(sX + 30, sY + iAdjY + 55 + iLoc * 15, cTemp.c_str(), GameColors::UIMagicBlue);
					PutString(sX + 190, sY + iAdjY + 55 + iLoc * 15, cTemp2.c_str(), GameColors::UIMagicBlue);
				}
				else
				{
					PutString(sX + 30, sY + iAdjY + 55 + iLoc * 15, cTemp.c_str(), GameColors::UILabel);
					PutString(sX + 190, sY + iAdjY + 55 + iLoc * 15, cTemp2.c_str(), GameColors::UILabel);
				}
			}
			iLoc++;
		}

	if ((Info().sView >= 1) && (BuildItemManager::Get().GetDisplayList()[Info().sView - 1] != 0))
		m_pGame->m_pSprite[InterfaceNdGame2]->Draw(sX + iAdjX + 225, sY + iAdjY + 210, 23);
	else m_pGame->m_pSprite[InterfaceNdGame2]->Draw(sX + iAdjX + 225, sY + iAdjY + 210, 23, hb::shared::sprite::DrawParams::TintedAlpha(GameColors::UIBlack.r, GameColors::UIBlack.g, GameColors::UIBlack.b, 0.7f));

	if (BuildItemManager::Get().GetDisplayList()[Info().sView + 13] != 0)
		m_pGame->m_pSprite[InterfaceNdGame2]->Draw(sX + iAdjX + 225, sY + iAdjY + 230, 24);
	else m_pGame->m_pSprite[InterfaceNdGame2]->Draw(sX + iAdjX + 225, sY + iAdjY + 230, 24, hb::shared::sprite::DrawParams::TintedAlpha(GameColors::UIBlack.r, GameColors::UIBlack.g, GameColors::UIBlack.b, 0.7f));

	if ((cLB != 0) && (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::Manufacture)) {
		if ((msX >= sX + iAdjX + 225) && (msX <= sX + iAdjX + 245) && (msY >= sY + iAdjY + 210) && (msY <= sY + iAdjY + 230)) {
			Info().sView--;
		}

		if ((msX >= sX + iAdjX + 225) && (msX <= sX + iAdjX + 245) && (msY >= sY + iAdjY + 230) && (msY <= sY + iAdjY + 250)) {
			if (BuildItemManager::Get().GetDisplayList()[Info().sView + 13] != 0)
				Info().sView++;
		}
	}
	if ((msZ != 0) && (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::Manufacture)) {
		Info().sView = Info().sView - msZ / 60;

	}
	if (BuildItemManager::Get().GetDisplayList()[Info().sView + 12] == 0)
	{
		while (1)
		{
			Info().sView--;
			if (Info().sView < 1) break;
			if (BuildItemManager::Get().GetDisplayList()[Info().sView + 12] != 0) break;
		}
	}
	if (Info().sView < 0) Info().sView = 0;

	PutAlignedString(sX, sX + Info().sSizeX, sY + 265, DRAW_DIALOGBOX_SKILLDLG2, GameColors::UILabel);
	PutAlignedString(sX, sX + Info().sSizeX, sY + 280, DRAW_DIALOGBOX_SKILLDLG3, GameColors::UILabel);
	PutAlignedString(sX, sX + Info().sSizeX, sY + 295, DRAW_DIALOGBOX_SKILLDLG4, GameColors::UILabel);
	PutAlignedString(sX, sX + Info().sSizeX, sY + 310, DRAW_DIALOGBOX_SKILLDLG5, GameColors::UILabel);
	PutAlignedString(sX, sX + Info().sSizeX, sY + 340, DRAW_DIALOGBOX_SKILLDLG6, GameColors::UILabel);
}

void DialogBox_Manufacture::DrawManufactureWaiting(short sX, short sY, short msX, short msY)
{
	int iAdjX = -1;
	int iAdjY = -7;
	uint32_t dwTime = m_pGame->m_dwCurTime;
	short szX = Info().sSizeX;
	std::string cTemp;
	int iLoc;

	DrawNewDialogBox(InterfaceNdGame3, sX, sY, 0);
	DrawNewDialogBox(InterfaceNdText, sX, sY, 8);
	m_pGame->m_pSprite[ItemPackPivotPoint + BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_iSprH]->Draw(sX + iAdjX + 62 + 5, sY + iAdjY + 84 + 17, BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_iSprFrame);

	auto itemInfo2 = ItemNameFormatter::Get().Format(m_pGame->FindItemIdByName(BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_cName.c_str()),  0);
	cTemp = itemInfo2.name.c_str();
	PutString(sX + iAdjX + 44 + 10 + 60, sY + iAdjY + 55, cTemp.c_str(), GameColors::UIWhite);

	cTemp = std::format(DRAW_DIALOGBOX_SKILLDLG7, BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_iSkillLimit, BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_iMaxSkill);
	PutString(sX + iAdjX + 44 + 10 + 60, sY + iAdjY + 55 + 2 * 15, cTemp.c_str(), GameColors::UILabel);
	PutString(sX + iAdjX + 44 + 10 + 60, sY + iAdjY + 55 + 3 * 15 + 5, DRAW_DIALOGBOX_SKILLDLG8, GameColors::UILabel);

	iLoc = 4;
	for (int elem = 1; elem <= 6; elem++)
	{
		if (BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_iElementCount[elem] != 0)
		{
			const char* elemName = nullptr;
			bool elemFlag = false;
			switch (elem) {
			case 1: elemName = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_cElementName1.c_str(); elemFlag = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_bElementFlag[1]; break;
			case 2: elemName = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_cElementName2.c_str(); elemFlag = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_bElementFlag[2]; break;
			case 3: elemName = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_cElementName3.c_str(); elemFlag = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_bElementFlag[3]; break;
			case 4: elemName = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_cElementName4.c_str(); elemFlag = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_bElementFlag[4]; break;
			case 5: elemName = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_cElementName5.c_str(); elemFlag = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_bElementFlag[5]; break;
			case 6: elemName = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_cElementName6.c_str(); elemFlag = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_bElementFlag[6]; break;
			}
			auto itemInfo3 = ItemNameFormatter::Get().Format(m_pGame->FindItemIdByName(elemName),  0);
			cTemp = itemInfo3.name.c_str();
			if (elemFlag)
				PutString(sX + iAdjX + 44 + 20 + 60, sY + iAdjY + 55 + iLoc * 15 + 5, cTemp.c_str(), GameColors::UILabel);
			else
				PutString(sX + iAdjX + 44 + 20 + 60, sY + iAdjY + 55 + iLoc * 15 + 5, cTemp.c_str(), GameColors::UIDisabled);
			iLoc++;
		}
	}

	if (BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_bBuildEnabled == true)
	{
		// Draw ingredient slots
		for (int slot = 0; slot < 6; slot++)
		{
			int slotX = (slot % 3) * 45;
			int slotY = (slot / 3) * 45;
			m_pGame->m_pSprite[InterfaceAddInterface]->Draw(sX + iAdjX + 55 + 30 + slotX + 13, sY + iAdjY + 55 + slotY + 180, 2);
		}

		// Draw items in slots
		if (Info().sV1 != -1) {
			CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV1]->m_sIDnum);
			if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 30 + 13, sY + iAdjY + 55 + 180, pCfg->m_sSpriteFrame);
		}
		if (Info().sV2 != -1) {
			CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV2]->m_sIDnum);
			if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 45 + 30 + 13, sY + iAdjY + 55 + 180, pCfg->m_sSpriteFrame);
		}
		if (Info().sV3 != -1) {
			CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV3]->m_sIDnum);
			if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 90 + 30 + 13, sY + iAdjY + 55 + 180, pCfg->m_sSpriteFrame);
		}
		if (Info().sV4 != -1) {
			CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV4]->m_sIDnum);
			if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 30 + 13, sY + iAdjY + 100 + 180, pCfg->m_sSpriteFrame);
		}
		if (Info().sV5 != -1) {
			CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV5]->m_sIDnum);
			if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 45 + 30 + 13, sY + iAdjY + 100 + 180, pCfg->m_sSpriteFrame);
		}
		if (Info().sV6 != -1) {
			CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV6]->m_sIDnum);
			if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 90 + 30 + 13, sY + iAdjY + 100 + 180, pCfg->m_sSpriteFrame);
		}

		PutAlignedString(sX, sX + szX, sY + iAdjY + 230 + 75, DRAW_DIALOGBOX_SKILLDLG15, GameColors::UILabel);
		PutAlignedString(sX, sX + szX, sY + iAdjY + 245 + 75, DRAW_DIALOGBOX_SKILLDLG16, GameColors::UILabel);
		PutAlignedString(sX, sX + szX, sY + iAdjY + 260 + 75, DRAW_DIALOGBOX_SKILLDLG17, GameColors::UILabel);

		if ((msX >= sX + iAdjX + 32) && (msX <= sX + iAdjX + 95) && (msY >= sY + iAdjY + 353) && (msY <= sY + iAdjY + 372))
			hb::shared::text::DrawText(GameFont::Bitmap1, sX + iAdjX + 25, sY + iAdjY + 330 + 23, "Back", hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicBlue));
		else hb::shared::text::DrawText(GameFont::Bitmap1, sX + iAdjX + 25, sY + iAdjY + 330 + 23, "Back", hb::shared::text::TextStyle::WithHighlight(GameColors::BmpBtnNormal));

		if ((msX >= sX + iAdjX + 160) && (msX <= sX + iAdjX + 255) && (msY >= sY + iAdjY + 353) && (msY <= sY + iAdjY + 372)) {
			if (Info().cStr[4] == 1)
				hb::shared::text::DrawText(GameFont::Bitmap1, sX + iAdjX + 153, sY + iAdjY + 330 + 23, "Manufacture", hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicBlue));
			else hb::shared::text::DrawText(GameFont::Bitmap1, sX + iAdjX + 153, sY + iAdjY + 330 + 23, "Manufacture", hb::shared::text::TextStyle::WithHighlight(GameColors::BmpBtnActive));
		}
		else {
			if (Info().cStr[4] == 1)
				hb::shared::text::DrawText(GameFont::Bitmap1, sX + iAdjX + 153, sY + iAdjY + 330 + 23, "Manufacture", hb::shared::text::TextStyle::WithHighlight(GameColors::BmpBtnNormal));
			else hb::shared::text::DrawText(GameFont::Bitmap1, sX + iAdjX + 153, sY + iAdjY + 330 + 23, "Manufacture", hb::shared::text::TextStyle::WithHighlight(GameColors::BmpBtnActive));
		}
	}
	else {
		PutAlignedString(sX, sX + szX, sY + iAdjY + 200 + 75, DRAW_DIALOGBOX_SKILLDLG18, GameColors::UILabel);
		PutAlignedString(sX, sX + szX, sY + iAdjY + 215 + 75, DRAW_DIALOGBOX_SKILLDLG19, GameColors::UILabel);
		PutAlignedString(sX, sX + szX, sY + iAdjY + 230 + 75, DRAW_DIALOGBOX_SKILLDLG20, GameColors::UILabel);
		if ((msX >= sX + iAdjX + 32) && (msX <= sX + iAdjX + 95) && (msY >= sY + iAdjY + 353) && (msY <= sY + iAdjY + 372))
			hb::shared::text::DrawText(GameFont::Bitmap1, sX + iAdjX + 25, sY + iAdjY + 330 + 23, "Back", hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicBlue));
		else hb::shared::text::DrawText(GameFont::Bitmap1, sX + iAdjX + 25, sY + iAdjY + 330 + 23, "Back", hb::shared::text::TextStyle::WithHighlight(GameColors::BmpBtnNormal));
	}
}

void DialogBox_Manufacture::DrawManufactureInProgress(short sX, short sY)
{
	int iAdjX = -1;
	int iAdjY = -7;
	uint32_t dwTime = m_pGame->m_dwCurTime;
	short szX = Info().sSizeX;
	std::string cTemp;
	int iLoc;

	DrawNewDialogBox(InterfaceNdGame3, sX, sY, 0);
	DrawNewDialogBox(InterfaceNdText, sX, sY, 8);
	m_pGame->m_pSprite[ItemPackPivotPoint + BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_iSprH]->Draw(sX + iAdjX + 62 + 5, sY + iAdjY + 84 + 17, BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_iSprFrame);

	auto itemInfo4 = ItemNameFormatter::Get().Format(m_pGame->FindItemIdByName(BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_cName.c_str()),  0);
	cTemp = itemInfo4.name.c_str();
	PutString(sX + iAdjX + 44 + 10 + 60, sY + iAdjY + 55, cTemp.c_str(), GameColors::UIWhite);

	cTemp = std::format(DRAW_DIALOGBOX_SKILLDLG7, BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_iSkillLimit, BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_iMaxSkill);
	PutString(sX + iAdjX + 44 + 10 + 60, sY + iAdjY + 55 + 2 * 15, cTemp.c_str(), GameColors::UILabel);
	PutString(sX + iAdjX + 44 + 10 + 60, sY + iAdjY + 55 + 3 * 15 + 5, DRAW_DIALOGBOX_SKILLDLG8, GameColors::UILabel);

	iLoc = 4;
	for (int elem = 1; elem <= 6; elem++)
	{
		if (BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_iElementCount[elem] != 0)
		{
			const char* elemName = nullptr;
			bool elemFlag = false;
			switch (elem) {
			case 1: elemName = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_cElementName1.c_str(); elemFlag = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_bElementFlag[1]; break;
			case 2: elemName = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_cElementName2.c_str(); elemFlag = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_bElementFlag[2]; break;
			case 3: elemName = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_cElementName3.c_str(); elemFlag = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_bElementFlag[3]; break;
			case 4: elemName = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_cElementName4.c_str(); elemFlag = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_bElementFlag[4]; break;
			case 5: elemName = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_cElementName5.c_str(); elemFlag = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_bElementFlag[5]; break;
			case 6: elemName = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_cElementName6.c_str(); elemFlag = BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_bElementFlag[6]; break;
			}
			auto itemInfo5 = ItemNameFormatter::Get().Format(m_pGame->FindItemIdByName(elemName),  0);
			cTemp = itemInfo5.name.c_str();
			if (elemFlag)
				PutString(sX + iAdjX + 44 + 20 + 60, sY + iAdjY + 55 + iLoc * 15 + 5, cTemp.c_str(), GameColors::UILabel);
			else
				PutString(sX + iAdjX + 44 + 20 + 60, sY + iAdjY + 55 + iLoc * 15 + 5, cTemp.c_str(), GameColors::UIDisabledMed);
			iLoc++;
		}
	}

	// Draw ingredient slots
	for (int slot = 0; slot < 6; slot++)
	{
		int slotX = (slot % 3) * 45;
		int slotY = (slot / 3) * 45;
		m_pGame->m_pSprite[InterfaceAddInterface]->Draw(sX + iAdjX + 55 + 30 + slotX + 13, sY + iAdjY + 55 + slotY + 180, 2);
	}

	// Draw items in slots
	if (Info().sV1 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV1]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 30 + 13, sY + iAdjY + 55 + 180, pCfg->m_sSpriteFrame);
	}
	if (Info().sV2 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV2]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 45 + 30 + 13, sY + iAdjY + 55 + 180, pCfg->m_sSpriteFrame);
	}
	if (Info().sV3 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV3]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 90 + 30 + 13, sY + iAdjY + 55 + 180, pCfg->m_sSpriteFrame);
	}
	if (Info().sV4 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV4]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 30 + 13, sY + iAdjY + 100 + 180, pCfg->m_sSpriteFrame);
	}
	if (Info().sV5 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV5]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 45 + 30 + 13, sY + iAdjY + 100 + 180, pCfg->m_sSpriteFrame);
	}
	if (Info().sV6 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV6]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + 90 + 30 + 13, sY + iAdjY + 100 + 180, pCfg->m_sSpriteFrame);
	}

	PutString(sX + iAdjX + 33, sY + iAdjY + 230 + 75, DRAW_DIALOGBOX_SKILLDLG29, GameColors::UILabel);
	PutString(sX + iAdjX + 33, sY + iAdjY + 245 + 75, DRAW_DIALOGBOX_SKILLDLG30, GameColors::UILabel);

	if ((dwTime - Info().dwT1) > 1000)
	{
		Info().dwT1 = dwTime;
		Info().cStr[1]++;
		if (Info().cStr[1] >= 7) Info().cStr[1] = 7;
	}

	if (Info().cStr[1] == 4)
	{
		m_pGame->bSendCommand(MsgId::CommandCommon, CommonType::BuildItem, 0, 0, 0, 0, BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_cName.c_str());
		Info().cStr[1]++;
	}
}

void DialogBox_Manufacture::DrawManufactureDone(short sX, short sY, short msX, short msY)
{
	int iAdjX = -1;
	int iAdjY = -7;
	uint32_t dwTime = m_pGame->m_dwCurTime;
	std::string cTemp;

	DrawNewDialogBox(InterfaceNdGame3, sX, sY, 0);
	DrawNewDialogBox(InterfaceNdText, sX, sY, 8);
	m_pGame->m_pSprite[ItemPackPivotPoint + BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_iSprH]->Draw(sX + iAdjX + 62 + 5, sY + iAdjY + 84 + 17, BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_iSprFrame);

	auto itemInfo6 = ItemNameFormatter::Get().Format(m_pGame->FindItemIdByName(BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_cName.c_str()),  0);

	cTemp = itemInfo6.name.c_str();
	PutString(sX + iAdjX + 44 + 10 + 60, sY + iAdjY + 55, cTemp.c_str(), GameColors::UIWhite);

	if (Info().cStr[2] == 1) {
		PutString(sX + iAdjX + 33 + 11, sY + iAdjY + 200 - 45, DRAW_DIALOGBOX_SKILLDLG31, GameColors::UILabel);

		std::string resultBuf;
		if (static_cast<ItemType>(Info().sV1) == ItemType::Material) {
			resultBuf = std::format(DRAW_DIALOGBOX_SKILLDLG32, Info().cStr[3]);
			PutString(sX + iAdjX + 33 + 11, sY + iAdjY + 215 - 45, resultBuf.c_str(), GameColors::UILabel);
		}
		else {
			resultBuf = std::format(DRAW_DIALOGBOX_SKILLDLG33, static_cast<int>(Info().cStr[3]) + 100);
			PutString(sX + iAdjX + 33, sY + iAdjY + 215 - 45, resultBuf.c_str(), GameColors::UILabel);
		}
	}
	else {
		PutString(sX + iAdjX + 33 + 11, sY + iAdjY + 200, DRAW_DIALOGBOX_SKILLDLG34, GameColors::UILabel);
	}

	if ((msX >= sX + iAdjX + 32) && (msX <= sX + iAdjX + 95) && (msY >= sY + iAdjY + 353) && (msY <= sY + iAdjY + 372))
		hb::shared::text::DrawText(GameFont::Bitmap1, sX + iAdjX + 35, sY + iAdjY + 330 + 23, "Back", hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicBlue));
	else hb::shared::text::DrawText(GameFont::Bitmap1, sX + iAdjX + 35, sY + iAdjY + 330 + 23, "Back", hb::shared::text::TextStyle::WithHighlight(GameColors::BmpBtnNormal));
}

void DialogBox_Manufacture::DrawCraftingWaiting(short sX, short sY, short msX, short msY)
{
	int iAdjX = 5;
	int iAdjY = 8;
	uint32_t dwTime = m_pGame->m_dwCurTime;

	m_pGame->m_pSprite[InterfaceCrafting]->Draw(sX, sY, 0);

	if (Info().sV1 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV1]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + (1 - (rand() % 3)), sY + iAdjY + 55 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame);
	}
	if (Info().sV2 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV2]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 65 + 45 + (1 - (rand() % 3)), sY + iAdjY + 40 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame);
	}
	if (Info().sV3 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV3]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 65 + 90 + (1 - (rand() % 3)), sY + iAdjY + 55 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame);
	}
	if (Info().sV4 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV4]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 65 + (1 - (rand() % 3)), sY + iAdjY + 100 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame);
	}
	if (Info().sV5 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV5]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 65 + 45 + (1 - (rand() % 3)), sY + iAdjY + 115 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame);
	}
	if (Info().sV6 != -1) {
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV6]->m_sIDnum);
		if (pCfg) m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 75 + 90 + (1 - (rand() % 3)), sY + iAdjY + 100 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame);
	}

	if ((msX >= sX + iAdjX + 60) && (msX <= sX + iAdjX + 153) && (msY >= sY + iAdjY + 175) && (msY <= sY + iAdjY + 195))
		hb::shared::text::DrawText(GameFont::Bitmap1, sX + iAdjX + 60, sY + iAdjY + 175, "Try Now!", hb::shared::text::TextStyle::WithHighlight(GameColors::BmpBtnBlue));
	else hb::shared::text::DrawText(GameFont::Bitmap1, sX + iAdjX + 60, sY + iAdjY + 175, "Try Now!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicBlue));
}

void DialogBox_Manufacture::DrawCraftingInProgress(short sX, short sY)
{
	int iAdjX = 5;
	int iAdjY = 8;
	uint32_t dwTime = m_pGame->m_dwCurTime;

	m_pGame->m_pSprite[InterfaceCrafting]->Draw(sX, sY, 0);

	if (Info().sV1 != -1)
	{
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV1]->m_sIDnum);
		if (pCfg) {
			m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 55 + (1 - (rand() % 3)), sY + iAdjY + 55 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame);
			if ((pCfg->GetItemType() == ItemType::Equip) && (pCfg->GetEquipPos() == EquipPos::Neck))
				m_pGame->m_iContributionPrice = 10;
		}
	}
	if (Info().sV2 != -1)
	{
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV2]->m_sIDnum);
		if (pCfg) {
			m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 65 + 45 + (1 - (rand() % 3)), sY + iAdjY + 40 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame);
			if ((pCfg->GetItemType() == ItemType::Equip) && (pCfg->GetEquipPos() == EquipPos::Neck))
				m_pGame->m_iContributionPrice = 10;
		}
	}
	if (Info().sV3 != -1)
	{
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV3]->m_sIDnum);
		if (pCfg) {
			m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 65 + 90 + (1 - (rand() % 3)), sY + iAdjY + 55 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame);
			if ((pCfg->GetItemType() == ItemType::Equip) && (pCfg->GetEquipPos() == EquipPos::Neck))
				m_pGame->m_iContributionPrice = 10;
		}
	}
	if (Info().sV4 != -1)
	{
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV4]->m_sIDnum);
		if (pCfg) {
			m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 65 + (1 - (rand() % 3)), sY + iAdjY + 100 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame);
			if ((pCfg->GetItemType() == ItemType::Equip) && (pCfg->GetEquipPos() == EquipPos::Neck))
				m_pGame->m_iContributionPrice = 10;
		}
	}
	if (Info().sV5 != -1)
	{
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV5]->m_sIDnum);
		if (pCfg) {
			m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 65 + 45 + (1 - (rand() % 3)), sY + iAdjY + 115 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame);
			if ((pCfg->GetItemType() == ItemType::Equip) && (pCfg->GetEquipPos() == EquipPos::Neck))
				m_pGame->m_iContributionPrice = 10;
		}
	}
	if (Info().sV6 != -1)
	{
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV6]->m_sIDnum);
		if (pCfg) {
			m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + iAdjX + 75 + 90 + (1 - (rand() % 3)), sY + iAdjY + 100 + (1 - (rand() % 3)), pCfg->m_sSpriteFrame);
			if ((pCfg->GetItemType() == ItemType::Equip) && (pCfg->GetEquipPos() == EquipPos::Neck))
				m_pGame->m_iContributionPrice = 10;
		}
	}

	hb::shared::text::DrawText(GameFont::Bitmap1, sX + iAdjX + 60, sY + iAdjY + 175, "Creating...", hb::shared::text::TextStyle::WithHighlight(GameColors::BmpBtnRed));

	if ((dwTime - Info().dwT1) > 1000)
	{
		Info().dwT1 = dwTime;
		Info().cStr[1]++;
	}
	if (Info().cStr[1] >= 5)
	{
		m_pGame->bSendCommand(MsgId::CommandCommon, CommonType::CraftItem, 0, 0, 0, 0, 0);
		m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::Manufacture);
		m_pGame->PlayGameSound('E', 42, 0);
	}
}

bool DialogBox_Manufacture::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;
	int iAdjX = 5;
	int iAdjY = 8;

	switch (Info().cMode) {
	case 1: // Alchemy
		if ((msX >= sX + iAdjX + 60) && (msX <= sX + iAdjX + 153) && (msY >= sY + iAdjY + 175) && (msY <= sY + iAdjY + 195))
		{
			Info().cMode = 2;
			Info().cStr[0] = 1;
			Info().dwT1 = m_pGame->m_dwCurTime;
			m_pGame->PlayGameSound('E', 14, 5);
			m_pGame->AddEventList(DLGBOX_CLICK_SKILLDLG1, 10);
			m_pGame->PlayGameSound('E', 41, 0);
			return true;
		}
		break;

	case 7: // Crafting
		if ((msX >= sX + iAdjX + 60) && (msX <= sX + iAdjX + 153) && (msY >= sY + iAdjY + 175) && (msY <= sY + iAdjY + 195))
		{
			if (Info().sV1 == -1)
			{
				m_pGame->AddEventList(DLGBOX_CLICK_SKILLDLG2, 10);
				m_pGame->PlayGameSound('E', 14, 5);
			}
			else
			{
				Info().cMode = 8;
				Info().dwT1 = m_pGame->m_dwCurTime;
				Info().cStr[1] = 1;
				m_pGame->PlayGameSound('E', 14, 5);
				m_pGame->AddEventList(DLGBOX_CLICK_SKILLDLG3, 10);
				m_pGame->PlayGameSound('E', 51, 0);
			}
			return true;
		}
		break;

	case 3: // Manufacture list
		iAdjX = 5;
		iAdjY = 8;
		for (int i = 0; i < 13; i++)
			if (BuildItemManager::Get().GetDisplayList()[i + Info().sView] != 0)
			{
				if ((msX >= sX + iAdjX + 44) && (msX <= sX + iAdjX + 135 + 44) && (msY >= sY + iAdjY + 55 + i * 15) && (msY <= sY + iAdjY + 55 + 14 + i * 15)) {
					Info().cMode = 4;
					Info().cStr[0] = i + Info().sView;
					m_pGame->PlayGameSound('E', 14, 5);
					return true;
				}
			}
		break;

	case 4: // Manufacture waiting
		iAdjX = -1;
		iAdjY = -7;
		if (BuildItemManager::Get().GetDisplayList()[Info().cStr[0]]->m_bBuildEnabled == true)
		{
			if ((msX >= sX + iAdjX + 32) && (msX <= sX + iAdjX + 95) && (msY >= sY + iAdjY + 353) && (msY <= sY + iAdjY + 372)) {
				// Back
				ResetItemSlots();
				Info().cMode = 3;
				m_pGame->PlayGameSound('E', 14, 5);
				return true;
			}

			if ((msX >= sX + iAdjX + 160) && (msX <= sX + iAdjX + 255) && (msY >= sY + iAdjY + 353) && (msY <= sY + iAdjY + 372))
			{
				// Manufacture
				if (Info().cStr[4] == 1)
				{
					Info().cMode = 5;
					Info().cStr[1] = 0;
					Info().dwT1 = m_pGame->m_dwCurTime;
					m_pGame->PlayGameSound('E', 14, 5);
					m_pGame->PlayGameSound('E', 44, 0);
				}
				return true;
			}
		}
		else
		{
			if ((msX >= sX + iAdjX + 32) && (msX <= sX + iAdjX + 95) && (msY >= sY + iAdjY + 353) && (msY <= sY + iAdjY + 372))
			{
				// Back
				ResetItemSlots();
				Info().cMode = 3;
				m_pGame->PlayGameSound('E', 14, 5);
				return true;
			}
		}
		break;

	case 6: // Manufacture done
		iAdjX = -1;
		iAdjY = -7;
		if ((msX >= sX + iAdjX + 32) && (msX <= sX + iAdjX + 95) && (msY >= sY + iAdjY + 353) && (msY <= sY + iAdjY + 372)) {
			// Back
			ResetItemSlots();
			Info().cMode = 3;
			m_pGame->PlayGameSound('E', 14, 5);
			return true;
		}
		break;
	}

	return false;
}

// Helper: Check if clicking on item in a manufacture slot and handle selection
bool DialogBox_Manufacture::CheckSlotItemClick(int slotIndex, int itemIdx, int drawX, int drawY, short msX, short msY)
{
	if (itemIdx == -1 || m_pGame->m_pItemList[itemIdx] == nullptr)
		return false;

	CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[itemIdx]->m_sIDnum);
	if (!pCfg) return false;

	int spriteIdx = ItemPackPivotPoint + pCfg->m_sSprite;
	m_pGame->m_pSprite[spriteIdx]->CalculateBounds(drawX, drawY, pCfg->m_sSpriteFrame);
	auto bounds = m_pGame->m_pSprite[spriteIdx]->GetBoundRect();

	if (msX > bounds.left && msX < bounds.right && msY > bounds.top && msY < bounds.bottom)
	{
		// Clear the slot
		switch (slotIndex)
		{
		case 1: Info().sV1 = -1; break;
		case 2: Info().sV2 = -1; break;
		case 3: Info().sV3 = -1; break;
		case 4: Info().sV4 = -1; break;
		case 5: Info().sV5 = -1; break;
		case 6: Info().sV6 = -1; break;
		}
		m_pGame->m_bIsItemDisabled[itemIdx] = false;
		CursorTarget::SetSelection(SelectedObjectType::Item, itemIdx, msX - drawX, msY - drawY);
		return true;
	}
	return false;
}

PressResult DialogBox_Manufacture::OnPress(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;
	const int iAdjX = 5;
	const int iAdjY = 10;

	short sArray[7] = { 0 };
	sArray[1] = Info().sV1;
	sArray[2] = Info().sV2;
	sArray[3] = Info().sV3;
	sArray[4] = Info().sV4;
	sArray[5] = Info().sV5;
	sArray[6] = Info().sV6;

	switch (Info().cMode)
	{
	case 1: // Alchemy waiting
		for (int i = 1; i <= 6; i++)
		{
			int itemDrawX, itemDrawY;
			switch (i)
			{
			case 1: itemDrawX = sX + iAdjX + 55;          itemDrawY = sY + iAdjY + 55; break;
			case 2: itemDrawX = sX + iAdjX + 55 + 45 * 1; itemDrawY = sY + iAdjY + 55; break;
			case 3: itemDrawX = sX + iAdjX + 55 + 45 * 2; itemDrawY = sY + iAdjY + 55; break;
			case 4: itemDrawX = sX + iAdjX + 55;          itemDrawY = sY + iAdjY + 100; break;
			case 5: itemDrawX = sX + iAdjX + 55 + 45 * 1; itemDrawY = sY + iAdjY + 100; break;
			case 6: itemDrawX = sX + iAdjX + 55 + 45 * 2; itemDrawY = sY + iAdjY + 100; break;
			default: continue;
			}
			if (CheckSlotItemClick(i, sArray[i], itemDrawX, itemDrawY, msX, msY))
				return PressResult::ItemSelected;
		}
		break;

	case 4: // Manufacture waiting
		for (int i = 1; i <= 6; i++)
		{
			int itemDrawX, itemDrawY;
			switch (i)
			{
			case 1: itemDrawX = sX + iAdjX + 55 + 30 + 13;          itemDrawY = sY + iAdjY + 55 + 180; break;
			case 2: itemDrawX = sX + iAdjX + 55 + 45 * 1 + 30 + 13; itemDrawY = sY + iAdjY + 55 + 180; break;
			case 3: itemDrawX = sX + iAdjX + 55 + 45 * 2 + 30 + 13; itemDrawY = sY + iAdjY + 55 + 180; break;
			case 4: itemDrawX = sX + iAdjX + 55 + 30 + 13;          itemDrawY = sY + iAdjY + 100 + 180; break;
			case 5: itemDrawX = sX + iAdjX + 55 + 45 * 1 + 30 + 13; itemDrawY = sY + iAdjY + 100 + 180; break;
			case 6: itemDrawX = sX + iAdjX + 55 + 45 * 2 + 30 + 13; itemDrawY = sY + iAdjY + 100 + 180; break;
			default: continue;
			}
			if (CheckSlotItemClick(i, sArray[i], itemDrawX, itemDrawY, msX, msY))
			{
				Info().cStr[4] = static_cast<char>(BuildItemManager::Get().ValidateCurrentRecipe());
				return PressResult::ItemSelected;
			}
		}
		break;

	case 7: // Crafting waiting
		for (int i = 1; i <= 6; i++)
		{
			int itemDrawX, itemDrawY;
			switch (i)
			{
			case 1: itemDrawX = sX + iAdjX + 55;          itemDrawY = sY + iAdjY + 55; break;
			case 2: itemDrawX = sX + iAdjX + 65 + 45 * 1; itemDrawY = sY + iAdjY + 40; break;
			case 3: itemDrawX = sX + iAdjX + 65 + 45 * 2; itemDrawY = sY + iAdjY + 55; break;
			case 4: itemDrawX = sX + iAdjX + 65;          itemDrawY = sY + iAdjY + 100; break;
			case 5: itemDrawX = sX + iAdjX + 65 + 45 * 1; itemDrawY = sY + iAdjY + 115; break;
			case 6: itemDrawX = sX + iAdjX + 75 + 45 * 2; itemDrawY = sY + iAdjY + 100; break;
			default: continue;
			}
			if (CheckSlotItemClick(i, sArray[i], itemDrawX, itemDrawY, msX, msY))
				return PressResult::ItemSelected;
		}
		break;
	}

	return PressResult::Normal;
}

bool DialogBox_Manufacture::TryAddItemToSlot(char cItemID, bool updateBuildStatus)
{
	auto& info = Info();
	int* slots[] = { &info.sV1, &info.sV2, &info.sV3, &info.sV4, &info.sV5, &info.sV6 };

	for (int i = 0; i < 6; i++)
	{
		if (*slots[i] == -1)
		{
			*slots[i] = cItemID;
			if (updateBuildStatus)
				info.cStr[4] = static_cast<char>(BuildItemManager::Get().ValidateCurrentRecipe());

			// Only disable non-stackable items (stackable consumables can be added multiple times)
			CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[cItemID]->m_sIDnum);
			if (!pCfg || pCfg->GetItemType() != ItemType::Consume ||
				m_pGame->m_pItemList[cItemID]->m_dwCount <= 1)
			{
				m_pGame->m_bIsItemDisabled[cItemID] = true;
			}
			return true;
		}
	}
	return false;
}

bool DialogBox_Manufacture::OnItemDrop(short msX, short msY)
{
	if (m_pGame->m_pPlayer->m_Controller.GetCommand() < 0) return false;

	char cItemID = static_cast<char>(CursorTarget::GetSelectedID());
	if (m_pGame->m_pItemList[cItemID] == nullptr) return false;
	if (m_pGame->m_bIsItemDisabled[cItemID]) return false;
	CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[cItemID]->m_sIDnum);
	if (!pCfg) return false;

	// Check if other dialogs are blocking
	if (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropExternal))
	{
		AddEventList(BITEMDROP_SKILLDIALOG1, 10);
		return false;
	}
	if (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::NpcActionQuery) &&
		(m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cMode == 1 ||
		 m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cMode == 2))
	{
		AddEventList(BITEMDROP_SKILLDIALOG1, 10);
		return false;
	}
	if (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::SellOrRepair))
	{
		AddEventList(BITEMDROP_SKILLDIALOG1, 10);
		return false;
	}

	auto& info = Info();

	switch (info.cMode) {
	case 1: // Alchemy
	{
		// Check consumable item count - can't add if all instances are already used
		if (pCfg->GetItemType() == ItemType::Consume)
		{
			int iConsumeNum = 0;
			if (info.sV1 == cItemID) iConsumeNum++;
			if (info.sV2 == cItemID) iConsumeNum++;
			if (info.sV3 == cItemID) iConsumeNum++;
			if (info.sV4 == cItemID) iConsumeNum++;
			if (info.sV5 == cItemID) iConsumeNum++;
			if (info.sV6 == cItemID) iConsumeNum++;
			if (iConsumeNum >= static_cast<int>(m_pGame->m_pItemList[cItemID]->m_dwCount)) return false;
		}

		// Only allow EAT, CONSUME, or NONE item types for alchemy
		if (pCfg->GetItemType() != ItemType::Eat &&
			pCfg->GetItemType() != ItemType::Consume &&
			pCfg->GetItemType() != ItemType::None)
		{
			return false;
		}

		if (!TryAddItemToSlot(cItemID, false))
			AddEventList(BITEMDROP_SKILLDIALOG4, 10);
		break;
	}

	case 4: // Manufacture
	{
		// Check consumable item count
		if (pCfg->GetItemType() == ItemType::Consume)
		{
			int iConsumeNum = 0;
			if (info.sV1 == cItemID) iConsumeNum++;
			if (info.sV2 == cItemID) iConsumeNum++;
			if (info.sV3 == cItemID) iConsumeNum++;
			if (info.sV4 == cItemID) iConsumeNum++;
			if (info.sV5 == cItemID) iConsumeNum++;
			if (info.sV6 == cItemID) iConsumeNum++;
			if (iConsumeNum >= static_cast<int>(m_pGame->m_pItemList[cItemID]->m_dwCount)) return false;
		}

		if (!TryAddItemToSlot(cItemID, true))
			AddEventList(BITEMDROP_SKILLDIALOG4, 10);
		break;
	}

	case 7: // Crafting
	{
		// Only allow specific item types for crafting
		if (pCfg->GetItemType() != ItemType::None &&      // Merien Stone
			pCfg->GetItemType() != ItemType::Equip &&     // Necklaces, Rings
			pCfg->GetItemType() != ItemType::Consume &&   // Stones
			pCfg->GetItemType() != ItemType::Material)    // Craftwares
		{
			return false;
		}

		if (!TryAddItemToSlot(cItemID, false))
			AddEventList(BITEMDROP_SKILLDIALOG4, 10);
		break;
	}
	}

	return true;
}

void DialogBox_Manufacture::ResetItemSlots()
{
	if ((Info().sV1 != -1) && (m_pGame->m_pItemList[Info().sV1] != 0))
		m_pGame->m_bIsItemDisabled[Info().sV1] = false;
	if ((Info().sV2 != -1) && (m_pGame->m_pItemList[Info().sV2] != 0))
		m_pGame->m_bIsItemDisabled[Info().sV2] = false;
	if ((Info().sV3 != -1) && (m_pGame->m_pItemList[Info().sV3] != 0))
		m_pGame->m_bIsItemDisabled[Info().sV3] = false;
	if ((Info().sV4 != -1) && (m_pGame->m_pItemList[Info().sV4] != 0))
		m_pGame->m_bIsItemDisabled[Info().sV4] = false;
	if ((Info().sV5 != -1) && (m_pGame->m_pItemList[Info().sV5] != 0))
		m_pGame->m_bIsItemDisabled[Info().sV5] = false;
	if ((Info().sV6 != -1) && (m_pGame->m_pItemList[Info().sV6] != 0))
		m_pGame->m_bIsItemDisabled[Info().sV6] = false;

	Info().sV1 = -1;
	Info().sV2 = -1;
	Info().sV3 = -1;
	Info().sV4 = -1;
	Info().sV5 = -1;
	Info().sV6 = -1;
	Info().cStr[0] = 0;
	Info().cStr[1] = 0;
	Info().cStr[4] = 0;
}
