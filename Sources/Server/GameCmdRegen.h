#pragma once

#include "GameChatCommand.h"
#include "AdminLevel.h"

class GameCmdRegen : public GameChatCommand
{
public:
	const char* get_name() const override { return "regen"; }
	int get_default_level() const override { return hb::shared::admin::GameMaster + 1; }
	bool execute(CGame* game, int client_h, const char* args) override;
};
