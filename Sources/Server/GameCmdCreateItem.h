#pragma once

#include "GameChatCommand.h"
#include "AdminLevel.h"

class GameCmdCreateItem : public GameChatCommand
{
public:
	const char* GetName() const override { return "createitem"; }
	int GetDefaultLevel() const override { return hb::admin::Administrator; }
	bool Execute(CGame* pGame, int iClientH, const char* pArgs) override;
};
