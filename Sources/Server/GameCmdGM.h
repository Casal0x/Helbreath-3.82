#pragma once

#include "GameChatCommand.h"
#include "AdminLevel.h"

class GameCmdGM : public GameChatCommand
{
public:
	const char* GetName() const override { return "gm"; }
	int GetDefaultLevel() const override { return hb::admin::GameMaster; }
	bool RequiresGMMode() const override { return false; }
	bool Execute(CGame* pGame, int iClientH, const char* pArgs) override;
};
