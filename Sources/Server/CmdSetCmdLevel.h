#pragma once

#include "ServerCommand.h"

class CmdSetCmdLevel : public ServerCommand
{
public:
	const char* GetName() const override { return "setcmdlevel"; }
	const char* GetDescription() const override { return "Set required admin level for a chat command"; }
	const char* GetHelp() const override { return "Usage: setcmdlevel <command> <level>"; }
	void Execute(CGame* pGame, const char* pArgs) override;
};
