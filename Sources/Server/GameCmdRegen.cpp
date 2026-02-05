#include <windows.h>
#include "GameCmdRegen.h"
#include "Game.h"
#include <cstring>
#include <cstdio>

bool GameCmdRegen::Execute(CGame* pGame, int iClientH, const char* pArgs)
{
	if (pGame->m_pClientList[iClientH] == nullptr)
		return true;

	int iTargetH = iClientH;

	if (pArgs != nullptr && pArgs[0] != '\0')
	{
		iTargetH = pGame->FindClientByName(pArgs);
		if (iTargetH == 0)
		{
			pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_NOTICEMSG, 0, 0, 0, "Player not found.");
			return true;
		}
	}

	if (pGame->m_pClientList[iTargetH] == nullptr)
		return true;

	pGame->m_pClientList[iTargetH]->m_iHP = pGame->iGetMaxHP(iTargetH);
	pGame->m_pClientList[iTargetH]->m_iMP = pGame->iGetMaxMP(iTargetH);
	pGame->m_pClientList[iTargetH]->m_iSP = pGame->iGetMaxSP(iTargetH);

	pGame->SendNotifyMsg(0, iTargetH, DEF_NOTIFY_HP, 0, 0, 0, 0);
	pGame->SendNotifyMsg(0, iTargetH, DEF_NOTIFY_MP, 0, 0, 0, 0);
	pGame->SendNotifyMsg(0, iTargetH, DEF_NOTIFY_SP, 0, 0, 0, 0);

	if (iTargetH != iClientH)
	{
		pGame->SendNotifyMsg(0, iTargetH, DEF_NOTIFY_NOTICEMSG, 0, 0, 0, "Your health has been fully restored by a GM.");

		char buf[64];
		std::snprintf(buf, sizeof(buf), "Restored %s's HP/MP/SP to full.", pGame->m_pClientList[iTargetH]->m_cCharName);
		pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_NOTICEMSG, 0, 0, 0, buf);
	}
	else
	{
		pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_NOTICEMSG, 0, 0, 0, "HP/MP/SP restored to full.");
	}

	return true;
}
