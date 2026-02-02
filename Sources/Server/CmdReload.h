#pragma once

#include "ServerCommand.h"

class CmdReload : public ServerCommand
{
public:
	const char* GetName() const override { return "reload"; }
	const char* GetDescription() const override { return "Reload config tables from database"; }
	const char* GetHelp() const override { return "Usage: reload <items|magic|skills|npcs|all>\n  Reloads configuration from GameConfigs.db at runtime.\n  For items/magic/skills, pushes updates to all connected clients."; }
	void Execute(CGame* pGame, const char* pArgs) override;
};
