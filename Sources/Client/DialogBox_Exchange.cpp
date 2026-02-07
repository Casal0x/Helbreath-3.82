#include "DialogBox_Exchange.h"
#include "CursorTarget.h"
#include "Game.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include "lan_eng.h"

using namespace hb::item;

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

	m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_NEWEXCHANGE, sX, sY, 0);

	switch (Info().cMode) {
	case 1: // Not yet confirmed exchange
		PutAlignedString(sX + 80, sX + 180, sY + 38, m_pGame->m_pPlayer->m_cPlayerName, GameColors::UIDarkGreen);
		if (m_pGame->m_stDialogBoxExchangeInfo[4].sV1 != -1)
			PutAlignedString(sX + 250, sX + 540, sY + 38, m_pGame->m_stDialogBoxExchangeInfo[4].cStr2, GameColors::UIDarkGreen);

		DrawItems(sX, sY, msX, msY, 0, 8);

		if ((m_pGame->m_stDialogBoxExchangeInfo[0].sV1 != -1) && (m_pGame->m_stDialogBoxExchangeInfo[4].sV1 == -1)) {
			PutAlignedString(sX, sX + szX, sY + 245, DRAW_DIALOGBOX_EXCHANGE9, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 260, DRAW_DIALOGBOX_EXCHANGE10, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 275, DRAW_DIALOGBOX_EXCHANGE11, GameColors::UILabel);
			TextLib::DrawText(GameFont::Bitmap1, sX + 220, sY + 310, "Exchange", TextLib::TextStyle::WithHighlight(GameColors::BmpBtnHover));
		}
		else if ((m_pGame->m_stDialogBoxExchangeInfo[0].sV1 == -1) && (m_pGame->m_stDialogBoxExchangeInfo[4].sV1 != -1)) {
			PutAlignedString(sX, sX + szX, sY + 215, DRAW_DIALOGBOX_EXCHANGE12, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 230, DRAW_DIALOGBOX_EXCHANGE13, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 245, DRAW_DIALOGBOX_EXCHANGE14, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 260, DRAW_DIALOGBOX_EXCHANGE15, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 275, DRAW_DIALOGBOX_EXCHANGE16, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 290, DRAW_DIALOGBOX_EXCHANGE17, GameColors::UILabel);
			TextLib::DrawText(GameFont::Bitmap1, sX + 220, sY + 310, "Exchange", TextLib::TextStyle::WithHighlight(GameColors::BmpBtnHover));
		}
		else if ((m_pGame->m_stDialogBoxExchangeInfo[0].sV1 != -1) && (m_pGame->m_stDialogBoxExchangeInfo[4].sV1 != -1)) {
			PutAlignedString(sX, sX + szX, sY + 215, DRAW_DIALOGBOX_EXCHANGE18, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 230, DRAW_DIALOGBOX_EXCHANGE19, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 245, DRAW_DIALOGBOX_EXCHANGE20, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 260, DRAW_DIALOGBOX_EXCHANGE21, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 275, DRAW_DIALOGBOX_EXCHANGE22, GameColors::UILabel);
			PutAlignedString(sX, sX + szX, sY + 290, DRAW_DIALOGBOX_EXCHANGE23, GameColors::UILabel);
			if ((msX >= sX + 200) && (msX <= sX + 200 + DEF_BTNSZX) && (msY >= sY + 310) && (msY <= sY + 310 + DEF_BTNSZY))
				TextLib::DrawText(GameFont::Bitmap1, sX + 220, sY + 310, "Exchange", TextLib::TextStyle::WithHighlight(GameColors::UIMagicBlue));
			else
				TextLib::DrawText(GameFont::Bitmap1, sX + 220, sY + 310, "Exchange", TextLib::TextStyle::WithHighlight(GameColors::BmpBtnNormal));
		}
		if ((msX >= sX + 450) && (msX <= sX + 450 + DEF_BTNSZX) && (msY >= sY + 310) && (msY <= sY + 310 + DEF_BTNSZY)
			&& (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ConfirmExchange) == false))
			TextLib::DrawText(GameFont::Bitmap1, sX + 450, sY + 310, "Cancel", TextLib::TextStyle::WithHighlight(GameColors::UIMagicBlue));
		else
			TextLib::DrawText(GameFont::Bitmap1, sX + 450, sY + 310, "Cancel", TextLib::TextStyle::WithHighlight(GameColors::BmpBtnNormal));
		break;

	case 2: // You have confirmed the exchange
		PutAlignedString(sX + 80, sX + 180, sY + 38, m_pGame->m_pPlayer->m_cPlayerName, GameColors::UIDarkGreen);
		if (m_pGame->m_stDialogBoxExchangeInfo[4].sV1 != -1)
			PutAlignedString(sX + 250, sX + 540, sY + 38, m_pGame->m_stDialogBoxExchangeInfo[4].cStr2, GameColors::UIDarkGreen);

		DrawItems(sX, sY, msX, msY, 0, 8);

		char exchangeBuf[128];
		snprintf(exchangeBuf, sizeof(exchangeBuf), DRAW_DIALOGBOX_EXCHANGE33, m_pGame->m_stDialogBoxExchangeInfo[4].cStr2);
		PutAlignedString(sX, sX + szX, sY + 215, exchangeBuf, GameColors::UILabel);
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
				m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + m_pGame->m_stDialogBoxExchangeInfo[i].sV1]->Draw(sX + sXadd, sY + 130, m_pGame->m_stDialogBoxExchangeInfo[i].sV2);
			}
			else {
				switch (m_pGame->m_stDialogBoxExchangeInfo[i].sV1) {
				case 1:  // Swds
				case 2:  // Bows
				case 3:  // Shields
				case 15: // Axes hammers
				case 17: // Wands
					m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + m_pGame->m_stDialogBoxExchangeInfo[i].sV1]->Draw(sX + sXadd, sY + 130, m_pGame->m_stDialogBoxExchangeInfo[i].sV2, SpriteLib::DrawParams::Tint(GameColors::Weapons[cItemColor].r - GameColors::Base.r, GameColors::Weapons[cItemColor].g - GameColors::Base.g, GameColors::Weapons[cItemColor].b - GameColors::Base.b));
					break;
				default:
					m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + m_pGame->m_stDialogBoxExchangeInfo[i].sV1]->Draw(sX + sXadd, sY + 130, m_pGame->m_stDialogBoxExchangeInfo[i].sV2, SpriteLib::DrawParams::Tint(GameColors::Items[cItemColor].r - GameColors::Base.r, GameColors::Items[cItemColor].g - GameColors::Base.g, GameColors::Items[cItemColor].b - GameColors::Base.b));
					break;
				}
			}

			DrawItemInfo(sX, sY, Info().sSizeX, msX, msY, i, sXadd);
		}
	}
}

void DialogBox_Exchange::DrawItemInfo(short sX, short sY, short szX, short msX, short msY, int iItemIndex, short sXadd)
{
	char cNameStr[120], cSubStr1[120], cSubStr2[120];
	char cTxt[120], cTxt2[128];
	int iLoc;

	m_pGame->GetItemName(m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sItemID,
		m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].dwV1, cNameStr, cSubStr1, cSubStr2);

	if ((msX >= sX + sXadd - 6) && (msX <= sX + sXadd + 42) && (msY >= sY + 61) && (msY <= sY + 200)) {
		std::snprintf(cTxt, sizeof(cTxt), "%s", cNameStr);
		if (m_pGame->m_bIsSpecial) {
			PutAlignedString(sX + 15, sX + 155, sY + 215, cTxt, GameColors::UIItemName_Special);
			PutAlignedString(sX + 16, sX + 156, sY + 215, cTxt, GameColors::UIItemName_Special);
		}
		else {
			PutAlignedString(sX + 15, sX + 155, sY + 215, cTxt, GameColors::UILabel);
			PutAlignedString(sX + 16, sX + 156, sY + 215, cTxt, GameColors::UILabel);
		}

		iLoc = 0;
		if (strlen(cSubStr1) != 0) {
			PutAlignedString(sX + 16, sX + 155, sY + 235 + iLoc, cSubStr1, GameColors::UIBlack);
			iLoc += 15;
		}
		if (strlen(cSubStr2) != 0) {
			PutAlignedString(sX + 16, sX + 155, sY + 235 + iLoc, cSubStr2, GameColors::UIBlack);
			iLoc += 15;
		}

		if (m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV3 != 1) {
			if (m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV3 > 1) {
				m_pGame->FormatCommaNumber(m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV3, cTxt2, sizeof(cTxt2));
			}
			else {
				snprintf(cTxt2, sizeof(cTxt2), DRAW_DIALOGBOX_EXCHANGE2, m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV3);
			}
			PutAlignedString(sX + 16, sX + 155, sY + 235 + iLoc, cTxt2, GameColors::UILabel);
			iLoc += 15;
		}

		if (m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV5 != -1) {
			// Crafting Magins completion fix
			if (m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV1 == 22) {
				if ((m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV2 > 5) &&
					(m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV2 < 10)) {
					std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME2, (m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV7 - 100));
				}
			}
			else if (m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV1 == 6) {
				std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME1, (m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV7 - 100));
			}
			else {
				std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME2, m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV7);
			}
			PutAlignedString(sX + 16, sX + 155, sY + 235 + iLoc, cTxt, GameColors::UILabel);
			iLoc += 15;
		}

		if (iLoc < 45) {
			std::snprintf(cTxt, sizeof(cTxt), DRAW_DIALOGBOX_EXCHANGE3, m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV5,
				m_pGame->m_stDialogBoxExchangeInfo[iItemIndex].sV6);
			PutAlignedString(sX + 16, sX + 155, sY + 235 + iLoc, cTxt, GameColors::UILabel);
		}
	}
}

bool DialogBox_Exchange::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	switch (Info().cMode) {
	case 1: // Not yet confirmed the exchange
		if ((msX >= sX + 220) && (msX <= sX + 220 + DEF_BTNSZX) && (msY >= sY + 310) && (msY <= sY + 310 + DEF_BTNSZY)) {
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
		if ((msX >= sX + 450) && (msX <= sX + 450 + DEF_BTNSZX) && (msY >= sY + 310) && (msY <= sY + 310 + DEF_BTNSZY)
			&& (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ConfirmExchange) == false)) {
			// Cancel button
			m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::Exchange);
			m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::Map);
			bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_CANCELEXCHANGEITEM, 0, 0, 0, 0, 0);
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
		bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_SETEXCHANGEITEM, 0, cItemID, 1, 0, 0);
	}

	return true;
}
