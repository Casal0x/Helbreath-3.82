#define _WINSOCKAPI_
#include <windows.h>
#include "ChatLog.h"
#include "winmain.h"
#include <cstdio>
#include <direct.h>

ChatLog& ChatLog::Get()
{
	static ChatLog instance;
	return instance;
}

void ChatLog::Initialize()
{
	if (m_bInitialized)
		return;

	_mkdir("GameLogs");

	FILE* pFile = fopen("GameLogs\\Chat.log", "w");
	if (pFile != nullptr)
	{
		fclose(pFile);
		PutLogList((char*)"(!) Chat log initialized: GameLogs\\Chat.log");
	}
	else
	{
		PutLogList((char*)"(!) WARNING: Could not create GameLogs\\Chat.log");
	}

	m_bInitialized = true;
}

void ChatLog::Write(int chatType, const char* playerName, const char* mapName, const char* message, const char* whisperTarget)
{
	if (!m_bInitialized)
		return;

	FILE* pFile = fopen("GameLogs\\Chat.log", "a");
	if (pFile == nullptr)
		return;

	SYSTEMTIME st;
	GetLocalTime(&st);

	const char* label;
	switch (chatType)
	{
	case 0:  label = "Local"; break;
	case 1:  label = "Guild"; break;
	case 2:  label = "Global"; break;
	case 3:  label = "Alliance"; break;
	case 4:  label = "Party"; break;
	case 10: label = "GM"; break;
	case 20: label = "Whisper"; break;
	default: label = "Unknown"; break;
	}

	if (chatType == 20 && whisperTarget != nullptr)
	{
		fprintf(pFile, "[%02d:%02d:%02d] [%s] %s -> %s: %s\n",
			st.wHour, st.wMinute, st.wSecond,
			label, playerName, whisperTarget, message);
	}
	else if (chatType == 0)
	{
		fprintf(pFile, "[%02d:%02d:%02d] [%s] %s (%s): %s\n",
			st.wHour, st.wMinute, st.wSecond,
			label, playerName, mapName, message);
	}
	else
	{
		fprintf(pFile, "[%02d:%02d:%02d] [%s] %s: %s\n",
			st.wHour, st.wMinute, st.wSecond,
			label, playerName, message);
	}

	fclose(pFile);
}

void ChatLog::Shutdown()
{
}
