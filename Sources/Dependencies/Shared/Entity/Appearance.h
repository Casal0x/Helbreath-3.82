#pragma once

#include "Packet/PacketCommon.h"
#include "Item/ItemEnums.h"
#include <cstdint>
#include <cstring>

namespace hb::shared::entity {

HB_PACK_BEGIN

// EntityAppearance: NPC appearance using what was appr2.
// NPCs don't have equipment; appr2 encodes sub-type and special frame.
struct HB_PACKED EntityAppearance
{
	uint8_t sub_type;        // (appr2 & 0xFF00) >> 8  (crop type, crusade ownership)
	uint8_t special_frame;   // appr2 & 0x00FF          (NPC special animation frame)

	bool HasSpecialState() const { return sub_type != 0 || special_frame != 0; }
	void clear() { sub_type = 0; special_frame = 0; }
};

// PlayerAppearance: Individual fields that replace sAppr1-4 + iApprColor.
// Used in packet structs (must be packed) and in CClient/CTile storage.
// All fields are uint8_t to match the 4-bit nibble ranges of the original packed format.
struct HB_PACKED PlayerAppearance
{
	// Body (was sAppr1)
	uint8_t underwear_type;  // sAppr1 & 0x000F
	uint8_t hair_color;      // (sAppr1 & 0x00F0) >> 4
	uint8_t hair_style;      // (sAppr1 & 0x0F00) >> 8
	uint8_t skin_color;      // (sAppr1 & 0xF000) >> 12

	// Weapons (was appr2)
	uint8_t shield_type;     // appr2 & 0x000F
	uint8_t weapon_type;     // (appr2 & 0x0FF0) >> 4  (8-bit range)
	bool is_walking;         // (appr2 & 0xF000) >> 12  (combat mode)

	// Armor (was sAppr3)
	uint8_t arm_armor_type;   // sAppr3 & 0x000F
	uint8_t helm_type;       // (sAppr3 & 0x00F0) >> 4
	uint8_t pants_type;      // (sAppr3 & 0x0F00) >> 8
	uint8_t armor_type;      // (sAppr3 & 0xF000) >> 12

	// Effects (was sAppr4)
	uint8_t weapon_glare;    // sAppr4 & 0x0003
	uint8_t shield_glare;    // (sAppr4 & 0x000C) >> 2
	uint8_t effect_type;     // (sAppr4 & 0x00F0) >> 4
	bool hide_armor;         // (sAppr4 & 0x0080) >> 7
	uint8_t mantle_type;     // (sAppr4 & 0x0F00) >> 8
	uint8_t boots_type;      // (sAppr4 & 0xF000) >> 12

	// Colors (was iApprColor)
	uint8_t weapon_color;    // (iApprColor >> 28) & 0xF
	uint8_t shield_color;    // (iApprColor >> 24) & 0xF
	uint8_t armor_color;     // (iApprColor >> 20) & 0xF
	uint8_t mantle_color;    // (iApprColor >> 16) & 0xF
	uint8_t arm_color;       // (iApprColor >> 12) & 0xF
	uint8_t pants_color;     // (iApprColor >> 8) & 0xF
	uint8_t boots_color;     // (iApprColor >> 4) & 0xF
	uint8_t helm_color;      // iApprColor & 0xF

	// NPC sub-type/special frame (always 0 for players, populated from EntityAppearance for NPCs)
	uint8_t sub_type;        // (was appr2 upper byte for NPCs) crop type, crusade ownership
	uint8_t special_frame;   // (was appr2 lower byte for NPCs) NPC special animation frame

	bool HasNpcSpecialState() const { return sub_type != 0 || special_frame != 0; }

	void clear() { std::memset(this, 0, sizeof(*this)); }

	// Populate NPC fields from an EntityAppearance (used client-side after receiving NPC packets)
	void SetFromNpcAppearance(const EntityAppearance& npc)
	{
		clear();
		sub_type = npc.sub_type;
		special_frame = npc.special_frame;
	}
};

HB_PACK_END

// Apply a single equipped item's visual data to an appearance struct.
// Shared between client and server to keep appearance computation in sync.
inline void ApplyEquipAppearance(PlayerAppearance& appearance, hb::shared::item::EquipPos pos, int apprValue, int itemColor)
{
	using namespace hb::shared::item;
	switch (pos) {
	case EquipPos::Head:
		appearance.helm_type = static_cast<uint8_t>(apprValue);
		appearance.helm_color = static_cast<uint8_t>(itemColor);
		break;
	case EquipPos::Body:
		if (apprValue >= 100) {
			appearance.armor_type = static_cast<uint8_t>(apprValue - 100);
			appearance.hide_armor = true;
		} else {
			appearance.armor_type = static_cast<uint8_t>(apprValue);
			appearance.hide_armor = false;
		}
		appearance.armor_color = static_cast<uint8_t>(itemColor);
		break;
	case EquipPos::Arms:
		appearance.arm_armor_type = static_cast<uint8_t>(apprValue);
		appearance.arm_color = static_cast<uint8_t>(itemColor);
		break;
	case EquipPos::Pants:
		appearance.pants_type = static_cast<uint8_t>(apprValue);
		appearance.pants_color = static_cast<uint8_t>(itemColor);
		break;
	case EquipPos::Leggings:
		appearance.boots_type = static_cast<uint8_t>(apprValue);
		appearance.boots_color = static_cast<uint8_t>(itemColor);
		break;
	case EquipPos::LeftHand:
		appearance.shield_type = static_cast<uint8_t>(apprValue);
		appearance.shield_color = static_cast<uint8_t>(itemColor);
		break;
	case EquipPos::RightHand:
	case EquipPos::TwoHand:
		appearance.weapon_type = static_cast<uint8_t>(apprValue);
		appearance.weapon_color = static_cast<uint8_t>(itemColor);
		break;
	case EquipPos::Back:
		appearance.mantle_type = static_cast<uint8_t>(apprValue);
		appearance.mantle_color = static_cast<uint8_t>(itemColor);
		break;
	case EquipPos::FullBody:
		appearance.armor_type = static_cast<uint8_t>(apprValue);
		appearance.mantle_color = 0;
		break;
	default:
		break;
	}
}

} // namespace hb::shared::entity
