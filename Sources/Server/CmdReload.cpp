#include "CmdReload.h"
#include "Game.h"
#include "SkillManager.h"
#include "MagicManager.h"
#include "ItemManager.h"
#include <cstdio>
#include <cstring>
#include "Log.h"
#include "StringCompat.h"

void CmdReload::Execute(CGame* pGame, const char* pArgs)
{
	if (pArgs == nullptr || pArgs[0] == '\0')
	{
		hb::logger::log("Usage: reload <items|magic|skills|npcs|all>");
		return;
	}

	bool bItems = false;
	bool bMagic = false;
	bool bSkills = false;
	bool bNpcs = false;

	if (hb_stricmp(pArgs, "items") == 0)
		bItems = true;
	else if (hb_stricmp(pArgs, "magic") == 0)
		bMagic = true;
	else if (hb_stricmp(pArgs, "skills") == 0)
		bSkills = true;
	else if (hb_stricmp(pArgs, "npcs") == 0)
		bNpcs = true;
	else if (hb_stricmp(pArgs, "all") == 0)
	{
		bItems = true;
		bMagic = true;
		bSkills = true;
		bNpcs = true;
	}
	else
	{
		hb::logger::log("Unknown reload target: '{}'. Use items, magic, skills, npcs, or all.", pArgs);
		return;
	}

	// Send reload notification to clients first (shows top bar message)
	if (bItems || bMagic || bSkills || bNpcs)
		pGame->SendConfigReloadNotification(bItems, bMagic, bSkills, bNpcs);

	// Reload configs from database
	if (bItems)  pGame->m_pItemManager->ReloadItemConfigs();
	if (bMagic)  pGame->m_pMagicManager->ReloadMagicConfigs();
	if (bSkills) pGame->m_pSkillManager->ReloadSkillConfigs();
	if (bNpcs)   pGame->ReloadNpcConfigs();

	// Stream updated config data to clients
	if (bItems || bMagic || bSkills || bNpcs)
		pGame->PushConfigReloadToClients(bItems, bMagic, bSkills, bNpcs);
}
