#pragma once

#include <cstdint>

class CPlayer;

class CombatSystem
{
public:
	static CombatSystem& Get();

	void SetPlayer(CPlayer& player);

	int GetAttackType() const;
	int GetWeaponSkillType() const;
	bool CanSuperAttack() const;

private:
	CombatSystem() = default;
	CPlayer* m_player = nullptr;
};
