#pragma once

#include "ServerCommand.h"

class CmdHelp : public ServerCommand
{
public:
	CmdHelp(const std::vector<std::unique_ptr<ServerCommand>>& commands)
		: m_commands(commands) {}

	const char* GetName() const override { return "help"; }
	const char* GetDescription() const override { return "List available commands"; }
	void Execute(CGame* pGame, const char* pArgs) override;

private:
	const std::vector<std::unique_ptr<ServerCommand>>& m_commands;
};
