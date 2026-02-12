#include "InventoryManager.h"
#include "Game.h"
#include "TeleportManager.h"
#include "ItemNameFormatter.h"
#include "lan_eng.h"
#include "NetMessages.h"
#include <format>

using namespace hb::shared::net;
using hb::shared::item::ItemType;
using hb::shared::item::EquipPos;
using hb::shared::item::ToInt;

InventoryManager& InventoryManager::Get()
{
	static InventoryManager instance;
	return instance;
}

void InventoryManager::SetGame(CGame* pGame)
{
	m_game = pGame;
}

void InventoryManager::SetItemOrder(int cWhere, int cItemID)
{
	if (cItemID < 0 || cItemID >= hb::shared::limits::MaxItems) return;
	int i;

	switch (cWhere) {
	case 0:
		for (i = 0; i < hb::shared::limits::MaxItems; i++)
			if (m_game->m_cItemOrder[i] == cItemID)
				m_game->m_cItemOrder[i] = -1;

		for (i = 1; i < hb::shared::limits::MaxItems; i++)
			if ((m_game->m_cItemOrder[i - 1] == -1) && (m_game->m_cItemOrder[i] != -1)) {
				m_game->m_cItemOrder[i - 1] = m_game->m_cItemOrder[i];
				m_game->m_cItemOrder[i] = -1;
			}

		for (i = 0; i < hb::shared::limits::MaxItems; i++)
			if (m_game->m_cItemOrder[i] == -1) {
				m_game->m_cItemOrder[i] = cItemID;
				return;
			}
		break;
	}
}

int InventoryManager::CalcTotalWeight()
{
	int i, iWeight, iCnt, iTemp;
	iCnt = 0;
	iWeight = 0;
	for (i = 0; i < hb::shared::limits::MaxItems; i++)
		if (m_game->m_pItemList[i] != 0)
		{
			CItem* pCfg = m_game->GetItemConfig(m_game->m_pItemList[i]->m_sIDnum);
			if (pCfg && ((pCfg->GetItemType() == ItemType::Consume)
				|| (pCfg->GetItemType() == ItemType::Arrow)))
			{
				iTemp = pCfg->m_wWeight * m_game->m_pItemList[i]->m_dwCount;
				if (m_game->m_pItemList[i]->m_sIDnum == hb::shared::item::ItemId::Gold) iTemp = iTemp / 20;
				iWeight += iTemp;
			}
			else if (pCfg) iWeight += pCfg->m_wWeight;
			iCnt++;
		}

	return iWeight;
}

int InventoryManager::GetTotalItemCount()
{
	int i, iCnt;
	iCnt = 0;
	for (i = 0; i < hb::shared::limits::MaxItems; i++)
		if (m_game->m_pItemList[i] != 0) iCnt++;
	return iCnt;
}

int InventoryManager::GetBankItemCount()
{
	int i, iCnt;

	iCnt = 0;
	for (i = 0; i < hb::shared::limits::MaxBankItems; i++)
		if (m_game->m_pBankList[i] != 0) iCnt++;

	return iCnt;
}

void InventoryManager::EraseItem(int cItemID)
{
	if (cItemID < 0 || cItemID >= hb::shared::limits::MaxItems) return;
	int i;
	for (i = 0; i < 6; i++)
	{
		if (m_game->m_sShortCut[i] == cItemID)
		{
			std::string G_cTxt;
			auto itemInfo = ItemNameFormatter::Get().Format(m_game->m_pItemList[cItemID].get());
			if (i < 3) G_cTxt = std::format(ERASE_ITEM, itemInfo.name.c_str(), itemInfo.effect.c_str(), itemInfo.extra.c_str(), i + 1);
			else G_cTxt = std::format(ERASE_ITEM, itemInfo.name.c_str(), itemInfo.effect.c_str(), itemInfo.extra.c_str(), i + 7);
			m_game->AddEventList(G_cTxt.c_str(), 10);
			m_game->m_sShortCut[i] = -1;
		}
	}

	if (cItemID == m_game->m_sRecentShortCut)
		m_game->m_sRecentShortCut = -1;
	// ItemOrder
	for (i = 0; i < hb::shared::limits::MaxItems; i++)
		if (m_game->m_cItemOrder[i] == cItemID)
			m_game->m_cItemOrder[i] = -1;
	for (i = 1; i < hb::shared::limits::MaxItems; i++)
		if ((m_game->m_cItemOrder[i - 1] == -1) && (m_game->m_cItemOrder[i] != -1))
		{
			m_game->m_cItemOrder[i - 1] = m_game->m_cItemOrder[i];
			m_game->m_cItemOrder[i] = -1;
		}
	// ItemList
	m_game->m_pItemList[cItemID].reset();
	m_game->m_bIsItemEquipped[cItemID] = false;
	m_game->m_bIsItemDisabled[cItemID] = false;
}

bool InventoryManager::CheckItemOperationEnabled(int cItemID)
{
	if (cItemID < 0 || cItemID >= hb::shared::limits::MaxItems) return false;
	if (m_game->m_pItemList[cItemID] == 0) return false;
	if (m_game->m_pPlayer->m_Controller.GetCommand() < 0) return false;
	if (TeleportManager::Get().IsRequested()) return false;
	if (m_game->m_bIsItemDisabled[cItemID] == true) return false;

	if ((m_game->m_pItemList[cItemID]->m_sSpriteFrame == 155) && (m_game->m_bUsingSlate == true))
	{
		if ((m_game->m_cMapIndex == 35) || (m_game->m_cMapIndex == 36) || (m_game->m_cMapIndex == 37))
		{
			m_game->AddEventList(DEF_MSG_NOTIFY_SLATE_WRONG_MAP, 10);
			return false;
		}
		m_game->AddEventList(DEF_MSG_NOTIFY_SLATE_ALREADYUSING, 10);
		return false;
	}

	if (m_game->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropExternal) == true)
	{
		m_game->AddEventList(BCHECK_ITEM_OPERATION_ENABLE1, 10);
		return false;
	}

	if (m_game->m_dialogBoxManager.IsEnabled(DialogBoxId::NpcActionQuery) == true)
	{
		m_game->AddEventList(BCHECK_ITEM_OPERATION_ENABLE1, 10);
		return false;
	}

	if (m_game->m_dialogBoxManager.IsEnabled(DialogBoxId::SellOrRepair) == true)
	{
		m_game->AddEventList(BCHECK_ITEM_OPERATION_ENABLE1, 10);
		return false;
	}

	if (m_game->m_dialogBoxManager.IsEnabled(DialogBoxId::Manufacture) == true)
	{
		m_game->AddEventList(BCHECK_ITEM_OPERATION_ENABLE1, 10);
		return false;
	}

	if (m_game->m_dialogBoxManager.IsEnabled(DialogBoxId::Exchange) == true)
	{
		m_game->AddEventList(BCHECK_ITEM_OPERATION_ENABLE1, 10);
		return false;
	}

	if (m_game->m_dialogBoxManager.IsEnabled(DialogBoxId::SellList) == true)
	{
		m_game->AddEventList(BCHECK_ITEM_OPERATION_ENABLE1, 10);
		return false;
	}

	if (m_game->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropConfirm) == true)
	{
		m_game->AddEventList(BCHECK_ITEM_OPERATION_ENABLE1, 10);
		return false;
	}

	return true;
}

void InventoryManager::UnequipSlot(int cEquipPos)
{
	if (cEquipPos < 0 || cEquipPos >= hb::shared::item::DEF_MAXITEMEQUIPPOS) return;
	std::string G_cTxt;
	if (m_game->m_sItemEquipmentStatus[cEquipPos] < 0) return;
	// Remove Angelic Stats
	CItem* pCfgEq = m_game->GetItemConfig(m_game->m_pItemList[m_game->m_sItemEquipmentStatus[cEquipPos]]->m_sIDnum);
	if ((cEquipPos >= 11)
		&& (pCfgEq && pCfgEq->GetItemType() == ItemType::Equip))
	{
		short sItemID = m_game->m_sItemEquipmentStatus[cEquipPos];
		if (m_game->m_pItemList[sItemID]->m_sIDnum == hb::shared::item::ItemId::AngelicPandentSTR)
			m_game->m_pPlayer->m_iAngelicStr = 0;
		else if (m_game->m_pItemList[sItemID]->m_sIDnum == hb::shared::item::ItemId::AngelicPandentDEX)
			m_game->m_pPlayer->m_iAngelicDex = 0;
		else if (m_game->m_pItemList[sItemID]->m_sIDnum == hb::shared::item::ItemId::AngelicPandentINT)
			m_game->m_pPlayer->m_iAngelicInt = 0;
		else if (m_game->m_pItemList[sItemID]->m_sIDnum == hb::shared::item::ItemId::AngelicPandentMAG)
			m_game->m_pPlayer->m_iAngelicMag = 0;
	}

	auto itemInfo2 = ItemNameFormatter::Get().Format(m_game->m_pItemList[m_game->m_sItemEquipmentStatus[cEquipPos]].get());
	G_cTxt = std::format(ITEM_EQUIPMENT_RELEASED, itemInfo2.name.c_str());
	m_game->AddEventList(G_cTxt.c_str(), 10);
	m_game->m_bIsItemEquipped[m_game->m_sItemEquipmentStatus[cEquipPos]] = false;
	m_game->m_sItemEquipmentStatus[cEquipPos] = -1;
}

void InventoryManager::EquipItem(int cItemID)
{
	if (cItemID < 0 || cItemID >= hb::shared::limits::MaxItems) return;
	std::string G_cTxt;
	if (CheckItemOperationEnabled(cItemID) == false) return;
	if (m_game->m_bIsItemEquipped[cItemID] == true) return;
	CItem* pCfg = m_game->GetItemConfig(m_game->m_pItemList[cItemID]->m_sIDnum);
	if (!pCfg) return;
	if (pCfg->GetEquipPos() == EquipPos::None)
	{
		m_game->AddEventList(BITEMDROP_CHARACTER3, 10);
		return;
	}
	if (m_game->m_pItemList[cItemID]->m_wCurLifeSpan == 0)
	{
		m_game->AddEventList(BITEMDROP_CHARACTER1, 10);
		return;
	}
	if (pCfg->m_wWeight / 100 > m_game->m_pPlayer->m_iStr + m_game->m_pPlayer->m_iAngelicStr)
	{
		m_game->AddEventList(BITEMDROP_CHARACTER2, 10);
		return;
	}
	if (((m_game->m_pItemList[cItemID]->m_dwAttribute & 0x00000001) == 0) && (pCfg->m_sLevelLimit > m_game->m_pPlayer->m_iLevel))
	{
		m_game->AddEventList(BITEMDROP_CHARACTER4, 10);
		return;
	}
	if (m_game->m_bSkillUsingStatus == true)
	{
		m_game->AddEventList(BITEMDROP_CHARACTER5, 10);
		return;
	}
	if (pCfg->m_cGenderLimit != 0)
	{
		switch (m_game->m_pPlayer->m_sPlayerType) {
		case 1:
		case 2:
		case 3:
			if (pCfg->m_cGenderLimit != 1)
			{
				m_game->AddEventList(BITEMDROP_CHARACTER6, 10);
				return;
			}
			break;
		case 4:
		case 5:
		case 6:
			if (pCfg->m_cGenderLimit != 2)
			{
				m_game->AddEventList(BITEMDROP_CHARACTER7, 10);
				return;
			}
			break;
		}
	}

	m_game->bSendCommand(MsgId::CommandCommon, CommonType::EquipItem, 0, cItemID, 0, 0, 0);
	m_game->m_sRecentShortCut = cItemID;
	UnequipSlot(pCfg->m_cEquipPos);
	switch (pCfg->GetEquipPos()) {
	case EquipPos::Head:
	case EquipPos::Body:
	case EquipPos::Arms:
	case EquipPos::Pants:
	case EquipPos::Leggings:
	case EquipPos::Back:
		UnequipSlot(ToInt(EquipPos::FullBody));
		break;
	case EquipPos::FullBody:
		UnequipSlot(ToInt(EquipPos::Head));
		UnequipSlot(ToInt(EquipPos::Body));
		UnequipSlot(ToInt(EquipPos::Arms));
		UnequipSlot(ToInt(EquipPos::Pants));
		UnequipSlot(ToInt(EquipPos::Leggings));
		UnequipSlot(ToInt(EquipPos::Back));
		break;
	case EquipPos::LeftHand:
	case EquipPos::RightHand:
		UnequipSlot(ToInt(EquipPos::TwoHand));
		break;
	case EquipPos::TwoHand:
		UnequipSlot(ToInt(EquipPos::RightHand));
		UnequipSlot(ToInt(EquipPos::LeftHand));
		break;
	}

	m_game->m_sItemEquipmentStatus[pCfg->m_cEquipPos] = cItemID;
	m_game->m_bIsItemEquipped[cItemID] = true;

	// Add Angelic Stats
	if ((pCfg->GetItemType() == ItemType::Equip)
		&& (pCfg->m_cEquipPos >= 11))
	{
		int iAngelValue = (m_game->m_pItemList[cItemID]->m_dwAttribute & 0xF0000000) >> 28;
		if (m_game->m_pItemList[cItemID]->m_sIDnum == hb::shared::item::ItemId::AngelicPandentSTR)
			m_game->m_pPlayer->m_iAngelicStr = 1 + iAngelValue;
		else if (m_game->m_pItemList[cItemID]->m_sIDnum == hb::shared::item::ItemId::AngelicPandentDEX)
			m_game->m_pPlayer->m_iAngelicDex = 1 + iAngelValue;
		else if (m_game->m_pItemList[cItemID]->m_sIDnum == hb::shared::item::ItemId::AngelicPandentINT)
			m_game->m_pPlayer->m_iAngelicInt = 1 + iAngelValue;
		else if (m_game->m_pItemList[cItemID]->m_sIDnum == hb::shared::item::ItemId::AngelicPandentMAG)
			m_game->m_pPlayer->m_iAngelicMag = 1 + iAngelValue;
	}

	auto itemInfo3 = ItemNameFormatter::Get().Format(m_game->m_pItemList[cItemID].get());
	G_cTxt = std::format(BITEMDROP_CHARACTER9, itemInfo3.name.c_str());
	m_game->AddEventList(G_cTxt.c_str(), 10);
	m_game->PlayGameSound('E', 28, 0);
}
