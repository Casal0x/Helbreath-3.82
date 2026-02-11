#pragma once

#include "GameChatCommand.h"
#include "AdminLevel.h"

class GameCmdCome : public GameChatCommand
{
public:
	const char* GetName() const override { return "come"; }
	int GetDefaultLevel() const override { return hb::shared::admin::GameMaster + 1; }
	bool Execute(CGame* pGame, int iClientH, const char* pArgs) override;
};
