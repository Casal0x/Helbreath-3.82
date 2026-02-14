#include "Game.h"
#include "GuildManager.h"

namespace NetworkMessageHandlers {

void handle_create_new_guild_response(CGame* game, char* data)
{
	game->m_guild_manager.handle_create_new_guild_response(data);
}

void handle_disband_guild_response(CGame* game, char* data)
{
	game->m_guild_manager.handle_disband_guild_response(data);
}

void handle_guild_disbanded(CGame* game, char* data)
{
	game->m_guild_manager.handle_guild_disbanded(data);
}

void handle_new_guilds_man(CGame* game, char* data)
{
	game->m_guild_manager.handle_new_guilds_man(data);
}

void handle_dismiss_guilds_man(CGame* game, char* data)
{
	game->m_guild_manager.handle_dismiss_guilds_man(data);
}

void handle_cannot_join_more_guilds_man(CGame* game, char* data)
{
	game->m_guild_manager.handle_cannot_join_more_guilds_man(data);
}

void handle_join_guild_approve(CGame* game, char* data)
{
	game->m_guild_manager.handle_join_guild_approve(data);
}

void handle_join_guild_reject(CGame* game, char* data)
{
	game->m_guild_manager.handle_join_guild_reject(data);
}

void handle_dismiss_guild_approve(CGame* game, char* data)
{
	game->m_guild_manager.handle_dismiss_guild_approve(data);
}

void handle_dismiss_guild_reject(CGame* game, char* data)
{
	game->m_guild_manager.handle_dismiss_guild_reject(data);
}

void handle_query_join_guild_permission(CGame* game, char* data)
{
	game->m_guild_manager.handle_query_join_guild_permission(data);
}

void handle_query_dismiss_guild_permission(CGame* game, char* data)
{
	game->m_guild_manager.handle_query_dismiss_guild_permission(data);
}

void handle_req_guild_name_answer(CGame* game, char* data)
{
	game->m_guild_manager.handle_req_guild_name_answer(data);
}

void handle_no_guild_master_level(CGame* game, char* data)
{
	game->m_guild_manager.handle_no_guild_master_level(data);
}

void handle_success_ban_guild_man(CGame* game, char* data)
{
	game->m_guild_manager.handle_success_ban_guild_man(data);
}

void handle_cannot_ban_guild_man(CGame* game, char* data)
{
	game->m_guild_manager.handle_cannot_ban_guild_man(data);
}

} // namespace NetworkMessageHandlers
