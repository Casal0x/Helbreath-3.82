#include "EquipmentIndices.h"
#include "EntityRenderState.h"
#include "SpriteID.h"
#include "ConfigManager.h"
using namespace hb::client::sprite_id;

EquipmentIndices EquipmentIndices::CalcPlayer(const CEntityRenderState& state, int bodyPose, int weaponPose, int shieldPose)
{
	EquipmentIndices eq = {};
	const auto& appr = state.m_appearance;
	bool isFemale = state.IsFemale();

	// Gender-specific sprite base IDs
	int UNDIES  = isFemale ? UndiesW    : UndiesM;
	int HAIR    = isFemale ? HairW      : HairM;
	int ARMOR   = isFemale ? BodyArmorW : BodyArmorM;
	int BERK    = isFemale ? BerkW      : BerkM;
	int LEGG    = isFemale ? LeggW      : LeggM;
	int BOOT    = isFemale ? BootW      : BootM;
	int WEAPON  = isFemale ? WeaponW    : WeaponM;
	int SHIELD  = isFemale ? ShieldW    : ShieldM;
	int MANTLE  = isFemale ? MantleW    : MantleM;
	int HEAD    = isFemale ? HeadW      : HeadM;

	// Body index
	eq.iBodyIndex = 500 + (state.m_sOwnerType - 1) * 8 * 15 + (bodyPose * 8);

	// Equipment indices — each uses bodyPose as the pose offset
	eq.iUndiesIndex = UNDIES + appr.iUnderwearType * 15 + bodyPose;
	eq.iHairIndex   = HAIR + appr.iHairStyle * 15 + bodyPose;

	// Body armor (hidden armor = no armor drawn)
	if (!appr.bHideArmor && appr.iArmorType != 0)
		eq.iBodyArmorIndex = ARMOR + appr.iArmorType * 15 + bodyPose;
	else
		eq.iBodyArmorIndex = -1;

	eq.iArmArmorIndex = (appr.iArmArmorType != 0) ? BERK + appr.iArmArmorType * 15 + bodyPose : -1;
	eq.iPantsIndex    = (appr.iPantsType != 0)     ? LEGG + appr.iPantsType * 15 + bodyPose    : -1;
	eq.iBootsIndex    = (appr.iBootsType != 0)     ? BOOT + appr.iBootsType * 15 + bodyPose    : -1;
	eq.iMantleIndex   = (appr.iMantleType != 0)    ? MANTLE + appr.iMantleType * 15 + bodyPose : -1;
	eq.iHelmIndex     = (appr.iHelmType != 0)      ? HEAD + appr.iHelmType * 15 + bodyPose     : -1;

	// Weapon — uses separate weaponPose; -1 means no weapon drawn
	if (weaponPose >= 0 && appr.iWeaponType != 0)
		eq.iWeaponIndex = WEAPON + appr.iWeaponType * 64 + 8 * weaponPose + (state.m_iDir - 1);
	else
		eq.iWeaponIndex = -1;

	// Shield — uses separate shieldPose; -1 means no shield drawn
	if (shieldPose >= 0 && appr.iShieldType != 0)
		eq.iShieldIndex = SHIELD + appr.iShieldType * 8 + shieldPose;
	else
		eq.iShieldIndex = -1;

	// Female skirt check (pants type 1)
	eq.iSkirtDraw = (isFemale && appr.iPantsType == 1) ? 1 : 0;

	// Colors and glare initialized to 0 by = {} above
	return eq;
}

EquipmentIndices EquipmentIndices::CalcNpc(const CEntityRenderState& state, int npcPose)
{
	EquipmentIndices eq = {};

	eq.iBodyIndex      = Mob + (state.m_sOwnerType - 10) * 8 * 7 + (npcPose * 8);
	eq.iUndiesIndex    = -1;
	eq.iHairIndex      = -1;
	eq.iBodyArmorIndex = -1;
	eq.iArmArmorIndex  = -1;
	eq.iPantsIndex     = -1;
	eq.iBootsIndex     = -1;
	eq.iWeaponIndex    = -1;
	eq.iShieldIndex    = -1;
	eq.iMantleIndex    = -1;
	eq.iHelmIndex      = -1;
	eq.iSkirtDraw      = 0;

	// Colors and glare initialized to 0 by = {} above
	return eq;
}

void EquipmentIndices::CalcColors(const CEntityRenderState& state)
{
	if (ConfigManager::Get().GetDetailLevel() == 0)
	{
		iWeaponColor = 0;
		iShieldColor = 0;
		iArmorColor  = 0;
		iMantleColor = 0;
		iArmColor    = 0;
		iPantsColor  = 0;
		iBootsColor  = 0;
		iHelmColor   = 0;
	}
	else
	{
		iWeaponColor = state.m_appearance.iWeaponColor;
		iShieldColor = state.m_appearance.iShieldColor;
		iArmorColor  = state.m_appearance.iArmorColor;
		iMantleColor = state.m_appearance.iMantleColor;
		iArmColor    = state.m_appearance.iArmColor;
		iPantsColor  = state.m_appearance.iPantsColor;
		iBootsColor  = state.m_appearance.iBootsColor;
		iHelmColor   = state.m_appearance.iHelmColor;
	}

	// Note: glare fields are swapped in the original code
	iWeaponGlare = state.m_appearance.iShieldGlare;
	iShieldGlare = state.m_appearance.iWeaponGlare;
}
