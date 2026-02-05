#pragma once

#include "GameChatCommand.h"
#include "AdminLevel.h"

class GameCmdGiveItem : public GameChatCommand
{
public:
	const char* GetName() const override { return "giveitem"; }
	int GetDefaultLevel() const override { return hb::admin::Administrator; }
	bool Execute(CGame* pGame, int iClientH, const char* pArgs) override;
};
