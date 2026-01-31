#pragma once

#include "ServerCommand.h"

class CmdGiveItem : public ServerCommand
{
public:
	const char* GetName() const override { return "giveitem"; }
	const char* GetDescription() const override { return "Give an item to a player"; }
	const char* GetHelp() const override { return "Usage: giveitem <playername> <item_id> <amount>"; }
	void Execute(CGame* pGame, const char* pArgs) override;
};
