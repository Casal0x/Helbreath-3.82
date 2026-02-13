#pragma once

#include "ServerCommand.h"

#ifdef _WIN32
	#include <windows.h>
#endif

class CmdShowChat : public ServerCommand
{
public:
	const char* GetName() const override { return "showchat"; }
	const char* GetDescription() const override { return "Open live chat viewer in a new terminal"; }
	const char* GetHelp() const override { return "Usage: showchat\n  Opens a new terminal window that tails GameLogs/Chat.log in real-time.\n  Only one viewer can be open at a time."; }
	void Execute(CGame* pGame, const char* pArgs) override;

private:
#ifdef _WIN32
	HANDLE m_hProcess = nullptr;
#else
	int m_pid = 0;
#endif
};
