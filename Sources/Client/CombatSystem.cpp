#include "CombatSystem.h"
#include "Player.h"

CombatSystem& CombatSystem::Get()
{
	static CombatSystem instance;
	return instance;
}

void CombatSystem::SetPlayer(CPlayer& player)
{
	m_player = &player;
}

// Snoopy: added StormBlade
int CombatSystem::GetAttackType() const
{
	if (!m_player) return 0;
	uint16_t wWeaponType;
	wWeaponType = m_player->m_playerAppearance.iWeaponType;
	if (wWeaponType == 0)
	{
		if ((m_player->m_iSuperAttackLeft > 0) && (m_player->m_bSuperAttackMode == true) && (m_player->m_iSkillMastery[5] >= 100)) return 20;
		else return 1;		// Boxe
	}
	else if ((wWeaponType >= 1) && (wWeaponType <= 2))
	{
		if ((m_player->m_iSuperAttackLeft > 0) && (m_player->m_bSuperAttackMode == true) && (m_player->m_iSkillMastery[7] >= 100)) return 21;
		else return 1;		//Dag, SS
	}
	else if ((wWeaponType > 2) && (wWeaponType < 20))
	{
		if ((wWeaponType == 7) || (wWeaponType == 18)) // Added Kloness Esterk
		{
			if ((m_player->m_iSuperAttackLeft > 0) && (m_player->m_bSuperAttackMode == true) && (m_player->m_iSkillMastery[9] >= 100)) return 22;
			else return 1;  // Esterk
		}
		else if (wWeaponType == 15)
		{
			if ((m_player->m_iSuperAttackLeft > 0) && (m_player->m_bSuperAttackMode == true) && (m_player->m_iSkillMastery[8] >= 100)) return 30;
			else return 5;  // StormBlade
		}
		else
		{
			if ((m_player->m_iSuperAttackLeft > 0) && (m_player->m_bSuperAttackMode == true) && (m_player->m_iSkillMastery[8] >= 100)) return 23;
			else return 1;	// LongSwords
		}
	}
	else if ((wWeaponType >= 20) && (wWeaponType <= 29))
	{
		if (wWeaponType == 29) {
			// Type 29 is a Long Sword variant
			if ((m_player->m_iSuperAttackLeft > 0) && (m_player->m_bSuperAttackMode == true) && (m_player->m_iSkillMastery[8] >= 100)) return 23;
			else return 1;		// LS
		}
		if ((m_player->m_iSuperAttackLeft > 0) && (m_player->m_bSuperAttackMode == true) && (m_player->m_iSkillMastery[10] >= 100)) return 24;
		else return 1;		// Axes
	}
	else if ((wWeaponType >= 30) && (wWeaponType <= 33))
	{
		if (wWeaponType == 33) {
			// Type 33 is a Long Sword variant
			if ((m_player->m_iSuperAttackLeft > 0) && (m_player->m_bSuperAttackMode == true) && (m_player->m_iSkillMastery[8] >= 100)) return 23;
			else return 1;		// LS
		}
		if ((m_player->m_iSuperAttackLeft > 0) && (m_player->m_bSuperAttackMode == true) && (m_player->m_iSkillMastery[14] >= 100)) return 26;
		else return 1;		// Hammers
	}
	else if ((wWeaponType >= 34) && (wWeaponType < 40))
	{
		if ((m_player->m_iSuperAttackLeft > 0) && (m_player->m_bSuperAttackMode == true) && (m_player->m_iSkillMastery[21] >= 100)) return 27;
		else return 1;		// Wands
	}
	else if (wWeaponType >= 40)
	{
		if ((m_player->m_iSuperAttackLeft > 0) && (m_player->m_bSuperAttackMode == true) && (m_player->m_iSkillMastery[6] >= 100)) return 25;
		else return 2;		// Bows
	}
	return 0;
}

int CombatSystem::GetWeaponSkillType() const
{
	if (!m_player) return 1;
	uint16_t wWeaponType;
	wWeaponType = m_player->m_playerAppearance.iWeaponType;
	if (wWeaponType == 0)
	{
		return 5; // Openhand
	}
	else if ((wWeaponType >= 1) && (wWeaponType < 3))
	{
		return 7; // SS
	}
	else if ((wWeaponType >= 3) && (wWeaponType < 20))
	{
		if ((wWeaponType == 7) || (wWeaponType == 18)) // Esterk or KlonessEsterk
			return 9; // Fencing
		else return 8; // LS
	}
	else if ((wWeaponType >= 20) && (wWeaponType <= 29))
	{
		if (wWeaponType == 29) return 8; // LS (LightingBlade)
		return 10; // Axe (20..28)
	}
	else if ((wWeaponType >= 30) && (wWeaponType <= 33))
	{
		if (wWeaponType == 33) return 8; // LS (BlackShadow)
		return 14; // Hammer (30,31,32)
	}
	else if ((wWeaponType >= 34) && (wWeaponType < 40))
	{
		return 21; // Wand
	}
	else if (wWeaponType >= 40)
	{
		return 6;  // Bow
	}
	return 1; // Fishing !
}

bool CombatSystem::CanSuperAttack() const
{
	if (!m_player) return false;
	return m_player->m_iSuperAttackLeft > 0
		&& m_player->m_bSuperAttackMode
		&& m_player->m_iSkillMastery[GetWeaponSkillType()] >= 100;
}
