// guild_manager.h: Handles client-side guild network messages.
// Extracted from NetworkMessages_Guild.cpp (Phase B4).

#pragma once

class CGame;

class guild_manager
{
public:
	guild_manager() = default;
	~guild_manager() = default;

	void set_game(CGame* game) { m_game = game; }

	// Guild creation/disband responses
	void handle_create_new_guild_response(char* data);
	void handle_disband_guild_response(char* data);

	// Guild notification handlers
	void handle_guild_disbanded(char* data);
	void handle_new_guilds_man(char* data);
	void handle_dismiss_guilds_man(char* data);
	void handle_cannot_join_more_guilds_man(char* data);

	// Guild membership responses
	void handle_join_guild_approve(char* data);
	void handle_join_guild_reject(char* data);
	void handle_dismiss_guild_approve(char* data);
	void handle_dismiss_guild_reject(char* data);

	// Guild queries
	void handle_query_join_guild_permission(char* data);
	void handle_query_dismiss_guild_permission(char* data);
	void handle_req_guild_name_answer(char* data);

	// Simple notification handlers
	void handle_no_guild_master_level(char* data);
	void handle_success_ban_guild_man(char* data);
	void handle_cannot_ban_guild_man(char* data);

private:
	static void update_location_flags(CGame* game, const char* location);
	CGame* m_game = nullptr;
};
