#include "ChatLog.h"
#include <cstdio>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <ctime>

extern void PutLogList(char* cMsg);

static const char* const CHATLOG_DIR = "GameLogs";
static const char* const CHATLOG_FILE = "GameLogs/Chat.log";

ChatLog& ChatLog::Get()
{
	static ChatLog instance;
	return instance;
}

void ChatLog::Initialize()
{
	if (m_bInitialized)
		return;

	std::filesystem::create_directories(CHATLOG_DIR);

	std::ofstream ofs(CHATLOG_FILE, std::ios::trunc);
	if (ofs.is_open())
	{
		ofs.close();
		PutLogList((char*)"(!) Chat log initialized: GameLogs/Chat.log");
	}
	else
	{
		PutLogList((char*)"(!) WARNING: Could not create GameLogs/Chat.log");
	}

	m_bInitialized = true;
}

void ChatLog::Write(int chatType, const char* playerName, const char* mapName, const char* message, const char* whisperTarget)
{
	if (!m_bInitialized)
		return;

	std::ofstream ofs(CHATLOG_FILE, std::ios::app);
	if (!ofs.is_open())
		return;

	auto now = std::chrono::system_clock::now();
	std::time_t tt = std::chrono::system_clock::to_time_t(now);
	struct tm tmLocal;
#ifdef _WIN32
	localtime_s(&tmLocal, &tt);
#else
	localtime_r(&tt, &tmLocal);
#endif

	const char* label;
	switch (chatType)
	{
	case 0:  label = "Local"; break;
	case 1:  label = "Guild"; break;
	case 2:  label = "Global"; break;
	case 3:  label = "Alliance"; break;
	case 4:  label = "Party"; break;
	case 10: label = "Broadcast"; break;
	case 20: label = "Whisper"; break;
	default: label = "Unknown"; break;
	}

	char line[512];
	if (chatType == 20 && whisperTarget != nullptr)
	{
		std::snprintf(line, sizeof(line), "[%02d:%02d:%02d] [%s] %s -> %s: %s",
			tmLocal.tm_hour, tmLocal.tm_min, tmLocal.tm_sec,
			label, playerName, whisperTarget, message);
	}
	else if (chatType == 0)
	{
		std::snprintf(line, sizeof(line), "[%02d:%02d:%02d] [%s] %s (%s): %s",
			tmLocal.tm_hour, tmLocal.tm_min, tmLocal.tm_sec,
			label, playerName, mapName, message);
	}
	else
	{
		std::snprintf(line, sizeof(line), "[%02d:%02d:%02d] [%s] %s: %s",
			tmLocal.tm_hour, tmLocal.tm_min, tmLocal.tm_sec,
			label, playerName, message);
	}

	ofs << line << '\n';
}

void ChatLog::Shutdown()
{
}
