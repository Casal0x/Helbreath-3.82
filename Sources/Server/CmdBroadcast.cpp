#include "CmdBroadcast.h"
#include "Game.h"
#include <cstdio>
#include "Log.h"

void CmdBroadcast::execute(CGame* game, const char* args)
{
	if (args == nullptr || args[0] == '\0')
	{
		hb::logger::log("Usage: broadcast <message>");
		return;
	}

	game->broadcast_server_message(args);

	hb::logger::log("Broadcast sent: {}", args);
}
