#include "DialogBox_Slates.h"
#include "CursorTarget.h"
#include "Game.h"
#include "GameFonts.h"
#include "lan_eng.h"
#include "SpriteID.h"
#include "TextLibExt.h"
#include "NetMessages.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::shared::item;
using namespace hb::client::sprite_id;

DialogBox_Slates::DialogBox_Slates(CGame* game)
	: IDialogBox(DialogBoxId::Slates, game)
{
	set_default_rect(100 , 60 , 258, 339);
	set_can_close_on_right_click(false);
}

void DialogBox_Slates::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
	int adj_x, adj_y;
	short sX, sY;
	uint32_t time = m_game->m_cur_time;

	adj_x = 5;
	adj_y = 8;

	switch (Info().m_mode) {
	case 1:
		sX = Info().m_x;
		sY = Info().m_y;
		adj_x = -1;
		adj_y = -7;

		draw_new_dialog_box(InterfaceNdInventory, sX, sY, 4);

		if (Info().m_v1 != -1) {
			draw_new_dialog_box(InterfaceNdInventory, sX + 20, sY + 12, 5);
		}
		if (Info().m_v2 != -1) {
			draw_new_dialog_box(InterfaceNdInventory, sX + 20, sY + 87, 6);
		}
		if (Info().m_v3 != -1) {
			draw_new_dialog_box(InterfaceNdInventory, sX + 85, sY + 32, 7);
		}
		if (Info().m_v4 != -1) {
			draw_new_dialog_box(InterfaceNdInventory, sX + 70, sY + 97, 8);
		}

		if ((Info().m_v1 != -1) && (Info().m_v2 != -1) && (Info().m_v3 != -1) && (Info().m_v4 != -1)) {
			if ((mouse_x >= sX + 120) && (mouse_x <= sX + 180) && (mouse_y >= sY + 150) && (mouse_y <= sY + 165))
				hb::shared::text::draw_text(GameFont::Bitmap1, sX + 120, sY + 150, "Casting", hb::shared::text::TextStyle::with_highlight(GameColors::UIMagicBlue));
			else hb::shared::text::draw_text(GameFont::Bitmap1, sX + 120, sY + 150, "Casting", hb::shared::text::TextStyle::with_highlight(GameColors::BmpBtnNormal));
		}
		break;

	case 2:
		m_game->play_game_sound('E', 16, 0);
		if (Info().m_str[0] != 0)
		{
			sX = Info().m_x + adj_x + (Info().m_str[0] - (rand() % (Info().m_str[0] * 2)));
			sY = Info().m_y + adj_y + (Info().m_str[0] - (rand() % (Info().m_str[0] * 2)));
		}
		else
		{
			sX = Info().m_x;
			sY = Info().m_y;
		}
		m_game->m_sprite[InterfaceNdInventory]->draw(sX, sY, 4);
		m_game->m_sprite[InterfaceNdInventory]->draw(sX + 22, sY + 14, 3);
		put_aligned_string(199, 438, 201, "KURURURURURURURURU!!!", GameColors::UISlatesPink);
		put_aligned_string(200, 439, 200, "KURURURURURURURURU!!!", GameColors::UISlatesCyan);

		if ((time - Info().m_dw_t1) > 1000)
		{
			Info().m_dw_t1 = time;
			Info().m_str[0]++;
		}
		if (Info().m_str[0] >= 5)
		{
			send_command(MsgId::CommandCommon, CommonType::ReqCreateSlate, 0, Info().m_v1, Info().m_v2, Info().m_v3, 0, Info().m_v4);
			disable_dialog_box(DialogBoxId::Slates);
		}
		break;
	}
}

bool DialogBox_Slates::on_click(short mouse_x, short mouse_y)
{
	short sX, sY;

	sX = Info().m_x;
	sY = Info().m_y;

	switch (Info().m_mode) {
	case 1:
		if ((Info().m_v1 != -1) && (Info().m_v2 != -1) && (Info().m_v3 != -1) && (Info().m_v4 != -1)) {
			if ((mouse_x >= sX + 120) && (mouse_x <= sX + 180) && (mouse_y >= sY + 150) && (mouse_y <= sY + 165)) {
				Info().m_mode = 2;
				play_sound_effect('E', 14, 5);
			}
		}
		break;
	}
	return false;
}

bool DialogBox_Slates::on_item_drop(short mouse_x, short mouse_y)
{
	if (m_game->m_player->m_Controller.get_command() < 0) return false;

	int item_id = CursorTarget::get_selected_id();
	if (item_id < 0 || item_id >= hb::shared::limits::MaxItems) return false;
	if (m_game->m_item_list[item_id] == nullptr) return false;
	if (m_game->m_is_item_disabled[item_id]) return false;

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

	switch (Info().m_mode) {
	case 1:
	{
		// Only accept slate items (sprite frame 151-154)
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[item_id]->m_id_num);
		if (cfg && (cfg->get_item_type() == ItemType::UseSkillEnableDialogBox) &&
			(cfg->m_sprite_frame >= 151) &&
			(cfg->m_sprite_frame <= 154))
		{
			std::string item_id_text;
			switch (cfg->m_sprite_frame) {
			case 151:
				if (Info().m_v1 == -1) {
					m_game->m_is_item_disabled[item_id] = true;
					Info().m_v1 = item_id;
					item_id_text = std::format("Item ID : {}", item_id);
					add_event_list(item_id_text.c_str(), 10);
				}
				break;
			case 152:
				if (Info().m_v2 == -1) {
					m_game->m_is_item_disabled[item_id] = true;
					Info().m_v2 = item_id;
					item_id_text = std::format("Item ID : {}", item_id);
					add_event_list(item_id_text.c_str(), 10);
				}
				break;
			case 153:
				if (Info().m_v3 == -1) {
					m_game->m_is_item_disabled[item_id] = true;
					Info().m_v3 = item_id;
					item_id_text = std::format("Item ID : {}", item_id);
					add_event_list(item_id_text.c_str(), 10);
				}
				break;
			case 154:
				if (Info().m_v4 == -1) {
					m_game->m_is_item_disabled[item_id] = true;
					Info().m_v4 = item_id;
					item_id_text = std::format("Item ID : {}", item_id);
					add_event_list(item_id_text.c_str(), 10);
				}
				break;
			}
		}
		break;
	}
	}

	return true;
}
