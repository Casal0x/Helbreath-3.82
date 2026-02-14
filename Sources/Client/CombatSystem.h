#pragma once

#include <cstdint>

class CPlayer;

class combat_system
{
public:
	static combat_system& get();

	void set_player(CPlayer& player);

	int get_attack_type() const;
	int get_weapon_skill_type() const;
	bool can_super_attack() const;

private:
	combat_system() = default;
	CPlayer* m_player = nullptr;
};
