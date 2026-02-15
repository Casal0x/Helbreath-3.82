#pragma once

#include <cstdint>
#include "DirectionHelpers.h"
using hb::shared::direction::direction;

class CGame;

class MagicManager
{
public:
	MagicManager() = default;
	~MagicManager() = default;

	void set_game(CGame* game) { m_game = game; }

	// Magic config
	bool send_client_magic_configs(int client_h);
	void reload_magic_configs();

	// Magic casting
	void player_magic_handler(int client_h, int dX, int dY, short type, bool item_effect = false, int v1 = 0, uint16_t targetObjectID = 0);
	int client_motion_magic_handler(int client_h, short sX, short sY, direction dir);

	// Magic study
	void request_study_magic_handler(int client_h, const char* name, bool is_purchase = true);
	int get_magic_number(char* magic_name, int* req_int, int* cost);
	void get_magic_ability_handler(int client_h);

	// Magic checks
	bool check_magic_int(int client_h);
	bool check_client_magic_frequency(int client_h, uint32_t client_time);
	int get_weather_magic_bonus_effect(short type, char wheather_status);

private:
	CGame* m_game = nullptr;
};
