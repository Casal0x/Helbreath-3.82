#pragma once

#include "GameChatCommand.h"

class GameCmdBlock : public GameChatCommand
{
public:
	const char* GetName() const override { return "block"; }
	bool Execute(CGame* pGame, int iClientH, const char* pArgs) override;
};
