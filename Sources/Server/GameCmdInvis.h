#pragma once

#include "GameChatCommand.h"
#include "AdminLevel.h"

class GameCmdInvis : public GameChatCommand
{
public:
	const char* GetName() const override { return "invis"; }
	int GetDefaultLevel() const override { return hb::admin::GameMaster; }
	bool Execute(CGame* pGame, int iClientH, const char* pArgs) override;
};
