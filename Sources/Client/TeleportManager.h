#pragma once

#include <array>
#include <cstring>
#include <string>

class CGame;

struct TeleportEntry
{
	int index = 0;
	std::string mapname;
	int x = 0;
	int y = 0;
	int cost = 0;
};

class teleport_manager
{
public:
	static teleport_manager& get();
	void set_game(CGame* game);
	void reset();

	// Response handlers (moved from CGame)
	void handle_teleport_list(char* data);
	void handle_charged_teleport(char* data);
	void handle_heldenian_teleport_list(char* data);

	// Teleport list access
	int get_map_count() const { return m_map_count; }
	void set_map_count(int count) { m_map_count = count; }
	auto& get_list() { return m_list; }
	const auto& get_list() const { return m_list; }

	// Crusade teleport location
	int get_loc_x() const { return m_loc_x; }
	int get_loc_y() const { return m_loc_y; }
	void set_location(int x, int y) { m_loc_x = x; m_loc_y = y; }
	const char* get_map_name() { return m_map_name.c_str(); }
	const char* get_map_name() const { return m_map_name.c_str(); }
	void set_map_name(const char* src, size_t len) { m_map_name.assign(src, len); }

	// Request state
	bool is_requested() const { return m_is_requested; }
	void set_requested(bool val) { m_is_requested = val; }

private:
	teleport_manager();
	~teleport_manager();

	CGame* m_game = nullptr;
	int m_map_count = 0;
	std::array<TeleportEntry, 20> m_list{};
	bool m_is_requested = false;
	int m_loc_x = -1;
	int m_loc_y = -1;
	std::string m_map_name;
};
