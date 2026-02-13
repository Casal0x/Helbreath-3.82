#include "CmdShowChat.h"
#include <cstdio>
#include <filesystem>
#include "Log.h"

#ifdef _WIN32

void CmdShowChat::Execute(CGame* pGame, const char* pArgs)
{
	if (m_hProcess != nullptr)
	{
		DWORD dwExitCode = 0;
		if (GetExitCodeProcess(m_hProcess, &dwExitCode) && dwExitCode == STILL_ACTIVE)
		{
			hb::logger::log("Chat viewer is already open.");
			return;
		}
		CloseHandle(m_hProcess);
		m_hProcess = nullptr;
	}

	std::string logPath = std::filesystem::absolute("gamelogs/chat.log").string();

	char szCmdLine[1024];
	std::snprintf(szCmdLine, sizeof(szCmdLine),
		"cmd.exe /c \"title HB Chat && powershell -NoProfile -ExecutionPolicy Bypass -Command \"\"Get-Content '%s' -Wait -Tail 0\"\"\"",
		logPath.c_str());

	STARTUPINFOA si = {};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi = {};

	BOOL bResult = CreateProcessA(
		nullptr,
		szCmdLine,
		nullptr,
		nullptr,
		FALSE,
		CREATE_NEW_CONSOLE,
		nullptr,
		nullptr,
		&si,
		&pi
	);

	if (bResult)
	{
		CloseHandle(pi.hThread);
		m_hProcess = pi.hProcess;
		hb::logger::log("Chat viewer opened in new terminal.");
	}
	else
	{
		hb::logger::log("Failed to open chat viewer (error {}).", GetLastError());
	}
}

#else // POSIX

void CmdShowChat::Execute(CGame* pGame, const char* pArgs)
{
	std::string logPath = std::filesystem::absolute("gamelogs/chat.log").string();
	std::string cmd = "tail -f \"" + logPath + "\" &";
	int ret = std::system(cmd.c_str());
	if (ret == 0)
		hb::logger::log("Chat viewer started (tail -f).");
	else
		hb::logger::log("Failed to open chat viewer (exit code {}).", ret);
}

#endif
