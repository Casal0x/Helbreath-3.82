#include "DialogBox_Exchange.h"
#include "CursorTarget.h"
#include "Game.h"
#include "ItemNameFormatter.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include "lan_eng.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::shared::item;
using namespace hb::client::sprite_id;

DialogBox_Exchange::DialogBox_Exchange(CGame* game)
	: IDialogBox(DialogBoxId::Exchange, game)
{
	set_default_rect(100 , 30 , 520, 357);
}

void DialogBox_Exchange::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
	if (!m_game->ensure_item_configs_loaded()) return;
	short sX = Info().m_x;
	short sY = Info().m_y;
	short size_x = Info().m_size_x;

	m_game->draw_new_dialog_box(InterfaceNdNewExchange, sX, sY, 0);

	switch (Info().m_mode) {
	case 1: // Not yet confirmed exchange
		put_aligned_string(sX + 80, sX + 180, sY + 38, m_game->m_player->m_player_name.c_str(), GameColors::UIDarkGreen);
		if (m_game->m_dialog_box_exchange_info[4].v1 != -1)
			put_aligned_string(sX + 250, sX + 540, sY + 38, m_game->m_dialog_box_exchange_info[4].str2.c_str(), GameColors::UIDarkGreen);

		draw_items(sX, sY, mouse_x, mouse_y, 0, 8);

		if ((m_game->m_dialog_box_exchange_info[0].v1 != -1) && (m_game->m_dialog_box_exchange_info[4].v1 == -1)) {
			put_aligned_string(sX, sX + size_x, sY + 245, DRAW_DIALOGBOX_EXCHANGE9, GameColors::UILabel);
			put_aligned_string(sX, sX + size_x, sY + 260, DRAW_DIALOGBOX_EXCHANGE10, GameColors::UILabel);
			put_aligned_string(sX, sX + size_x, sY + 275, DRAW_DIALOGBOX_EXCHANGE11, GameColors::UILabel);
			hb::shared::text::draw_text(GameFont::Bitmap1, sX + 220, sY + 310, "Exchange", hb::shared::text::TextStyle::with_highlight(GameColors::BmpBtnHover));
		}
		else if ((m_game->m_dialog_box_exchange_info[0].v1 == -1) && (m_game->m_dialog_box_exchange_info[4].v1 != -1)) {
			put_aligned_string(sX, sX + size_x, sY + 215, DRAW_DIALOGBOX_EXCHANGE12, GameColors::UILabel);
			put_aligned_string(sX, sX + size_x, sY + 230, DRAW_DIALOGBOX_EXCHANGE13, GameColors::UILabel);
			put_aligned_string(sX, sX + size_x, sY + 245, DRAW_DIALOGBOX_EXCHANGE14, GameColors::UILabel);
			put_aligned_string(sX, sX + size_x, sY + 260, DRAW_DIALOGBOX_EXCHANGE15, GameColors::UILabel);
			put_aligned_string(sX, sX + size_x, sY + 275, DRAW_DIALOGBOX_EXCHANGE16, GameColors::UILabel);
			put_aligned_string(sX, sX + size_x, sY + 290, DRAW_DIALOGBOX_EXCHANGE17, GameColors::UILabel);
			hb::shared::text::draw_text(GameFont::Bitmap1, sX + 220, sY + 310, "Exchange", hb::shared::text::TextStyle::with_highlight(GameColors::BmpBtnHover));
		}
		else if ((m_game->m_dialog_box_exchange_info[0].v1 != -1) && (m_game->m_dialog_box_exchange_info[4].v1 != -1)) {
			put_aligned_string(sX, sX + size_x, sY + 215, DRAW_DIALOGBOX_EXCHANGE18, GameColors::UILabel);
			put_aligned_string(sX, sX + size_x, sY + 230, DRAW_DIALOGBOX_EXCHANGE19, GameColors::UILabel);
			put_aligned_string(sX, sX + size_x, sY + 245, DRAW_DIALOGBOX_EXCHANGE20, GameColors::UILabel);
			put_aligned_string(sX, sX + size_x, sY + 260, DRAW_DIALOGBOX_EXCHANGE21, GameColors::UILabel);
			put_aligned_string(sX, sX + size_x, sY + 275, DRAW_DIALOGBOX_EXCHANGE22, GameColors::UILabel);
			put_aligned_string(sX, sX + size_x, sY + 290, DRAW_DIALOGBOX_EXCHANGE23, GameColors::UILabel);
			if ((mouse_x >= sX + 200) && (mouse_x <= sX + 200 + ui_layout::btn_size_x) && (mouse_y >= sY + 310) && (mouse_y <= sY + 310 + ui_layout::btn_size_y))
				hb::shared::text::draw_text(GameFont::Bitmap1, sX + 220, sY + 310, "Exchange", hb::shared::text::TextStyle::with_highlight(GameColors::UIMagicBlue));
			else
				hb::shared::text::draw_text(GameFont::Bitmap1, sX + 220, sY + 310, "Exchange", hb::shared::text::TextStyle::with_highlight(GameColors::BmpBtnNormal));
		}
		if ((mouse_x >= sX + 450) && (mouse_x <= sX + 450 + ui_layout::btn_size_x) && (mouse_y >= sY + 310) && (mouse_y <= sY + 310 + ui_layout::btn_size_y)
			&& (m_game->m_dialog_box_manager.is_enabled(DialogBoxId::ConfirmExchange) == false))
			hb::shared::text::draw_text(GameFont::Bitmap1, sX + 450, sY + 310, "Cancel", hb::shared::text::TextStyle::with_highlight(GameColors::UIMagicBlue));
		else
			hb::shared::text::draw_text(GameFont::Bitmap1, sX + 450, sY + 310, "Cancel", hb::shared::text::TextStyle::with_highlight(GameColors::BmpBtnNormal));
		break;

	case 2: // You have confirmed the exchange
		put_aligned_string(sX + 80, sX + 180, sY + 38, m_game->m_player->m_player_name.c_str(), GameColors::UIDarkGreen);
		if (m_game->m_dialog_box_exchange_info[4].v1 != -1)
			put_aligned_string(sX + 250, sX + 540, sY + 38, m_game->m_dialog_box_exchange_info[4].str2.c_str(), GameColors::UIDarkGreen);

		draw_items(sX, sY, mouse_x, mouse_y, 0, 8);

		std::string exchangeBuf;
		exchangeBuf = std::format(DRAW_DIALOGBOX_EXCHANGE33, m_game->m_dialog_box_exchange_info[4].str2);
		put_aligned_string(sX, sX + size_x, sY + 215, exchangeBuf.c_str(), GameColors::UILabel);
		put_aligned_string(sX, sX + size_x, sY + 230, DRAW_DIALOGBOX_EXCHANGE34, GameColors::UILabel);
		put_aligned_string(sX, sX + size_x, sY + 245, DRAW_DIALOGBOX_EXCHANGE35, GameColors::UILabel);
		put_aligned_string(sX, sX + size_x, sY + 260, DRAW_DIALOGBOX_EXCHANGE36, GameColors::UILabel);
		put_aligned_string(sX, sX + size_x, sY + 275, DRAW_DIALOGBOX_EXCHANGE37, GameColors::UILabel);
		put_aligned_string(sX, sX + size_x, sY + 290, DRAW_DIALOGBOX_EXCHANGE38, GameColors::UILabel);
		put_aligned_string(sX, sX + size_x, sY + 305, DRAW_DIALOGBOX_EXCHANGE39, GameColors::UILabel);
		break;
	}
}

void DialogBox_Exchange::draw_items(short sX, short sY, short mouse_x, short mouse_y, int start_index, int end_index)
{
	uint32_t time = m_game->m_cur_time;
	char item_color;
	short xadd;

	for (int i = start_index; i < end_index; i++) {
		xadd = (58 * i) + 48;
		if (i > 3) xadd += 20;

		if (m_game->m_dialog_box_exchange_info[i].v1 != -1) {
			item_color = m_game->m_dialog_box_exchange_info[i].v4;
			if (item_color == 0) {
				m_game->m_sprite[ItemPackPivotPoint + m_game->m_dialog_box_exchange_info[i].v1]->draw(sX + xadd, sY + 130, m_game->m_dialog_box_exchange_info[i].v2);
			}
			else {
				switch (m_game->m_dialog_box_exchange_info[i].v1) {
				case 1:  // Swds
				case 2:  // Bows
				case 3:  // Shields
				case 15: // Axes hammers
				case 17: // Wands
					m_game->m_sprite[ItemPackPivotPoint + m_game->m_dialog_box_exchange_info[i].v1]->draw(sX + xadd, sY + 130, m_game->m_dialog_box_exchange_info[i].v2, hb::shared::sprite::DrawParams::tint(GameColors::Weapons[item_color].r, GameColors::Weapons[item_color].g, GameColors::Weapons[item_color].b));
					break;
				default:
					m_game->m_sprite[ItemPackPivotPoint + m_game->m_dialog_box_exchange_info[i].v1]->draw(sX + xadd, sY + 130, m_game->m_dialog_box_exchange_info[i].v2, hb::shared::sprite::DrawParams::tint(GameColors::Items[item_color].r, GameColors::Items[item_color].g, GameColors::Items[item_color].b));
					break;
				}
			}

			draw_item_info(sX, sY, Info().m_size_x, mouse_x, mouse_y, i, xadd);
		}
	}
}

void DialogBox_Exchange::draw_item_info(short sX, short sY, short size_x, short mouse_x, short mouse_y, int item_index, short xadd)
{
	std::string txt, txt2;
	int loc;

	auto itemInfo = item_name_formatter::get().format(m_game->m_dialog_box_exchange_info[item_index].item_id,  m_game->m_dialog_box_exchange_info[item_index].dw_v1);

	if ((mouse_x >= sX + xadd - 6) && (mouse_x <= sX + xadd + 42) && (mouse_y >= sY + 61) && (mouse_y <= sY + 200)) {
		txt = itemInfo.name.c_str();
		if (itemInfo.is_special) {
			put_aligned_string(sX + 15, sX + 155, sY + 215, txt.c_str(), GameColors::UIItemName_Special);
			put_aligned_string(sX + 16, sX + 156, sY + 215, txt.c_str(), GameColors::UIItemName_Special);
		}
		else {
			put_aligned_string(sX + 15, sX + 155, sY + 215, txt.c_str(), GameColors::UILabel);
			put_aligned_string(sX + 16, sX + 156, sY + 215, txt.c_str(), GameColors::UILabel);
		}

		loc = 0;
		if (itemInfo.effect.size() != 0) {
			put_aligned_string(sX + 16, sX + 155, sY + 235 + loc, itemInfo.effect.c_str(), GameColors::UIBlack);
			loc += 15;
		}
		if (itemInfo.extra.size() != 0) {
			put_aligned_string(sX + 16, sX + 155, sY + 235 + loc, itemInfo.extra.c_str(), GameColors::UIBlack);
			loc += 15;
		}

		if (m_game->m_dialog_box_exchange_info[item_index].v3 != 1) {
			if (m_game->m_dialog_box_exchange_info[item_index].v3 > 1) {
				txt2 = m_game->format_comma_number(m_game->m_dialog_box_exchange_info[item_index].v3);
			}
			else {
				txt2 = std::format(DRAW_DIALOGBOX_EXCHANGE2, m_game->m_dialog_box_exchange_info[item_index].v3);
			}
			put_aligned_string(sX + 16, sX + 155, sY + 235 + loc, txt2.c_str(), GameColors::UILabel);
			loc += 15;
		}

		if (m_game->m_dialog_box_exchange_info[item_index].v5 != -1) {
			// Crafting Magins completion fix
			if (m_game->m_dialog_box_exchange_info[item_index].v1 == 22) {
				if ((m_game->m_dialog_box_exchange_info[item_index].v2 > 5) &&
					(m_game->m_dialog_box_exchange_info[item_index].v2 < 10)) {
					txt = std::format(GET_ITEM_NAME2, (m_game->m_dialog_box_exchange_info[item_index].v7 - 100));
				}
			}
			else if (m_game->m_dialog_box_exchange_info[item_index].v1 == 6) {
				txt = std::format(GET_ITEM_NAME1, (m_game->m_dialog_box_exchange_info[item_index].v7 - 100));
			}
			else {
				txt = std::format(GET_ITEM_NAME2, m_game->m_dialog_box_exchange_info[item_index].v7);
			}
			put_aligned_string(sX + 16, sX + 155, sY + 235 + loc, txt.c_str(), GameColors::UILabel);
			loc += 15;
		}

		if (loc < 45) {
			txt = std::format(DRAW_DIALOGBOX_EXCHANGE3, m_game->m_dialog_box_exchange_info[item_index].v5, m_game->m_dialog_box_exchange_info[item_index].v6);
			put_aligned_string(sX + 16, sX + 155, sY + 235 + loc, txt.c_str(), GameColors::UILabel);
		}
	}
}

bool DialogBox_Exchange::on_click(short mouse_x, short mouse_y)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	switch (Info().m_mode) {
	case 1: // Not yet confirmed the exchange
		if ((mouse_x >= sX + 220) && (mouse_x <= sX + 220 + ui_layout::btn_size_x) && (mouse_y >= sY + 310) && (mouse_y <= sY + 310 + ui_layout::btn_size_y)) {
			// Exchange button
			if ((m_game->m_dialog_box_exchange_info[0].v1 != -1) && (m_game->m_dialog_box_exchange_info[4].v1 != -1)) {
				play_sound_effect('E', 14, 5);
				Info().m_mode = 2;
				// Show confirmation dialog
				m_game->m_dialog_box_manager.enable_dialog_box(DialogBoxId::ConfirmExchange, 0, 0, 0);
				m_game->m_dialog_box_manager.Info(DialogBoxId::ConfirmExchange).m_mode = 1;
			}
			return true;
		}
		if ((mouse_x >= sX + 450) && (mouse_x <= sX + 450 + ui_layout::btn_size_x) && (mouse_y >= sY + 310) && (mouse_y <= sY + 310 + ui_layout::btn_size_y)
			&& (m_game->m_dialog_box_manager.is_enabled(DialogBoxId::ConfirmExchange) == false)) {
			// Cancel button
			m_game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::Exchange);
			m_game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::Map);
			send_command(MsgId::CommandCommon, CommonType::cancel_exchange_item, 0, 0, 0, 0, 0);
			play_sound_effect('E', 14, 5);
			return true;
		}
		break;

	case 2: // Someone already confirmed the exchange
		break;
	}

	return false;
}

bool DialogBox_Exchange::on_item_drop(short mouse_x, short mouse_y)
{
	if (m_game->m_player->m_Controller.get_command() < 0) return false;
	if (m_game->m_dialog_box_exchange_info[3].v1 != -1) return false; // Already 4 items

	int item_id = CursorTarget::get_selected_id();
	if (item_id < 0 || item_id >= hb::shared::limits::MaxItems) return false;

	// Find first empty exchange slot
	int slot = -1;
	if (m_game->m_dialog_box_exchange_info[0].v1 == -1) slot = 0;
	else if (m_game->m_dialog_box_exchange_info[1].v1 == -1) slot = 1;
	else if (m_game->m_dialog_box_exchange_info[2].v1 == -1) slot = 2;
	else if (m_game->m_dialog_box_exchange_info[3].v1 == -1) slot = 3;
	else return false; // Impossible case

	// Stackable items - open quantity dialog
	CItem* cfg = m_game->get_item_config(m_game->m_item_list[item_id]->m_id_num);
	if (cfg && ((cfg->get_item_type() == ItemType::Consume) ||
		(cfg->get_item_type() == ItemType::Arrow)) &&
		(m_game->m_item_list[item_id]->m_count > 1))
	{
		auto& dropInfo = m_game->m_dialog_box_manager.Info(DialogBoxId::ItemDropExternal);
		dropInfo.m_x = mouse_x - 140;
		dropInfo.m_y = mouse_y - 70;
		if (dropInfo.m_y < 0) dropInfo.m_y = 0;
		dropInfo.m_v1 = m_game->m_player->m_player_x + 1;
		dropInfo.m_v2 = m_game->m_player->m_player_y + 1;
		dropInfo.m_v3 = 1000;
		dropInfo.m_v4 = item_id;
		m_game->m_dialog_box_exchange_info[slot].item_id = item_id;
		std::memset(dropInfo.m_str, 0, sizeof(dropInfo.m_str));
		m_game->m_dialog_box_manager.enable_dialog_box(DialogBoxId::ItemDropExternal, item_id,
			m_game->m_item_list[item_id]->m_count, 0);
	}
	else
	{
		// Single item - add directly
		m_game->m_dialog_box_exchange_info[slot].item_id = item_id;
		m_game->m_is_item_disabled[item_id] = true;
		send_command(MsgId::CommandCommon, CommonType::set_exchange_item, 0, item_id, 1, 0, 0);
	}

	return true;
}
