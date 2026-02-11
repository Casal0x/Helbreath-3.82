#include "ItemNameFormatter.h"
#include "Item/Item.h"
#include "lan_eng.h"
#include "GameConstants.h"
#include "OwnerType.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>

using hb::shared::item::ItemType;
using hb::shared::item::EquipPos;

ItemNameFormatter& ItemNameFormatter::Get()
{
	static ItemNameFormatter instance;
	return instance;
}

void ItemNameFormatter::SetItemConfigs(const std::array<std::unique_ptr<CItem>, 5000>& configs)
{
	m_item_configs = &configs;
}

CItem* ItemNameFormatter::GetConfig(int iItemID) const
{
	if (!m_item_configs || iItemID <= 0 || iItemID >= 5000) return nullptr;
	return (*m_item_configs)[iItemID].get();
}

void ItemNameFormatter::Format(CItem* pItem, char* pStr1, char* pStr2, char* pStr3)
{
	char cTxt[256], cTxt2[256];
	uint32_t dwType1, dwType2, dwValue1, dwValue2, dwValue3;

	m_is_special = false;
	std::memset(pStr1, 0, 64);
	std::memset(pStr2, 0, 64);
	std::memset(pStr3, 0, 64);

	CItem* pCfg = GetConfig(pItem->m_sIDnum);
	if (!pCfg) {
		std::snprintf(pStr1, 64, "%s", "Unknown Item");
		return;
	}

	const char* cName = pCfg->GetDisplayName();

	if (hb::shared::item::IsSpecialItem(pItem->m_sIDnum)) m_is_special = true;

	if ((pItem->m_dwAttribute & 0x00000001) != 0)
	{
		m_is_special = true;
		std::snprintf(pStr1, 64, "%s", cName);
		if (pCfg->GetItemType() == ItemType::Material)
			std::snprintf(pStr2, 64, GET_ITEM_NAME1, pItem->m_sItemSpecEffectValue2);
		else
		{
			if (pCfg->GetEquipPos() == EquipPos::LeftFinger)
			{
				std::snprintf(pStr2, 64, GET_ITEM_NAME2, pItem->m_sItemSpecEffectValue2);
			}
			else
			{
				std::snprintf(pStr2, 64, GET_ITEM_NAME2, pItem->m_sItemSpecEffectValue2 + 100);
			}
		}
	}
	else
	{
		char scratch[128];
		if (pItem->m_dwCount == 1)
			std::snprintf(scratch, sizeof(scratch), "%s", cName);
		else std::snprintf(scratch, sizeof(scratch), DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM1, pItem->m_dwCount, cName);
		std::snprintf(pStr1, 64, "%s", scratch);
	}

	if ((pItem->m_dwAttribute & 0x00F0F000) != 0)
	{
		m_is_special = true;
		dwType1 = (pItem->m_dwAttribute & 0x00F00000) >> 20;
		dwValue1 = (pItem->m_dwAttribute & 0x000F0000) >> 16;
		dwType2 = (pItem->m_dwAttribute & 0x0000F000) >> 12;
		dwValue2 = (pItem->m_dwAttribute & 0x00000F00) >> 8;
		if (dwType1 != 0)
		{
			std::memset(cTxt, 0, sizeof(cTxt));
			switch (dwType1) {
			case 1: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME3);   break;
			case 2: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME4);   break;
			case 3: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME5);   break;
			case 4: break;
			case 5: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME6);   break;
			case 6: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME7);   break;
			case 7: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME8);   break;
			case 8: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME9);   break;
			case 9: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME10);  break;
			case hb::shared::owner::Slime: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME11); break;
			case hb::shared::owner::Skeleton: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME12); break;
			case hb::shared::owner::StoneGolem: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME13); break;
			}
			std::snprintf(cTxt + strlen(cTxt), sizeof(cTxt) - strlen(cTxt), "%s", pStr1);
			std::memset(pStr1, 0, 64);
			std::snprintf(pStr1, 64, "%s", cTxt);

			std::memset(cTxt, 0, sizeof(cTxt));
			switch (dwType1) {
			case 1: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME14, dwValue1); break;
			case 2: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME15, dwValue1 * 5); break;
			case 3: break;
			case 4: break;
			case 5: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME16); break;
			case 6: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME17, dwValue1 * 4); break;
			case 7: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME18); break;
			case 8: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME19, dwValue1 * 7); break;
			case 9: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME20); break;
			case hb::shared::owner::Slime: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME21, dwValue1 * 3); break;
			case hb::shared::owner::Skeleton: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME22, dwValue1); break;
			case hb::shared::owner::StoneGolem: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME23, dwValue1); break;
			}
			std::snprintf(pStr2 + strlen(pStr2), 64 - strlen(pStr2), "%s", cTxt);

			if (dwType2 != 0) {
				std::memset(cTxt, 0, sizeof(cTxt));
				switch (dwType2) {
				case 1:  std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME24, dwValue2 * 7); break;
				case 2:  std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME25, dwValue2 * 7); break;
				case 3:  std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME26, dwValue2 * 7); break;
				case 4:  std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME27, dwValue2 * 7); break;
				case 5:  std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME28, dwValue2 * 7); break;
				case 6:  std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME29, dwValue2 * 7); break;
				case 7:  std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME30, dwValue2 * 7); break;
				case 8:  std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME31, dwValue2 * 3); break;
				case 9:  std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME32, dwValue2 * 3); break;
				case hb::shared::owner::Slime: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME33, dwValue2);   break;
				case hb::shared::owner::Skeleton: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME34, dwValue2 * 10); break;
				case hb::shared::owner::StoneGolem: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME35, dwValue2 * 10); break;
				}
				std::snprintf(pStr3, 64, "%s", cTxt);
			}
		}
	}

	dwValue3 = (pItem->m_dwAttribute & 0xF0000000) >> 28;
	if (dwValue3 > 0)
	{
		if (pStr1[strlen(pStr1) - 2] == '+')
		{
			dwValue3 = atoi((char*)(pStr1 + strlen(pStr1) - 1)) + dwValue3;
			std::memset(cTxt, 0, sizeof(cTxt));
			memcpy(cTxt, pStr1, strlen(pStr1) - 2);
			std::memset(cTxt2, 0, sizeof(cTxt2));
			std::snprintf(cTxt2, sizeof(cTxt2), "%s+%d", cTxt, dwValue3);
			std::memset(pStr1, 0, 64);
			std::snprintf(pStr1, 64, "%s", cTxt2);
		}
		else
		{
			std::memset(cTxt, 0, sizeof(cTxt));
			std::snprintf(cTxt, sizeof(cTxt), "+%d", dwValue3);
			std::snprintf(pStr1 + strlen(pStr1), 64 - strlen(pStr1), "%s", cTxt);
		}
	}

	// Display mana save effect if present
	auto effectType = pCfg->GetItemEffectType();
	int iManaSaveValue = 0;
	if (effectType == hb::shared::item::ItemEffectType::AttackManaSave)
	{
		iManaSaveValue = pCfg->m_sItemEffectValue4;
	}
	else if (effectType == hb::shared::item::ItemEffectType::AddEffect &&
	         pCfg->m_sItemEffectValue1 == hb::shared::item::ToInt(hb::shared::item::AddEffectType::ManaSave))
	{
		iManaSaveValue = pCfg->m_sItemEffectValue2;
	}

	if (iManaSaveValue > 0)
	{
		m_is_special = true;
		std::memset(cTxt, 0, sizeof(cTxt));
		std::snprintf(cTxt, sizeof(cTxt), "Mana Save +%d%%", iManaSaveValue);
		// Add to pStr2 if empty, otherwise pStr3
		if (pStr2[0] == '\0')
			std::snprintf(pStr2, 64, "%s", cTxt);
		else if (pStr3[0] == '\0')
			std::snprintf(pStr3, 64, "%s", cTxt);
	}
}

void ItemNameFormatter::Format(short sItemId, uint32_t dwAttribute, char* pStr1, char* pStr2, char* pStr3)
{
	char cTxt[256], cTxt2[256];
	uint32_t dwType1, dwType2, dwValue1, dwValue2, dwValue3;

	m_is_special = false;
	std::memset(pStr1, 0, 64);
	std::memset(pStr2, 0, 64);
	std::memset(pStr3, 0, 64);

	// Look up item config by ID to get display name
	const char* cName = nullptr;
	CItem* pCfg = GetConfig(sItemId);
	if (pCfg != nullptr) {
		cName = pCfg->m_cName;
	}
	if (cName == nullptr || cName[0] == '\0') {
		std::snprintf(pStr1, 64, "%s", "Unknown Item");
		return;
	}
	std::snprintf(pStr1, 64, "%s", cName);

	if ((dwAttribute & 0x00F0F000) != 0)
	{
		m_is_special = true;
		dwType1 = (dwAttribute & 0x00F00000) >> 20;
		dwValue1 = (dwAttribute & 0x000F0000) >> 16;
		dwType2 = (dwAttribute & 0x0000F000) >> 12;
		dwValue2 = (dwAttribute & 0x00000F00) >> 8;
		if (dwType1 != 0)
		{
			std::memset(cTxt, 0, sizeof(cTxt));
			switch (dwType1) {
			case 1: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME3); break;
			case 2: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME4); break;
			case 3: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME5); break;
			case 4: break;
			case 5: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME6); break;
			case 6: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME7); break;
			case 7: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME8); break;
			case 8: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME9); break;
			case 9: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME10); break;
			case hb::shared::owner::Slime: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME11); break;
			case hb::shared::owner::Skeleton: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME12); break;
			case hb::shared::owner::StoneGolem: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME13); break;
			}
			std::snprintf(cTxt + strlen(cTxt), sizeof(cTxt) - strlen(cTxt), "%s", pStr1);
			std::memset(pStr1, 0, 64);
			std::snprintf(pStr1, 64, "%s", cTxt);

			std::memset(cTxt, 0, sizeof(cTxt));
			switch (dwType1) {
			case 1: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME14, dwValue1); break;
			case 2: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME15, dwValue1 * 5); break;
			case 3: break;
			case 4: break;
			case 5: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME16); break;
			case 6: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME17, dwValue1 * 4); break;
			case 7: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME18); break;
			case 8: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME19, dwValue1 * 7); break;
			case 9: std::snprintf(cTxt, sizeof(cTxt), "%s", GET_ITEM_NAME20); break;
			case hb::shared::owner::Slime: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME21, dwValue1 * 3); break;
			case hb::shared::owner::Skeleton: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME22, dwValue1); break;
			case hb::shared::owner::StoneGolem: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME23, dwValue1); break;
			}
			std::snprintf(pStr2 + strlen(pStr2), 64 - strlen(pStr2), "%s", cTxt);

			if (dwType2 != 0)
			{
				std::memset(cTxt, 0, sizeof(cTxt));
				switch (dwType2) {
				case 1:  std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME24, dwValue2 * 7);  break;
				case 2:  std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME25, dwValue2 * 7);  break;
				case 3:  std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME26, dwValue2 * 7);  break;
				case 4:  std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME27, dwValue2 * 7);  break;
				case 5:  std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME28, dwValue2 * 7);  break;
				case 6:  std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME29, dwValue2 * 7);  break;
				case 7:  std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME30, dwValue2 * 7);  break;
				case 8:  std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME31, dwValue2 * 3);  break;
				case 9:  std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME32, dwValue2 * 3);  break;
				case hb::shared::owner::Slime: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME33, dwValue2);    break;
				case hb::shared::owner::Skeleton: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME34, dwValue2 * 10); break;
				case hb::shared::owner::StoneGolem: std::snprintf(cTxt, sizeof(cTxt), GET_ITEM_NAME35, dwValue2 * 10); break;
				}
				std::snprintf(pStr3, 64, "%s", cTxt);
			}
		}
	}

	dwValue3 = (dwAttribute & 0xF0000000) >> 28;
	if (dwValue3 > 0)
	{
		if (pStr1[strlen(pStr1) - 2] == '+')
		{
			dwValue3 = atoi((char*)(pStr1 + strlen(pStr1) - 1)) + dwValue3;
			std::memset(cTxt, 0, sizeof(cTxt));
			memcpy(cTxt, pStr1, strlen(pStr1) - 2);
			std::memset(cTxt2, 0, sizeof(cTxt2));
			std::snprintf(cTxt2, sizeof(cTxt2), "%s+%d", cTxt, dwValue3);
			std::memset(pStr1, 0, 64);
			std::snprintf(pStr1, 64, "%s", cTxt2);
		}
		else
		{
			std::memset(cTxt, 0, sizeof(cTxt));
			std::snprintf(cTxt, sizeof(cTxt), "+%d", dwValue3);
			std::snprintf(pStr1 + strlen(pStr1), 64 - strlen(pStr1), "%s", cTxt);
		}
	}
}
