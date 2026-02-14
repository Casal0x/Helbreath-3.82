#include "ItemNameFormatter.h"
#include "Item/Item.h"
#include "lan_eng.h"
#include "GameConstants.h"
#include "OwnerType.h"

#include <format>
#include <string>

using hb::shared::item::ItemType;
using hb::shared::item::EquipPos;

item_name_formatter& item_name_formatter::get()
{
	static item_name_formatter instance;
	return instance;
}

void item_name_formatter::set_item_configs(const std::array<std::unique_ptr<CItem>, 5000>& configs)
{
	m_item_configs = &configs;
}

CItem* item_name_formatter::get_config(int item_id) const
{
	if (!m_item_configs || item_id <= 0 || item_id >= 5000) return nullptr;
	return (*m_item_configs)[item_id].get();
}

ItemNameInfo item_name_formatter::format(CItem* item)
{
	ItemNameInfo result;
	std::string txt;
	uint32_t type1, type2, value1, value2, value3;

	CItem* cfg = get_config(item->m_id_num);
	if (!cfg) {
		result.name = "Unknown Item";
		return result;
	}

	const char* name = cfg->get_display_name();

	if (hb::shared::item::is_special_item(item->m_id_num)) result.is_special = true;

	if ((item->m_attribute & 0x00000001) != 0)
	{
		result.is_special = true;
		result.name = name;
		if (cfg->get_item_type() == ItemType::Material)
			result.effect = std::format(GET_ITEM_NAME1, item->m_item_special_effect_value2);
		else
		{
			if (cfg->get_equip_pos() == EquipPos::LeftFinger)
			{
				result.effect = std::format(GET_ITEM_NAME2, item->m_item_special_effect_value2);
			}
			else
			{
				result.effect = std::format(GET_ITEM_NAME2, item->m_item_special_effect_value2 + 100);
			}
		}
	}
	else
	{
		if (item->m_count == 1)
			result.name = name;
		else result.name = std::format(DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM1, item->m_count, name);
	}

	if ((item->m_attribute & 0x00F0F000) != 0)
	{
		result.is_special = true;
		type1 = (item->m_attribute & 0x00F00000) >> 20;
		value1 = (item->m_attribute & 0x000F0000) >> 16;
		type2 = (item->m_attribute & 0x0000F000) >> 12;
		value2 = (item->m_attribute & 0x00000F00) >> 8;
		if (type1 != 0)
		{
			switch (type1) {
			case 1: txt = GET_ITEM_NAME3;   break;
			case 2: txt = GET_ITEM_NAME4;   break;
			case 3: txt = GET_ITEM_NAME5;   break;
			case 4: break;
			case 5: txt = GET_ITEM_NAME6;   break;
			case 6: txt = GET_ITEM_NAME7;   break;
			case 7: txt = GET_ITEM_NAME8;   break;
			case 8: txt = GET_ITEM_NAME9;   break;
			case 9: txt = GET_ITEM_NAME10;  break;
			case hb::shared::owner::Slime: txt = GET_ITEM_NAME11; break;
			case hb::shared::owner::Skeleton: txt = GET_ITEM_NAME12; break;
			case hb::shared::owner::StoneGolem: txt = GET_ITEM_NAME13; break;
			}
			txt += result.name;
			result.name = txt;

			switch (type1) {
			case 1: txt = std::format(GET_ITEM_NAME14, value1); break;
			case 2: txt = std::format(GET_ITEM_NAME15, value1 * 5); break;
			case 3: break;
			case 4: break;
			case 5: txt = GET_ITEM_NAME16; break;
			case 6: txt = std::format(GET_ITEM_NAME17, value1 * 4); break;
			case 7: txt = GET_ITEM_NAME18; break;
			case 8: txt = std::format(GET_ITEM_NAME19, value1 * 7); break;
			case 9: txt = GET_ITEM_NAME20; break;
			case hb::shared::owner::Slime: txt = std::format(GET_ITEM_NAME21, value1 * 3); break;
			case hb::shared::owner::Skeleton: txt = std::format(GET_ITEM_NAME22, value1); break;
			case hb::shared::owner::StoneGolem: txt = std::format(GET_ITEM_NAME23, value1); break;
			}
			result.effect += txt;

			if (type2 != 0) {
				switch (type2) {
				case 1:  txt = std::format(GET_ITEM_NAME24, value2 * 7); break;
				case 2:  txt = std::format(GET_ITEM_NAME25, value2 * 7); break;
				case 3:  txt = std::format(GET_ITEM_NAME26, value2 * 7); break;
				case 4:  txt = std::format(GET_ITEM_NAME27, value2 * 7); break;
				case 5:  txt = std::format(GET_ITEM_NAME28, value2 * 7); break;
				case 6:  txt = std::format(GET_ITEM_NAME29, value2 * 7); break;
				case 7:  txt = std::format(GET_ITEM_NAME30, value2 * 7); break;
				case 8:  txt = std::format(GET_ITEM_NAME31, value2 * 3); break;
				case 9:  txt = std::format(GET_ITEM_NAME32, value2 * 3); break;
				case hb::shared::owner::Slime: txt = std::format(GET_ITEM_NAME33, value2);   break;
				case hb::shared::owner::Skeleton: txt = std::format(GET_ITEM_NAME34, value2 * 10); break;
				case hb::shared::owner::StoneGolem: txt = std::format(GET_ITEM_NAME35, value2 * 10); break;
				}
				result.extra = txt;
			}
		}
	}

	value3 = (item->m_attribute & 0xF0000000) >> 28;
	if (value3 > 0)
	{
		auto plusPos = result.name.rfind('+');
		if (plusPos != std::string::npos && plusPos + 1 < result.name.size())
		{
			try {
				int existingPlus = std::stoi(result.name.substr(plusPos + 1));
				value3 += existingPlus;
				result.name = std::format("{}+{}", result.name.substr(0, plusPos), value3);
			} catch (...) {
				result.name += std::format("+{}", value3);
			}
		}
		else
		{
			result.name += std::format("+{}", value3);
		}
	}

	// Display mana save effect if present
	auto effectType = cfg->get_item_effect_type();
	int mana_save_value = 0;
	if (effectType == hb::shared::item::ItemEffectType::AttackManaSave)
	{
		mana_save_value = cfg->m_item_effect_value4;
	}
	else if (effectType == hb::shared::item::ItemEffectType::add_effect &&
	         cfg->m_item_effect_value1 == hb::shared::item::to_int(hb::shared::item::AddEffectType::ManaSave))
	{
		mana_save_value = cfg->m_item_effect_value2;
	}

	if (mana_save_value > 0)
	{
		result.is_special = true;
		txt = std::format("Mana save +{}%", mana_save_value);
		if (result.effect.empty())
			result.effect = txt;
		else if (result.extra.empty())
			result.extra = txt;
	}

	return result;
}

ItemNameInfo item_name_formatter::format(short item_id, uint32_t attribute)
{
	ItemNameInfo result;
	std::string txt;
	uint32_t type1, type2, value1, value2, value3;

	// Look up item config by ID to get display name
	const char* name = nullptr;
	CItem* cfg = get_config(item_id);
	if (cfg != nullptr) {
		name = cfg->m_name;
	}
	if (name == nullptr || name[0] == '\0') {
		result.name = "Unknown Item";
		return result;
	}
	result.name = name;

	if ((attribute & 0x00F0F000) != 0)
	{
		result.is_special = true;
		type1 = (attribute & 0x00F00000) >> 20;
		value1 = (attribute & 0x000F0000) >> 16;
		type2 = (attribute & 0x0000F000) >> 12;
		value2 = (attribute & 0x00000F00) >> 8;
		if (type1 != 0)
		{
			switch (type1) {
			case 1: txt = GET_ITEM_NAME3; break;
			case 2: txt = GET_ITEM_NAME4; break;
			case 3: txt = GET_ITEM_NAME5; break;
			case 4: break;
			case 5: txt = GET_ITEM_NAME6; break;
			case 6: txt = GET_ITEM_NAME7; break;
			case 7: txt = GET_ITEM_NAME8; break;
			case 8: txt = GET_ITEM_NAME9; break;
			case 9: txt = GET_ITEM_NAME10; break;
			case hb::shared::owner::Slime: txt = GET_ITEM_NAME11; break;
			case hb::shared::owner::Skeleton: txt = GET_ITEM_NAME12; break;
			case hb::shared::owner::StoneGolem: txt = GET_ITEM_NAME13; break;
			}
			txt += result.name;
			result.name = txt;

			switch (type1) {
			case 1: txt = std::format(GET_ITEM_NAME14, value1); break;
			case 2: txt = std::format(GET_ITEM_NAME15, value1 * 5); break;
			case 3: break;
			case 4: break;
			case 5: txt = GET_ITEM_NAME16; break;
			case 6: txt = std::format(GET_ITEM_NAME17, value1 * 4); break;
			case 7: txt = GET_ITEM_NAME18; break;
			case 8: txt = std::format(GET_ITEM_NAME19, value1 * 7); break;
			case 9: txt = GET_ITEM_NAME20; break;
			case hb::shared::owner::Slime: txt = std::format(GET_ITEM_NAME21, value1 * 3); break;
			case hb::shared::owner::Skeleton: txt = std::format(GET_ITEM_NAME22, value1); break;
			case hb::shared::owner::StoneGolem: txt = std::format(GET_ITEM_NAME23, value1); break;
			}
			result.effect += txt;

			if (type2 != 0)
			{
				switch (type2) {
				case 1:  txt = std::format(GET_ITEM_NAME24, value2 * 7);  break;
				case 2:  txt = std::format(GET_ITEM_NAME25, value2 * 7);  break;
				case 3:  txt = std::format(GET_ITEM_NAME26, value2 * 7);  break;
				case 4:  txt = std::format(GET_ITEM_NAME27, value2 * 7);  break;
				case 5:  txt = std::format(GET_ITEM_NAME28, value2 * 7);  break;
				case 6:  txt = std::format(GET_ITEM_NAME29, value2 * 7);  break;
				case 7:  txt = std::format(GET_ITEM_NAME30, value2 * 7);  break;
				case 8:  txt = std::format(GET_ITEM_NAME31, value2 * 3);  break;
				case 9:  txt = std::format(GET_ITEM_NAME32, value2 * 3);  break;
				case hb::shared::owner::Slime: txt = std::format(GET_ITEM_NAME33, value2);    break;
				case hb::shared::owner::Skeleton: txt = std::format(GET_ITEM_NAME34, value2 * 10); break;
				case hb::shared::owner::StoneGolem: txt = std::format(GET_ITEM_NAME35, value2 * 10); break;
				}
				result.extra = txt;
			}
		}
	}

	value3 = (attribute & 0xF0000000) >> 28;
	if (value3 > 0)
	{
		auto plusPos = result.name.rfind('+');
		if (plusPos != std::string::npos && plusPos + 1 < result.name.size())
		{
			try {
				int existingPlus = std::stoi(result.name.substr(plusPos + 1));
				value3 += existingPlus;
				result.name = std::format("{}+{}", result.name.substr(0, plusPos), value3);
			} catch (...) {
				result.name += std::format("+{}", value3);
			}
		}
		else
		{
			result.name += std::format("+{}", value3);
		}
	}

	return result;
}
