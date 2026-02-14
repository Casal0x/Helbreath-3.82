#pragma once

#include "GameChatCommand.h"

class GameCmdUnblock : public GameChatCommand
{
public:
	const char* get_name() const override { return "unblock"; }
	bool execute(CGame* game, int client_h, const char* args) override;
};
