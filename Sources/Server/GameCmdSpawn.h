#pragma once

#include "GameChatCommand.h"
#include "AdminLevel.h"

class GameCmdSpawn : public GameChatCommand
{
public:
	const char* GetName() const override { return "spawn"; }
	int GetDefaultLevel() const override { return hb::admin::Developer; }
	bool Execute(CGame* pGame, int iClientH, const char* pArgs) override;
};
