#pragma once

#include "Packet/PacketCommon.h"
#include "Item/ItemEnums.h"
#include <cstdint>
#include <cstring>

HB_PACK_BEGIN

// EntityAppearance: NPC appearance using what was sAppr2.
// NPCs don't have equipment; sAppr2 encodes sub-type and special frame.
struct HB_PACKED EntityAppearance
{
	uint8_t iSubType;        // (sAppr2 & 0xFF00) >> 8  (crop type, crusade ownership)
	uint8_t iSpecialFrame;   // sAppr2 & 0x00FF          (NPC special animation frame)

	bool HasSpecialState() const { return iSubType != 0 || iSpecialFrame != 0; }
	void Clear() { iSubType = 0; iSpecialFrame = 0; }
};

// PlayerAppearance: Individual fields that replace sAppr1-4 + iApprColor.
// Used in packet structs (must be packed) and in CClient/CTile storage.
// All fields are uint8_t to match the 4-bit nibble ranges of the original packed format.
struct HB_PACKED PlayerAppearance
{
	// Body (was sAppr1)
	uint8_t iUnderwearType;  // sAppr1 & 0x000F
	uint8_t iHairColor;      // (sAppr1 & 0x00F0) >> 4
	uint8_t iHairStyle;      // (sAppr1 & 0x0F00) >> 8
	uint8_t iSkinColor;      // (sAppr1 & 0xF000) >> 12

	// Weapons (was sAppr2)
	uint8_t iShieldType;     // sAppr2 & 0x000F
	uint8_t iWeaponType;     // (sAppr2 & 0x0FF0) >> 4  (8-bit range)
	bool bIsWalking;         // (sAppr2 & 0xF000) >> 12  (combat mode)

	// Armor (was sAppr3)
	uint8_t iArmArmorType;   // sAppr3 & 0x000F
	uint8_t iHelmType;       // (sAppr3 & 0x00F0) >> 4
	uint8_t iPantsType;      // (sAppr3 & 0x0F00) >> 8
	uint8_t iArmorType;      // (sAppr3 & 0xF000) >> 12

	// Effects (was sAppr4)
	uint8_t iWeaponGlare;    // sAppr4 & 0x0003
	uint8_t iShieldGlare;    // (sAppr4 & 0x000C) >> 2
	uint8_t iEffectType;     // (sAppr4 & 0x00F0) >> 4
	bool bHideArmor;         // (sAppr4 & 0x0080) >> 7
	uint8_t iMantleType;     // (sAppr4 & 0x0F00) >> 8
	uint8_t iBootsType;      // (sAppr4 & 0xF000) >> 12

	// Colors (was iApprColor)
	uint8_t iWeaponColor;    // (iApprColor >> 28) & 0xF
	uint8_t iShieldColor;    // (iApprColor >> 24) & 0xF
	uint8_t iArmorColor;     // (iApprColor >> 20) & 0xF
	uint8_t iMantleColor;    // (iApprColor >> 16) & 0xF
	uint8_t iArmColor;       // (iApprColor >> 12) & 0xF
	uint8_t iPantsColor;     // (iApprColor >> 8) & 0xF
	uint8_t iBootsColor;     // (iApprColor >> 4) & 0xF
	uint8_t iHelmColor;      // iApprColor & 0xF

	// NPC sub-type/special frame (always 0 for players, populated from EntityAppearance for NPCs)
	uint8_t iSubType;        // (was sAppr2 upper byte for NPCs) crop type, crusade ownership
	uint8_t iSpecialFrame;   // (was sAppr2 lower byte for NPCs) NPC special animation frame

	bool HasNpcSpecialState() const { return iSubType != 0 || iSpecialFrame != 0; }

	void Clear() { std::memset(this, 0, sizeof(*this)); }

	// Populate NPC fields from an EntityAppearance (used client-side after receiving NPC packets)
	void SetFromNpcAppearance(const EntityAppearance& npc)
	{
		Clear();
		iSubType = npc.iSubType;
		iSpecialFrame = npc.iSpecialFrame;
	}
};

HB_PACK_END

// Apply a single equipped item's visual data to an appearance struct.
// Shared between client and server to keep appearance computation in sync.
inline void ApplyEquipAppearance(PlayerAppearance& appearance, hb::item::EquipPos pos, int apprValue, int itemColor)
{
	using namespace hb::item;
	switch (pos) {
	case EquipPos::Head:
		appearance.iHelmType = static_cast<uint8_t>(apprValue);
		appearance.iHelmColor = static_cast<uint8_t>(itemColor);
		break;
	case EquipPos::Body:
		if (apprValue >= 100) {
			appearance.iArmorType = static_cast<uint8_t>(apprValue - 100);
			appearance.bHideArmor = true;
		} else {
			appearance.iArmorType = static_cast<uint8_t>(apprValue);
			appearance.bHideArmor = false;
		}
		appearance.iArmorColor = static_cast<uint8_t>(itemColor);
		break;
	case EquipPos::Arms:
		appearance.iArmArmorType = static_cast<uint8_t>(apprValue);
		appearance.iArmColor = static_cast<uint8_t>(itemColor);
		break;
	case EquipPos::Pants:
		appearance.iPantsType = static_cast<uint8_t>(apprValue);
		appearance.iPantsColor = static_cast<uint8_t>(itemColor);
		break;
	case EquipPos::Leggings:
		appearance.iBootsType = static_cast<uint8_t>(apprValue);
		appearance.iBootsColor = static_cast<uint8_t>(itemColor);
		break;
	case EquipPos::LeftHand:
		appearance.iShieldType = static_cast<uint8_t>(apprValue);
		appearance.iShieldColor = static_cast<uint8_t>(itemColor);
		break;
	case EquipPos::RightHand:
	case EquipPos::TwoHand:
		appearance.iWeaponType = static_cast<uint8_t>(apprValue);
		appearance.iWeaponColor = static_cast<uint8_t>(itemColor);
		break;
	case EquipPos::Back:
		appearance.iMantleType = static_cast<uint8_t>(apprValue);
		appearance.iMantleColor = static_cast<uint8_t>(itemColor);
		break;
	case EquipPos::FullBody:
		appearance.iArmorType = static_cast<uint8_t>(apprValue);
		appearance.iMantleColor = 0;
		break;
	default:
		break;
	}
}
