#pragma once

#include "ServerCommand.h"

class CmdSetAdmin : public ServerCommand
{
public:
	const char* GetName() const override { return "setadmin"; }
	const char* GetDescription() const override { return "Set a player as admin"; }
	const char* GetHelp() const override { return "Usage: setadmin <charname> [level] | setadmin <charname> resetip"; }
	void Execute(CGame* pGame, const char* pArgs) override;
};
