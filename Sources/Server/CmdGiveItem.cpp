#include <windows.h>
#include "CmdGiveItem.h"
#include "Game.h"
#include "winmain.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

void CmdGiveItem::Execute(CGame* pGame, const char* pArgs)
{
	if (pArgs == nullptr || pArgs[0] == '\0')
	{
		PutLogList((char*)"Usage: giveitem <playername> <item_id> <amount>");
		return;
	}

	// Parse player name
	char cPlayerName[32];
	std::memset(cPlayerName, 0, sizeof(cPlayerName));
	const char* p = pArgs;
	int i = 0;
	while (*p != '\0' && *p != ' ' && *p != '\t' && i < (int)(sizeof(cPlayerName) - 1))
	{
		cPlayerName[i++] = *p++;
	}
	cPlayerName[i] = '\0';

	// Skip whitespace
	while (*p == ' ' || *p == '\t')
		p++;

	if (*p == '\0')
	{
		PutLogList((char*)"Usage: giveitem <playername> <item_id> <amount>");
		return;
	}

	// Parse item ID
	int iItemID = std::atoi(p);
	while (*p != '\0' && *p != ' ' && *p != '\t')
		p++;

	// Skip whitespace
	while (*p == ' ' || *p == '\t')
		p++;

	if (*p == '\0')
	{
		PutLogList((char*)"Usage: giveitem <playername> <item_id> <amount>");
		return;
	}

	// Parse amount
	int iAmount = std::atoi(p);

	// Find player by name
	int iClientH = 0;
	for (int j = 1; j < DEF_MAXCLIENTS; j++)
	{
		if (pGame->m_pClientList[j] != nullptr &&
			_stricmp(pGame->m_pClientList[j]->m_cCharName, cPlayerName) == 0)
		{
			iClientH = j;
			break;
		}
	}

	if (iClientH == 0)
	{
		char buf[256];
		std::snprintf(buf, sizeof(buf), "Player '%s' not found.", cPlayerName);
		PutLogList(buf);
		return;
	}

	// Validate item ID
	if (iItemID < 0 || iItemID >= DEF_MAXITEMTYPES || pGame->m_pItemConfigList[iItemID] == nullptr)
	{
		char buf[256];
		std::snprintf(buf, sizeof(buf), "Invalid item ID: %d.", iItemID);
		PutLogList(buf);
		return;
	}

	// Create item
	CItem* pItem = new CItem;
	if (pGame->_bInitItemAttr(pItem, iItemID) == false)
	{
		delete pItem;
		char buf[256];
		std::snprintf(buf, sizeof(buf), "Failed to initialize item ID: %d.", iItemID);
		PutLogList(buf);
		return;
	}
	pItem->m_dwCount = iAmount;

	// Save item name before bAddItem (which may delete the item on failure)
	char cItemName[64];
	std::memset(cItemName, 0, sizeof(cItemName));
	std::strncpy(cItemName, pItem->m_cName, sizeof(cItemName) - 1);

	// Give item to player
	if (pGame->bAddItem(iClientH, pItem, 0) == false)
	{
		PutLogList((char*)"Failed to give item: player inventory full.");
		return;
	}

	// Success
	char buf[256];
	std::snprintf(buf, sizeof(buf), "Gave %dx %s (ID: %d) to %s.", iAmount, cItemName, iItemID, cPlayerName);
	PutLogList(buf);
}
