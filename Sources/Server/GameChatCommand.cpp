#define _WINSOCKAPI_
#include <windows.h>
#include "GameChatCommand.h"
#include "GameCmdWhisper.h"
#include "Game.h"
#include "winmain.h"
#include <cstring>
#include <cstdio>

GameChatCommandManager& GameChatCommandManager::Get()
{
	static GameChatCommandManager instance;
	return instance;
}

void GameChatCommandManager::Initialize(CGame* pGame)
{
	if (m_bInitialized)
		return;

	m_pGame = pGame;
	RegisterBuiltInCommands();
	m_bInitialized = true;
}

void GameChatCommandManager::RegisterCommand(std::unique_ptr<GameChatCommand> command)
{
	m_commands.push_back(std::move(command));
}

bool GameChatCommandManager::ProcessCommand(int iClientH, const char* pMessage, uint32_t dwMsgSize)
{
	if (m_pGame == nullptr || pMessage == nullptr)
		return false;

	if (pMessage[0] != '/')
		return false;

	const char* pCommand = pMessage + 1;

	for (const auto& cmd : m_commands)
	{
		const char* cmdName = cmd->GetName();
		size_t cmdLen = std::strlen(cmdName);

		if (_strnicmp(pCommand, cmdName, cmdLen) == 0)
		{
			char nextChar = pCommand[cmdLen];
			if (nextChar == '\0' || nextChar == ' ' || nextChar == '\t')
			{
				const char* pArgs = pCommand + cmdLen;
				while (*pArgs == ' ' || *pArgs == '\t')
					pArgs++;

				LogCommand(iClientH, pMessage);
				return cmd->Execute(m_pGame, iClientH, pArgs);
			}
		}
	}

	return false;
}

void GameChatCommandManager::LogCommand(int iClientH, const char* pCommand)
{
	if (m_pGame == nullptr || m_pGame->m_pClientList[iClientH] == nullptr)
		return;

	FILE* pFile = fopen("GameLogs\\Commands.log", "a");
	if (pFile == nullptr)
		return;

	SYSTEMTIME st;
	GetLocalTime(&st);

	fprintf(pFile, "[%02d:%02d:%02d] %s: %s\n",
		st.wHour, st.wMinute, st.wSecond,
		m_pGame->m_pClientList[iClientH]->m_cCharName,
		pCommand);

	fclose(pFile);
}

void GameChatCommandManager::RegisterBuiltInCommands()
{
	RegisterCommand(std::make_unique<GameCmdWhisper>());
}
