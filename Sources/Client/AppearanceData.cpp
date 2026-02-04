#include "AppearanceData.h"
#include <cstring>

void PlayerAppearance::Unpack(short sAppr1, short sAppr2, short sAppr3, short sAppr4, int apprColor)
{
	// sAppr1: underwear, hair color, hair style, skin color
	iUnderwearType = (sAppr1 & 0x000F);
	iHairColor     = (sAppr1 & 0x00F0) >> 4;
	iHairStyle     = (sAppr1 & 0x0F00) >> 8;
	iSkinColor     = (sAppr1 & 0xF000) >> 12;

	// sAppr2: shield, weapon, walking mode
	sRawAppr2      = sAppr2;
	iShieldType    = (sAppr2 & 0x000F);
	iWeaponType    = (sAppr2 & 0x0FF0) >> 4;
	iIsWalking     = (sAppr2 & 0xF000) >> 12;

	// sAppr3: arm armor, helm, pants, body armor
	iArmArmorType  = (sAppr3 & 0x000F);
	iHelmType      = (sAppr3 & 0x00F0) >> 4;
	iPantsType     = (sAppr3 & 0x0F00) >> 8;
	iArmorType     = (sAppr3 & 0xF000) >> 12;

	// sAppr4: weapon glare, shield glare, effect, hide armor, mantle, boots
	iWeaponGlare   = (sAppr4 & 0x0003);
	iShieldGlare   = (sAppr4 & 0x000C) >> 2;
	iEffectType    = (sAppr4 & 0x00F0) >> 4;
	iHideArmor     = (sAppr4 & 0x0080) >> 7;
	iMantleType    = (sAppr4 & 0x0F00) >> 8;
	iBootsType     = (sAppr4 & 0xF000) >> 12;

	// Colors
	iApprColor     = apprColor;
	iWeaponColor   = (apprColor & 0xF0000000) >> 28;
	iShieldColor   = (apprColor & 0x0F000000) >> 24;
	iArmorColor    = (apprColor & 0x00F00000) >> 20;
	iMantleColor   = (apprColor & 0x000F0000) >> 16;
	iArmColor      = (apprColor & 0x0000F000) >> 12;
	iPantsColor    = (apprColor & 0x00000F00) >> 8;
	iBootsColor    = (apprColor & 0x000000F0) >> 4;
	iHelmColor     = (apprColor & 0x0000000F);
}

void PlayerAppearance::Clear()
{
	std::memset(this, 0, sizeof(*this));
}
