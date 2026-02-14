#pragma once

#include "ServerCommand.h"

class CmdReload : public ServerCommand
{
public:
	const char* get_name() const override { return "reload"; }
	const char* GetDescription() const override { return "Reload config tables from database"; }
	const char* GetHelp() const override { return "Usage: reload <items|magic|skills|npcs|all>\n  Reloads configuration from gameconfigs.db at runtime.\n  For items/magic/skills, pushes updates to all connected clients."; }
	void execute(CGame* game, const char* args) override;
};
