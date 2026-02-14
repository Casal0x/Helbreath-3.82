#include "CombatSystem.h"
#include "Player.h"

combat_system& combat_system::get()
{
	static combat_system instance;
	return instance;
}

void combat_system::set_player(CPlayer& player)
{
	m_player = &player;
}

// Snoopy: added StormBlade
int combat_system::get_attack_type() const
{
	if (!m_player) return 0;
	uint16_t weapon_type;
	weapon_type = m_player->m_playerAppearance.weapon_type;
	if (weapon_type == 0)
	{
		if ((m_player->m_super_attack_left > 0) && (m_player->m_super_attack_mode == true) && (m_player->m_skill_mastery[5] >= 100)) return 20;
		else return 1;		// Boxe
	}
	else if ((weapon_type >= 1) && (weapon_type <= 2))
	{
		if ((m_player->m_super_attack_left > 0) && (m_player->m_super_attack_mode == true) && (m_player->m_skill_mastery[7] >= 100)) return 21;
		else return 1;		//Dag, SS
	}
	else if ((weapon_type > 2) && (weapon_type < 20))
	{
		if ((weapon_type == 7) || (weapon_type == 18)) // Added Kloness Esterk
		{
			if ((m_player->m_super_attack_left > 0) && (m_player->m_super_attack_mode == true) && (m_player->m_skill_mastery[9] >= 100)) return 22;
			else return 1;  // Esterk
		}
		else if (weapon_type == 15)
		{
			if ((m_player->m_super_attack_left > 0) && (m_player->m_super_attack_mode == true) && (m_player->m_skill_mastery[8] >= 100)) return 30;
			else return 5;  // StormBlade
		}
		else
		{
			if ((m_player->m_super_attack_left > 0) && (m_player->m_super_attack_mode == true) && (m_player->m_skill_mastery[8] >= 100)) return 23;
			else return 1;	// LongSwords
		}
	}
	else if ((weapon_type >= 20) && (weapon_type <= 29))
	{
		if (weapon_type == 29) {
			// Type 29 is a Long Sword variant
			if ((m_player->m_super_attack_left > 0) && (m_player->m_super_attack_mode == true) && (m_player->m_skill_mastery[8] >= 100)) return 23;
			else return 1;		// LS
		}
		if ((m_player->m_super_attack_left > 0) && (m_player->m_super_attack_mode == true) && (m_player->m_skill_mastery[10] >= 100)) return 24;
		else return 1;		// Axes
	}
	else if ((weapon_type >= 30) && (weapon_type <= 33))
	{
		if (weapon_type == 33) {
			// Type 33 is a Long Sword variant
			if ((m_player->m_super_attack_left > 0) && (m_player->m_super_attack_mode == true) && (m_player->m_skill_mastery[8] >= 100)) return 23;
			else return 1;		// LS
		}
		if ((m_player->m_super_attack_left > 0) && (m_player->m_super_attack_mode == true) && (m_player->m_skill_mastery[14] >= 100)) return 26;
		else return 1;		// Hammers
	}
	else if ((weapon_type >= 34) && (weapon_type < 40))
	{
		if ((m_player->m_super_attack_left > 0) && (m_player->m_super_attack_mode == true) && (m_player->m_skill_mastery[21] >= 100)) return 27;
		else return 1;		// Wands
	}
	else if (weapon_type >= 40)
	{
		if ((m_player->m_super_attack_left > 0) && (m_player->m_super_attack_mode == true) && (m_player->m_skill_mastery[6] >= 100)) return 25;
		else return 2;		// Bows
	}
	return 0;
}

int combat_system::get_weapon_skill_type() const
{
	if (!m_player) return 1;
	uint16_t weapon_type;
	weapon_type = m_player->m_playerAppearance.weapon_type;
	if (weapon_type == 0)
	{
		return 5; // Openhand
	}
	else if ((weapon_type >= 1) && (weapon_type < 3))
	{
		return 7; // SS
	}
	else if ((weapon_type >= 3) && (weapon_type < 20))
	{
		if ((weapon_type == 7) || (weapon_type == 18)) // Esterk or KlonessEsterk
			return 9; // Fencing
		else return 8; // LS
	}
	else if ((weapon_type >= 20) && (weapon_type <= 29))
	{
		if (weapon_type == 29) return 8; // LS (LightingBlade)
		return 10; // Axe (20..28)
	}
	else if ((weapon_type >= 30) && (weapon_type <= 33))
	{
		if (weapon_type == 33) return 8; // LS (BlackShadow)
		return 14; // Hammer (30,31,32)
	}
	else if ((weapon_type >= 34) && (weapon_type < 40))
	{
		return 21; // Wand
	}
	else if (weapon_type >= 40)
	{
		return 6;  // Bow
	}
	return 1; // Fishing !
}

bool combat_system::can_super_attack() const
{
	if (!m_player) return false;
	return m_player->m_super_attack_left > 0
		&& m_player->m_super_attack_mode
		&& m_player->m_skill_mastery[get_weapon_skill_type()] >= 100;
}
