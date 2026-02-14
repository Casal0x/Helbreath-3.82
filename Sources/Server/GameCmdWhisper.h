#pragma once

#include "GameChatCommand.h"

class GameCmdWhisper : public GameChatCommand
{
public:
	const char* get_name() const override { return "to"; }
	bool execute(CGame* game, int client_h, const char* args) override;
};
