#include "CmdShutdown.h"
#include "ServerConsole.h"
#include "Game.h"
#include "Log.h"
#include "ServerLogChannels.h"
#include "NetMessages.h"
#include "TimeUtils.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>

using namespace hb::shared::net;
using namespace hb::server::config;

void CmdShutdown::execute(CGame* game, const char* args)
{
	int delay_seconds = 0;
	const char* message = nullptr;

	// Parse: shutdown [seconds] [message]
	if (args != nullptr && args[0] != '\0')
	{
		char* end = nullptr;
		long val = std::strtol(args, &end, 10);
		if (end != args && val >= 0)
		{
			delay_seconds = static_cast<int>(val);

			// Skip whitespace after number to find message
			while (*end == ' ' || *end == '\t')
				end++;

			if (*end != '\0')
				message = end;
		}
		else
		{
			hb::console::error("Usage: shutdown [seconds] [message]");
			return;
		}
	}

	hb::console::info("Initiating graceful shutdown...");
	hb::logger::log<hb::log_channel::commands>("shutdown: initiated (delay={}s)", delay_seconds);

	// Save all players immediately as a safety snapshot
	int count = game->save_all_players();
	hb::console::success("Saved {} player(s)", count);

	// Send shutdown notice dialog to all connected players (mode=1 = warning)
	for (int i = 1; i < MaxClients; i++)
	{
		if (game->m_client_list[i] != nullptr && game->m_client_list[i]->m_is_init_complete)
			game->send_notify_msg(0, i, Notify::ServerShutdown, 1, 0, 0, nullptr);
	}

	// Send broadcast message with timing info and custom text
	char buf[512];
	if (delay_seconds > 0)
	{
		if (message != nullptr)
			std::snprintf(buf, sizeof(buf), "Server shutting down in %d seconds. %s", delay_seconds, message);
		else
			std::snprintf(buf, sizeof(buf), "Server shutting down in %d seconds.", delay_seconds);
	}
	else
	{
		if (message != nullptr)
			std::snprintf(buf, sizeof(buf), "Server shutting down now. %s", message);
		else
			std::snprintf(buf, sizeof(buf), "Server shutting down now.");
	}
	game->broadcast_server_message(buf);

	if (delay_seconds > 0)
	{
		// Schedule delayed shutdown
		game->m_shutdown_start_time = GameClock::GetTimeMS();
		game->m_shutdown_delay_ms = static_cast<uint32_t>(delay_seconds) * 1000;
		hb::console::info("Shutdown scheduled in {} seconds.", delay_seconds);
	}
	else
	{
		// Immediate shutdown
		game->m_on_exit_process = true;
		game->m_exit_process_time = GameClock::GetTimeMS();
		hb::console::info("Disconnecting players and shutting down...");
	}
}
