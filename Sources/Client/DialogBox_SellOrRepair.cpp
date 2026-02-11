#include "DialogBox_SellOrRepair.h"
#include "Game.h"
#include "GlobalDef.h"
#include "SpriteID.h"
#include "lan_eng.h"
#include "NetMessages.h"

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
DialogBox_SellOrRepair::DialogBox_SellOrRepair(CGame* pGame)
	: IDialogBox(DialogBoxId::SellOrRepair, pGame)
{
	SetDefaultRect(337 , 57 , 258, 339);
}

void DialogBox_SellOrRepair::OnDraw(short msX, short msY, short msZ, char cLB)
{
	if (!m_pGame->EnsureItemConfigsLoaded()) return;
	short sX, sY;
	uint32_t dwTime = m_pGame->m_dwCurTime;
	char cItemID, cItemColor, cTxt[120], cTemp[120], cStr2[120], cStr3[120];

	sX = Info().sX;
	sY = Info().sY;

	switch (Info().cMode) {
	case 1:
		DrawNewDialogBox(InterfaceNdGame2, sX, sY, 2);
		DrawNewDialogBox(InterfaceNdText, sX, sY, 11);

		cItemID = Info().sV1;

		cItemColor = m_pGame->m_pItemList[cItemID]->m_cItemColor;
		if (cItemColor == 0)
			m_pGame->m_pSprite[ItemPackPivotPoint + m_pGame->m_pItemList[cItemID]->m_sSprite]->Draw(sX + 62 + 15, sY + 84 + 30, m_pGame->m_pItemList[cItemID]->m_sSpriteFrame);
		else
		{
			switch (m_pGame->m_pItemList[cItemID]->m_sSprite) {
			case 1: // Swds
			case 2: // Bows
			case 3: // Shields
			case 15: // Axes hammers
				m_pGame->m_pSprite[ItemPackPivotPoint + m_pGame->m_pItemList[cItemID]->m_sSprite]->Draw(sX + 62 + 15, sY + 84 + 30, m_pGame->m_pItemList[cItemID]->m_sSpriteFrame, hb::shared::sprite::DrawParams::Tint(GameColors::Weapons[cItemColor].r, GameColors::Weapons[cItemColor].g, GameColors::Weapons[cItemColor].b));
				break;
			default: m_pGame->m_pSprite[ItemPackPivotPoint + m_pGame->m_pItemList[cItemID]->m_sSprite]->Draw(sX + 62 + 15, sY + 84 + 30, m_pGame->m_pItemList[cItemID]->m_sSpriteFrame, hb::shared::sprite::DrawParams::Tint(GameColors::Items[cItemColor].r, GameColors::Items[cItemColor].g, GameColors::Items[cItemColor].b));
				break;
			}
		}
		std::memset(cTemp, 0, sizeof(cTemp));
		std::memset(cStr2, 0, sizeof(cStr2));
		std::memset(cStr3, 0, sizeof(cStr3));

		m_pGame->GetItemName(m_pGame->m_pItemList[cItemID].get(), cTemp, cStr2, cStr3);
		if (Info().sV4 == 1) std::snprintf(cTxt, sizeof(cTxt), "%s", cTemp);
		else std::snprintf(cTxt, sizeof(cTxt), DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM1, Info().sV4, cTemp);

		if (m_pGame->m_bIsSpecial)
		{
			PutAlignedString(sX + 25, sX + 240, sY + 60, cTxt, GameColors::UIItemName_Special);
			PutAlignedString(sX + 25 + 1, sX + 240 + 1, sY + 60, cTxt, GameColors::UIItemName_Special);
		}
		else
		{
			PutAlignedString(sX + 25, sX + 240, sY + 60, cTxt, GameColors::UILabel);
			PutAlignedString(sX + 25 + 1, sX + 240 + 1, sY + 60, cTxt, GameColors::UILabel);
		}

		std::snprintf(cTxt, sizeof(cTxt), DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM2, Info().sV2);
		PutString(sX + 95 + 15, sY + 53 + 60, cTxt, GameColors::UILabel);
		std::snprintf(cTxt, sizeof(cTxt), DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM3, Info().sV3);
		PutString(sX + 95 + 15, sY + 53 + 75, cTxt, GameColors::UILabel);
		PutString(sX + 55, sY + 190, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM4, GameColors::UILabel);

		if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 39);
		else DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 38);

		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 17);
		else DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 16);
		break;

	case 2:
		DrawNewDialogBox(InterfaceNdGame2, sX, sY, 2);
		DrawNewDialogBox(InterfaceNdText, sX, sY, 10);
		cItemID = Info().sV1;
		cItemColor = m_pGame->m_pItemList[cItemID]->m_cItemColor;
		if (cItemColor == 0)
			m_pGame->m_pSprite[ItemPackPivotPoint + m_pGame->m_pItemList[cItemID]->m_sSprite]->Draw(sX + 62 + 15, sY + 84 + 30, m_pGame->m_pItemList[cItemID]->m_sSpriteFrame);
		else
		{
			switch (m_pGame->m_pItemList[cItemID]->m_sSprite) {
			case 1: // Swds
			case 2: // Bows
			case 3: // Shields
			case 15: // Axes hammers
				m_pGame->m_pSprite[ItemPackPivotPoint + m_pGame->m_pItemList[cItemID]->m_sSprite]->Draw(sX + 62 + 15, sY + 84 + 30, m_pGame->m_pItemList[cItemID]->m_sSpriteFrame, hb::shared::sprite::DrawParams::Tint(GameColors::Weapons[cItemColor].r, GameColors::Weapons[cItemColor].g, GameColors::Weapons[cItemColor].b));
				break;

			default: m_pGame->m_pSprite[ItemPackPivotPoint + m_pGame->m_pItemList[cItemID]->m_sSprite]->Draw(sX + 62 + 15, sY + 84 + 30, m_pGame->m_pItemList[cItemID]->m_sSpriteFrame, hb::shared::sprite::DrawParams::Tint(GameColors::Items[cItemColor].r, GameColors::Items[cItemColor].g, GameColors::Items[cItemColor].b));
				break;
			}
		}
		std::memset(cTemp, 0, sizeof(cTemp));
		std::memset(cStr2, 0, sizeof(cStr2));
		std::memset(cStr3, 0, sizeof(cStr3));
		m_pGame->GetItemName(m_pGame->m_pItemList[cItemID].get(), cTemp, cStr2, cStr3);
		std::snprintf(cTxt, sizeof(cTxt), "%s", cTemp);
		if (m_pGame->m_bIsSpecial)
		{
			PutAlignedString(sX + 25, sX + 240, sY + 60, cTxt, GameColors::UIItemName_Special);
			PutAlignedString(sX + 25 + 1, sX + 240 + 1, sY + 60, cTxt, GameColors::UIItemName_Special);
		}
		else
		{
			PutAlignedString(sX + 25, sX + 240, sY + 60, cTxt, GameColors::UILabel);
			PutAlignedString(sX + 25 + 1, sX + 240 + 1, sY + 60, cTxt, GameColors::UILabel);
		}
		std::snprintf(cTxt, sizeof(cTxt), DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM2, Info().sV2);
		PutString(sX + 95 + 15, sY + 53 + 60, cTxt, GameColors::UILabel);
		std::snprintf(cTxt, sizeof(cTxt), DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM6, Info().sV3);
		PutString(sX + 95 + 15, sY + 53 + 75, cTxt, GameColors::UILabel);
		PutString(sX + 55, sY + 190, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM7, GameColors::UILabel);

		if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 43);
		else DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 42);

		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 17);
		else DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 16);
		break;

	case 3:
		DrawNewDialogBox(InterfaceNdGame2, sX, sY, 2);
		DrawNewDialogBox(InterfaceNdText, sX, sY, 11);

		PutString(sX + 55, sY + 100, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM8, GameColors::UILabel);
		PutString(sX + 55, sY + 120, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM9, GameColors::UILabel);
		PutString(sX + 55, sY + 135, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM10, GameColors::UILabel);
		break;

	case 4:
		DrawNewDialogBox(InterfaceNdGame2, sX, sY, 2);
		DrawNewDialogBox(InterfaceNdText, sX, sY, 10);

		PutString(sX + 55, sY + 100, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM11, GameColors::UILabel);
		PutString(sX + 55, sY + 120, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM9, GameColors::UILabel);
		PutString(sX + 55, sY + 135, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM10, GameColors::UILabel);
		break;
	}
}

bool DialogBox_SellOrRepair::OnClick(short msX, short msY)
{
	short sX, sY;

	sX = Info().sX;
	sY = Info().sY;

	switch (Info().cMode) {
	case 1:
	{
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV1]->m_sIDnum);
		if ((msX >= sX + 30) && (msX <= sX + 30 + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			// Sell
			if (pCfg) m_pGame->bSendCommand(MsgId::CommandCommon, CommonType::ReqSellItemConfirm, 0, Info().sV1, Info().sV4, Info().sV3, pCfg->m_cName);
			Info().cMode = 3;
			return true;
		}
		if ((msX >= sX + 154) && (msX <= sX + 154 + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			// Cancel
			m_pGame->m_bIsItemDisabled[Info().sV1] = false;
			m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::SellOrRepair);
			return true;
		}
		break;
	}

	case 2:
	{
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sV1]->m_sIDnum);
		if ((msX >= sX + 30) && (msX <= sX + 30 + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			// Repair
			if (pCfg) m_pGame->bSendCommand(MsgId::CommandCommon, CommonType::ReqRepairItemConfirm, 0, Info().sV1, 0, 0, pCfg->m_cName);
			Info().cMode = 4;
			return true;
		}
		if ((msX >= sX + 154) && (msX <= sX + 154 + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			// Cancel
			m_pGame->m_bIsItemDisabled[Info().sV1] = false;
			m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::SellOrRepair);
			return true;
		}
		break;
	}
	}

	return false;
}
