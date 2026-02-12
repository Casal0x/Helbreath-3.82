#include "ItemNameFormatter.h"
#include "Item/Item.h"
#include "lan_eng.h"
#include "GameConstants.h"
#include "OwnerType.h"

#include <format>
#include <string>

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

ItemNameInfo ItemNameFormatter::Format(CItem* pItem)
{
	ItemNameInfo result;
	std::string cTxt;
	uint32_t dwType1, dwType2, dwValue1, dwValue2, dwValue3;

	CItem* pCfg = GetConfig(pItem->m_sIDnum);
	if (!pCfg) {
		result.name = "Unknown Item";
		return result;
	}

	const char* cName = pCfg->GetDisplayName();

	if (hb::shared::item::IsSpecialItem(pItem->m_sIDnum)) result.is_special = true;

	if ((pItem->m_dwAttribute & 0x00000001) != 0)
	{
		result.is_special = true;
		result.name = cName;
		if (pCfg->GetItemType() == ItemType::Material)
			result.effect = std::format(GET_ITEM_NAME1, pItem->m_sItemSpecEffectValue2);
		else
		{
			if (pCfg->GetEquipPos() == EquipPos::LeftFinger)
			{
				result.effect = std::format(GET_ITEM_NAME2, pItem->m_sItemSpecEffectValue2);
			}
			else
			{
				result.effect = std::format(GET_ITEM_NAME2, pItem->m_sItemSpecEffectValue2 + 100);
			}
		}
	}
	else
	{
		if (pItem->m_dwCount == 1)
			result.name = cName;
		else result.name = std::format(DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM1, pItem->m_dwCount, cName);
	}

	if ((pItem->m_dwAttribute & 0x00F0F000) != 0)
	{
		result.is_special = true;
		dwType1 = (pItem->m_dwAttribute & 0x00F00000) >> 20;
		dwValue1 = (pItem->m_dwAttribute & 0x000F0000) >> 16;
		dwType2 = (pItem->m_dwAttribute & 0x0000F000) >> 12;
		dwValue2 = (pItem->m_dwAttribute & 0x00000F00) >> 8;
		if (dwType1 != 0)
		{
			switch (dwType1) {
			case 1: cTxt = GET_ITEM_NAME3;   break;
			case 2: cTxt = GET_ITEM_NAME4;   break;
			case 3: cTxt = GET_ITEM_NAME5;   break;
			case 4: break;
			case 5: cTxt = GET_ITEM_NAME6;   break;
			case 6: cTxt = GET_ITEM_NAME7;   break;
			case 7: cTxt = GET_ITEM_NAME8;   break;
			case 8: cTxt = GET_ITEM_NAME9;   break;
			case 9: cTxt = GET_ITEM_NAME10;  break;
			case hb::shared::owner::Slime: cTxt = GET_ITEM_NAME11; break;
			case hb::shared::owner::Skeleton: cTxt = GET_ITEM_NAME12; break;
			case hb::shared::owner::StoneGolem: cTxt = GET_ITEM_NAME13; break;
			}
			cTxt += result.name;
			result.name = cTxt;

			switch (dwType1) {
			case 1: cTxt = std::format(GET_ITEM_NAME14, dwValue1); break;
			case 2: cTxt = std::format(GET_ITEM_NAME15, dwValue1 * 5); break;
			case 3: break;
			case 4: break;
			case 5: cTxt = GET_ITEM_NAME16; break;
			case 6: cTxt = std::format(GET_ITEM_NAME17, dwValue1 * 4); break;
			case 7: cTxt = GET_ITEM_NAME18; break;
			case 8: cTxt = std::format(GET_ITEM_NAME19, dwValue1 * 7); break;
			case 9: cTxt = GET_ITEM_NAME20; break;
			case hb::shared::owner::Slime: cTxt = std::format(GET_ITEM_NAME21, dwValue1 * 3); break;
			case hb::shared::owner::Skeleton: cTxt = std::format(GET_ITEM_NAME22, dwValue1); break;
			case hb::shared::owner::StoneGolem: cTxt = std::format(GET_ITEM_NAME23, dwValue1); break;
			}
			result.effect += cTxt;

			if (dwType2 != 0) {
				switch (dwType2) {
				case 1:  cTxt = std::format(GET_ITEM_NAME24, dwValue2 * 7); break;
				case 2:  cTxt = std::format(GET_ITEM_NAME25, dwValue2 * 7); break;
				case 3:  cTxt = std::format(GET_ITEM_NAME26, dwValue2 * 7); break;
				case 4:  cTxt = std::format(GET_ITEM_NAME27, dwValue2 * 7); break;
				case 5:  cTxt = std::format(GET_ITEM_NAME28, dwValue2 * 7); break;
				case 6:  cTxt = std::format(GET_ITEM_NAME29, dwValue2 * 7); break;
				case 7:  cTxt = std::format(GET_ITEM_NAME30, dwValue2 * 7); break;
				case 8:  cTxt = std::format(GET_ITEM_NAME31, dwValue2 * 3); break;
				case 9:  cTxt = std::format(GET_ITEM_NAME32, dwValue2 * 3); break;
				case hb::shared::owner::Slime: cTxt = std::format(GET_ITEM_NAME33, dwValue2);   break;
				case hb::shared::owner::Skeleton: cTxt = std::format(GET_ITEM_NAME34, dwValue2 * 10); break;
				case hb::shared::owner::StoneGolem: cTxt = std::format(GET_ITEM_NAME35, dwValue2 * 10); break;
				}
				result.extra = cTxt;
			}
		}
	}

	dwValue3 = (pItem->m_dwAttribute & 0xF0000000) >> 28;
	if (dwValue3 > 0)
	{
		if (result.name.size() >= 2 && result.name[result.name.size() - 2] == '+')
		{
			dwValue3 = std::stoi(result.name.substr(result.name.size() - 1)) + dwValue3;
			result.name = std::format("{}+{}", result.name.substr(0, result.name.size() - 2), dwValue3);
		}
		else
		{
			result.name += std::format("+{}", dwValue3);
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
		result.is_special = true;
		cTxt = std::format("Mana Save +{}%", iManaSaveValue);
		if (result.effect.empty())
			result.effect = cTxt;
		else if (result.extra.empty())
			result.extra = cTxt;
	}

	return result;
}

ItemNameInfo ItemNameFormatter::Format(short sItemId, uint32_t dwAttribute)
{
	ItemNameInfo result;
	std::string cTxt;
	uint32_t dwType1, dwType2, dwValue1, dwValue2, dwValue3;

	// Look up item config by ID to get display name
	const char* cName = nullptr;
	CItem* pCfg = GetConfig(sItemId);
	if (pCfg != nullptr) {
		cName = pCfg->m_cName;
	}
	if (cName == nullptr || cName[0] == '\0') {
		result.name = "Unknown Item";
		return result;
	}
	result.name = cName;

	if ((dwAttribute & 0x00F0F000) != 0)
	{
		result.is_special = true;
		dwType1 = (dwAttribute & 0x00F00000) >> 20;
		dwValue1 = (dwAttribute & 0x000F0000) >> 16;
		dwType2 = (dwAttribute & 0x0000F000) >> 12;
		dwValue2 = (dwAttribute & 0x00000F00) >> 8;
		if (dwType1 != 0)
		{
			switch (dwType1) {
			case 1: cTxt = GET_ITEM_NAME3; break;
			case 2: cTxt = GET_ITEM_NAME4; break;
			case 3: cTxt = GET_ITEM_NAME5; break;
			case 4: break;
			case 5: cTxt = GET_ITEM_NAME6; break;
			case 6: cTxt = GET_ITEM_NAME7; break;
			case 7: cTxt = GET_ITEM_NAME8; break;
			case 8: cTxt = GET_ITEM_NAME9; break;
			case 9: cTxt = GET_ITEM_NAME10; break;
			case hb::shared::owner::Slime: cTxt = GET_ITEM_NAME11; break;
			case hb::shared::owner::Skeleton: cTxt = GET_ITEM_NAME12; break;
			case hb::shared::owner::StoneGolem: cTxt = GET_ITEM_NAME13; break;
			}
			cTxt += result.name;
			result.name = cTxt;

			switch (dwType1) {
			case 1: cTxt = std::format(GET_ITEM_NAME14, dwValue1); break;
			case 2: cTxt = std::format(GET_ITEM_NAME15, dwValue1 * 5); break;
			case 3: break;
			case 4: break;
			case 5: cTxt = GET_ITEM_NAME16; break;
			case 6: cTxt = std::format(GET_ITEM_NAME17, dwValue1 * 4); break;
			case 7: cTxt = GET_ITEM_NAME18; break;
			case 8: cTxt = std::format(GET_ITEM_NAME19, dwValue1 * 7); break;
			case 9: cTxt = GET_ITEM_NAME20; break;
			case hb::shared::owner::Slime: cTxt = std::format(GET_ITEM_NAME21, dwValue1 * 3); break;
			case hb::shared::owner::Skeleton: cTxt = std::format(GET_ITEM_NAME22, dwValue1); break;
			case hb::shared::owner::StoneGolem: cTxt = std::format(GET_ITEM_NAME23, dwValue1); break;
			}
			result.effect += cTxt;

			if (dwType2 != 0)
			{
				switch (dwType2) {
				case 1:  cTxt = std::format(GET_ITEM_NAME24, dwValue2 * 7);  break;
				case 2:  cTxt = std::format(GET_ITEM_NAME25, dwValue2 * 7);  break;
				case 3:  cTxt = std::format(GET_ITEM_NAME26, dwValue2 * 7);  break;
				case 4:  cTxt = std::format(GET_ITEM_NAME27, dwValue2 * 7);  break;
				case 5:  cTxt = std::format(GET_ITEM_NAME28, dwValue2 * 7);  break;
				case 6:  cTxt = std::format(GET_ITEM_NAME29, dwValue2 * 7);  break;
				case 7:  cTxt = std::format(GET_ITEM_NAME30, dwValue2 * 7);  break;
				case 8:  cTxt = std::format(GET_ITEM_NAME31, dwValue2 * 3);  break;
				case 9:  cTxt = std::format(GET_ITEM_NAME32, dwValue2 * 3);  break;
				case hb::shared::owner::Slime: cTxt = std::format(GET_ITEM_NAME33, dwValue2);    break;
				case hb::shared::owner::Skeleton: cTxt = std::format(GET_ITEM_NAME34, dwValue2 * 10); break;
				case hb::shared::owner::StoneGolem: cTxt = std::format(GET_ITEM_NAME35, dwValue2 * 10); break;
				}
				result.extra = cTxt;
			}
		}
	}

	dwValue3 = (dwAttribute & 0xF0000000) >> 28;
	if (dwValue3 > 0)
	{
		if (result.name.size() >= 2 && result.name[result.name.size() - 2] == '+')
		{
			dwValue3 = std::stoi(result.name.substr(result.name.size() - 1)) + dwValue3;
			result.name = std::format("{}+{}", result.name.substr(0, result.name.size() - 2), dwValue3);
		}
		else
		{
			result.name += std::format("+{}", dwValue3);
		}
	}

	return result;
}
