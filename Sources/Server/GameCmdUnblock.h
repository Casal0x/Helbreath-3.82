#pragma once

#include "GameChatCommand.h"

class GameCmdUnblock : public GameChatCommand
{
public:
	const char* GetName() const override { return "unblock"; }
	bool Execute(CGame* pGame, int iClientH, const char* pArgs) override;
};
