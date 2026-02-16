#include "CmdReload.h"
#include "ServerConsole.h"
#include "Game.h"
#include "SkillManager.h"
#include "MagicManager.h"
#include "ItemManager.h"
#include "Log.h"
#include "ServerLogChannels.h"
#include "StringCompat.h"

void CmdReload::execute(CGame* game, const char* args)
{
	if (args == nullptr || args[0] == '\0')
	{
		hb::console::error("Usage: reload <items|magic|skills|npcs|shops|all>");
		return;
	}

	bool items = false;
	bool magic = false;
	bool skills = false;
	bool npcs = false;
	bool shops = false;

	if (hb_stricmp(args, "items") == 0)
		items = true;
	else if (hb_stricmp(args, "magic") == 0)
		magic = true;
	else if (hb_stricmp(args, "skills") == 0)
		skills = true;
	else if (hb_stricmp(args, "npcs") == 0)
		npcs = true;
	else if (hb_stricmp(args, "shops") == 0)
		shops = true;
	else if (hb_stricmp(args, "all") == 0)
	{
		items = true;
		magic = true;
		skills = true;
		npcs = true;
		shops = true;
	}
	else
	{
		hb::console::error("Unknown reload target: '{}'. Use items, magic, skills, npcs, shops, or all.", args);
		return;
	}

	// Send reload notification to clients first (shows top bar message)
	if (items || magic || skills || npcs || shops)
		game->send_config_reload_notification(items, magic, skills, npcs);

	// Reload configs from database
	if (items)  game->m_item_manager->reload_item_configs();
	if (magic)  game->m_magic_manager->reload_magic_configs();
	if (skills) game->m_skill_manager->reload_skill_configs();
	if (npcs)   game->reload_npc_configs();
	if (shops)  game->reload_shop_configs();

	// Stream updated config data to clients
	if (items || magic || skills || npcs)
		game->push_config_reload_to_clients(items, magic, skills, npcs);

	hb::console::success("Reload complete: {}", args);
	hb::logger::log<hb::log_channel::commands>("reload {}", args);
}
