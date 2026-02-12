#pragma once

#include <array>
#include <cstring>
#include <string>

class CGame;

struct TeleportEntry
{
	int iIndex = 0;
	std::string mapname;
	int iX = 0;
	int iY = 0;
	int iCost = 0;
};

class TeleportManager
{
public:
	static TeleportManager& Get();
	void SetGame(CGame* pGame);
	void Reset();

	// Response handlers (moved from CGame)
	void HandleTeleportList(char* pData);
	void HandleChargedTeleport(char* pData);
	void HandleHeldenianTeleportList(char* pData);

	// Teleport list access
	int GetMapCount() const { return m_map_count; }
	void SetMapCount(int count) { m_map_count = count; }
	auto& GetList() { return m_list; }
	const auto& GetList() const { return m_list; }

	// Crusade teleport location
	int GetLocX() const { return m_loc_x; }
	int GetLocY() const { return m_loc_y; }
	void SetLocation(int x, int y) { m_loc_x = x; m_loc_y = y; }
	const char* GetMapName() { return m_map_name.c_str(); }
	const char* GetMapName() const { return m_map_name.c_str(); }
	void SetMapName(const char* src, size_t len) { m_map_name.assign(src, len); }

	// Request state
	bool IsRequested() const { return m_is_requested; }
	void SetRequested(bool val) { m_is_requested = val; }

private:
	TeleportManager();
	~TeleportManager();

	CGame* m_game = nullptr;
	int m_map_count = 0;
	std::array<TeleportEntry, 20> m_list{};
	bool m_is_requested = false;
	int m_loc_x = -1;
	int m_loc_y = -1;
	std::string m_map_name;
};
