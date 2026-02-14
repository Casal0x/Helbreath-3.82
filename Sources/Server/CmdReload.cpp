#include "CmdReload.h"
#include "Game.h"
#include "SkillManager.h"
#include "MagicManager.h"
#include "ItemManager.h"
#include <cstdio>
#include <cstring>
#include "Log.h"
#include "StringCompat.h"

void CmdReload::execute(CGame* game, const char* args)
{
	if (args == nullptr || args[0] == '\0')
	{
		hb::logger::log("Usage: reload <items|magic|skills|npcs|all>");
		return;
	}

	bool items = false;
	bool magic = false;
	bool skills = false;
	bool npcs = false;

	if (hb_stricmp(args, "items") == 0)
		items = true;
	else if (hb_stricmp(args, "magic") == 0)
		magic = true;
	else if (hb_stricmp(args, "skills") == 0)
		skills = true;
	else if (hb_stricmp(args, "npcs") == 0)
		npcs = true;
	else if (hb_stricmp(args, "all") == 0)
	{
		items = true;
		magic = true;
		skills = true;
		npcs = true;
	}
	else
	{
		hb::logger::log("Unknown reload target: '{}'. Use items, magic, skills, npcs, or all.", args);
		return;
	}

	// Send reload notification to clients first (shows top bar message)
	if (items || magic || skills || npcs)
		game->send_config_reload_notification(items, magic, skills, npcs);

	// Reload configs from database
	if (items)  game->m_item_manager->reload_item_configs();
	if (magic)  game->m_magic_manager->reload_magic_configs();
	if (skills) game->m_skill_manager->reload_skill_configs();
	if (npcs)   game->reload_npc_configs();

	// Stream updated config data to clients
	if (items || magic || skills || npcs)
		game->push_config_reload_to_clients(items, magic, skills, npcs);
}
