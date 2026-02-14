#pragma once

#include "GameChatCommand.h"
#include "AdminLevel.h"

class GameCmdGiveItem : public GameChatCommand
{
public:
	const char* get_name() const override { return "giveitem"; }
	int get_default_level() const override { return hb::shared::admin::Administrator; }
	bool execute(CGame* game, int client_h, const char* args) override;
};
