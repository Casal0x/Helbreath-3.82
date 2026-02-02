#include <windows.h>
#include "CmdShowChat.h"
#include "winmain.h"
#include <cstdio>

void CmdShowChat::Execute(CGame* pGame, const char* pArgs)
{
	if (m_hProcess != nullptr)
	{
		DWORD dwExitCode = 0;
		if (GetExitCodeProcess(m_hProcess, &dwExitCode) && dwExitCode == STILL_ACTIVE)
		{
			PutLogList((char*)"(!) Chat viewer is already open.");
			return;
		}
		CloseHandle(m_hProcess);
		m_hProcess = nullptr;
	}

	char szLogPath[MAX_PATH];
	GetFullPathNameA("GameLogs\\Chat.log", MAX_PATH, szLogPath, nullptr);

	char szCmdLine[1024];
	std::snprintf(szCmdLine, sizeof(szCmdLine),
		"cmd.exe /c \"title HB Chat && powershell -NoProfile -ExecutionPolicy Bypass -Command \"\"Get-Content '%s' -Wait -Tail 0\"\"\"",
		szLogPath);

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
		PutLogList((char*)"(!) Chat viewer opened in new terminal.");
	}
	else
	{
		char szError[256];
		std::snprintf(szError, sizeof(szError), "(!) Failed to open chat viewer (error %lu).", GetLastError());
		PutLogList(szError);
	}
}
