#include "DialogBox_Inventory.h"
#include "CursorTarget.h"
#include "Game.h"
#include "IInput.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"

using namespace hb::item;

DialogBox_Inventory::DialogBox_Inventory(CGame* pGame)
	: IDialogBox(DialogBoxId::Inventory, pGame)
{
	SetCanCloseOnRightClick(true);
	SetDefaultRect(380 , 210 , 225, 185);
}

// Helper: Draw a single inventory item with proper coloring and state
void DialogBox_Inventory::DrawInventoryItem(CItem* pItem, int itemIdx, int baseX, int baseY)
{
	CItem* pCfg = m_pGame->GetItemConfig(pItem->m_sIDnum);
	if (pCfg == nullptr) return;

	char cItemColor = pItem->m_cItemColor;
	bool bDisabled = m_pGame->m_bIsItemDisabled[itemIdx];
	bool bIsWeapon = (pCfg->GetEquipPos() == EquipPos::LeftHand) ||
	                 (pCfg->GetEquipPos() == EquipPos::RightHand) ||
	                 (pCfg->GetEquipPos() == EquipPos::TwoHand);

	int drawX = baseX + ITEM_OFFSET_X + pItem->m_sX;
	int drawY = baseY + ITEM_OFFSET_Y + pItem->m_sY;
	auto pSprite = m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + pCfg->m_sSprite];
	uint32_t dwTime = m_pGame->m_dwCurTime;

	// Select color arrays (weapons use different color set)
	const Color* colors = bIsWeapon ? GameColors::Weapons : GameColors::Items;
	// (wG/wB merged into Color array above)


	if (cItemColor == 0)
	{
		// No color tint
		if (bDisabled)
			pSprite->Draw(drawX, drawY, pCfg->m_sSpriteFrame, SpriteLib::DrawParams::Alpha(0.25f));
		else
			pSprite->Draw(drawX, drawY, pCfg->m_sSpriteFrame);
	}
	else
	{
		// Apply color tint
		int r = colors[cItemColor].r;
		int g = colors[cItemColor].g;
		int b = colors[cItemColor].b;

		if (bDisabled)
			pSprite->Draw(drawX, drawY, pCfg->m_sSpriteFrame, SpriteLib::DrawParams::TintedAlpha(r, g, b, 0.7f));
		else
			pSprite->Draw(drawX, drawY, pCfg->m_sSpriteFrame, SpriteLib::DrawParams::Tint(r, g, b));
	}

	// Show item count for consumables and arrows
	if ((pCfg->GetItemType() == ItemType::Consume) || (pCfg->GetItemType() == ItemType::Arrow))
	{
		char countBuf[32];
		m_pGame->FormatCommaNumber(static_cast<uint32_t>(pItem->m_dwCount), countBuf, sizeof(countBuf));
		TextLib::DrawText(GameFont::Default, baseX + COUNT_OFFSET_X + pItem->m_sX, baseY + COUNT_OFFSET_Y + pItem->m_sY, countBuf, TextLib::TextStyle::WithShadow(GameColors::UIDescription));
	}
}

void DialogBox_Inventory::OnDraw(short msX, short msY, short msZ, char cLB)
{
	if (!m_pGame->EnsureItemConfigsLoaded()) return;
	short sX = Info().sX;
	short sY = Info().sY;

	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_INVENTORY, sX, sY, 0);

	// Draw all inventory items
	for (int i = 0; i < DEF_MAXITEMS; i++)
	{
		int itemIdx = m_pGame->m_cItemOrder[i];
		if (itemIdx == -1) continue;

		if (m_pGame->m_pItemList[itemIdx] == nullptr)
			continue;

		CItem* pItem = m_pGame->m_pItemList[itemIdx].get();
		if (pItem == nullptr) continue;

		// Skip items that are selected (being dragged) or equipped
		bool bSelected = (CursorTarget::GetSelectedType() == SelectedObjectType::Item) &&
		                 (CursorTarget::GetSelectedID() == itemIdx);
		bool bEquipped = m_pGame->m_bIsItemEquipped[itemIdx];

		if (!bSelected && !bEquipped)
		{
			DrawInventoryItem(pItem, itemIdx, sX, sY);
		}
	}

	// Item Upgrade button hover
	if ((msX >= sX + BTN_UPGRADE_X1) && (msX <= sX + BTN_UPGRADE_X2) &&
	    (msY >= sY + BTN_Y1) && (msY <= sY + BTN_Y2))
	{
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_INVENTORY, sX + BTN_UPGRADE_X1, sY + BTN_Y1, 1);
	}

	// Manufacture button hover
	if ((msX >= sX + BTN_MANUFACTURE_X1) && (msX <= sX + BTN_MANUFACTURE_X2) &&
	    (msY >= sY + BTN_Y1) && (msY <= sY + BTN_Y2))
	{
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_INVENTORY, sX + BTN_MANUFACTURE_X1, sY + BTN_Y1, 2);
	}
}

bool DialogBox_Inventory::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	// Item Upgrade button
	if ((msX >= sX + BTN_UPGRADE_X1) && (msX <= sX + BTN_UPGRADE_X2) &&
	    (msY >= sY + BTN_Y1) && (msY <= sY + BTN_Y2))
	{
		EnableDialogBox(DialogBoxId::ItemUpgrade, 5, 0, 0);
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	// Manufacture button
	if ((msX >= sX + BTN_MANUFACTURE_X1) && (msX <= sX + BTN_MANUFACTURE_X2) &&
	    (msY >= sY + BTN_Y1) && (msY <= sY + BTN_Y2))
	{
		if (m_pGame->m_pPlayer->m_iSkillMastery[13] == 0)
		{
			AddEventList(DLGBOXCLICK_INVENTORY1, 10);
			AddEventList(DLGBOXCLICK_INVENTORY2, 10);
		}
		else if (m_pGame->m_bSkillUsingStatus)
		{
			AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY5, 10);
			return true;
		}
		else if (m_pGame->_bIsItemOnHand())
		{
			AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY4, 10);
			return true;
		}
		else
		{
			// Look for manufacturing hammer
			for (int i = 0; i < DEF_MAXITEMS; i++)
			{
				CItem* pItem = m_pGame->m_pItemList[i].get();
				if (pItem == nullptr) continue;
				CItem* pCfg = m_pGame->GetItemConfig(pItem->m_sIDnum);
				if (pCfg != nullptr &&
				    pCfg->GetItemType() == ItemType::UseSkillEnableDialogBox &&
				    pCfg->m_sSpriteFrame == 113 &&
				    pItem->m_wCurLifeSpan > 0)
				{
					EnableDialogBox(DialogBoxId::Manufacture, 3, 0, 0);
					AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY12, 10);
					PlaySoundEffect('E', 14, 5);
					return true;
				}
			}
			AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY14, 10);
		}
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	return false;
}

// Helper: Find the clicked inventory item
char DialogBox_Inventory::FindClickedItem(short msX, short msY, short sX, short sY)
{
	for (int i = 0; i < DEF_MAXITEMS; i++)
	{
		if (m_pGame->m_cItemOrder[DEF_MAXITEMS - 1 - i] == -1) continue;
		char cItemID = m_pGame->m_cItemOrder[DEF_MAXITEMS - 1 - i];
		if (m_pGame->m_pItemList[cItemID] == nullptr) continue;

		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[cItemID]->m_sIDnum);
		if (pCfg == nullptr) continue;

		int spriteIdx = DEF_SPRID_ITEMPACK_PIVOTPOINT + pCfg->m_sSprite;
		int drawX = sX + ITEM_OFFSET_X + m_pGame->m_pItemList[cItemID]->m_sX;
		int drawY = sY + ITEM_OFFSET_Y + m_pGame->m_pItemList[cItemID]->m_sY;

		m_pGame->m_pSprite[spriteIdx]->CalculateBounds(drawX, drawY, pCfg->m_sSpriteFrame);
		auto bounds = m_pGame->m_pSprite[spriteIdx]->GetBoundRect();

		if (!m_pGame->m_bIsItemDisabled[cItemID] && !m_pGame->m_bIsItemEquipped[cItemID] &&
			msX > bounds.left && msX < bounds.right && msY > bounds.top && msY < bounds.bottom)
		{
			return cItemID;
		}
	}
	return -1;
}

bool DialogBox_Inventory::OnDoubleClick(short msX, short msY)
{
	if (m_pGame->m_bItemUsingStatus)
	{
		AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY1, 10);
		return true;
	}

	short sX = Info().sX;
	short sY = Info().sY;

	char cItemID = FindClickedItem(msX, msY, sX, sY);
	if (cItemID == -1) return false;

	m_pGame->_SetItemOrder(0, cItemID);

	CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[cItemID]->m_sIDnum);
	if (pCfg == nullptr) return false;

	char cStr1[64], cStr2[64], cStr3[64];
	m_pGame->GetItemName(m_pGame->m_pItemList[cItemID].get(), cStr1, cStr2, cStr3);

	// Check if at repair shop
	if (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::SaleMenu) &&
		!m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::SellOrRepair) &&
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV3 == 24)
	{
		if (pCfg->GetEquipPos() != EquipPos::None)
		{
			bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQ_REPAIRITEM, 0, cItemID,
				m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV3, 0,
				pCfg->m_cName,
				m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV4);
			return true;
		}
	}

	// Bank dialog - drop item there
	if (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::Bank))
	{
		m_pGame->m_dialogBoxManager.GetDialogBox(DialogBoxId::Bank)->OnItemDrop(msX, msY);
		return true;
	}
	// Sell list dialog
	else if (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::SellList))
	{
		m_pGame->m_dialogBoxManager.GetDialogBox(DialogBoxId::SellList)->OnItemDrop(msX, msY);
		return true;
	}
	// Item upgrade dialog
	else if (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemUpgrade))
	{
		m_pGame->m_dialogBoxManager.GetDialogBox(DialogBoxId::ItemUpgrade)->OnItemDrop(msX, msY);
		return true;
	}

	// Handle consumable/depletable items
	if (pCfg->GetItemType() == ItemType::UseDeplete ||
		pCfg->GetItemType() == ItemType::UsePerm ||
		pCfg->GetItemType() == ItemType::Arrow ||
		pCfg->GetItemType() == ItemType::Eat)
	{
		if (!m_pGame->bCheckItemOperationEnabled(cItemID)) return true;

		// Check damage cooldown for scrolls
		if ((m_pGame->m_dwCurTime - m_pGame->m_dwDamagedTime) < 10000)
		{
			if ((pCfg->m_sSprite == 6) &&
				(pCfg->m_sSpriteFrame == 9 ||
				 pCfg->m_sSpriteFrame == 89))
			{
				std::snprintf(m_pGame->G_cTxt, sizeof(m_pGame->G_cTxt), BDLBBOX_DOUBLE_CLICK_INVENTORY3, cStr1);
				AddEventList(m_pGame->G_cTxt, 10);
				return true;
			}
		}

		bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQ_USEITEM, 0, cItemID, 0, 0, 0);

		if (pCfg->GetItemType() == ItemType::UseDeplete ||
			pCfg->GetItemType() == ItemType::Eat)
		{
			m_pGame->m_bIsItemDisabled[cItemID] = true;
			m_pGame->m_bItemUsingStatus = true;
		}
	}

	// Handle skill items (pointing mode)
	if (pCfg->GetItemType() == ItemType::UseSkill)
	{
		if (m_pGame->_bIsItemOnHand())
		{
			AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY4, 10);
			return true;
		}
		if (m_pGame->m_bSkillUsingStatus)
		{
			AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY5, 10);
			return true;
		}
		if (m_pGame->m_pItemList[cItemID]->m_wCurLifeSpan == 0)
		{
			AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY6, 10);
		}
		else
		{
			m_pGame->m_bIsGetPointingMode = true;
			m_pGame->m_iPointCommandType = cItemID;
			char cTxt[120];
			std::snprintf(cTxt, sizeof(cTxt), BDLBBOX_DOUBLE_CLICK_INVENTORY7, cStr1);
			AddEventList(cTxt, 10);
		}
	}

	// Handle deplete-dest items (use on other items)
	if (pCfg->GetItemType() == ItemType::UseDepleteDest)
	{
		if (m_pGame->_bIsItemOnHand())
		{
			AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY4, 10);
			return true;
		}
		if (m_pGame->m_bSkillUsingStatus)
		{
			AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY13, 10);
			return true;
		}
		if (m_pGame->m_pItemList[cItemID]->m_wCurLifeSpan == 0)
		{
			AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY6, 10);
		}
		else
		{
			m_pGame->m_bIsGetPointingMode = true;
			m_pGame->m_iPointCommandType = cItemID;
			char cTxt[120];
			std::snprintf(cTxt, sizeof(cTxt), BDLBBOX_DOUBLE_CLICK_INVENTORY8, cStr1);
			AddEventList(cTxt, 10);
		}
	}

	// Handle skill items that enable dialog boxes (alchemy pot, anvil, crafting, slates)
	if (pCfg->GetItemType() == ItemType::UseSkillEnableDialogBox)
	{
		if (m_pGame->_bIsItemOnHand())
		{
			AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY4, 10);
			return true;
		}
		if (m_pGame->m_bSkillUsingStatus)
		{
			AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY5, 10);
			return true;
		}
		if (m_pGame->m_pItemList[cItemID]->m_wCurLifeSpan == 0)
		{
			AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY6, 10);
		}
		else
		{
			switch (pCfg->m_sSpriteFrame)
			{
			case 55: // Alchemy pot
				if (m_pGame->m_pPlayer->m_iSkillMastery[12] == 0)
					AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY9, 10);
				else
				{
					EnableDialogBox(DialogBoxId::Manufacture, 1, 0, 0);
					AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY10, 10);
				}
				break;

			case 113: // Smith's Anvil
				if (m_pGame->m_pPlayer->m_iSkillMastery[13] == 0)
					AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY11, 10);
				else
				{
					EnableDialogBox(DialogBoxId::Manufacture, 3, 0, 0);
					AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY12, 10);
				}
				break;

			case 0: // Crafting
				EnableDialogBox(DialogBoxId::Manufacture, 7, 0, 0);
				AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY17, 10);
				break;

			case 151:
			case 152:
			case 153:
			case 154: // Slates
				EnableDialogBox(DialogBoxId::Slates, 1, 0, 0);
				break;
			}
		}
	}

	// If alchemy/manufacture/crafting dialog is open, drop item there
	if (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::Manufacture))
	{
		char mode = m_pGame->m_dialogBoxManager.Info(DialogBoxId::Manufacture).cMode;
		if (mode == 1 || mode == 4 || mode == 7)
			m_pGame->m_dialogBoxManager.GetDialogBox(DialogBoxId::Manufacture)->OnItemDrop(msX, msY);
	}

	// Auto-equip equipment items
	if (pCfg->GetItemType() == ItemType::Equip)
	{
		CursorTarget::SetSelection(SelectedObjectType::Item, (short)cItemID, 0, 0);
		m_pGame->m_dialogBoxManager.GetDialogBox(DialogBoxId::CharacterInfo)->OnItemDrop(msX, msY);
		CursorTarget::ClearSelection();
	}

	return true;
}

PressResult DialogBox_Inventory::OnPress(short msX, short msY)
{
	// Don't allow item selection if certain dialogs are open
	if (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropExternal)) return PressResult::Normal;
	if (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropConfirm)) return PressResult::Normal;

	short sX = Info().sX;
	short sY = Info().sY;

	// Check items in reverse order (topmost first)
	for (int i = 0; i < DEF_MAXITEMS; i++)
	{
		char cItemID = m_pGame->m_cItemOrder[DEF_MAXITEMS - 1 - i];
		if (cItemID == -1) continue;

		CItem* pItem = m_pGame->m_pItemList[cItemID].get();
		if (pItem == nullptr) continue;

		CItem* pCfg = m_pGame->GetItemConfig(pItem->m_sIDnum);
		if (pCfg == nullptr) continue;

		// Skip disabled or equipped items
		if (m_pGame->m_bIsItemDisabled[cItemID]) continue;
		if (m_pGame->m_bIsItemEquipped[cItemID]) continue;

		// Calculate item bounds
		int spriteIdx = DEF_SPRID_ITEMPACK_PIVOTPOINT + pCfg->m_sSprite;
		int itemDrawX = sX + ITEM_OFFSET_X + pItem->m_sX;
		int itemDrawY = sY + ITEM_OFFSET_Y + pItem->m_sY;

		m_pGame->m_pSprite[spriteIdx]->CalculateBounds(itemDrawX, itemDrawY, pCfg->m_sSpriteFrame);
		auto bounds = m_pGame->m_pSprite[spriteIdx]->GetBoundRect();

		// Check if click is within item bounds
		if (msX > bounds.left && msX < bounds.right &&
			msY > bounds.top && msY < bounds.bottom)
		{
			// Pixel-perfect collision check
			if (m_pGame->m_pSprite[spriteIdx]->CheckCollision(itemDrawX, itemDrawY, pCfg->m_sSpriteFrame, msX, msY))
			{
				// Bring item to top of order
				m_pGame->_SetItemOrder(0, cItemID);

				// Handle pointing mode (using items on other items)
				bool bHandledPointing = false;
				if (m_pGame->m_bIsGetPointingMode &&
					m_pGame->m_iPointCommandType >= 0 &&
					m_pGame->m_iPointCommandType < 100 &&
					m_pGame->m_pItemList[m_pGame->m_iPointCommandType] != nullptr &&
					m_pGame->m_iPointCommandType != cItemID)
				{
					CItem* pPointCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[m_pGame->m_iPointCommandType]->m_sIDnum);
					if (pPointCfg != nullptr &&
						pPointCfg->GetItemType() == ItemType::UseDepleteDest)
					{
						m_pGame->PointCommandHandler(0, 0, cItemID);
						m_pGame->m_bIsGetPointingMode = false;
						bHandledPointing = true;
					}
				}
				if (!bHandledPointing)
				{
					// Select the item for dragging
					CursorTarget::SetSelection(SelectedObjectType::Item, cItemID,
						msX - itemDrawX, msY - itemDrawY);
				}
				return PressResult::ItemSelected;
			}
		}
	}

	return PressResult::Normal;
}

bool DialogBox_Inventory::OnItemDrop(short msX, short msY)
{
	if (m_pGame->m_pPlayer->m_Controller.GetCommand() < 0) return false;

	char cSelectedID = (char)CursorTarget::GetSelectedID();
	if (m_pGame->m_pItemList[cSelectedID] == nullptr) return false;

	// Can't move equipped items while using a skill
	if (m_pGame->m_bSkillUsingStatus && m_pGame->m_bIsItemEquipped[cSelectedID])
	{
		AddEventList(BITEMDROP_INVENTORY1, 10);
		return false;
	}
	if (m_pGame->m_bIsItemDisabled[cSelectedID]) return false;

	// Calculate new position in inventory grid
	short sX = Info().sX;
	short sY = Info().sY;
	short dX = msX - sX - ITEM_OFFSET_X - CursorTarget::GetDragDistX();
	short dY = msY - sY - ITEM_OFFSET_Y - CursorTarget::GetDragDistY();

	// Clamp to valid inventory area
	if (dY < -10) dY = -10;
	if (dX < 0) dX = 0;
	if (dX > 170) dX = 170;
	if (dY > 95) dY = 95;

	m_pGame->m_pItemList[cSelectedID]->m_sX = dX;
	m_pGame->m_pItemList[cSelectedID]->m_sY = dY;

	// Shift+drop: move all items with the same name to this position
	if (Input::IsShiftDown())
	{
		for (int i = 0; i < DEF_MAXITEMS; i++)
		{
			if (m_pGame->m_cItemOrder[DEF_MAXITEMS - 1 - i] != -1)
			{
				char cItemID = m_pGame->m_cItemOrder[DEF_MAXITEMS - 1 - i];
				if (m_pGame->m_pItemList[cItemID] != nullptr &&
					m_pGame->m_pItemList[cItemID]->m_sIDnum == m_pGame->m_pItemList[cSelectedID]->m_sIDnum)
				{
					m_pGame->m_pItemList[cItemID]->m_sX = dX;
					m_pGame->m_pItemList[cItemID]->m_sY = dY;
					bSendCommand(MSGID_REQUEST_SETITEMPOS, 0, cItemID, dX, dY, 0, 0);
				}
			}
		}
	}
	else
	{
		bSendCommand(MSGID_REQUEST_SETITEMPOS, 0, cSelectedID, dX, dY, 0, 0);
	}

	// If item was equipped, unequip it
	if (m_pGame->m_bIsItemEquipped[cSelectedID])
	{
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[cSelectedID]->m_sIDnum);
		if (pCfg == nullptr) return false;

		char cStr1[64], cStr2[64], cStr3[64];
		char cTxt[120];
		m_pGame->GetItemName(m_pGame->m_pItemList[cSelectedID].get(), cStr1, cStr2, cStr3);
		std::snprintf(cTxt, sizeof(cTxt), ITEM_EQUIPMENT_RELEASED, cStr1);
		AddEventList(cTxt, 10);

		{
			short sID = m_pGame->m_pItemList[cSelectedID]->m_sIDnum;
			if (sID == hb::item::ItemId::AngelicPandentSTR || sID == hb::item::ItemId::AngelicPandentDEX ||
				sID == hb::item::ItemId::AngelicPandentINT || sID == hb::item::ItemId::AngelicPandentMAG)
				m_pGame->PlayGameSound('E', 53, 0);
			else
				m_pGame->PlayGameSound('E', 29, 0);
		}

		// Remove Angelic Stats
		if (pCfg->m_cEquipPos >= 11 &&
			pCfg->GetItemType() == ItemType::Equip)
		{
			if (m_pGame->m_pItemList[cSelectedID]->m_sIDnum == hb::item::ItemId::AngelicPandentSTR)
				m_pGame->m_pPlayer->m_iAngelicStr = 0;
			else if (m_pGame->m_pItemList[cSelectedID]->m_sIDnum == hb::item::ItemId::AngelicPandentDEX)
				m_pGame->m_pPlayer->m_iAngelicDex = 0;
			else if (m_pGame->m_pItemList[cSelectedID]->m_sIDnum == hb::item::ItemId::AngelicPandentINT)
				m_pGame->m_pPlayer->m_iAngelicInt = 0;
			else if (m_pGame->m_pItemList[cSelectedID]->m_sIDnum == hb::item::ItemId::AngelicPandentMAG)
				m_pGame->m_pPlayer->m_iAngelicMag = 0;
		}

		bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_RELEASEITEM, 0, cSelectedID, 0, 0, 0);
		m_pGame->m_bIsItemEquipped[cSelectedID] = false;
		m_pGame->m_sItemEquipmentStatus[pCfg->m_cEquipPos] = -1;
	}

	return true;
}
