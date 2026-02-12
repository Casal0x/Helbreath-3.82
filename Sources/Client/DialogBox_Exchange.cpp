#include "DialogBox_Exchange.h"
#include "CursorTarget.h"
#include "Game.h"
#include "ItemNameFormatter.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include "lan_eng.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::shared::item;
using namespace hb::client::sprite_id;

DialogBox_Exchange::DialogBox_Exchange(CGame* pGame)
	: IDialogBox(DialogBoxId::Exchange, pGame)
{
	SetDefaultRect(100 , 30 , 520, 357);
}

void DialogBox_Exchange::OnDraw(short msX, short msY, short msZ, char cLB)
{
	if (!m_pGame->EnsureItemConfigsLoaded()) return;
	short sX = Info().sX;
	short sY = Info().sY;
	short szX = Info().sSizeX;

	m_pGame->DrawNewDialogBox(InterfaceNdNewExchange, sX, sY, 0);

	switch (Info().cMode) {
	case 1: // Not yet confirmed exchange
		PutAlignedString(sX + 80, sX + 180, sY + 38, m_pGame->m_pPlayer->m_cPlayerName, GameColors::UIDarkGreen);
		if (m_pGame->m_stDialogBoxExchangeInfo[4].sV1 != -1)
			PutAlignedString(sX + 250, sX + 540, sY + 38, m_pGame->m_stDialogBoxExchangeInfo[4].cStr2.c_str(), GameColors::UIDarkGreen);

		DrawItems(sX, sY, msX, msY, 0, 8);

		if ((m_pGame->m_stDialogBoxExchangeInfo[0].sV1 != -1) && (m_pGame->m_stDialogBoxExchangeInfo[4].sV1 == -1)) {
			PutAlignedString(sX, sX + szX, sY + 245, DRAW_DIALOGBOX_EXCHANGE9, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 260, DRAW_DIALOGBOX_EXCHANGE10, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 275, DRAW_DIALOGBOX_EXCHANGE11, GameColors::UILabel);
			hb::shared::text::DrawText(GameFont::Bitmap1, sX + 220, sY + 310, "Exchange", hb::shared::text::TextStyle::WithHighlight(GameColors::BmpBtnHover));
		}
		else if ((m_pGame->m_stDialogBoxExchangeInfo[0].sV1 == -1) && (m_pGame->m_stDialogBoxExchangeInfo[4].sV1 != -1)) {
			PutAlignedString(sX, sX + szX, sY + 215, DRAW_DIALOGBOX_EXCHANGE12, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 230, DRAW_DIALOGBOX_EXCHANGE13, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 245, DRAW_DIALOGBOX_EXCHANGE14, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 260, DRAW_DIALOGBOX_EXCHANGE15, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 275, DRAW_DIALOGBOX_EXCHANGE16, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 290, DRAW_DIALOGBOX_EXCHANGE17, GameColors::UILabel);
			hb::shared::text::DrawText(GameFont::Bitmap1, sX + 220, sY + 310, "Exchange", hb::shared::text::TextStyle::WithHighlight(GameColors::BmpBtnHover));
		}
		else if ((m_pGame->m_stDialogBoxExchangeInfo[0].sV1 != -1) && (m_pGame->m_stDialogBoxExchangeInfo[4].sV1 != -1)) {
			PutAlignedString(sX, sX + szX, sY + 215, DRAW_DIALOGBOX_EXCHANGE18, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 230, DRAW_DIALOGBOX_EXCHANGE19, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 245, DRAW_DIALOGBOX_EXCHANGE20, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 260, DRAW_DIALOGBOX_EXCHANGE21, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 275, DRAW_DIALOGBOX_EXCHANGE22, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 290, DRAW_DIALOGBOX_EXCHANGE23, GameColors::UILabel);
			if ((msX >= sX + 200) && (msX <= sX + 200 + ui_layout::btn_size_x) && (msY >= sY + 310) && (msY <= sY + 310 + ui_layout::btn_size_y))
				hb::shared::text::DrawText(GameFont::Bitmap1, sX + 220, sY + 310, "Exchange", hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicBlue));
			else
				hb::shared::text::DrawText(GameFont::Bitmap1, sX + 220, sY + 310, "Exchange", hb::shared::text::TextStyle::WithHighlight(GameColors::BmpBtnNormal));
		}
		if ((msX >= sX + 450) && (msX <= sX + 450 + ui_layout::btn_size_x) && (msY >= sY + 310) && (msY <= sY + 310 + ui_layout::btn_size_y)
			&& (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ConfirmExchange) == false))
			hb::shared::text::DrawText(GameFont::Bitmap1, sX + 450, sY + 310, "Cancel", hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicBlue));
		else
			hb::shared::text::DrawText(GameFont::Bitmap1, sX + 450, sY + 310, "Cancel", hb::shared::text::TextStyle::WithHighlight(GameColors::BmpBtnNormal));
		break;

	case 2: // You have confirmed the exchange
		PutAlignedString(sX + 80, sX + 180, sY + 38, m_pGame->m_pPlayer->m_cPlayerName, GameColors::UIDarkGreen);
		if (m_pGame->m_stDialogBoxExchangeInfo[4].sV1 != -1)
			PutAlignedString(sX + 250, sX + 540, sY + 38, m_pGame->m_stDialogBoxExchangeInfo[4].cStr2.c_str(), GameColors::UIDarkGreen);

		DrawItems(sX, sY, msX, msY, 0, 8);

		std::string exchangeBuf;
		exchangeBuf = std::format(DRAW_DIALOGBOX_EXCHANGE33, m_pGame->m_stDialogBoxExchangeInfo[4].cStr2);
		PutAlignedString(sX, sX + szX, sY + 215, exchangeBuf.c_str(), GameColors::UILabel);
		PutAlignedString(sX, sX + szX, sY + 230, DRAW_DIALOGBOX_EXCHANGE34, GameColors::UILabel);
		PutAlignedString(sX, sX + szX, sY + 245, DRAW_DIALOGBOX_EXCHANGE35, GameColors::UILabel);
		PutAlignedString(sX, sX + szX, sY + 260, DRAW_DIALOGBOX_EXCHANGE36, GameColors::UILabel);
		PutAlignedString(sX, sX + szX, sY + 275, DRAW_DIALOGBOX_EXCHANGE37, GameColors::UILabel);
		PutAlignedString(sX, sX + szX, sY + 290, DRAW_DIALOGBOX_EXCHANGE38, GameColors::UILabel);
		PutAlignedString(sX, sX + szX, sY + 305, DRAW_DIALOGBOX_EXCHANGE39, GameColors::UILabel);
		break;
	}
}

void DialogBox_Exchange::DrawItems(short sX, short sY, short msX, short msY, int iStartIndex, int iEndIndex)
{
	uint32_t dwTime = m_pGame->m_dwCurTime;
	char cItemColor;
	short sXadd;

	for (int i = iStartIndex; i < iEndIndex; i++) {
		sXadd = (58 * i) + 48;
		if (i > 3) sXadd += 20;

		if (m_pGame->m_stDialogBoxExchangeInfo[i].sV1 != -1) {
			cItemColor = m_pGame->m_stDialogBoxExchangeInfo[i].sV4;
			if (cItemColor == 0) {
				m_pGame->m_pSprite[ItemPackPivotPoint + m_pGame->m_stDialogBoxExchangeInfo[i].sV1]->Draw(sX + sXadd, sY + 130, m_pGame->m_stDialogBoxExchangeInfo[i].sV2);
			}
			else {
				switch (m_pGame->m_stDialogBoxExchangeInfo[i].sV1) {
				case 1:  // Swds
				case 2:  // Bows
				case 3:  // Shields
				case 15: // Axes hammers
				case 17: // Wands
					m_pGame->m_pSprite[ItemPackPivotPoint + m_pGame->m_stDialogBoxExchangeInfo[i].sV1]->Draw(sX + sXadd, sY + 130, m_pGame->m_stDialogBoxExchangeInfo[i].sV2, hb::shared::sprite::DrawParams::Tint(GameColors::Weapons[cItemColor].r, GameColors::Weapons[cItemColor].g, GameColors::Weapons[cItemColor].b));
					break;
				default:
					m_pGame->m_pSprite[ItemPackPivotPoint + m_pGame->m_stDialogBoxExchangeInfo[i].sV1]->Draw(sX + sXadd, sY + 130, m_pGame->m_stDialogBoxExchangeInfo[i].sV2, hb::shared::sprite::DrawParams::Tint(GameColors::Items[cItemColor].r, GameColors::Items[cItemColor].g, GameColors::Items[cItemColor].b));
					break;
				}
			}

			DrawItemInfo(sX, sY, Info().sSizeX, msX, msY, i, sXadd);
		}
	}
}

void DialogBox_Exchange::DrawItemInfo(short sX, short sY, short szX, short msX, short msY, int iItemIndex, short sXadd)
{
	std::string cTxt, cTxt2;
	int iLoc;

	auto itemInfo = ItemNameFormatter::Get().Format(m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sItemID,  m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].dwV1);

	if ((msX >= sX + sXadd - 6) && (msX <= sX + sXadd + 42) && (msY >= sY + 61) && (msY <= sY + 200)) {
		cTxt = itemInfo.name.c_str();
		if (itemInfo.is_special) {
			PutAlignedString(sX + 15, sX + 155, sY + 215, cTxt.c_str(), GameColors::UIItemName_Special);
			PutAlignedString(sX + 16, sX + 156, sY + 215, cTxt.c_str(), GameColors::UIItemName_Special);
		}
		else {
			PutAlignedString(sX + 15, sX + 155, sY + 215, cTxt.c_str(), GameColors::UILabel);
			PutAlignedString(sX + 16, sX + 156, sY + 215, cTxt.c_str(), GameColors::UILabel);
		}

		iLoc = 0;
		if (itemInfo.effect.size() != 0) {
			PutAlignedString(sX + 16, sX + 155, sY + 235 + iLoc, itemInfo.effect.c_str(), GameColors::UIBlack);
			iLoc += 15;
		}
		if (itemInfo.extra.size() != 0) {
			PutAlignedString(sX + 16, sX + 155, sY + 235 + iLoc, itemInfo.extra.c_str(), GameColors::UIBlack);
			iLoc += 15;
		}

		if (m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV3 != 1) {
			if (m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV3 > 1) {
				cTxt2 = m_pGame->FormatCommaNumber(m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV3);
			}
			else {
				cTxt2 = std::format(DRAW_DIALOGBOX_EXCHANGE2, m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV3);
			}
			PutAlignedString(sX + 16, sX + 155, sY + 235 + iLoc, cTxt2.c_str(), GameColors::UILabel);
			iLoc += 15;
		}

		if (m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV5 != -1) {
			// Crafting Magins completion fix
			if (m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV1 == 22) {
				if ((m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV2 > 5) &&
					(m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV2 < 10)) {
					cTxt = std::format(GET_ITEM_NAME2, (m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV7 - 100));
				}
			}
			else if (m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV1 == 6) {
				cTxt = std::format(GET_ITEM_NAME1, (m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV7 - 100));
			}
			else {
				cTxt = std::format(GET_ITEM_NAME2, m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV7);
			}
			PutAlignedString(sX + 16, sX + 155, sY + 235 + iLoc, cTxt.c_str(), GameColors::UILabel);
			iLoc += 15;
		}

		if (iLoc < 45) {
			cTxt = std::format(DRAW_DIALOGBOX_EXCHANGE3, m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV5, m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV6);
			PutAlignedString(sX + 16, sX + 155, sY + 235 + iLoc, cTxt.c_str(), GameColors::UILabel);
		}
	}
}

bool DialogBox_Exchange::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	switch (Info().cMode) {
	case 1: // Not yet confirmed the exchange
		if ((msX >= sX + 220) && (msX <= sX + 220 + ui_layout::btn_size_x) && (msY >= sY + 310) && (msY <= sY + 310 + ui_layout::btn_size_y)) {
			// Exchange button
			if ((m_pGame->m_stDialogBoxExchangeInfo[0].sV1 != -1) && (m_pGame->m_stDialogBoxExchangeInfo[4].sV1 != -1)) {
				PlaySoundEffect('E', 14, 5);
				Info().cMode = 2;
				// Show confirmation dialog
				m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::ConfirmExchange, 0, 0, 0);
				m_pGame->m_dialogBoxManager.Info(DialogBoxId::ConfirmExchange).cMode = 1;
			}
			return true;
		}
		if ((msX >= sX + 450) && (msX <= sX + 450 + ui_layout::btn_size_x) && (msY >= sY + 310) && (msY <= sY + 310 + ui_layout::btn_size_y)
			&& (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ConfirmExchange) == false)) {
			// Cancel button
			m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::Exchange);
			m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::Map);
			bSendCommand(MsgId::CommandCommon, CommonType::CancelExchangeItem, 0, 0, 0, 0, 0);
			PlaySoundEffect('E', 14, 5);
			return true;
		}
		break;

	case 2: // Someone already confirmed the exchange
		break;
	}

	return false;
}

bool DialogBox_Exchange::OnItemDrop(short msX, short msY)
{
	if (m_pGame->m_pPlayer->m_Controller.GetCommand() < 0) return false;
	if (m_pGame->m_stDialogBoxExchangeInfo[3].sV1 != -1) return false; // Already 4 items

	char cItemID = (char)CursorTarget::GetSelectedID();

	// Find first empty exchange slot
	int iSlot = -1;
	if (m_pGame->m_stDialogBoxExchangeInfo[0].sV1 == -1) iSlot = 0;
	else if (m_pGame->m_stDialogBoxExchangeInfo[1].sV1 == -1) iSlot = 1;
	else if (m_pGame->m_stDialogBoxExchangeInfo[2].sV1 == -1) iSlot = 2;
	else if (m_pGame->m_stDialogBoxExchangeInfo[3].sV1 == -1) iSlot = 3;
	else return false; // Impossible case

	// Stackable items - open quantity dialog
	CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[cItemID]->m_sIDnum);
	if (pCfg && ((pCfg->GetItemType() == ItemType::Consume) ||
		(pCfg->GetItemType() == ItemType::Arrow)) &&
		(m_pGame->m_pItemList[cItemID]->m_dwCount > 1))
	{
		auto& dropInfo = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal);
		dropInfo.sX = msX - 140;
		dropInfo.sY = msY - 70;
		if (dropInfo.sY < 0) dropInfo.sY = 0;
		dropInfo.sV1 = m_pGame->m_pPlayer->m_sPlayerX + 1;
		dropInfo.sV2 = m_pGame->m_pPlayer->m_sPlayerY + 1;
		dropInfo.sV3 = 1000;
		dropInfo.sV4 = cItemID;
		m_pGame->m_stDialogBoxExchangeInfo[iSlot].sItemID = cItemID;
		std::memset(dropInfo.cStr, 0, sizeof(dropInfo.cStr));
		m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::ItemDropExternal, cItemID,
			m_pGame->m_pItemList[cItemID]->m_dwCount, 0);
	}
	else
	{
		// Single item - add directly
		m_pGame->m_stDialogBoxExchangeInfo[iSlot].sItemID = cItemID;
		m_pGame->m_bIsItemDisabled[cItemID] = true;
		bSendCommand(MsgId::CommandCommon, CommonType::SetExchangeItem, 0, cItemID, 1, 0, 0);
	}

	return true;
}
