#include "DialogBox_Bank.h"
#include "CursorTarget.h"
#include "Game.h"
#include "IInput.h"
#include "lan_eng.h"

using namespace hb::item;

DialogBox_Bank::DialogBox_Bank(CGame* pGame)
	: IDialogBox(DialogBoxId::Bank, pGame)
{
	SetDefaultRect(60 , 50 , 258, 339);
	Info().sV1 = 13; // Number of visible item lines in scrollable list
}

void DialogBox_Bank::OnDraw(short msX, short msY, short msZ, char cLB)
{
	if (!m_pGame->EnsureItemConfigsLoaded()) return;
	short sX = Info().sX;
	short sY = Info().sY;
	short szX = Info().sSizeX - 5;

	m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 2);
	m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_TEXT, sX, sY, 21);

	switch (Info().cMode) {
	case -1:
		PutString(sX + 30 + 15, sY + 70, DRAW_DIALOGBOX_BANK1, GameColors::UIBlack.ToColorRef());
		PutString(sX + 30 + 15, sY + 85, DRAW_DIALOGBOX_BANK2, GameColors::UIBlack.ToColorRef());
		break;

	case 0:
		DrawItemList(sX, sY, szX, msX, msY, msZ, cLB);
		break;
	}
}

void DialogBox_Bank::DrawItemList(short sX, short sY, short szX, short msX, short msY, short msZ, char cLB)
{
	char cStr1[64], cStr2[64], cStr3[64];
	bool bFlag = false;
	int iLoc = 45;

	std::memset(cStr1, 0, sizeof(cStr1));
	std::memset(cStr2, 0, sizeof(cStr2));
	std::memset(cStr3, 0, sizeof(cStr3));

	for (int i = 0; i < Info().sV1; i++) {
		int itemIndex = i + Info().sView;
		if ((itemIndex < DEF_MAXBANKITEMS) && (m_pGame->m_pBankList[itemIndex] != 0)) {
			m_pGame->GetItemName(m_pGame->m_pBankList[itemIndex].get(), cStr1, cStr2, cStr3);

			if ((msX > sX + 30) && (msX < sX + 210) && (msY >= sY + 110 + i * 15) && (msY <= sY + 124 + i * 15)) {
				bFlag = true;
				PutAlignedString(sX, sX + szX, sY + 110 + i * 15, cStr1, GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);
				DrawItemDetails(sX, sY, szX, itemIndex, iLoc);
			}
			else {
				if (m_pGame->m_bIsSpecial)
					PutAlignedString(sX, sX + szX, sY + 110 + i * 15, cStr1, GameColors::UIItemName_Special.r, GameColors::UIItemName_Special.g, GameColors::UIItemName_Special.b);
				else
					PutAlignedString(sX, sX + szX, sY + 110 + i * 15, cStr1, GameColors::UIBlack.r, GameColors::UIBlack.g, GameColors::UIBlack.b);
			}
		}
	}

	// Count total items for scrollbar
	int iTotalLines = 0;
	for (int i = 0; i < DEF_MAXBANKITEMS; i++)
		if (m_pGame->m_pBankList[i] != 0) iTotalLines++;

	DrawScrollbar(sX, sY, iTotalLines, msX, msY, msZ, cLB);

	if (!bFlag) {
		PutAlignedString(sX, sX + szX, sY + 45, DRAW_DIALOGBOX_BANK3);
		PutAlignedString(sX, sX + szX, sY + 60, DRAW_DIALOGBOX_BANK4);
		PutAlignedString(sX, sX + szX, sY + 75, DRAW_DIALOGBOX_BANK5);
	}
}

void DialogBox_Bank::DrawItemDetails(short sX, short sY, short szX, int iItemIndex, int iLoc)
{
	char cStr1[64], cStr2[64], cStr3[64];
	std::memset(cStr1, 0, sizeof(cStr1));
	std::memset(cStr2, 0, sizeof(cStr2));
	std::memset(cStr3, 0, sizeof(cStr3));

	CItem* pItem = m_pGame->m_pBankList[iItemIndex].get();
	CItem* pCfg = m_pGame->GetItemConfig(pItem->m_sIDnum);
	if (pCfg == nullptr) return;

	m_pGame->GetItemName(pItem, cStr1, cStr2, cStr3);

	if (m_pGame->m_bIsSpecial)
		PutAlignedString(sX + 70, sX + szX, sY + iLoc, cStr1, GameColors::UIItemName_Special.r, GameColors::UIItemName_Special.g, GameColors::UIItemName_Special.b);
	else
		PutAlignedString(sX + 70, sX + szX, sY + iLoc, cStr1, GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b);

	if (strlen(cStr2) > 0) {
		iLoc += 15;
		PutAlignedString(sX + 70, sX + szX, sY + iLoc, cStr2, GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b);
	}
	if (strlen(cStr3) > 0) {
		iLoc += 15;
		PutAlignedString(sX + 70, sX + szX, sY + iLoc, cStr3, GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b);
	}

	// Level limit
	if (pCfg->m_sLevelLimit != 0 &&
		pItem->m_dwAttribute & 0x00000001) {
		iLoc += 15;
		char buf[128];
		snprintf(buf, sizeof(buf), "%s: %d", DRAW_DIALOGBOX_SHOP24, pCfg->m_sLevelLimit);
		PutAlignedString(sX + 70, sX + szX, sY + iLoc, buf, GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b);
	}

	// Weight for equipment
	if ((pCfg->GetEquipPos() != EquipPos::None) &&
		(pCfg->m_wWeight >= 1100)) {
		iLoc += 15;
		int _wWeight = 0;
		if (pCfg->m_wWeight % 100) _wWeight = 1;
		char buf[128];
		snprintf(buf, sizeof(buf), DRAW_DIALOGBOX_SHOP15, pCfg->m_wWeight / 100 + _wWeight);
		PutAlignedString(sX + 70, sX + szX, sY + iLoc, buf, GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b);
	}

	// Draw item sprite
	char cItemColor = pItem->m_cItemColor;
	if (cItemColor == 0) {
		m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + pCfg->m_sSprite]->Draw(sX + 60, sY + 68, pCfg->m_sSpriteFrame);
	}
	else {
		if ((pCfg->GetEquipPos() == EquipPos::LeftHand) ||
			(pCfg->GetEquipPos() == EquipPos::RightHand) ||
			(pCfg->GetEquipPos() == EquipPos::TwoHand)) {
			m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + pCfg->m_sSprite]->Draw(sX + 60, sY + 68, pCfg->m_sSpriteFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[cItemColor].r - GameColors::Base.r, GameColors::Weapons[cItemColor].g - GameColors::Base.g, GameColors::Weapons[cItemColor].b - GameColors::Base.b));
		}
		else {
			m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + pCfg->m_sSprite]->Draw(sX + 60, sY + 68, pCfg->m_sSpriteFrame, SpriteLib::DrawParams::Tint(GameColors::Items[cItemColor].r - GameColors::Base.r, GameColors::Items[cItemColor].g - GameColors::Base.g, GameColors::Items[cItemColor].b - GameColors::Base.b));
		}
	}
}

void DialogBox_Bank::DrawScrollbar(short sX, short sY, int iTotalLines, short msX, short msY, short msZ, char cLB)
{
	int iPointerLoc;
	double d1, d2, d3;

	if (iTotalLines > Info().sV1) {
		d1 = (double)Info().sView;
		d2 = (double)(iTotalLines - Info().sV1);
		d3 = (274.0f * d1) / d2;
		iPointerLoc = (int)d3;
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 3);
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX + 242, sY + iPointerLoc + 35, 7);
	}
	else {
		iPointerLoc = 0;
	}

	if (cLB != 0 && (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::Bank) && iTotalLines > Info().sV1) {
		if ((msX >= sX + 230) && (msX <= sX + 260) && (msY >= sY + 40) && (msY <= sY + 320)) {
			d1 = (double)(msY - (sY + 35));
			d2 = (double)(iTotalLines - Info().sV1);
			d3 = (d1 * d2) / 274.0f;
			Info().sView = (int)(d3 + 0.5);
		}
		else if ((msX >= sX + 230) && (msX <= sX + 260) && (msY > sY + 10) && (msY < sY + 40)) {
			Info().sView = 0;
		}
	}
	else {
		Info().bIsScrollSelected = false;
	}

	if (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::Bank && msZ != 0) {
		if (iTotalLines > 50)
			Info().sView = Info().sView - msZ / 30;
		else {
			if (msZ > 0) Info().sView--;
			if (msZ < 0) Info().sView++;
		}
	}

	if (iTotalLines > Info().sV1 && Info().sView > iTotalLines - Info().sV1)
		Info().sView = iTotalLines - Info().sV1;
	if (iTotalLines <= Info().sV1)
		Info().sView = 0;
	if (Info().sView < 0)
		Info().sView = 0;
}

bool DialogBox_Bank::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	switch (Info().cMode) {
	case -1:
		break;

	case 0:
		for (int i = 0; i < Info().sV1; i++) {
			if ((msX > sX + 30) && (msX < sX + 210) && (msY >= sY + 110 + i * 15) && (msY <= sY + 124 + i * 15)) {
				int itemIndex = Info().sView + i;
				if ((itemIndex < DEF_MAXBANKITEMS) && (m_pGame->m_pBankList[itemIndex] != 0)) {
					if (m_pGame->_iGetTotalItemNum() >= 50) {
						AddEventList(DLGBOX_CLICK_BANK1, 10);
						return true;
					}
					bSendCommand(MSGID_REQUEST_RETRIEVEITEM, 0, 0, itemIndex, 0, 0, 0);
					Info().cMode = -1;
					PlaySoundEffect('E', 14, 5);
				}
				return true;
			}
		}
		break;
	}

	return false;
}

bool DialogBox_Bank::OnItemDrop(short msX, short msY)
{
	auto& giveInfo = m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem);
	giveInfo.sV1 = CursorTarget::GetSelectedID();

	if (m_pGame->m_pPlayer->m_Controller.GetCommand() < 0) return false;
	if (m_pGame->m_pItemList[giveInfo.sV1] == nullptr) return false;
	if (m_pGame->m_bIsItemDisabled[giveInfo.sV1]) return false;

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
	if (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropConfirm))
	{
		AddEventList(BITEMDROP_SKILLDIALOG1, 10);
		return false;
	}

	// Stackable items - open quantity dialog
	CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[giveInfo.sV1]->m_sIDnum);
	if (pCfg == nullptr) return false;

	if (((pCfg->GetItemType() == ItemType::Consume) ||
		(pCfg->GetItemType() == ItemType::Arrow)) &&
		(m_pGame->m_pItemList[giveInfo.sV1]->m_dwCount > 1))
	{
		auto& dropInfo = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal);
		dropInfo.sX = msX - 140;
		dropInfo.sY = msY - 70;
		if (dropInfo.sY < 0) dropInfo.sY = 0;
		dropInfo.sV1 = m_pGame->m_pPlayer->m_sPlayerX + 1;
		dropInfo.sV2 = m_pGame->m_pPlayer->m_sPlayerY + 1;
		dropInfo.sV3 = 1002; // NPC
		dropInfo.sV4 = giveInfo.sV1;
		std::memset(dropInfo.cStr, 0, sizeof(dropInfo.cStr));
		m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::ItemDropExternal, giveInfo.sV1,
			m_pGame->m_pItemList[giveInfo.sV1]->m_dwCount, 0);
	}
	else
	{
		// Single item - deposit directly
		if (m_pGame->_iGetBankItemCount() >= (m_pGame->iMaxBankItems - 1))
			AddEventList(DLGBOX_CLICK_NPCACTION_QUERY9, 10);
		else
			bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_GIVEITEMTOCHAR, giveInfo.sV1, 1,
				giveInfo.sV5, giveInfo.sV6, pCfg->m_cName, giveInfo.sV4);
	}

	return true;
}

PressResult DialogBox_Bank::OnPress(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	// Scroll bar region
	if ((msX >= sX + 240) && (msX <= sX + 260) && (msY >= sY + 40) && (msY <= sY + 320))
	{
		return PressResult::ScrollClaimed;
	}

	return PressResult::Normal;
}
