// GuildManager.h: Handles client-side guild network messages.
// Extracted from NetworkMessages_Guild.cpp (Phase B4).

#pragma once

class CGame;

class GuildManager
{
public:
	GuildManager() = default;
	~GuildManager() = default;

	void SetGame(CGame* pGame) { m_pGame = pGame; }

	// Guild creation/disband responses
	void HandleCreateNewGuildResponse(char* pData);
	void HandleDisbandGuildResponse(char* pData);

	// Guild notification handlers
	void HandleGuildDisbanded(char* pData);
	void HandleNewGuildsMan(char* pData);
	void HandleDismissGuildsMan(char* pData);
	void HandleCannotJoinMoreGuildsMan(char* pData);

	// Guild membership responses
	void HandleJoinGuildApprove(char* pData);
	void HandleJoinGuildReject(char* pData);
	void HandleDismissGuildApprove(char* pData);
	void HandleDismissGuildReject(char* pData);

	// Guild queries
	void HandleQueryJoinGuildPermission(char* pData);
	void HandleQueryDismissGuildPermission(char* pData);
	void HandleReqGuildNameAnswer(char* pData);

	// Simple notification handlers
	void HandleNoGuildMasterLevel(char* pData);
	void HandleSuccessBanGuildMan(char* pData);
	void HandleCannotBanGuildMan(char* pData);

private:
	static void UpdateLocationFlags(CGame* pGame, const char* cLocation);
	CGame* m_pGame = nullptr;
};
