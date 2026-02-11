#pragma once

#include "GameChatCommand.h"
#include "AdminLevel.h"

class GameCmdGoto : public GameChatCommand
{
public:
	const char* GetName() const override { return "goto"; }
	int GetDefaultLevel() const override { return hb::shared::admin::GameMaster; }
	bool Execute(CGame* pGame, int iClientH, const char* pArgs) override;
};
