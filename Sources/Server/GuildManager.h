// GuildManager.h: Handles server-side guild operations.
// Extracted from Game.cpp (Phase B4).

#pragma once

#include <cstddef>
#include <cstdint>

class CGame;

class GuildManager
{
public:
	GuildManager() = default;
	~GuildManager() = default;

	void set_game(CGame* game) { m_game = game; }

	// Guild creation/disband (network handlers)
	void request_create_new_guild_handler(int client_h, char* data, size_t msg_size);
	void response_create_new_guild_handler(char* data, int type);
	void request_disband_guild_handler(int client_h, char* data, size_t msg_size);
	void response_disband_guild_handler(char* data, int type);

	// Guild membership
	void join_guild_approve_handler(int client_h, const char* name);
	void join_guild_reject_handler(int client_h, const char* name);
	void dismiss_guild_approve_handler(int client_h, const char* name);
	void dismiss_guild_reject_handler(int client_h, const char* name);

	// Guild messaging
	void send_guild_msg(int client_h, uint16_t notify_msg_type, short v1, short v2, char* string);
	void guild_notify_handler(char* data, size_t msg_size);

	// Guild user commands
	void user_command_ban_guildsman(int client_h, char* data, size_t msg_size);
	void user_command_dismiss_guild(int client_h, char* data, size_t msg_size);

	// File-based guild management
	void request_create_new_guild(int client_h, char* data);
	void request_disband_guild(int client_h, char* data);

	// Guild info
	void request_guild_name_handler(int client_h, int object_id, int index);

private:
	CGame* m_game = nullptr;
};
