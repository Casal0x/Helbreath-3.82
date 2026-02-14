#pragma once

#include "GameChatCommand.h"

class GameCmdBlock : public GameChatCommand
{
public:
	const char* get_name() const override { return "block"; }
	bool execute(CGame* game, int client_h, const char* args) override;
};
