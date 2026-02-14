#pragma once

#include <array>
#include <memory>
#include "GameConstants.h"

class CItem;
class CGame;

class shop_manager
{
public:
	static shop_manager& get();
	void set_game(CGame* game);

	void request_shop_menu(char type);
	void handle_response(char* data);

	void clear_items();
	bool has_items() const;

	auto& get_item_list() { return m_item_list; }
	int16_t get_pending_shop_type() const { return m_pending_shop_type; }
	void set_pending_shop_type(int16_t type) { m_pending_shop_type = type; }

private:
	shop_manager();
	~shop_manager();

	void send_request(int16_t npcType);

	CGame* m_game = nullptr;
	std::array<std::unique_ptr<CItem>, game_limits::max_menu_items> m_item_list;
	int16_t m_pending_shop_type = 0;
};
