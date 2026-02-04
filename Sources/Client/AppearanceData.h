#pragma once
#include <cstdint>

// Unpacked player appearance - extracted from sAppr1-4 bit fields.
// Network packets still use the packed format; this struct is populated
// client-side immediately after reception so rendering code can use
// named fields instead of inline bit masking.
struct PlayerAppearance
{
	// From sAppr1
	int iUnderwearType;    // sAppr1 & 0x000F
	int iHairColor;        // (sAppr1 & 0x00F0) >> 4
	int iHairStyle;        // (sAppr1 & 0x0F00) >> 8
	int iSkinColor;        // (sAppr1 & 0xF000) >> 12

	// From sAppr2
	int iShieldType;       // sAppr2 & 0x000F
	int iWeaponType;       // (sAppr2 & 0x0FF0) >> 4
	int iIsWalking;        // (sAppr2 & 0xF000) >> 12

	// From sAppr3
	int iArmArmorType;     // sAppr3 & 0x000F
	int iHelmType;         // (sAppr3 & 0x00F0) >> 4
	int iPantsType;        // (sAppr3 & 0x0F00) >> 8
	int iArmorType;        // (sAppr3 & 0xF000) >> 12

	// From sAppr4
	int iWeaponGlare;      // sAppr4 & 0x0003
	int iShieldGlare;      // (sAppr4 & 0x000C) >> 2
	int iEffectType;       // (sAppr4 & 0x00F0) >> 4
	int iHideArmor;        // (sAppr4 & 0x0080) >> 7  (bool-like)
	int iMantleType;       // (sAppr4 & 0x0F00) >> 8
	int iBootsType;        // (sAppr4 & 0xF000) >> 12

	// Colors (from iApprColor)
	int iApprColor;        // raw iApprColor
	int iWeaponColor;      // (iApprColor & 0xF0000000) >> 28
	int iShieldColor;      // (iApprColor & 0x0F000000) >> 24
	int iArmorColor;       // (iApprColor & 0x00F00000) >> 20
	int iMantleColor;      // (iApprColor & 0x000F0000) >> 16
	int iArmColor;         // (iApprColor & 0x0000F000) >> 12
	int iPantsColor;       // (iApprColor & 0x00000F00) >> 8
	int iBootsColor;       // (iApprColor & 0x000000F0) >> 4
	int iHelmColor;        // (iApprColor & 0x0000000F)

	// Raw packed sAppr2 value (needed for NPC rendering which uses sAppr2 differently)
	short sRawAppr2;

	void Unpack(short sAppr1, short sAppr2, short sAppr3, short sAppr4, int iApprColor);
	void Clear();
};
