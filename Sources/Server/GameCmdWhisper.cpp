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

		char cName[11] = {};
		pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_WHISPERMODEOFF, 0, 0, 0, cName);
		return true;
	}

	// Extract player name (max 10 chars)
	char cName[11] = {};
	size_t nameLen = std::strlen(pArgs);
	if (nameLen > 10) nameLen = 10;

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

	// Search for player on this server
	for (int i = 1; i < DEF_MAXCLIENTS; i++)
	{
		if (pGame->m_pClientList[i] != nullptr &&
			std::memcmp(pGame->m_pClientList[i]->m_cCharName, cName, 10) == 0)
		{
			// Can't whisper yourself
			if (i == iClientH)
				return true;

			pGame->m_pClientList[iClientH]->m_iWhisperPlayerIndex = i;
			std::memset(pGame->m_pClientList[iClientH]->m_cWhisperPlayerName, 0,
				sizeof(pGame->m_pClientList[iClientH]->m_cWhisperPlayerName));
			std::strcpy(pGame->m_pClientList[iClientH]->m_cWhisperPlayerName, cName);
			break;
		}
	}

	if (pGame->m_pClientList[iClientH]->m_iWhisperPlayerIndex == -1)
	{
		// Player not found on local server - store name and mark as checking
		std::memset(pGame->m_pClientList[iClientH]->m_cWhisperPlayerName, 0,
			sizeof(pGame->m_pClientList[iClientH]->m_cWhisperPlayerName));
		std::strcpy(pGame->m_pClientList[iClientH]->m_cWhisperPlayerName, cName);
		pGame->m_pClientList[iClientH]->m_bIsCheckingWhisperPlayer = true;
	}
	else
	{
		pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_WHISPERMODEON, 0, 0, 0,
			pGame->m_pClientList[iClientH]->m_cWhisperPlayerName);
	}

	return true;
}
