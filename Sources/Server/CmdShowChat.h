#pragma once

#include "ServerCommand.h"
#include <windows.h>

class CmdShowChat : public ServerCommand
{
public:
	const char* GetName() const override { return "showchat"; }
	const char* GetDescription() const override { return "Open live chat viewer in a new terminal"; }
	void Execute(CGame* pGame, const char* pArgs) override;

private:
	HANDLE m_hProcess = nullptr;
};
