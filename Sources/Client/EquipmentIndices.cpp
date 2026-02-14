#include "EquipmentIndices.h"
#include "EntityRenderState.h"
#include "SpriteID.h"
#include "ConfigManager.h"
using namespace hb::client::sprite_id;

EquipmentIndices EquipmentIndices::CalcPlayer(const CEntityRenderState& state, int bodyPose, int weaponPose, int shieldPose)
{
	EquipmentIndices eq = {};
	const auto& appr = state.m_appearance;
	bool female = state.is_female();

	// Gender-specific sprite base IDs
	int UNDIES  = female ? UndiesW    : UndiesM;
	int HAIR    = female ? HairW      : HairM;
	int ARMOR   = female ? BodyArmorW : BodyArmorM;
	int BERK    = female ? BerkW      : BerkM;
	int LEGG    = female ? LeggW      : LeggM;
	int BOOT    = female ? BootW      : BootM;
	int WEAPON  = female ? WeaponW    : WeaponM;
	int SHIELD  = female ? ShieldW    : ShieldM;
	int MANTLE  = female ? MantleW    : MantleM;
	int HEAD    = female ? HeadW      : HeadM;

	// Body index
	eq.m_body_index = 500 + (state.m_owner_type - 1) * 8 * 15 + (bodyPose * 8);

	// Equipment indices — each uses bodyPose as the pose offset
	eq.m_undies_index = UNDIES + appr.underwear_type * 15 + bodyPose;
	eq.m_hair_index   = HAIR + appr.hair_style * 15 + bodyPose;

	// Body armor (hidden armor = no armor drawn)
	if (!appr.hide_armor && appr.armor_type != 0)
		eq.m_body_armor_index = ARMOR + appr.armor_type * 15 + bodyPose;
	else
		eq.m_body_armor_index = -1;

	eq.m_arm_armor_index = (appr.arm_armor_type != 0) ? BERK + appr.arm_armor_type * 15 + bodyPose : -1;
	eq.m_pants_index    = (appr.pants_type != 0)     ? LEGG + appr.pants_type * 15 + bodyPose    : -1;
	eq.m_boots_index    = (appr.boots_type != 0)     ? BOOT + appr.boots_type * 15 + bodyPose    : -1;
	eq.m_mantle_index   = (appr.mantle_type != 0)    ? MANTLE + appr.mantle_type * 15 + bodyPose : -1;
	eq.m_helm_index     = (appr.helm_type != 0)      ? HEAD + appr.helm_type * 15 + bodyPose     : -1;

	// Weapon — uses separate weaponPose; -1 means no weapon drawn
	if (weaponPose >= 0 && appr.weapon_type != 0)
		eq.m_weapon_index = WEAPON + appr.weapon_type * 64 + 8 * weaponPose + (state.m_dir - 1);
	else
		eq.m_weapon_index = -1;

	// Shield — uses separate shieldPose; -1 means no shield drawn
	if (shieldPose >= 0 && appr.shield_type != 0)
		eq.m_shield_index = SHIELD + appr.shield_type * 8 + shieldPose;
	else
		eq.m_shield_index = -1;

	// Female skirt check (pants type 1)
	eq.m_skirt_draw = (female && appr.pants_type == 1) ? 1 : 0;

	// Colors and glare initialized to 0 by = {} above
	return eq;
}

EquipmentIndices EquipmentIndices::CalcNpc(const CEntityRenderState& state, int npcPose)
{
	EquipmentIndices eq = {};

	eq.m_body_index      = Mob + (state.m_owner_type - 10) * 8 * 7 + (npcPose * 8);
	eq.m_undies_index    = -1;
	eq.m_hair_index      = -1;
	eq.m_body_armor_index = -1;
	eq.m_arm_armor_index  = -1;
	eq.m_pants_index     = -1;
	eq.m_boots_index     = -1;
	eq.m_weapon_index    = -1;
	eq.m_shield_index    = -1;
	eq.m_mantle_index    = -1;
	eq.m_helm_index      = -1;
	eq.m_skirt_draw      = 0;

	// Colors and glare initialized to 0 by = {} above
	return eq;
}

void EquipmentIndices::calc_colors(const CEntityRenderState& state)
{
	if (config_manager::get().get_detail_level() == 0)
	{
		m_weapon_color = 0;
		m_shield_color = 0;
		m_armor_color  = 0;
		m_mantle_color = 0;
		m_arm_color    = 0;
		m_pants_color  = 0;
		m_boots_color  = 0;
		m_helm_color   = 0;
	}
	else
	{
		m_weapon_color = state.m_appearance.weapon_color;
		m_shield_color = state.m_appearance.shield_color;
		m_armor_color  = state.m_appearance.armor_color;
		m_mantle_color = state.m_appearance.mantle_color;
		m_arm_color    = state.m_appearance.arm_color;
		m_pants_color  = state.m_appearance.pants_color;
		m_boots_color  = state.m_appearance.boots_color;
		m_helm_color   = state.m_appearance.helm_color;
	}

	m_weapon_glare = state.m_appearance.weapon_glare;
	m_shield_glare = state.m_appearance.shield_glare;
}
