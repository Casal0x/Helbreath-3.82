#include <windows.h>
#include "GameCmdSpawn.h"
#include "Game.h"
#include "Npc.h"
#include <cstring>
#include <cstdio>

bool GameCmdSpawn::Execute(CGame* pGame, int iClientH, const char* pArgs)
{
	if (pGame->m_pClientList[iClientH] == nullptr)
		return true;

	int iNpcID = 0, iAmount = 1;
	if (pArgs == nullptr || pArgs[0] == '\0' || sscanf(pArgs, "%d %d", &iNpcID, &iAmount) < 1)
	{
		pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_NOTICEMSG, 0, 0, 0, "Usage: /spawn <npc_id> [amount]");
		return true;
	}

	if (iNpcID < 0 || iNpcID >= DEF_MAXNPCTYPES || pGame->m_pNpcConfigList[iNpcID] == nullptr)
	{
		pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_NOTICEMSG, 0, 0, 0, "Invalid NPC ID.");
		return true;
	}

	if (iAmount < 1) iAmount = 1;
	if (iAmount > 50) iAmount = 50;

	char* pNpcName = pGame->m_pNpcConfigList[iNpcID]->m_cNpcName;
	char* pMapName = pGame->m_pMapList[pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cName;
	int iSpawned = 0;

	for (int i = 0; i < iAmount; i++)
	{
		char cUniqueName[21];
		std::snprintf(cUniqueName, sizeof(cUniqueName), "GM-spawn%d", i);

		int tX = pGame->m_pClientList[iClientH]->m_sX;
		int tY = pGame->m_pClientList[iClientH]->m_sY;

		if (pGame->bCreateNewNpc(pNpcName, cUniqueName, pMapName, 0, 0, DEF_MOVETYPE_RANDOM,
			&tX, &tY, nullptr, nullptr, 0, -1, false, true, false, false, 0))
		{
			iSpawned++;
		}
	}

	char buf[128];
	std::snprintf(buf, sizeof(buf), "Spawned %d x %s (ID: %d).", iSpawned, pNpcName, iNpcID);
	pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_NOTICEMSG, 0, 0, 0, buf);

	return true;
}
