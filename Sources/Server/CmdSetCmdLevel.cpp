#include "CmdSetCmdLevel.h"
#include "Game.h"
#include "GameConfigSqliteStore.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "Log.h"

void CmdSetCmdLevel::execute(CGame* game, const char* args)
{
	if (args == nullptr || args[0] == '\0')
	{
		hb::logger::log("Usage: setcmdlevel <command> <level>");
		if (!game->m_command_permissions.empty())
		{
			hb::logger::log("Current command permissions:");
			for (const auto& pair : game->m_command_permissions)
			{
				char buf[256];
				if (pair.second.description.empty())
					std::snprintf(buf, sizeof(buf), "  /%s -> level %d", pair.first.c_str(), pair.second.admin_level);
				else
				hb::logger::log("/{} -> level {} ({})", pair.first.c_str(), pair.second.admin_level, pair.second.description.c_str());
			}
		}
		return;
	}

	// Parse command name (first word)
	char cmd_name[64] = {};
	const char* p = args;
	int ci = 0;
	while (*p != '\0' && *p != ' ' && *p != '\t' && ci < 63)
	{
		cmd_name[ci++] = *p++;
	}

	// Skip whitespace
	while (*p == ' ' || *p == '\t')
		p++;

	if (*p == '\0')
	{
		// Show current level for this command
		auto it = game->m_command_permissions.find(cmd_name);
		if (it != game->m_command_permissions.end())
		{
			hb::logger::log("Command '/{}' requires admin level {}", cmd_name, it->second.admin_level);
		}
		else
		{
			hb::logger::log("Command '/{}' not found in permissions table", cmd_name);
		}
		return;
	}

	int level = std::atoi(p);
	if (level < 0)
	{
		hb::logger::log("Level must be >= 0.");
		return;
	}

	// Update or create the permission entry, preserving existing description
	auto it = game->m_command_permissions.find(cmd_name);
	if (it != game->m_command_permissions.end())
	{
		it->second.admin_level = level;
	}
	else
	{
		CommandPermission perm;
		perm.admin_level = level;
		game->m_command_permissions[cmd_name] = perm;
	}

	// Save to DB
	sqlite3* configDb = nullptr;
	std::string dbPath;
	if (EnsureGameConfigDatabase(&configDb, dbPath, nullptr))
	{
		SaveCommandPermissions(configDb, game);
		CloseGameConfigDatabase(configDb);
	}

	hb::logger::log("Command '/{}' now requires admin level {}", cmd_name, level);
}
