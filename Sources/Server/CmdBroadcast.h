#pragma once

#include "ServerCommand.h"

class CmdBroadcast : public ServerCommand
{
public:
	const char* GetName() const override { return "broadcast"; }
	const char* GetDescription() const override { return "Send a GM message to all players"; }
	const char* GetHelp() const override { return "Usage: broadcast <message>\n  Sends a server-wide GM chat message visible to all connected players.\n  The message appears as from 'Server' in the GM chat channel."; }
	void Execute(CGame* pGame, const char* pArgs) override;
};
