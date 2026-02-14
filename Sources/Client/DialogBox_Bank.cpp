#include "DialogBox_Bank.h"
#include "CursorTarget.h"
#include "Game.h"
#include "InventoryManager.h"
#include "ItemNameFormatter.h"
#include "IInput.h"
#include "lan_eng.h"
#include <format>

using namespace hb::shared::net;
using namespace hb::shared::item;
using namespace hb::client::sprite_id;

DialogBox_Bank::DialogBox_Bank(CGame* game)
	: IDialogBox(DialogBoxId::Bank, game)
{
	set_default_rect(60 , 50 , 258, 339);
	Info().m_v1 = 13; // Number of visible item lines in scrollable list
}

void DialogBox_Bank::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
	if (!m_game->ensure_item_configs_loaded()) return;
	short sX = Info().m_x;
	short sY = Info().m_y;
	short size_x = Info().m_size_x - 5;

	m_game->draw_new_dialog_box(InterfaceNdGame2, sX, sY, 2);
	m_game->draw_new_dialog_box(InterfaceNdText, sX, sY, 21);

	switch (Info().m_mode) {
	case -1:
		put_string(sX + 30 + 15, sY + 70, DRAW_DIALOGBOX_BANK1, GameColors::UIBlack);
		put_string(sX + 30 + 15, sY + 85, DRAW_DIALOGBOX_BANK2, GameColors::UIBlack);
		break;

	case 0:
		draw_item_list(sX, sY, size_x, mouse_x, mouse_y, z, lb);
		break;
	}
}

void DialogBox_Bank::draw_item_list(short sX, short sY, short size_x, short mouse_x, short mouse_y, short z, char lb)
{
	bool flag = false;
	int loc = 45;


	for (int i = 0; i < Info().m_v1; i++) {
		int itemIndex = i + Info().m_view;
		if ((itemIndex < hb::shared::limits::MaxBankItems) && (m_game->m_bank_list[itemIndex] != 0)) {
			auto itemInfo = item_name_formatter::get().format(m_game->m_bank_list[itemIndex].get());

			if ((mouse_x > sX + 30) && (mouse_x < sX + 210) && (mouse_y >= sY + 110 + i * 15) && (mouse_y <= sY + 124 + i * 15)) {
				flag = true;
				put_aligned_string(sX, sX + size_x, sY + 110 + i * 15, itemInfo.name.c_str(), GameColors::UIWhite);
				draw_item_details(sX, sY, size_x, itemIndex, loc);
			}
			else {
				if (itemInfo.is_special)
					put_aligned_string(sX, sX + size_x, sY + 110 + i * 15, itemInfo.name.c_str(), GameColors::UIItemName_Special);
				else
					put_aligned_string(sX, sX + size_x, sY + 110 + i * 15, itemInfo.name.c_str(), GameColors::UIBlack);
			}
		}
	}

	// Count total items for scrollbar
	int total_lines = 0;
	for (int i = 0; i < hb::shared::limits::MaxBankItems; i++)
		if (m_game->m_bank_list[i] != 0) total_lines++;

	draw_scrollbar(sX, sY, total_lines, mouse_x, mouse_y, z, lb);

	if (!flag) {
		put_aligned_string(sX, sX + size_x, sY + 45, DRAW_DIALOGBOX_BANK3);
		put_aligned_string(sX, sX + size_x, sY + 60, DRAW_DIALOGBOX_BANK4);
		put_aligned_string(sX, sX + size_x, sY + 75, DRAW_DIALOGBOX_BANK5);
	}
}

void DialogBox_Bank::draw_item_details(short sX, short sY, short size_x, int item_index, int loc)
{

	CItem* item = m_game->m_bank_list[item_index].get();
	CItem* cfg = m_game->get_item_config(item->m_id_num);
	if (cfg == nullptr) return;

	auto itemInfo2 = item_name_formatter::get().format(item);

	if (itemInfo2.is_special)
		put_aligned_string(sX + 70, sX + size_x, sY + loc, itemInfo2.name.c_str(), GameColors::UIItemName_Special);
	else
		put_aligned_string(sX + 70, sX + size_x, sY + loc, itemInfo2.name.c_str(), GameColors::UIWhite);

	if (itemInfo2.effect.size() > 0) {
		loc += 15;
		put_aligned_string(sX + 70, sX + size_x, sY + loc, itemInfo2.effect.c_str(), GameColors::UIDisabled);
	}
	if (itemInfo2.extra.size() > 0) {
		loc += 15;
		put_aligned_string(sX + 70, sX + size_x, sY + loc, itemInfo2.extra.c_str(), GameColors::UIDisabled);
	}

	// Level limit
	if (cfg->m_level_limit != 0 &&
		item->m_attribute & 0x00000001) {
		loc += 15;
		auto buf = std::format("{}: {}", DRAW_DIALOGBOX_SHOP24, cfg->m_level_limit);
		put_aligned_string(sX + 70, sX + size_x, sY + loc, buf.c_str(), GameColors::UIDisabled);
	}

	// Weight for equipment
	if ((cfg->get_equip_pos() != EquipPos::None) &&
		(cfg->m_weight >= 1100)) {
		loc += 15;
		int _wWeight = 0;
		if (cfg->m_weight % 100) _wWeight = 1;
		auto strReq = std::format(DRAW_DIALOGBOX_SHOP15, cfg->m_weight / 100 + _wWeight);
		put_aligned_string(sX + 70, sX + size_x, sY + loc, strReq.c_str(), GameColors::UIDisabled);
	}

	// draw item sprite
	char item_color = item->m_item_color;
	if (item_color == 0) {
		m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + 60, sY + 68, cfg->m_sprite_frame);
	}
	else {
		if ((cfg->get_equip_pos() == EquipPos::LeftHand) ||
			(cfg->get_equip_pos() == EquipPos::RightHand) ||
			(cfg->get_equip_pos() == EquipPos::TwoHand)) {
			m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + 60, sY + 68, cfg->m_sprite_frame, hb::shared::sprite::DrawParams::tint(GameColors::Weapons[item_color].r, GameColors::Weapons[item_color].g, GameColors::Weapons[item_color].b));
		}
		else {
			m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + 60, sY + 68, cfg->m_sprite_frame, hb::shared::sprite::DrawParams::tint(GameColors::Items[item_color].r, GameColors::Items[item_color].g, GameColors::Items[item_color].b));
		}
	}
}

void DialogBox_Bank::draw_scrollbar(short sX, short sY, int total_lines, short mouse_x, short mouse_y, short z, char lb)
{
	int pointer_loc;
	double d1, d2, d3;

	if (total_lines > Info().m_v1) {
		d1 = static_cast<double>(Info().m_view);
		d2 = static_cast<double>(total_lines - Info().m_v1);
		d3 = (274.0f * d1) / d2;
		pointer_loc = static_cast<int>(d3);
		m_game->draw_new_dialog_box(InterfaceNdGame2, sX, sY, 3);
		m_game->draw_new_dialog_box(InterfaceNdGame2, sX + 242, sY + pointer_loc + 35, 7);
	}
	else {
		pointer_loc = 0;
	}

	if (lb != 0 && (m_game->m_dialog_box_manager.get_top_dialog_box_index() == DialogBoxId::Bank) && total_lines > Info().m_v1) {
		if ((mouse_x >= sX + 230) && (mouse_x <= sX + 260) && (mouse_y >= sY + 40) && (mouse_y <= sY + 320)) {
			d1 = static_cast<double>(mouse_y - (sY + 35));
			d2 = static_cast<double>(total_lines - Info().m_v1);
			d3 = (d1 * d2) / 274.0f;
			Info().m_view = static_cast<int>(d3 + 0.5);
		}
		else if ((mouse_x >= sX + 230) && (mouse_x <= sX + 260) && (mouse_y > sY + 10) && (mouse_y < sY + 40)) {
			Info().m_view = 0;
		}
	}
	else {
		Info().m_is_scroll_selected = false;
	}

	if (m_game->m_dialog_box_manager.get_top_dialog_box_index() == DialogBoxId::Bank && z != 0) {
		if (total_lines > 50)
			Info().m_view = Info().m_view - z / 30;
		else {
			if (z > 0) Info().m_view--;
			if (z < 0) Info().m_view++;
		}
	}

	if (total_lines > Info().m_v1 && Info().m_view > total_lines - Info().m_v1)
		Info().m_view = total_lines - Info().m_v1;
	if (total_lines <= Info().m_v1)
		Info().m_view = 0;
	if (Info().m_view < 0)
		Info().m_view = 0;
}

bool DialogBox_Bank::on_click(short mouse_x, short mouse_y)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	switch (Info().m_mode) {
	case -1:
		break;

	case 0:
		for (int i = 0; i < Info().m_v1; i++) {
			if ((mouse_x > sX + 30) && (mouse_x < sX + 210) && (mouse_y >= sY + 110 + i * 15) && (mouse_y <= sY + 124 + i * 15)) {
				int itemIndex = Info().m_view + i;
				if ((itemIndex < hb::shared::limits::MaxBankItems) && (m_game->m_bank_list[itemIndex] != 0)) {
					if (inventory_manager::get().get_total_item_count() >= 50) {
						add_event_list(DLGBOX_CLICK_BANK1, 10);
						return true;
					}
					send_command(MsgId::RequestRetrieveItem, 0, 0, itemIndex, 0, 0, 0);
					Info().m_mode = -1;
					play_sound_effect('E', 14, 5);
				}
				return true;
			}
		}
		break;
	}

	return false;
}

bool DialogBox_Bank::on_double_click(short mouse_x, short mouse_y)
{
	if (CursorTarget::GetSelectedType() == SelectedObjectType::Item)
		return on_item_drop(mouse_x, mouse_y);

	return false;
}

bool DialogBox_Bank::on_item_drop(short mouse_x, short mouse_y)
{
	auto& giveInfo = m_game->m_dialog_box_manager.Info(DialogBoxId::GiveItem);
	giveInfo.m_v1 = CursorTarget::get_selected_id();

	if (m_game->m_player->m_Controller.get_command() < 0) {
		return false;
	}
	if (m_game->m_item_list[giveInfo.m_v1] == nullptr) {
		return false;
	}
	if (m_game->m_is_item_disabled[giveInfo.m_v1]) {
		return false;
	}

	// Check if other dialogs are blocking
	if (m_game->m_dialog_box_manager.is_enabled(DialogBoxId::ItemDropExternal))
	{
		add_event_list(BITEMDROP_SKILLDIALOG1, 10);
		return false;
	}
	if (m_game->m_dialog_box_manager.is_enabled(DialogBoxId::NpcActionQuery) &&
		(m_game->m_dialog_box_manager.Info(DialogBoxId::NpcActionQuery).m_mode == 1 ||
		 m_game->m_dialog_box_manager.Info(DialogBoxId::NpcActionQuery).m_mode == 2))
	{
		add_event_list(BITEMDROP_SKILLDIALOG1, 10);
		return false;
	}
	if (m_game->m_dialog_box_manager.is_enabled(DialogBoxId::SellOrRepair))
	{
		add_event_list(BITEMDROP_SKILLDIALOG1, 10);
		return false;
	}
	if (m_game->m_dialog_box_manager.is_enabled(DialogBoxId::ItemDropConfirm))
	{
		add_event_list(BITEMDROP_SKILLDIALOG1, 10);
		return false;
	}

	// Stackable items - open quantity dialog
	CItem* cfg = m_game->get_item_config(m_game->m_item_list[giveInfo.m_v1]->m_id_num);
	if (cfg == nullptr) {
		return false;
	}

	if (((cfg->get_item_type() == ItemType::Consume) ||
		(cfg->get_item_type() == ItemType::Arrow)) &&
		(m_game->m_item_list[giveInfo.m_v1]->m_count > 1))
	{
		auto& dropInfo = m_game->m_dialog_box_manager.Info(DialogBoxId::ItemDropExternal);
		dropInfo.m_x = mouse_x - 140;
		dropInfo.m_y = mouse_y - 70;
		if (dropInfo.m_y < 0) dropInfo.m_y = 0;
		dropInfo.m_v1 = m_game->m_player->m_player_x + 1;
		dropInfo.m_v2 = m_game->m_player->m_player_y + 1;
		dropInfo.m_v3 = 1002; // NPC
		dropInfo.m_v4 = giveInfo.m_v1;
		std::memset(dropInfo.m_str, 0, sizeof(dropInfo.m_str));
		m_game->m_dialog_box_manager.enable_dialog_box(DialogBoxId::ItemDropExternal, giveInfo.m_v1,
			m_game->m_item_list[giveInfo.m_v1]->m_count, 0);
	}
	else
	{
		// Single item - deposit directly
		if (inventory_manager::get().get_bank_item_count() >= (m_game->m_max_bank_items - 1))
		{
			add_event_list(DLGBOX_CLICK_NPCACTION_QUERY9, 10);
		}
		else
		{
			send_command(MsgId::CommandCommon, CommonType::GiveItemToChar, giveInfo.m_v1, 1,
				giveInfo.m_v5, giveInfo.m_v6, cfg->m_name, giveInfo.m_v4);
		}
	}

	return true;
}

PressResult DialogBox_Bank::on_press(short mouse_x, short mouse_y)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	// Scroll bar region
	if ((mouse_x >= sX + 240) && (mouse_x <= sX + 260) && (mouse_y >= sY + 40) && (mouse_y <= sY + 320))
	{
		return PressResult::ScrollClaimed;
	}

	return PressResult::Normal;
}
