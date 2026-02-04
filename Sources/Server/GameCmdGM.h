#pragma once

#include "GameChatCommand.h"

class GameCmdGM : public GameChatCommand
{
public:
	const char* GetName() const override { return "gm"; }
	bool Execute(CGame* pGame, int iClientH, const char* pArgs) override;
};
