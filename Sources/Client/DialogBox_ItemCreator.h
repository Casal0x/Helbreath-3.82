// TESTER MENU — entire file is debug-only
#pragma once
#ifdef _DEBUG
#include "IDialogBox.h"
#include "Packet/PacketNotify.h"
#include "Item/ItemAttributes.h"
#include <string>
#include <vector>

class DialogBox_ItemCreator : public IDialogBox
{
public:
	DialogBox_ItemCreator(CGame* game);
	~DialogBox_ItemCreator() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
	void on_disable() override;

	void receive_search_results(const hb::net::PacketNotifyTesterItemSearchResult* pkt);
	void on_enter_pressed();

private:
	enum class item_category { none, weapon, armor, magic_weapon };

	int m_page = 0;

	// Search state
	std::string m_search_text;
	std::string m_last_sent_search;
	int m_result_count = 0;
	hb::net::TesterItemSearchEntry m_results[50]{};
	int m_selected_index = -1;
	int m_scroll_offset = 0;
	bool m_initial_load = false;

	// Track dialog position to fix cursor drift
	short m_last_sx = 0;
	short m_last_sy = 0;

	// Attribute configuration
	int m_prefix_index = 0;
	int m_prefix_value = 1;
	int m_secondary_index = 0;
	int m_secondary_value = 1;
	static constexpr int max_value = 13;

	item_category m_category = item_category::none;
	struct attr_option
	{
		int type;
		const char* name;
		int multiplier;
	};
	std::vector<attr_option> m_valid_prefixes;
	std::vector<attr_option> m_valid_secondaries;

	void build_valid_options(int16_t effect_type);
	static item_category classify_item(int16_t effect_type);
	static const char* category_name(item_category cat);
	std::string build_preview_string() const;

	void draw_search_page(short sX, short sY, short size_x, short mouse_x, short mouse_y, short z);
	void draw_configure_page(short sX, short sY, short size_x, short mouse_x, short mouse_y);
	bool on_click_search(short sX, short sY, short size_x, short mouse_x, short mouse_y);
	bool on_click_configure(short sX, short sY, short size_x, short mouse_x, short mouse_y);

	// UI helpers
	void draw_selector(int sX, int sY, int size_x, int y_offset, const char* label, bool hover) const;
};
#endif // _DEBUG
