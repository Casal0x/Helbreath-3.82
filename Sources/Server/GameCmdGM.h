#pragma once

#include "GameChatCommand.h"
#include "AdminLevel.h"

class GameCmdGM : public GameChatCommand
{
public:
	const char* get_name() const override { return "gm"; }
	int get_default_level() const override { return hb::shared::admin::GameMaster; }
	bool requires_gm_mode() const override { return false; }
	bool execute(CGame* game, int client_h, const char* args) override;
};
