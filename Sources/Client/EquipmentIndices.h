#pragma once

#include <cstdint>

class CEntityRenderState;

// Consolidated equipment sprite index calculation.
// Replaces the 42+ duplicated blocks of equipment index computation
// across all DrawObject_On* functions. Each function computes a bodyPose
// (and optionally separate weapon/shield poses) then calls CalcPlayer.
struct EquipmentIndices
{
	// Sprite indices (-1 = not drawn)
	int iBodyIndex;
	int iUndiesIndex;
	int iHairIndex;
	int iBodyArmorIndex;
	int iArmArmorIndex;
	int iPantsIndex;
	int iBootsIndex;
	int iWeaponIndex;
	int iShieldIndex;
	int iMantleIndex;
	int iHelmIndex;

	// Equipment colors (0 = no tint, >0 = palette index)
	int iWeaponColor;
	int iShieldColor;
	int iArmorColor;
	int iMantleColor;
	int iArmColor;
	int iPantsColor;
	int iBootsColor;
	int iHelmColor;

	// Glare effects
	int iWeaponGlare;
	int iShieldGlare;

	// Female skirt flag (pants type 1 on female character)
	int iSkirtDraw;

	// Calculate all equipment indices for a player character.
	// bodyPose: pose for body sprite and equipment layers (0-14 range)
	//   Body: 500 + (ownerType-1)*8*15 + (bodyPose*8)
	//   Equipment: base + type*15 + bodyPose
	// weaponPose: pose for weapon sprite (-1 = don't draw weapon)
	//   Weapon: base + type*64 + 8*weaponPose + (dir-1)
	// shieldPose: pose for shield sprite (-1 = don't draw shield)
	//   Shield: base + type*8 + shieldPose
	static EquipmentIndices CalcPlayer(const CEntityRenderState& state, int bodyPose, int weaponPose, int shieldPose);

	// Calculate body index for NPC/mob. All equipment indices set to -1.
	// npcPose: animation pose for the mob sprite
	//   Body: DEF_SPRID_MOB + (ownerType-10)*8*7 + (npcPose*8)
	// If HasNpcSpecialState(), caller should override body index and frame after calling this.
	static EquipmentIndices CalcNpc(const CEntityRenderState& state, int npcPose);

	// Fill color fields from entity appearance, respecting detail level.
	// When detail level is 0, all colors are set to 0 (no tinting).
	// Also sets glare values (note: glare fields are swapped in original code).
	void CalcColors(const CEntityRenderState& state);
};
