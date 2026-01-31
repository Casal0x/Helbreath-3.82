#pragma once

#include "GameChatCommand.h"

class GameCmdWhisper : public GameChatCommand
{
public:
	const char* GetName() const override { return "to"; }
	bool Execute(CGame* pGame, int iClientH, const char* pArgs) override;
};
