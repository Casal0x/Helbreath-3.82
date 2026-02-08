#include <windows.h>
#include "GameCmdWhisper.h"
#include "Game.h"
#include <cstring>

bool GameCmdWhisper::Execute(CGame* pGame, int iClientH, const char* pArgs)
{
	if (pGame->m_pClientList[iClientH] == nullptr)
		return true;

	if (pArgs == nullptr || pArgs[0] == '\0')
	{
		// No name = disable whisper mode
		pGame->m_pClientList[iClientH]->m_iWhisperPlayerIndex = -1;
		std::memset(pGame->m_pClientList[iClientH]->m_cWhisperPlayerName, 0,
			sizeof(pGame->m_pClientList[iClientH]->m_cWhisperPlayerName));
		pGame->m_pClientList[iClientH]->m_bIsCheckingWhisperPlayer = false;

		char cName[DEF_CHARNAME] = {};
		pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_WHISPERMODEOFF, 0, 0, 0, cName);
		return true;
	}

	// Extract player name (max 10 chars)
	char cName[DEF_CHARNAME] = {};
	size_t nameLen = std::strlen(pArgs);
	if (nameLen > DEF_CHARNAME - 1) nameLen = DEF_CHARNAME - 1;

	// Copy only the first word (stop at space)
	for (size_t i = 0; i < nameLen; i++)
	{
		if (pArgs[i] == ' ' || pArgs[i] == '\t')
			break;
		cName[i] = pArgs[i];
	}

	if (cName[0] == '\0')
		return true;

	pGame->m_pClientList[iClientH]->m_iWhisperPlayerIndex = -1;

	// Search for player on this server (case-insensitive)
	for(int i = 1; i < DEF_MAXCLIENTS; i++)
	{
		if (pGame->m_pClientList[i] != nullptr &&
			_strnicmp(pGame->m_pClientList[i]->m_cCharName, cName, DEF_CHARNAME - 1) == 0)
		{
			// Can't whisper yourself
			if (i == iClientH)
				return true;

			pGame->m_pClientList[iClientH]->m_iWhisperPlayerIndex = i;
			std::memset(pGame->m_pClientList[iClientH]->m_cWhisperPlayerName, 0,
				sizeof(pGame->m_pClientList[iClientH]->m_cWhisperPlayerName));
			std::strcpy(pGame->m_pClientList[iClientH]->m_cWhisperPlayerName,
				pGame->m_pClientList[i]->m_cCharName);
			break;
		}
	}

	if (pGame->m_pClientList[iClientH]->m_iWhisperPlayerIndex == -1)
	{
		pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_PLAYERNOTONGAME, 0, 0, 0, cName);
	}
	else
	{
		pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_WHISPERMODEON, 0, 0, 0,
			pGame->m_pClientList[iClientH]->m_cWhisperPlayerName);
	}

	return true;
}
