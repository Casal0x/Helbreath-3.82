// TESTER MENU — entire file is tester-only
#ifdef TESTER_ONLY
#include "DialogBox_ItemCreator.h"
#include "Game.h"
#include "GlobalDef.h"
#include "SpriteID.h"
#include "NetMessages.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include "TextInputManager.h"
#include "Item/ItemEnums.h"
#include <algorithm>
#include <format>
#include <cstring>

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
using namespace hb::shared::item;

// Layout constants
namespace layout
{
	// Shared
	constexpr int pad = 12;
	constexpr int content_x1 = 12;
	constexpr int content_x2 = 246;
	constexpr int content_w = content_x2 - content_x1;

	// Search page
	constexpr int search_bar_y = 30;
	constexpr int search_bar_h = 20;
	constexpr int list_y = 56;
	constexpr int row_h = 18;
	constexpr int list_rows = 12;
	constexpr int list_h = list_rows * row_h;
	constexpr int status_y = list_y + list_h + 2;

	// Configure page
	constexpr int item_name_y = 32;
	constexpr int item_cat_y = 48;
	constexpr int prefix_label_y = 66;
	constexpr int prefix_sel_y = 82;
	constexpr int prefix_val_y = 100;
	constexpr int effect_label_y = 122;
	constexpr int effect_sel_y = 138;
	constexpr int effect_val_y = 156;
	constexpr int preview_label_y = 182;
	constexpr int preview_text_y = 198;
	constexpr int btn_y = 232;
	constexpr int btn_w = 100;
}

DialogBox_ItemCreator::DialogBox_ItemCreator(CGame* game)
	: IDialogBox(DialogBoxId::ItemCreator, game)
{
	set_default_rect(0, 0, 258, 339);
	set_can_close_on_right_click(true);
}

void DialogBox_ItemCreator::on_disable()
{
	text_input_manager::get().end_input();
	m_initial_load = false;
	m_last_sent_search.clear();
}

DialogBox_ItemCreator::item_category DialogBox_ItemCreator::classify_item(int16_t effect_type)
{
	auto et = static_cast<ItemEffectType>(effect_type);
	if (et == ItemEffectType::AttackManaSave)
		return item_category::magic_weapon;
	if (is_attack_effect_type(et))
		return item_category::weapon;
	if (et == ItemEffectType::Defense)
		return item_category::armor;
	return item_category::none;
}

const char* DialogBox_ItemCreator::category_name(item_category cat)
{
	switch (cat)
	{
	case item_category::weapon:       return "Weapon";
	case item_category::armor:        return "Armor";
	case item_category::magic_weapon: return "Magic Weapon";
	default:                          return "Other";
	}
}

void DialogBox_ItemCreator::build_valid_options(int16_t effect_type)
{
	m_category = classify_item(effect_type);
	m_valid_prefixes.clear();
	m_valid_secondaries.clear();

	m_valid_prefixes.push_back({0, "None", 0});
	m_valid_secondaries.push_back({0, "None", 0});

	switch (m_category)
	{
	case item_category::weapon:
		m_valid_prefixes.push_back({1, "Critical", 1});
		m_valid_prefixes.push_back({2, "Poisoning", 5});
		m_valid_prefixes.push_back({3, "Righteous", 1});
		m_valid_prefixes.push_back({5, "Agile", 1});
		m_valid_prefixes.push_back({6, "Light", 4});
		m_valid_prefixes.push_back({7, "Sharp", 1});
		m_valid_prefixes.push_back({8, "Strong", 7});
		m_valid_prefixes.push_back({9, "Ancient", 1});
		m_valid_secondaries.push_back({2, "HitProb", 7});
		m_valid_secondaries.push_back({10, "ConsecAtk", 1});
		m_valid_secondaries.push_back({11, "ExpBonus", 10});
		m_valid_secondaries.push_back({12, "GoldBonus", 10});
		break;

	case item_category::armor:
		m_valid_prefixes.push_back({8, "Strong", 7});
		m_valid_prefixes.push_back({6, "Light", 4});
		m_valid_prefixes.push_back({11, "ManaConvert", 1});
		m_valid_prefixes.push_back({12, "CritChance", 1});
		m_valid_secondaries.push_back({3, "DefRatio", 7});
		m_valid_secondaries.push_back({1, "PoisonRes", 7});
		m_valid_secondaries.push_back({5, "SPRecov", 7});
		m_valid_secondaries.push_back({4, "HPRecov", 7});
		m_valid_secondaries.push_back({6, "MPRecov", 7});
		m_valid_secondaries.push_back({7, "MagicRes", 7});
		m_valid_secondaries.push_back({8, "PhysAbsorb", 3});
		m_valid_secondaries.push_back({9, "MagicAbsorb", 3});
		break;

	case item_category::magic_weapon:
		m_valid_prefixes.push_back({10, "Special", 3});
		m_valid_secondaries.push_back({2, "HitProb", 7});
		m_valid_secondaries.push_back({10, "ConsecAtk", 1});
		m_valid_secondaries.push_back({11, "ExpBonus", 10});
		m_valid_secondaries.push_back({12, "GoldBonus", 10});
		break;

	default:
		break;
	}
}

std::string DialogBox_ItemCreator::build_preview_string() const
{
	if (m_selected_index < 0 || m_selected_index >= m_result_count)
		return "";

	std::string result;

	if (m_prefix_index > 0 && m_prefix_index < static_cast<int>(m_valid_prefixes.size()))
	{
		result += m_valid_prefixes[m_prefix_index].name;
		result += " ";
	}

	result += m_results[m_selected_index].name;

	if (m_prefix_index > 0 && m_prefix_index < static_cast<int>(m_valid_prefixes.size()))
	{
		int real_val = m_prefix_value * m_valid_prefixes[m_prefix_index].multiplier;
		result += std::format(" {}", real_val);
	}

	if (m_secondary_index > 0 && m_secondary_index < static_cast<int>(m_valid_secondaries.size()))
	{
		int real_val = m_secondary_value * m_valid_secondaries[m_secondary_index].multiplier;
		result += std::format(" {} {}", m_valid_secondaries[m_secondary_index].name, real_val);
	}

	return result;
}

void DialogBox_ItemCreator::on_enter_pressed()
{
	// Live search handles everything — Enter is a no-op
}

void DialogBox_ItemCreator::receive_search_results(const hb::net::PacketNotifyTesterItemSearchResult* pkt)
{
	m_result_count = std::clamp(static_cast<int>(pkt->count), 0, 50);
	std::memcpy(m_results, pkt->entries, sizeof(m_results));
	m_selected_index = -1;
	m_scroll_offset = 0;
}

// Helper: draw a selector row  < Label >  with optional hover
void DialogBox_ItemCreator::draw_selector(int sX, int sY, int size_x, int y_offset,
	const char* label, bool hover) const
{
	auto text = std::format("<  {}  >", label);
	hb::shared::text::draw_text_aligned(GameFont::Default,
		sX, sY + y_offset, size_x, 15,
		text.c_str(),
		hb::shared::text::TextStyle::from_color(hover ? GameColors::UIWhite : GameColors::UIMagicBlue),
		hb::shared::text::Align::TopCenter);
}

// ---------------------------------------------------------------------------
// SEARCH PAGE
// ---------------------------------------------------------------------------

void DialogBox_ItemCreator::draw_search_page(short sX, short sY, short size_x, short mouse_x, short mouse_y, short z)
{
	// Title
	hb::shared::text::draw_text_aligned(GameFont::Bitmap1,
		sX, sY + 8, size_x, 15,
		"Create Item",
		hb::shared::text::TextStyle::with_integrated_shadow(GameColors::UIWarningRed),
		hb::shared::text::Align::TopCenter);

	// Text input
	if (!text_input_manager::get().is_active())
	{
		text_input_manager::get().start_input(sX + 70, sY + layout::search_bar_y + 5, 20, m_search_text);
		m_last_sx = sX;
		m_last_sy = sY;
	}
	else if (sX != m_last_sx || sY != m_last_sy)
	{
		text_input_manager::get().end_input();
		text_input_manager::get().start_input(sX + 70, sY + layout::search_bar_y + 5, 20, m_search_text);
		m_last_sx = sX;
		m_last_sy = sY;
	}

	put_string(sX + 16, sY + layout::search_bar_y + 5, "Search:", GameColors::UIWhite);

	// Live search: auto-send whenever text changes (including initial empty load)
	if (!m_initial_load || m_search_text != m_last_sent_search)
	{
		m_initial_load = true;
		m_last_sent_search = m_search_text;
		m_scroll_offset = 0;
		send_command(MsgId::CommandCommon, CommonType::TesterItemSearch,
			0, 0, 0, 0, m_search_text.empty() ? "" : m_search_text.c_str());
	}

	// Mouse wheel
	if (m_game->m_dialog_box_manager.get_top_dialog_box_index() == DialogBoxId::ItemCreator && z != 0)
	{
		m_scroll_offset -= z / 60;
		int max_scroll = std::max(0, m_result_count - layout::list_rows);
		m_scroll_offset = std::clamp(m_scroll_offset, 0, max_scroll);
	}

	// Results list
	for (int i = 0; i < layout::list_rows && (i + m_scroll_offset) < m_result_count; i++)
	{
		int idx = i + m_scroll_offset;
		auto& entry = m_results[idx];
		int ry = sY + layout::list_y + i * layout::row_h;

		bool hover = (mouse_x >= sX + layout::content_x1 && mouse_x <= sX + layout::content_x2
			&& mouse_y >= ry && mouse_y <= ry + layout::row_h - 2);

		auto color = hover ? GameColors::UIWhite : GameColors::UIMagicBlue;
		hb::shared::text::draw_text_aligned(GameFont::Default,
			sX + layout::content_x1 + 6, ry, layout::content_w - 12, 15,
			entry.name,
			hb::shared::text::TextStyle::from_color(color),
			hb::shared::text::Align::TopLeft);
	}

	// Status line
	if (m_result_count > 0)
	{
		int sy = sY + layout::status_y;
		auto count_str = std::format("{} found", m_result_count);
		put_string(sX + layout::content_x1 + 4, sy, count_str.c_str(), GameColors::UIBlack);

		if (m_result_count > layout::list_rows)
		{
			int max_scroll = m_result_count - layout::list_rows;
			auto scroll_str = std::format("[{}/{}]", m_scroll_offset + 1, max_scroll + 1);
			put_string(sX + 100, sy, scroll_str.c_str(), GameColors::UIBlack);
		}
	}

	// Close button (sprite)
	if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) &&
		(mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
	else
		draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

// ---------------------------------------------------------------------------
// CONFIGURE PAGE
// ---------------------------------------------------------------------------

void DialogBox_ItemCreator::draw_configure_page(short sX, short sY, short size_x, short mouse_x, short mouse_y)
{
	// Title
	hb::shared::text::draw_text_aligned(GameFont::Bitmap1,
		sX, sY + 8, size_x, 15,
		"Configure Item",
		hb::shared::text::TextStyle::with_integrated_shadow(GameColors::UIWarningRed),
		hb::shared::text::Align::TopCenter);

	// Item name + category
	if (m_selected_index >= 0 && m_selected_index < m_result_count)
	{
		hb::shared::text::draw_text_aligned(GameFont::Default,
			sX, sY + layout::item_name_y, size_x, 15,
			m_results[m_selected_index].name,
			hb::shared::text::TextStyle::from_color(GameColors::UIPaleYellow),
			hb::shared::text::Align::TopCenter);

		auto cat_str = std::format("({})", category_name(m_category));
		hb::shared::text::draw_text_aligned(GameFont::Default,
			sX, sY + layout::item_cat_y, size_x, 15,
			cat_str.c_str(),
			hb::shared::text::TextStyle::from_color(GameColors::UIBlack),
			hb::shared::text::Align::TopCenter);
	}

	if (m_category == item_category::none)
	{
		put_aligned_string(sX, sX + size_x, sY + layout::prefix_label_y + 10, "No attributes for this type.", GameColors::UIBlack);
		put_aligned_string(sX, sX + size_x, sY + layout::prefix_label_y + 30, "Item will be created plain.", GameColors::UIBlack);
	}
	else
	{
		// --- PREFIX SECTION ---
		put_string(sX + layout::content_x1 + 4, sY + layout::prefix_label_y, "Prefix:", GameColors::UIWhite);

		const char* prefix_name = (m_prefix_index < static_cast<int>(m_valid_prefixes.size()))
			? m_valid_prefixes[m_prefix_index].name : "None";
		bool pn_hover = (mouse_x >= sX + layout::content_x1 && mouse_x <= sX + layout::content_x2
			&& mouse_y >= sY + layout::prefix_sel_y && mouse_y <= sY + layout::prefix_sel_y + 14);
		draw_selector(sX, sY, size_x, layout::prefix_sel_y, prefix_name, pn_hover);

		// Prefix value
		if (m_prefix_index > 0 && m_prefix_index < static_cast<int>(m_valid_prefixes.size()))
		{
			int mult = m_valid_prefixes[m_prefix_index].multiplier;
			int real_val = m_prefix_value * mult;
			bool pv_hover = (mouse_x >= sX + layout::content_x1 && mouse_x <= sX + layout::content_x2
				&& mouse_y >= sY + layout::prefix_val_y && mouse_y <= sY + layout::prefix_val_y + 14);
			auto val_str = std::to_string(real_val);
			draw_selector(sX, sY, size_x, layout::prefix_val_y, val_str.c_str(), pv_hover);
		}

		// --- EFFECT SECTION ---
		put_string(sX + layout::content_x1 + 4, sY + layout::effect_label_y, "Effect:", GameColors::UIWhite);

		const char* sec_name = (m_secondary_index < static_cast<int>(m_valid_secondaries.size()))
			? m_valid_secondaries[m_secondary_index].name : "None";
		bool sn_hover = (mouse_x >= sX + layout::content_x1 && mouse_x <= sX + layout::content_x2
			&& mouse_y >= sY + layout::effect_sel_y && mouse_y <= sY + layout::effect_sel_y + 14);
		draw_selector(sX, sY, size_x, layout::effect_sel_y, sec_name, sn_hover);

		// Secondary value
		if (m_secondary_index > 0 && m_secondary_index < static_cast<int>(m_valid_secondaries.size()))
		{
			int mult = m_valid_secondaries[m_secondary_index].multiplier;
			int real_val = m_secondary_value * mult;
			bool sv_hover = (mouse_x >= sX + layout::content_x1 && mouse_x <= sX + layout::content_x2
				&& mouse_y >= sY + layout::effect_val_y && mouse_y <= sY + layout::effect_val_y + 14);
			auto val_str = std::to_string(real_val);
			draw_selector(sX, sY, size_x, layout::effect_val_y, val_str.c_str(), sv_hover);
		}

		// --- PREVIEW ---
		auto preview = build_preview_string();
		if (!preview.empty())
		{
			put_string(sX + layout::content_x1 + 4, sY + layout::preview_label_y, "Preview:", GameColors::UIWhite);
			hb::shared::text::draw_text_aligned(GameFont::Default,
				sX + layout::content_x1 + 4, sY + layout::preview_text_y, layout::content_w - 8, 15,
				preview.c_str(),
				hb::shared::text::TextStyle::from_color(GameColors::UIPaleYellow),
				hb::shared::text::Align::TopLeft);
		}
	}

	// --- BUTTONS ---
	int left_btn_x = sX + layout::content_x1 + 6;
	int right_btn_x = sX + layout::content_x2 - layout::btn_w - 6;

	bool create_hover = (mouse_x >= left_btn_x && mouse_x <= left_btn_x + layout::btn_w
		&& mouse_y >= sY + layout::btn_y && mouse_y <= sY + layout::btn_y + 18);
	hb::shared::text::draw_text_aligned(GameFont::Default,
		left_btn_x, sY + layout::btn_y, layout::btn_w, 15,
		"[Create]",
		hb::shared::text::TextStyle::from_color(create_hover ? GameColors::UIWhite : GameColors::UIMagicBlue),
		hb::shared::text::Align::TopCenter);

	bool back_hover = (mouse_x >= right_btn_x && mouse_x <= right_btn_x + layout::btn_w
		&& mouse_y >= sY + layout::btn_y && mouse_y <= sY + layout::btn_y + 18);
	hb::shared::text::draw_text_aligned(GameFont::Default,
		right_btn_x, sY + layout::btn_y, layout::btn_w, 15,
		"[<< Back]",
		hb::shared::text::TextStyle::from_color(back_hover ? GameColors::UIWhite : GameColors::UIMagicBlue),
		hb::shared::text::Align::TopCenter);

	// Close button (sprite)
	if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) &&
		(mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
	else
		draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

void DialogBox_ItemCreator::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
	short sX = Info().m_x;
	short sY = Info().m_y;
	short size_x = Info().m_size_x;

	draw_new_dialog_box(InterfaceNdGame2, sX, sY, 0);

	if (m_page == 0)
		draw_search_page(sX, sY, size_x, mouse_x, mouse_y, z);
	else
		draw_configure_page(sX, sY, size_x, mouse_x, mouse_y);
}

// ---------------------------------------------------------------------------
// CLICK HANDLERS
// ---------------------------------------------------------------------------

bool DialogBox_ItemCreator::on_click_search(short sX, short sY, short size_x, short mouse_x, short mouse_y)
{
	// Results list — clicking goes directly to configure
	for (int i = 0; i < layout::list_rows && (i + m_scroll_offset) < m_result_count; i++)
	{
		int idx = i + m_scroll_offset;
		int ry = sY + layout::list_y + i * layout::row_h;
		if (mouse_x >= sX + layout::content_x1 && mouse_x <= sX + layout::content_x2
			&& mouse_y >= ry && mouse_y <= ry + layout::row_h - 2)
		{
			m_selected_index = idx;
			text_input_manager::get().end_input();
			build_valid_options(m_results[idx].effect_type);
			m_prefix_index = 0;
			m_prefix_value = 1;
			m_secondary_index = 0;
			m_secondary_value = 1;
			m_page = 1;
			play_sound_effect('E', 14, 5);
			return true;
		}
	}

	// Close button
	if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) &&
		(mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		disable_this_dialog();
		play_sound_effect('E', 14, 5);
		return true;
	}

	return false;
}

bool DialogBox_ItemCreator::on_click_configure(short sX, short sY, short size_x, short mouse_x, short mouse_y)
{
	if (m_category != item_category::none)
	{
		// Prefix name click
		if (mouse_x >= sX + layout::content_x1 && mouse_x <= sX + layout::content_x2
			&& mouse_y >= sY + layout::prefix_sel_y && mouse_y <= sY + layout::prefix_sel_y + 14)
		{
			if (!m_valid_prefixes.empty())
				m_prefix_index = (m_prefix_index + 1) % static_cast<int>(m_valid_prefixes.size());
			m_prefix_value = 1;
			play_sound_effect('E', 14, 5);
			return true;
		}

		// Prefix value click
		if (m_prefix_index > 0
			&& mouse_x >= sX + layout::content_x1 && mouse_x <= sX + layout::content_x2
			&& mouse_y >= sY + layout::prefix_val_y && mouse_y <= sY + layout::prefix_val_y + 14)
		{
			m_prefix_value = (m_prefix_value % max_value) + 1;
			play_sound_effect('E', 14, 5);
			return true;
		}

		// Secondary name click
		if (mouse_x >= sX + layout::content_x1 && mouse_x <= sX + layout::content_x2
			&& mouse_y >= sY + layout::effect_sel_y && mouse_y <= sY + layout::effect_sel_y + 14)
		{
			if (!m_valid_secondaries.empty())
				m_secondary_index = (m_secondary_index + 1) % static_cast<int>(m_valid_secondaries.size());
			m_secondary_value = 1;
			play_sound_effect('E', 14, 5);
			return true;
		}

		// Secondary value click
		if (m_secondary_index > 0
			&& mouse_x >= sX + layout::content_x1 && mouse_x <= sX + layout::content_x2
			&& mouse_y >= sY + layout::effect_val_y && mouse_y <= sY + layout::effect_val_y + 14)
		{
			m_secondary_value = (m_secondary_value % max_value) + 1;
			play_sound_effect('E', 14, 5);
			return true;
		}
	}

	// Create button
	int left_btn_x = sX + layout::content_x1 + 6;
	if (mouse_x >= left_btn_x && mouse_x <= left_btn_x + layout::btn_w
		&& mouse_y >= sY + layout::btn_y && mouse_y <= sY + layout::btn_y + 18)
	{
		if (m_selected_index >= 0 && m_selected_index < m_result_count)
		{
			int item_id = m_results[m_selected_index].item_id;

			int prefix_type = (m_prefix_index < static_cast<int>(m_valid_prefixes.size()))
				? m_valid_prefixes[m_prefix_index].type : 0;
			int secondary_type = (m_secondary_index < static_cast<int>(m_valid_secondaries.size()))
				? m_valid_secondaries[m_secondary_index].type : 0;
			int pval = (prefix_type != 0) ? m_prefix_value : 0;
			int sval = (secondary_type != 0) ? m_secondary_value : 0;

			uint32_t attr = build_attribute(
				false,
				static_cast<AttributePrefixType>(prefix_type),
				static_cast<uint8_t>(pval),
				static_cast<SecondaryEffectType>(secondary_type),
				static_cast<uint8_t>(sval),
				0);
			send_command(MsgId::CommandCommon, CommonType::TesterCreateItem,
				0, item_id, static_cast<int>(attr), 0, 0);
		}
		play_sound_effect('E', 14, 5);
		return true;
	}

	// Back button
	int right_btn_x = sX + layout::content_x2 - layout::btn_w - 6;
	if (mouse_x >= right_btn_x && mouse_x <= right_btn_x + layout::btn_w
		&& mouse_y >= sY + layout::btn_y && mouse_y <= sY + layout::btn_y + 18)
	{
		m_page = 0;
		text_input_manager::get().start_input(sX + 70, sY + layout::search_bar_y + 5, 20, m_search_text);
		m_last_sx = sX;
		m_last_sy = sY;
		play_sound_effect('E', 14, 5);
		return true;
	}

	// Close button
	if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) &&
		(mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		disable_this_dialog();
		play_sound_effect('E', 14, 5);
		return true;
	}

	return false;
}

bool DialogBox_ItemCreator::on_click(short mouse_x, short mouse_y)
{
	short sX = Info().m_x;
	short sY = Info().m_y;
	short size_x = Info().m_size_x;

	if (m_page == 0)
		return on_click_search(sX, sY, size_x, mouse_x, mouse_y);
	else
		return on_click_configure(sX, sY, size_x, mouse_x, mouse_y);
}
#endif // TESTER_ONLY
