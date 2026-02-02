#include "DialogBox_SellOrRepair.h"
#include "Game.h"
#include "GlobalDef.h"
#include "SpriteID.h"
#include "lan_eng.h"
#include "NetMessages.h"

DialogBox_SellOrRepair::DialogBox_SellOrRepair(CGame* pGame)
	: IDialogBox(DialogBoxId::SellOrRepair, pGame)
{
	SetDefaultRect(337 + SCREENX(), 57 + SCREENY(), 258, 339);
}

void DialogBox_SellOrRepair::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX, sY;
	uint32_t dwTime = m_pGame->m_dwCurTime;
	char cItemID, cItemColor, cTxt[120], cTemp[120], cStr2[120], cStr3[120];

	sX = Info().sX;
	sY = Info().sY;

	switch (Info().cMode) {
	case 1:
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 2);
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_TEXT, sX, sY, 11);

		cItemID = Info().sV1;

		cItemColor = m_pGame->m_pItemList[cItemID]->m_cItemColor;
		if (cItemColor == 0)
			m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + m_pGame->m_pItemList[cItemID]->m_sSprite]->Draw(sX + 62 + 15, sY + 84 + 30, m_pGame->m_pItemList[cItemID]->m_sSpriteFrame);
		else
		{
			switch (m_pGame->m_pItemList[cItemID]->m_sSprite) {
			case 1: // Swds
			case 2: // Bows
			case 3: // Shields
			case 15: // Axes hammers
				m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + m_pGame->m_pItemList[cItemID]->m_sSprite]->Draw(sX + 62 + 15, sY + 84 + 30, m_pGame->m_pItemList[cItemID]->m_sSpriteFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[cItemColor].r - GameColors::Base.r, GameColors::Weapons[cItemColor].g - GameColors::Base.g, GameColors::Weapons[cItemColor].b - GameColors::Base.b));
				break;
			default: m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + m_pGame->m_pItemList[cItemID]->m_sSprite]->Draw(sX + 62 + 15, sY + 84 + 30, m_pGame->m_pItemList[cItemID]->m_sSpriteFrame, SpriteLib::DrawParams::Tint(GameColors::Items[cItemColor].r - GameColors::Base.r, GameColors::Items[cItemColor].g - GameColors::Base.g, GameColors::Items[cItemColor].b - GameColors::Base.b));
				break;
			}
		}
		std::memset(cTemp, 0, sizeof(cTemp));
		std::memset(cStr2, 0, sizeof(cStr2));
		std::memset(cStr3, 0, sizeof(cStr3));

		m_pGame->GetItemName(m_pGame->m_pItemList[cItemID].get(), cTemp, cStr2, cStr3);
		if (Info().sV4 == 1) strcpy(cTxt, cTemp);
		else wsprintf(cTxt, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM1, Info().sV4, cTemp);

		if (m_pGame->m_bIsSpecial)
		{
			PutAlignedString(sX + 25, sX + 240, sY + 60, cTxt, GameColors::UIItemName_Special.r, GameColors::UIItemName_Special.g, GameColors::UIItemName_Special.b);
			PutAlignedString(sX + 25 + 1, sX + 240 + 1, sY + 60, cTxt, GameColors::UIItemName_Special.r, GameColors::UIItemName_Special.g, GameColors::UIItemName_Special.b);
		}
		else
		{
			PutAlignedString(sX + 25, sX + 240, sY + 60, cTxt, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);
			PutAlignedString(sX + 25 + 1, sX + 240 + 1, sY + 60, cTxt, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);
		}

		wsprintf(cTxt, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM2, Info().sV2);
		PutString(sX + 95 + 15, sY + 53 + 60, cTxt, GameColors::UILabel.ToColorRef());
		wsprintf(cTxt, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM3, Info().sV3);
		PutString(sX + 95 + 15, sY + 53 + 75, cTxt, GameColors::UILabel.ToColorRef());
		PutString(sX + 55, sY + 190, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM4, GameColors::UILabel.ToColorRef());

		if ((msX >= sX + DEF_LBTNPOSX) && (msX <= sX + DEF_LBTNPOSX + DEF_BTNSZX) && (msY >= sY + DEF_BTNPOSY) && (msY <= sY + DEF_BTNPOSY + DEF_BTNSZY))
			DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_LBTNPOSX, sY + DEF_BTNPOSY, 39);
		else DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_LBTNPOSX, sY + DEF_BTNPOSY, 38);

		if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX) && (msY >= sY + DEF_BTNPOSY) && (msY <= sY + DEF_BTNPOSY + DEF_BTNSZY))
			DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 17);
		else DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 16);
		break;

	case 2:
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 2);
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_TEXT, sX, sY, 10);
		cItemID = Info().sV1;
		cItemColor = m_pGame->m_pItemList[cItemID]->m_cItemColor;
		if (cItemColor == 0)
			m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + m_pGame->m_pItemList[cItemID]->m_sSprite]->Draw(sX + 62 + 15, sY + 84 + 30, m_pGame->m_pItemList[cItemID]->m_sSpriteFrame);
		else
		{
			switch (m_pGame->m_pItemList[cItemID]->m_sSprite) {
			case 1: // Swds
			case 2: // Bows
			case 3: // Shields
			case 15: // Axes hammers
				m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + m_pGame->m_pItemList[cItemID]->m_sSprite]->Draw(sX + 62 + 15, sY + 84 + 30, m_pGame->m_pItemList[cItemID]->m_sSpriteFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[cItemColor].r - GameColors::Base.r, GameColors::Weapons[cItemColor].g - GameColors::Base.g, GameColors::Weapons[cItemColor].b - GameColors::Base.b));
				break;

			default: m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + m_pGame->m_pItemList[cItemID]->m_sSprite]->Draw(sX + 62 + 15, sY + 84 + 30, m_pGame->m_pItemList[cItemID]->m_sSpriteFrame, SpriteLib::DrawParams::Tint(GameColors::Items[cItemColor].r - GameColors::Base.r, GameColors::Items[cItemColor].g - GameColors::Base.g, GameColors::Items[cItemColor].b - GameColors::Base.b));
				break;
			}
		}
		std::memset(cTemp, 0, sizeof(cTemp));
		std::memset(cStr2, 0, sizeof(cStr2));
		std::memset(cStr3, 0, sizeof(cStr3));
		m_pGame->GetItemName(m_pGame->m_pItemList[cItemID].get(), cTemp, cStr2, cStr3);
		wsprintf(cTxt, "%s", cTemp);
		if (m_pGame->m_bIsSpecial)
		{
			PutAlignedString(sX + 25, sX + 240, sY + 60, cTxt, GameColors::UIItemName_Special.r, GameColors::UIItemName_Special.g, GameColors::UIItemName_Special.b);
			PutAlignedString(sX + 25 + 1, sX + 240 + 1, sY + 60, cTxt, GameColors::UIItemName_Special.r, GameColors::UIItemName_Special.g, GameColors::UIItemName_Special.b);
		}
		else
		{
			PutAlignedString(sX + 25, sX + 240, sY + 60, cTxt, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);
			PutAlignedString(sX + 25 + 1, sX + 240 + 1, sY + 60, cTxt, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);
		}
		wsprintf(cTxt, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM2, Info().sV2);
		PutString(sX + 95 + 15, sY + 53 + 60, cTxt, GameColors::UILabel.ToColorRef());
		wsprintf(cTxt, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM6, Info().sV3);
		PutString(sX + 95 + 15, sY + 53 + 75, cTxt, GameColors::UILabel.ToColorRef());
		PutString(sX + 55, sY + 190, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM7, GameColors::UILabel.ToColorRef());

		if ((msX >= sX + DEF_LBTNPOSX) && (msX <= sX + DEF_LBTNPOSX + DEF_BTNSZX) && (msY >= sY + DEF_BTNPOSY) && (msY <= sY + DEF_BTNPOSY + DEF_BTNSZY))
			DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_LBTNPOSX, sY + DEF_BTNPOSY, 43);
		else DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_LBTNPOSX, sY + DEF_BTNPOSY, 42);

		if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX) && (msY >= sY + DEF_BTNPOSY) && (msY <= sY + DEF_BTNPOSY + DEF_BTNSZY))
			DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 17);
		else DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 16);
		break;

	case 3:
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 2);
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_TEXT, sX, sY, 11);

		PutString(sX + 55, sY + 100, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM8, GameColors::UILabel.ToColorRef());
		PutString(sX + 55, sY + 120, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM9, GameColors::UILabel.ToColorRef());
		PutString(sX + 55, sY + 135, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM10, GameColors::UILabel.ToColorRef());
		break;

	case 4:
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 2);
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_TEXT, sX, sY, 10);

		PutString(sX + 55, sY + 100, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM11, GameColors::UILabel.ToColorRef());
		PutString(sX + 55, sY + 120, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM9, GameColors::UILabel.ToColorRef());
		PutString(sX + 55, sY + 135, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM10, GameColors::UILabel.ToColorRef());
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
		if ((msX >= sX + 30) && (msX <= sX + 30 + DEF_BTNSZX) && (msY >= sY + DEF_BTNPOSY) && (msY <= sY + DEF_BTNPOSY + DEF_BTNSZY)) {
			// Sell
			m_pGame->bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQ_SELLITEMCONFIRM, 0, Info().sV1, Info().sV4, Info().sV3, m_pGame->m_pItemList[Info().sV1]->m_cName);
			Info().cMode = 3;
			return true;
		}
		if ((msX >= sX + 154) && (msX <= sX + 154 + DEF_BTNSZX) && (msY >= sY + DEF_BTNPOSY) && (msY <= sY + DEF_BTNPOSY + DEF_BTNSZY)) {
			// Cancel
			m_pGame->m_bIsItemDisabled[Info().sV1] = false;
			m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::SellOrRepair);
			return true;
		}
		break;

	case 2:
		if ((msX >= sX + 30) && (msX <= sX + 30 + DEF_BTNSZX) && (msY >= sY + DEF_BTNPOSY) && (msY <= sY + DEF_BTNPOSY + DEF_BTNSZY)) {
			// Repair
			m_pGame->bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQ_REPAIRITEMCONFIRM, 0, Info().sV1, 0, 0, m_pGame->m_pItemList[Info().sV1]->m_cName);
			Info().cMode = 4;
			return true;
		}
		if ((msX >= sX + 154) && (msX <= sX + 154 + DEF_BTNSZX) && (msY >= sY + DEF_BTNPOSY) && (msY <= sY + DEF_BTNPOSY + DEF_BTNSZY)) {
			// Cancel
			m_pGame->m_bIsItemDisabled[Info().sV1] = false;
			m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::SellOrRepair);
			return true;
		}
		break;
	}

	return false;
}
