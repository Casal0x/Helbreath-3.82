#include "DialogBox_Slates.h"
#include "CursorTarget.h"
#include "Game.h"
#include "GameFonts.h"
#include "lan_eng.h"
#include "SpriteID.h"
#include "TextLibExt.h"
#include "NetMessages.h"

using namespace hb::shared::net;
using namespace hb::shared::item;

DialogBox_Slates::DialogBox_Slates(CGame* pGame)
	: IDialogBox(DialogBoxId::Slates, pGame)
{
	SetDefaultRect(100 , 60 , 258, 339);
}

void DialogBox_Slates::OnDraw(short msX, short msY, short msZ, char cLB)
{
	int iAdjX, iAdjY;
	short sX, sY;
	uint32_t dwTime = m_pGame->m_dwCurTime;

	iAdjX = 5;
	iAdjY = 8;

	switch (Info().cMode) {
	case 1:
		sX = Info().sX;
		sY = Info().sY;
		iAdjX = -1;
		iAdjY = -7;

		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_INVENTORY, sX, sY, 4);

		if (Info().sV1 != -1) {
			DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_INVENTORY, sX + 20, sY + 12, 5);
		}
		if (Info().sV2 != -1) {
			DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_INVENTORY, sX + 20, sY + 87, 6);
		}
		if (Info().sV3 != -1) {
			DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_INVENTORY, sX + 85, sY + 32, 7);
		}
		if (Info().sV4 != -1) {
			DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_INVENTORY, sX + 70, sY + 97, 8);
		}

		if ((Info().sV1 != -1) && (Info().sV2 != -1) && (Info().sV3 != -1) && (Info().sV4 != -1)) {
			if ((msX >= sX + 120) && (msX <= sX + 180) && (msY >= sY + 150) && (msY <= sY + 165))
				hb::shared::text::DrawText(GameFont::Bitmap1, sX + 120, sY + 150, "Casting", hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicBlue));
			else hb::shared::text::DrawText(GameFont::Bitmap1, sX + 120, sY + 150, "Casting", hb::shared::text::TextStyle::WithHighlight(GameColors::BmpBtnNormal));
		}
		break;

	case 2:
		m_pGame->PlayGameSound('E', 16, 0);
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
		m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_INVENTORY]->Draw(sX, sY, 4);
		m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_INVENTORY]->Draw(sX + 22, sY + 14, 3);
		PutAlignedString(199, 438, 201, "KURURURURURURURURU!!!", GameColors::UISlatesPink);
		PutAlignedString(200, 439, 200, "KURURURURURURURURU!!!", GameColors::UISlatesCyan);

		if ((dwTime - Info().dwT1) > 1000)
		{
			Info().dwT1 = dwTime;
			Info().cStr[0]++;
		}
		if (Info().cStr[0] >= 5)
		{
			bSendCommand(MsgId::CommandCommon, CommonType::ReqCreateSlate, 0, Info().sV1, Info().sV2, Info().sV3, 0, Info().sV4);
			DisableDialogBox(DialogBoxId::Slates);
		}
		break;
	}
}

bool DialogBox_Slates::OnClick(short msX, short msY)
{
	short sX, sY;

	sX = Info().sX;
	sY = Info().sY;

	switch (Info().cMode) {
	case 1:
		if ((Info().sV1 != -1) && (Info().sV2 != -1) && (Info().sV3 != -1) && (Info().sV4 != -1)) {
			if ((msX >= sX + 120) && (msX <= sX + 180) && (msY >= sY + 150) && (msY <= sY + 165)) {
				Info().cMode = 2;
				PlaySoundEffect('E', 14, 5);
			}
		}
		break;
	}
	return false;
}

bool DialogBox_Slates::OnItemDrop(short msX, short msY)
{
	if (m_pGame->m_pPlayer->m_Controller.GetCommand() < 0) return false;

	char cItemID = (char)CursorTarget::GetSelectedID();
	if (m_pGame->m_pItemList[cItemID] == nullptr) return false;
	if (m_pGame->m_bIsItemDisabled[cItemID]) return false;

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

	switch (Info().cMode) {
	case 1:
	{
		// Only accept slate items (sprite frame 151-154)
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[cItemID]->m_sIDnum);
		if (pCfg && (pCfg->GetItemType() == ItemType::UseSkillEnableDialogBox) &&
			(pCfg->m_sSpriteFrame >= 151) &&
			(pCfg->m_sSpriteFrame <= 154))
		{
			char cItemIDText[20];
			switch (pCfg->m_sSpriteFrame) {
			case 151:
				if (Info().sV1 == -1) {
					m_pGame->m_bIsItemDisabled[cItemID] = true;
					Info().sV1 = cItemID;
					std::snprintf(cItemIDText, sizeof(cItemIDText), "Item ID : %d", cItemID);
					AddEventList(cItemIDText, 10);
				}
				break;
			case 152:
				if (Info().sV2 == -1) {
					m_pGame->m_bIsItemDisabled[cItemID] = true;
					Info().sV2 = cItemID;
					std::snprintf(cItemIDText, sizeof(cItemIDText), "Item ID : %d", cItemID);
					AddEventList(cItemIDText, 10);
				}
				break;
			case 153:
				if (Info().sV3 == -1) {
					m_pGame->m_bIsItemDisabled[cItemID] = true;
					Info().sV3 = cItemID;
					std::snprintf(cItemIDText, sizeof(cItemIDText), "Item ID : %d", cItemID);
					AddEventList(cItemIDText, 10);
				}
				break;
			case 154:
				if (Info().sV4 == -1) {
					m_pGame->m_bIsItemDisabled[cItemID] = true;
					Info().sV4 = cItemID;
					std::snprintf(cItemIDText, sizeof(cItemIDText), "Item ID : %d", cItemID);
					AddEventList(cItemIDText, 10);
				}
				break;
			}
		}
		break;
	}
	}

	return true;
}
