#include "DialogBox_SellList.h"
#include "CursorTarget.h"
#include "Game.h"
#include "ItemNameFormatter.h"
#include "GlobalDef.h"
#include "lan_eng.h"

using namespace hb::shared::net;
using namespace hb::shared::item;
using namespace hb::client::sprite_id;

DialogBox_SellList::DialogBox_SellList(CGame* pGame)
	: IDialogBox(DialogBoxId::SellList, pGame)
{
	SetDefaultRect(170 , 70 , 258, 339);
}

void DialogBox_SellList::OnDraw(short msX, short msY, short msZ, char cLB)
{
	if (!m_pGame->EnsureItemConfigsLoaded()) return;
	short sX = Info().sX;
	short sY = Info().sY;
	short szX = Info().sSizeX;

	DrawNewDialogBox(InterfaceNdGame2, sX, sY, 2);
	DrawNewDialogBox(InterfaceNdText, sX, sY, 11);

	int iEmptyCount = 0;
	DrawItemList(sX, sY, szX, msX, msY, iEmptyCount);

	if (iEmptyCount == game_limits::max_sell_list) {
		DrawEmptyListMessage(sX, sY, szX);
	}

	bool bHasItems = (iEmptyCount < game_limits::max_sell_list);
	DrawButtons(sX, sY, msX, msY, bHasItems);
}

void DialogBox_SellList::DrawItemList(short sX, short sY, short szX, short msX, short msY, int& iEmptyCount)
{
	char cTxt[256], cStr1[64], cStr2[64], cStr3[64];

	for (int i = 0; i < game_limits::max_sell_list; i++)
	{
		if (m_pGame->m_stSellItemList[i].iIndex != -1)
		{
			int iItemIndex = m_pGame->m_stSellItemList[i].iIndex;
			std::memset(cStr1, 0, sizeof(cStr1));
			std::memset(cStr2, 0, sizeof(cStr2));
			std::memset(cStr3, 0, sizeof(cStr3));
			ItemNameFormatter::Get().Format(m_pGame->m_pItemList[iItemIndex].get(), cStr1, cStr2, cStr3);

			bool bHover = (msX > sX + 25) && (msX < sX + 250) && (msY >= sY + 55 + i * 15) && (msY <= sY + 55 + 14 + i * 15);

			if (m_pGame->m_stSellItemList[i].iAmount > 1)
			{
				// Multiple items
				std::memset(cTxt, 0, sizeof(cTxt));
				std::snprintf(cTxt, sizeof(cTxt), DRAW_DIALOGBOX_SELL_LIST1, m_pGame->m_stSellItemList[i].iAmount, cStr1);

				if (bHover)
					PutAlignedString(sX, sX + szX, sY + 55 + i * 15, cTxt, GameColors::UIWhite);
				else if (ItemNameFormatter::Get().IsSpecial())
					PutAlignedString(sX, sX + szX, sY + 55 + i * 15, cTxt, GameColors::UIItemName_Special);
				else
					PutAlignedString(sX, sX + szX, sY + 55 + i * 15, cTxt, GameColors::UILabel);
			}
			else
			{
				// Single item
				if (bHover)
				{
					if ((strlen(cStr2) == 0) && (strlen(cStr3) == 0))
					{
						PutAlignedString(sX, sX + szX, sY + 55 + i * 15, cStr1, GameColors::UIWhite);
					}
					else
					{
						std::memset(cTxt, 0, sizeof(cTxt));
						if ((strlen(cStr1) + strlen(cStr2) + strlen(cStr3)) < 36)
						{
							if ((strlen(cStr2) > 0) && (strlen(cStr3) > 0))
								std::snprintf(cTxt, sizeof(cTxt), "%s(%s, %s)", cStr1, cStr2, cStr3);
							else
								std::snprintf(cTxt, sizeof(cTxt), "%s(%s%s)", cStr1, cStr2, cStr3);
							PutAlignedString(sX, sX + szX, sY + 55 + i * 15, cTxt, GameColors::UIWhite);
						}
						else
						{
							if ((strlen(cStr2) > 0) && (strlen(cStr3) > 0))
								std::snprintf(cTxt, sizeof(cTxt), "(%s, %s)", cStr2, cStr3);
							else
								std::snprintf(cTxt, sizeof(cTxt), "(%s%s)", cStr2, cStr3);
							PutAlignedString(sX, sX + szX, sY + 55 + i * 15, cStr1, GameColors::UIWhite);
							PutAlignedString(sX, sX + szX, sY + 55 + i * 15 + 15, cTxt, GameColors::UIDisabled);
							i++;
						}
					}
				}
				else
				{
					if ((strlen(cStr2) == 0) && (strlen(cStr3) == 0))
					{
						if (ItemNameFormatter::Get().IsSpecial())
							PutAlignedString(sX, sX + szX, sY + 55 + i * 15, cStr1, GameColors::UIItemName_Special);
						else
							PutAlignedString(sX, sX + szX, sY + 55 + i * 15, cStr1, GameColors::UILabel);
					}
					else
					{
						std::memset(cTxt, 0, sizeof(cTxt));
						if ((strlen(cStr1) + strlen(cStr2) + strlen(cStr3)) < 36)
						{
							if ((strlen(cStr2) > 0) && (strlen(cStr3) > 0))
								std::snprintf(cTxt, sizeof(cTxt), "%s(%s, %s)", cStr1, cStr2, cStr3);
							else
								std::snprintf(cTxt, sizeof(cTxt), "%s(%s%s)", cStr1, cStr2, cStr3);

							if (ItemNameFormatter::Get().IsSpecial())
								PutAlignedString(sX, sX + szX, sY + 55 + i * 15, cTxt, GameColors::UIItemName_Special);
							else
								PutAlignedString(sX, sX + szX, sY + 55 + i * 15, cTxt, GameColors::UILabel);
						}
						else
						{
							if (ItemNameFormatter::Get().IsSpecial())
								PutAlignedString(sX, sX + szX, sY + 55 + i * 15, cStr1, GameColors::UIItemName_Special);
							else
								PutAlignedString(sX, sX + szX, sY + 55 + i * 15, cStr1, GameColors::UILabel);
						}
					}
				}
			}
		}
		else
		{
			iEmptyCount++;
		}
	}
}

void DialogBox_SellList::DrawEmptyListMessage(short sX, short sY, short szX)
{
	PutAlignedString(sX, sX + szX, sY + 55 + 30 + 282 - 117 - 170, DRAW_DIALOGBOX_SELL_LIST2);
	PutAlignedString(sX, sX + szX, sY + 55 + 45 + 282 - 117 - 170, DRAW_DIALOGBOX_SELL_LIST3);
	PutAlignedString(sX, sX + szX, sY + 55 + 60 + 282 - 117 - 170, DRAW_DIALOGBOX_SELL_LIST4);
	PutAlignedString(sX, sX + szX, sY + 55 + 75 + 282 - 117 - 170, DRAW_DIALOGBOX_SELL_LIST5);
	PutAlignedString(sX, sX + szX, sY + 55 + 95 + 282 - 117 - 170, DRAW_DIALOGBOX_SELL_LIST6);
	PutAlignedString(sX, sX + szX, sY + 55 + 110 + 282 - 117 - 170, DRAW_DIALOGBOX_SELL_LIST7);
	PutAlignedString(sX, sX + szX, sY + 55 + 125 + 282 - 117 - 170, DRAW_DIALOGBOX_SELL_LIST8);
	PutAlignedString(sX, sX + szX, sY + 55 + 155 + 282 - 117 - 170, DRAW_DIALOGBOX_SELL_LIST9);
}

void DialogBox_SellList::DrawButtons(short sX, short sY, short msX, short msY, bool bHasItems)
{
	// Sell button (only enabled when there are items)
	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) &&
		(msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y) && bHasItems)
		DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 39);
	else
		DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 38);

	// Cancel button
	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) &&
		(msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 17);
	else
		DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 16);
}

bool DialogBox_SellList::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	// Check if clicking on an item in the list to remove it
	for (int i = 0; i < game_limits::max_sell_list; i++)
	{
		if ((msX > sX + 25) && (msX < sX + 250) && (msY >= sY + 55 + i * 15) && (msY <= sY + 55 + 14 + i * 15))
		{
			if (m_pGame->m_pItemList[m_pGame->m_stSellItemList[i].iIndex] != 0)
			{
				// Re-enable the item
				m_pGame->m_bIsItemDisabled[m_pGame->m_stSellItemList[i].iIndex] = false;
				m_pGame->m_stSellItemList[i].iIndex = -1;

				m_pGame->PlayGameSound('E', 14, 5);

				// Compact the list
				for (int x = 0; x < game_limits::max_sell_list - 1; x++)
				{
					if (m_pGame->m_stSellItemList[x].iIndex == -1)
					{
						m_pGame->m_stSellItemList[x].iIndex = m_pGame->m_stSellItemList[x + 1].iIndex;
						m_pGame->m_stSellItemList[x].iAmount = m_pGame->m_stSellItemList[x + 1].iAmount;

						m_pGame->m_stSellItemList[x + 1].iIndex = -1;
						m_pGame->m_stSellItemList[x + 1].iAmount = 0;
					}
				}
			}
			return true;
		}
	}

	// Sell button
	if ((msX >= sX + 30) && (msX <= sX + 30 + ui_layout::btn_size_x) &&
		(msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		m_pGame->bSendCommand(MsgId::RequestSellItemList, 0, 0, 0, 0, 0, 0);
		m_pGame->PlayGameSound('E', 14, 5);
		DisableThisDialog();
		return true;
	}

	// Cancel button
	if ((msX >= sX + 154) && (msX <= sX + 154 + ui_layout::btn_size_x) &&
		(msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		m_pGame->PlayGameSound('E', 14, 5);
		DisableThisDialog();
		return true;
	}

	return false;
}

bool DialogBox_SellList::OnItemDrop(short msX, short msY)
{
	char cItemID = (char)CursorTarget::GetSelectedID();

	if (m_pGame->m_pItemList[cItemID] == nullptr) return false;
	if (m_pGame->m_bIsItemDisabled[cItemID]) return false;
	if (m_pGame->m_pPlayer->m_Controller.GetCommand() < 0) return false;

	// Check if item is already in sell list
	for (int i = 0; i < game_limits::max_sell_list; i++)
	{
		if (m_pGame->m_stSellItemList[i].iIndex == cItemID)
		{
			AddEventList(BITEMDROP_SELLLIST1, 10);
			return false;
		}
	}

	// Can't sell gold
	if (m_pGame->m_pItemList[cItemID]->m_sIDnum == hb::shared::item::ItemId::Gold)
	{
		AddEventList(BITEMDROP_SELLLIST2, 10);
		return false;
	}

	// Can't sell broken items
	if (m_pGame->m_pItemList[cItemID]->m_wCurLifeSpan == 0)
	{
		std::memset(m_pGame->G_cTxt, 0, sizeof(m_pGame->G_cTxt));
		char cStr1[64], cStr2[64], cStr3[64];
		ItemNameFormatter::Get().Format(m_pGame->m_pItemList[cItemID].get(), cStr1, cStr2, cStr3);
		std::snprintf(m_pGame->G_cTxt, sizeof(m_pGame->G_cTxt), NOTIFYMSG_CANNOT_SELL_ITEM2, cStr1);
		AddEventList(m_pGame->G_cTxt, 10);
		return false;
	}

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
		dropInfo.sV3 = 1001;
		dropInfo.sV4 = cItemID;
		std::memset(dropInfo.cStr, 0, sizeof(dropInfo.cStr));
		m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::ItemDropExternal, cItemID, m_pGame->m_pItemList[cItemID]->m_dwCount, 0);
		m_pGame->m_bIsItemDisabled[cItemID] = true;
	}
	else
	{
		// Add single item to sell list
		for (int i = 0; i < game_limits::max_sell_list; i++)
		{
			if (m_pGame->m_stSellItemList[i].iIndex == -1)
			{
				m_pGame->m_stSellItemList[i].iIndex = cItemID;
				m_pGame->m_stSellItemList[i].iAmount = 1;
				m_pGame->m_bIsItemDisabled[cItemID] = true;
				return true;
			}
		}
		AddEventList(BITEMDROP_SELLLIST3, 10);
	}

	return true;
}
