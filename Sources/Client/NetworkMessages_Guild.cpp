#include "Game.h"
#include "GuildManager.h"

namespace NetworkMessageHandlers {

void HandleCreateNewGuildResponse(CGame* pGame, char* pData)
{
	pGame->m_guildManager.HandleCreateNewGuildResponse(pData);
}

void HandleDisbandGuildResponse(CGame* pGame, char* pData)
{
	pGame->m_guildManager.HandleDisbandGuildResponse(pData);
}

void HandleGuildDisbanded(CGame* pGame, char* pData)
{
	pGame->m_guildManager.HandleGuildDisbanded(pData);
}

void HandleNewGuildsMan(CGame* pGame, char* pData)
{
	pGame->m_guildManager.HandleNewGuildsMan(pData);
}

void HandleDismissGuildsMan(CGame* pGame, char* pData)
{
	pGame->m_guildManager.HandleDismissGuildsMan(pData);
}

void HandleCannotJoinMoreGuildsMan(CGame* pGame, char* pData)
{
	pGame->m_guildManager.HandleCannotJoinMoreGuildsMan(pData);
}

void HandleJoinGuildApprove(CGame* pGame, char* pData)
{
	pGame->m_guildManager.HandleJoinGuildApprove(pData);
}

void HandleJoinGuildReject(CGame* pGame, char* pData)
{
	pGame->m_guildManager.HandleJoinGuildReject(pData);
}

void HandleDismissGuildApprove(CGame* pGame, char* pData)
{
	pGame->m_guildManager.HandleDismissGuildApprove(pData);
}

void HandleDismissGuildReject(CGame* pGame, char* pData)
{
	pGame->m_guildManager.HandleDismissGuildReject(pData);
}

void HandleQueryJoinGuildPermission(CGame* pGame, char* pData)
{
	pGame->m_guildManager.HandleQueryJoinGuildPermission(pData);
}

void HandleQueryDismissGuildPermission(CGame* pGame, char* pData)
{
	pGame->m_guildManager.HandleQueryDismissGuildPermission(pData);
}

void HandleReqGuildNameAnswer(CGame* pGame, char* pData)
{
	pGame->m_guildManager.HandleReqGuildNameAnswer(pData);
}

void HandleNoGuildMasterLevel(CGame* pGame, char* pData)
{
	pGame->m_guildManager.HandleNoGuildMasterLevel(pData);
}

void HandleSuccessBanGuildMan(CGame* pGame, char* pData)
{
	pGame->m_guildManager.HandleSuccessBanGuildMan(pData);
}

void HandleCannotBanGuildMan(CGame* pGame, char* pData)
{
	pGame->m_guildManager.HandleCannotBanGuildMan(pData);
}

} // namespace NetworkMessageHandlers
