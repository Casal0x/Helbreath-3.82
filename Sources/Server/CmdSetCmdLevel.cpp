#include "CmdSetCmdLevel.h"
#include "Game.h"
#include "GameConfigSqliteStore.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "Log.h"

void CmdSetCmdLevel::Execute(CGame* pGame, const char* pArgs)
{
	if (pArgs == nullptr || pArgs[0] == '\0')
	{
		hb::logger::log("Usage: setcmdlevel <command> <level>");
		if (!pGame->m_commandPermissions.empty())
		{
			hb::logger::log("Current command permissions:");
			for (const auto& pair : pGame->m_commandPermissions)
			{
				char buf[256];
				if (pair.second.sDescription.empty())
					std::snprintf(buf, sizeof(buf), "  /%s -> level %d", pair.first.c_str(), pair.second.iAdminLevel);
				else
				hb::logger::log("/{} -> level {} ({})", pair.first.c_str(), pair.second.iAdminLevel, pair.second.sDescription.c_str());
			}
		}
		return;
	}

	// Parse command name (first word)
	char cCmdName[64] = {};
	const char* p = pArgs;
	int ci = 0;
	while (*p != '\0' && *p != ' ' && *p != '\t' && ci < 63)
	{
		cCmdName[ci++] = *p++;
	}

	// Skip whitespace
	while (*p == ' ' || *p == '\t')
		p++;

	if (*p == '\0')
	{
		// Show current level for this command
		auto it = pGame->m_commandPermissions.find(cCmdName);
		if (it != pGame->m_commandPermissions.end())
		{
			hb::logger::log("Command '/{}' requires admin level {}", cCmdName, it->second.iAdminLevel);
		}
		else
		{
			hb::logger::log("Command '/{}' not found in permissions table", cCmdName);
		}
		return;
	}

	int iLevel = std::atoi(p);
	if (iLevel < 0)
	{
		hb::logger::log("Level must be >= 0.");
		return;
	}

	// Update or create the permission entry, preserving existing description
	auto it = pGame->m_commandPermissions.find(cCmdName);
	if (it != pGame->m_commandPermissions.end())
	{
		it->second.iAdminLevel = iLevel;
	}
	else
	{
		CommandPermission perm;
		perm.iAdminLevel = iLevel;
		pGame->m_commandPermissions[cCmdName] = perm;
	}

	// Save to DB
	sqlite3* configDb = nullptr;
	std::string dbPath;
	if (EnsureGameConfigDatabase(&configDb, dbPath, nullptr))
	{
		SaveCommandPermissions(configDb, pGame);
		CloseGameConfigDatabase(configDb);
	}

	hb::logger::log("Command '/{}' now requires admin level {}", cCmdName, iLevel);
}
