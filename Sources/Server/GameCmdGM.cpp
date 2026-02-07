#include <windows.h>
#include "GameCmdGM.h"
#include "Game.h"
#include <cstring>

bool GameCmdGM::Execute(CGame* pGame, int iClientH, const char* pArgs)
{
	if (pGame->m_pClientList[iClientH] == nullptr)
		return true;

	if (pArgs == nullptr || pArgs[0] == '\0')
	{
		pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_NOTICEMSG, 0, 0, 0, "Usage: /gm on | /gm off");
		return true;
	}

	if (_stricmp(pArgs, "on") == 0)
	{
		pGame->m_pClientList[iClientH]->m_bIsGMMode = true;
		pGame->m_pClientList[iClientH]->m_status.bGMMode = true;
		pGame->SendEventToNearClient_TypeA(static_cast<short>(iClientH), DEF_OWNERTYPE_PLAYER, MSGID_EVENT_MOTION, DEF_OBJECTNULLACTION, 0, 0, 0);
		pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_NOTICEMSG, 0, 0, 0, "GM mode enabled.");
		return true;
	}
	else if (_stricmp(pArgs, "off") == 0)
	{
		pGame->m_pClientList[iClientH]->m_bIsGMMode = false;
		pGame->m_pClientList[iClientH]->m_status.bGMMode = false;
		pGame->SendEventToNearClient_TypeA(static_cast<short>(iClientH), DEF_OWNERTYPE_PLAYER, MSGID_EVENT_MOTION, DEF_OBJECTNULLACTION, 0, 0, 0);
		pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_NOTICEMSG, 0, 0, 0, "GM mode disabled.");
		return true;
	}
	else
	{
		pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_NOTICEMSG, 0, 0, 0, "Usage: /gm on | /gm off");
		return true;
	}
}
