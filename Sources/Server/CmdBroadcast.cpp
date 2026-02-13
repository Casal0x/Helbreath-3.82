#include "CmdBroadcast.h"
#include "Game.h"
#include <cstdio>
#include "Log.h"

void CmdBroadcast::Execute(CGame* pGame, const char* pArgs)
{
	if (pArgs == nullptr || pArgs[0] == '\0')
	{
		hb::logger::log("Usage: broadcast <message>");
		return;
	}

	pGame->BroadcastServerMessage(pArgs);

	hb::logger::log("Broadcast sent: {}", pArgs);
}
