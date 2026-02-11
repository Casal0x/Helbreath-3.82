#include "DialogBox_ItemDrop.h"
#include "Game.h"
#include "lan_eng.h"

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
DialogBox_ItemDrop::DialogBox_ItemDrop(CGame* pGame)
	: IDialogBox(DialogBoxId::ItemDropConfirm, pGame)
{
	SetDefaultRect(0 , 0 , 270, 105);
}

void DialogBox_ItemDrop::OnDraw(short msX, short msY, short msZ, char cLB)
{
	if (!m_pGame->EnsureItemConfigsLoaded()) return;
	short sX = Info().sX;
	short sY = Info().sY;
	char cTxt[120], cStr1[64], cStr2[64], cStr3[64];

	DrawNewDialogBox(InterfaceNdGame1, sX, sY, 2);

	m_pGame->GetItemName(m_pGame->m_pItemList[Info().sView].get(), cStr1, cStr2, cStr3);

	if (strlen(Info().cStr) == 0)
		std::snprintf(cTxt, sizeof(cTxt), "%s", cStr1);

	// Item name (green if special, blue otherwise)
	if (m_pGame->m_bIsSpecial)
	{
		PutString(sX + 35, sY + 20, cTxt, GameColors::UIItemName_Special);
		PutString(sX + 36, sY + 20, cTxt, GameColors::UIItemName_Special);
	}
	else
	{
		PutString(sX + 35, sY + 20, cTxt, GameColors::UIMagicBlue);
		PutString(sX + 36, sY + 20, cTxt, GameColors::UIMagicBlue);
	}

	// "Do you want to drop?" text
	PutString(sX + 35, sY + 36, DRAW_DIALOGBOX_ITEM_DROP1, GameColors::UIMagicBlue);
	PutString(sX + 36, sY + 36, DRAW_DIALOGBOX_ITEM_DROP1, GameColors::UIMagicBlue);

	// Toggle option text
	if (m_pGame->m_bItemDrop)
	{
		// "Don't show this message again"
		if ((msX >= sX + 35) && (msX <= sX + 240) && (msY >= sY + 80) && (msY <= sY + 90))
		{
			PutString(sX + 35, sY + 80, DRAW_DIALOGBOX_ITEM_DROP2, GameColors::UIWhite);
			PutString(sX + 36, sY + 80, DRAW_DIALOGBOX_ITEM_DROP2, GameColors::UIWhite);
		}
		else
		{
			PutString(sX + 35, sY + 80, DRAW_DIALOGBOX_ITEM_DROP2, GameColors::UIMagicBlue);
			PutString(sX + 36, sY + 80, DRAW_DIALOGBOX_ITEM_DROP2, GameColors::UIMagicBlue);
		}
	}
	else
	{
		// "Show this message again"
		if ((msX >= sX + 35) && (msX <= sX + 240) && (msY >= sY + 80) && (msY <= sY + 90))
		{
			PutString(sX + 35, sY + 80, DRAW_DIALOGBOX_ITEM_DROP3, GameColors::UIWhite);
			PutString(sX + 36, sY + 80, DRAW_DIALOGBOX_ITEM_DROP3, GameColors::UIWhite);
		}
		else
		{
			PutString(sX + 35, sY + 80, DRAW_DIALOGBOX_ITEM_DROP3, GameColors::UIMagicBlue);
			PutString(sX + 36, sY + 80, DRAW_DIALOGBOX_ITEM_DROP3, GameColors::UIMagicBlue);
		}
	}

	// Yes button
	if ((msX >= sX + 30) && (msX <= sX + 30 + ui_layout::btn_size_x) && (msY >= sY + 55) && (msY <= sY + 55 + ui_layout::btn_size_y))
		DrawNewDialogBox(InterfaceNdButton, sX + 30, sY + 55, 19);
	else
		DrawNewDialogBox(InterfaceNdButton, sX + 30, sY + 55, 18);

	// No button
	if ((msX >= sX + 170) && (msX <= sX + 170 + ui_layout::btn_size_x) && (msY >= sY + 55) && (msY <= sY + 55 + ui_layout::btn_size_y))
		DrawNewDialogBox(InterfaceNdButton, sX + 170, sY + 55, 3);
	else
		DrawNewDialogBox(InterfaceNdButton, sX + 170, sY + 55, 2);
}

bool DialogBox_ItemDrop::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	if (m_pGame->m_pPlayer->m_Controller.GetCommand() < 0) return false;

	// Yes button - drop item
	if ((msX >= sX + 30) && (msX <= sX + 30 + ui_layout::btn_size_x) && (msY >= sY + 55) && (msY <= sY + 55 + ui_layout::btn_size_y))
	{
		Info().cMode = 3;
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[Info().sView]->m_sIDnum);
		if (pCfg) bSendCommand(MsgId::CommandCommon,
			CommonType::ItemDrop,
			0,
			Info().sView,
			1,
			0,
			pCfg->m_cName);
		DisableThisDialog();
		return true;
	}

	// No button - cancel
	if ((msX >= sX + 170) && (msX <= sX + 170 + ui_layout::btn_size_x) && (msY >= sY + 55) && (msY <= sY + 55 + ui_layout::btn_size_y))
	{
		for (int i = 0; i < game_limits::max_sell_list; i++)
			m_pGame->m_bIsItemDisabled[i] = false;

		DisableThisDialog();
		return true;
	}

	// Toggle "don't show again" option
	if ((msX >= sX + 35) && (msX <= sX + 240) && (msY >= sY + 80) && (msY <= sY + 90))
	{
		m_pGame->m_bItemDrop = !m_pGame->m_bItemDrop;
		return true;
	}

	return false;
}
