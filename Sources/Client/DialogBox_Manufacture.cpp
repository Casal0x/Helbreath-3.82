#include "DialogBox_Manufacture.h"
#include "CursorTarget.h"
#include "Game.h"
#include "BuildItemManager.h"
#include "ItemNameFormatter.h"
#include "GameFonts.h"
#include "GlobalDef.h"
#include "SpriteID.h"
#include "TextLibExt.h"
#include "lan_eng.h"
#include "NetMessages.h"
#include "IInput.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::shared::item;
using namespace hb::client::sprite_id;

DialogBox_Manufacture::DialogBox_Manufacture(CGame* game)
	: IDialogBox(DialogBoxId::Manufacture, game)
{
	set_default_rect(100 , 60 , 258, 339);
}

void DialogBox_Manufacture::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
	if (!m_game->ensure_item_configs_loaded()) return;
	int adj_x = 5;
	int adj_y = 8;
	short sX, sY;

	switch (Info().m_mode) {
	case 1: // Alchemy waiting for ingredients
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
		draw_alchemy_waiting(sX, sY, mouse_x, mouse_y);
		break;

	case 2: // Alchemy creating potion
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
		draw_alchemy_creating(sX, sY);
		break;

	case 3: // Manufacture: item list
		sX = Info().m_x;
		sY = Info().m_y;
		draw_manufacture_list(sX, sY, mouse_x, mouse_y, z, lb);
		break;

	case 4: // Manufacture: waiting for ingredients
		sX = Info().m_x;
		sY = Info().m_y;
		draw_manufacture_waiting(sX, sY, mouse_x, mouse_y);
		break;

	case 5: // Manufacture: in progress
		sX = Info().m_x;
		sY = Info().m_y;
		draw_manufacture_in_progress(sX, sY);
		break;

	case 6: // Manufacture: done
		sX = Info().m_x;
		sY = Info().m_y;
		draw_manufacture_done(sX, sY, mouse_x, mouse_y);
		break;

	case 7: // Crafting: waiting for ingredients
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
		draw_crafting_waiting(sX, sY, mouse_x, mouse_y);
		break;

	case 8: // Crafting: in progress
		if (Info().m_str[0] != 0)
		{
			sX = Info().m_x + 5 + (Info().m_str[0] - (rand() % (Info().m_str[0] * 2)));
			sY = Info().m_y + 8 + (Info().m_str[0] - (rand() % (Info().m_str[0] * 2)));
		}
		else
		{
			sX = Info().m_x;
			sY = Info().m_y;
		}
		draw_crafting_in_progress(sX, sY);
		break;
	}
}

void DialogBox_Manufacture::draw_alchemy_waiting(short sX, short sY, short mouse_x, short mouse_y)
{
	int adj_x = 5;
	int adj_y = 8;
	uint32_t time = m_game->m_cur_time;

	m_game->m_sprite[InterfaceAddInterface]->draw(sX, sY, 1);

	if (Info().m_v1 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v1]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint +
		cfg->m_sprite]->draw(sX + adj_x + 55 + (1 - (rand() % 3)), sY + adj_y + 55 + (1 - (rand() % 3)), cfg->m_sprite_frame, hb::shared::sprite::DrawParams::alpha_blend(0.5f));
	}

	if (Info().m_v2 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v2]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint +
		cfg->m_sprite]->draw(sX + adj_x + 55 + 45 * 1 + (1 - (rand() % 3)), sY + adj_y + 55 + (1 - (rand() % 3)), cfg->m_sprite_frame, hb::shared::sprite::DrawParams::alpha_blend(0.5f));
	}

	if (Info().m_v3 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v3]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint +
		cfg->m_sprite]->draw(sX + adj_x + 55 + 45 * 2 + (1 - (rand() % 3)), sY + adj_y + 55 + (1 - (rand() % 3)), cfg->m_sprite_frame, hb::shared::sprite::DrawParams::alpha_blend(0.5f));
	}

	if (Info().m_v4 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v4]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint +
		cfg->m_sprite]->draw(sX + adj_x + 55 + (1 - (rand() % 3)), sY + adj_y + 100 + (1 - (rand() % 3)), cfg->m_sprite_frame, hb::shared::sprite::DrawParams::alpha_blend(0.5f));
	}

	if (Info().m_v5 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v5]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint +
		cfg->m_sprite]->draw(sX + adj_x + 55 + 45 * 1 + (1 - (rand() % 3)), sY + adj_y + 100 + (1 - (rand() % 3)), cfg->m_sprite_frame, hb::shared::sprite::DrawParams::alpha_blend(0.5f));
	}

	if (Info().m_v6 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v6]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint +
		cfg->m_sprite]->draw(sX + adj_x + 55 + 45 * 2 + (1 - (rand() % 3)), sY + adj_y + 100 + (1 - (rand() % 3)), cfg->m_sprite_frame, hb::shared::sprite::DrawParams::alpha_blend(0.5f));
	}

	if ((mouse_x >= sX + adj_x + 60) && (mouse_x <= sX + adj_x + 153) && (mouse_y >= sY + adj_y + 175) && (mouse_y <= sY + adj_y + 195))
		hb::shared::text::draw_text(GameFont::Bitmap1, sX + adj_x + 60, sY + adj_y + 175, "Try Now!", hb::shared::text::TextStyle::with_highlight(GameColors::BmpBtnBlue));
	else hb::shared::text::draw_text(GameFont::Bitmap1, sX + adj_x + 60, sY + adj_y + 175, "Try Now!", hb::shared::text::TextStyle::with_highlight(GameColors::UIMagicBlue));
}

void DialogBox_Manufacture::draw_alchemy_creating(short sX, short sY)
{
	int adj_x = 5;
	int adj_y = 8;
	uint32_t time = m_game->m_cur_time;

	m_game->m_sprite[InterfaceAddInterface]->draw(sX, sY, 1);

	if (Info().m_v1 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v1]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint +
		cfg->m_sprite]->draw(sX + adj_x + 55 + (1 - (rand() % 3)), sY + adj_y + 55 + (1 - (rand() % 3)), cfg->m_sprite_frame, hb::shared::sprite::DrawParams::alpha_blend(0.5f));
	}

	if (Info().m_v2 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v2]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint +
		cfg->m_sprite]->draw(sX + adj_x + 55 + 45 * 1 + (1 - (rand() % 3)), sY + adj_y + 55 + (1 - (rand() % 3)), cfg->m_sprite_frame, hb::shared::sprite::DrawParams::alpha_blend(0.5f));
	}

	if (Info().m_v3 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v3]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint +
		cfg->m_sprite]->draw(sX + adj_x + 55 + 45 * 2 + (1 - (rand() % 3)), sY + adj_y + 55 + (1 - (rand() % 3)), cfg->m_sprite_frame, hb::shared::sprite::DrawParams::alpha_blend(0.5f));
	}

	if (Info().m_v4 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v4]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint +
		cfg->m_sprite]->draw(sX + adj_x + 55 + (1 - (rand() % 3)), sY + adj_y + 100 + (1 - (rand() % 3)), cfg->m_sprite_frame, hb::shared::sprite::DrawParams::alpha_blend(0.5f));
	}

	if (Info().m_v5 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v5]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint +
		cfg->m_sprite]->draw(sX + adj_x + 55 + 45 * 1 + (1 - (rand() % 3)), sY + adj_y + 100 + (1 - (rand() % 3)), cfg->m_sprite_frame, hb::shared::sprite::DrawParams::alpha_blend(0.5f));
	}

	if (Info().m_v6 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v6]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint +
		cfg->m_sprite]->draw(sX + adj_x + 55 + 45 * 2 + (1 - (rand() % 3)), sY + adj_y + 100 + (1 - (rand() % 3)), cfg->m_sprite_frame, hb::shared::sprite::DrawParams::alpha_blend(0.5f));
	}

	hb::shared::text::draw_text(GameFont::Bitmap1, sX + adj_x + 60, sY + adj_y + 175, "Creating...", hb::shared::text::TextStyle::with_highlight(GameColors::BmpBtnRed));

	if ((time - Info().m_dw_t1) > 1000)
	{
		Info().m_dw_t1 = time;
		Info().m_str[0]++;
	}

	if (Info().m_str[0] >= 5)
	{
		m_game->send_command(MsgId::CommandCommon, CommonType::ReqCreatePortion, 0, 0, 0, 0, 0);
		m_game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::Manufacture);
		m_game->play_game_sound('E', 42, 0);
	}
}

void DialogBox_Manufacture::draw_manufacture_list(short sX, short sY, short mouse_x, short mouse_y, short z, char lb)
{
	int adj_x = 5;
	int adj_y = 8;
	uint32_t time = m_game->m_cur_time;
	short size_x = Info().m_size_x;
	std::string temp, temp2;
	int loc;

	draw_new_dialog_box(InterfaceNdGame3, sX, sY, 0);
	draw_new_dialog_box(InterfaceNdText, sX, sY, 8);
	put_string(sX + adj_x + 44, sY + adj_y + 38, "Name", GameColors::UIBlack);
	put_string(sX + adj_x + 171, sY + adj_y + 38, "Max.Skill", GameColors::UIBlack);

	loc = 0;
	for (int i = 0; i < 13; i++)
		if (build_item_manager::get().get_display_list()[i + Info().m_view] != 0) {

			auto itemInfo = item_name_formatter::get().format(m_game->find_item_id_by_name(build_item_manager::get().get_display_list()[i + Info().m_view]->m_name.c_str()),  0);
			temp = itemInfo.name.c_str();
			temp2 = std::format("{}%", build_item_manager::get().get_display_list()[i + Info().m_view]->m_max_skill);

			if ((mouse_x >= sX + 30) && (mouse_x <= sX + 180) && (mouse_y >= sY + adj_y + 55 + loc * 15) && (mouse_y <= sY + adj_y + 69 + loc * 15))
			{
				put_string(sX + 30, sY + adj_y + 55 + loc * 15, temp.c_str(), GameColors::UIWhite);
				put_string(sX + 190, sY + adj_y + 55 + loc * 15, temp2.c_str(), GameColors::UIWhite);
			}
			else
			{
				if (build_item_manager::get().get_display_list()[i + Info().m_view]->m_build_enabled == true)
				{
					put_string(sX + 30, sY + adj_y + 55 + loc * 15, temp.c_str(), GameColors::UIMagicBlue);
					put_string(sX + 190, sY + adj_y + 55 + loc * 15, temp2.c_str(), GameColors::UIMagicBlue);
				}
				else
				{
					put_string(sX + 30, sY + adj_y + 55 + loc * 15, temp.c_str(), GameColors::UILabel);
					put_string(sX + 190, sY + adj_y + 55 + loc * 15, temp2.c_str(), GameColors::UILabel);
				}
			}
			loc++;
		}

	if ((Info().m_view >= 1) && (build_item_manager::get().get_display_list()[Info().m_view - 1] != 0))
		m_game->m_sprite[InterfaceNdGame2]->draw(sX + adj_x + 225, sY + adj_y + 210, 23);
	else m_game->m_sprite[InterfaceNdGame2]->draw(sX + adj_x + 225, sY + adj_y + 210, 23, hb::shared::sprite::DrawParams::tinted_alpha(GameColors::UIBlack.r, GameColors::UIBlack.g, GameColors::UIBlack.b, 0.7f));

	if (build_item_manager::get().get_display_list()[Info().m_view + 13] != 0)
		m_game->m_sprite[InterfaceNdGame2]->draw(sX + adj_x + 225, sY + adj_y + 230, 24);
	else m_game->m_sprite[InterfaceNdGame2]->draw(sX + adj_x + 225, sY + adj_y + 230, 24, hb::shared::sprite::DrawParams::tinted_alpha(GameColors::UIBlack.r, GameColors::UIBlack.g, GameColors::UIBlack.b, 0.7f));

	if ((lb != 0) && (m_game->m_dialog_box_manager.get_top_dialog_box_index() == DialogBoxId::Manufacture)) {
		if ((mouse_x >= sX + adj_x + 225) && (mouse_x <= sX + adj_x + 245) && (mouse_y >= sY + adj_y + 210) && (mouse_y <= sY + adj_y + 230)) {
			Info().m_view--;
		}

		if ((mouse_x >= sX + adj_x + 225) && (mouse_x <= sX + adj_x + 245) && (mouse_y >= sY + adj_y + 230) && (mouse_y <= sY + adj_y + 250)) {
			if (build_item_manager::get().get_display_list()[Info().m_view + 13] != 0)
				Info().m_view++;
		}
	}
	if ((z != 0) && (m_game->m_dialog_box_manager.get_top_dialog_box_index() == DialogBoxId::Manufacture)) {
		Info().m_view = Info().m_view - z / 60;

	}
	if (build_item_manager::get().get_display_list()[Info().m_view + 12] == 0)
	{
		while (1)
		{
			Info().m_view--;
			if (Info().m_view < 1) break;
			if (build_item_manager::get().get_display_list()[Info().m_view + 12] != 0) break;
		}
	}
	if (Info().m_view < 0) Info().m_view = 0;

	put_aligned_string(sX, sX + Info().m_size_x, sY + 265, DRAW_DIALOGBOX_SKILLDLG2, GameColors::UILabel);
	put_aligned_string(sX, sX + Info().m_size_x, sY + 280, DRAW_DIALOGBOX_SKILLDLG3, GameColors::UILabel);
	put_aligned_string(sX, sX + Info().m_size_x, sY + 295, DRAW_DIALOGBOX_SKILLDLG4, GameColors::UILabel);
	put_aligned_string(sX, sX + Info().m_size_x, sY + 310, DRAW_DIALOGBOX_SKILLDLG5, GameColors::UILabel);
	put_aligned_string(sX, sX + Info().m_size_x, sY + 340, DRAW_DIALOGBOX_SKILLDLG6, GameColors::UILabel);
}

void DialogBox_Manufacture::draw_manufacture_waiting(short sX, short sY, short mouse_x, short mouse_y)
{
	int adj_x = -1;
	int adj_y = -7;
	uint32_t time = m_game->m_cur_time;
	short size_x = Info().m_size_x;
	std::string temp;
	int loc;

	draw_new_dialog_box(InterfaceNdGame3, sX, sY, 0);
	draw_new_dialog_box(InterfaceNdText, sX, sY, 8);
	m_game->m_sprite[ItemPackPivotPoint + build_item_manager::get().get_display_list()[Info().m_str[0]]->m_sprite_handle]->draw(sX + adj_x + 62 + 5, sY + adj_y + 84 + 17, build_item_manager::get().get_display_list()[Info().m_str[0]]->m_sprite_frame);

	auto itemInfo2 = item_name_formatter::get().format(m_game->find_item_id_by_name(build_item_manager::get().get_display_list()[Info().m_str[0]]->m_name.c_str()),  0);
	temp = itemInfo2.name.c_str();
	put_string(sX + adj_x + 44 + 10 + 60, sY + adj_y + 55, temp.c_str(), GameColors::UIWhite);

	temp = std::format(DRAW_DIALOGBOX_SKILLDLG7, build_item_manager::get().get_display_list()[Info().m_str[0]]->m_skill_limit, build_item_manager::get().get_display_list()[Info().m_str[0]]->m_max_skill);
	put_string(sX + adj_x + 44 + 10 + 60, sY + adj_y + 55 + 2 * 15, temp.c_str(), GameColors::UILabel);
	put_string(sX + adj_x + 44 + 10 + 60, sY + adj_y + 55 + 3 * 15 + 5, DRAW_DIALOGBOX_SKILLDLG8, GameColors::UILabel);

	loc = 4;
	for (int elem = 1; elem <= 6; elem++)
	{
		if (build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_count[elem] != 0)
		{
			const char* elemName = nullptr;
			bool elemFlag = false;
			switch (elem) {
			case 1: elemName = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_name_1.c_str(); elemFlag = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_flag[1]; break;
			case 2: elemName = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_name_2.c_str(); elemFlag = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_flag[2]; break;
			case 3: elemName = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_name_3.c_str(); elemFlag = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_flag[3]; break;
			case 4: elemName = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_name_4.c_str(); elemFlag = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_flag[4]; break;
			case 5: elemName = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_name_5.c_str(); elemFlag = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_flag[5]; break;
			case 6: elemName = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_name_6.c_str(); elemFlag = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_flag[6]; break;
			}
			auto itemInfo3 = item_name_formatter::get().format(m_game->find_item_id_by_name(elemName),  0);
			temp = itemInfo3.name.c_str();
			if (elemFlag)
				put_string(sX + adj_x + 44 + 20 + 60, sY + adj_y + 55 + loc * 15 + 5, temp.c_str(), GameColors::UILabel);
			else
				put_string(sX + adj_x + 44 + 20 + 60, sY + adj_y + 55 + loc * 15 + 5, temp.c_str(), GameColors::UIDisabled);
			loc++;
		}
	}

	if (build_item_manager::get().get_display_list()[Info().m_str[0]]->m_build_enabled == true)
	{
		// draw ingredient slots
		for (int slot = 0; slot < 6; slot++)
		{
			int slotX = (slot % 3) * 45;
			int slotY = (slot / 3) * 45;
			m_game->m_sprite[InterfaceAddInterface]->draw(sX + adj_x + 55 + 30 + slotX + 13, sY + adj_y + 55 + slotY + 180, 2);
		}

		// draw items in slots
		if (Info().m_v1 != -1) {
			CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v1]->m_id_num);
			if (cfg) m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 55 + 30 + 13, sY + adj_y + 55 + 180, cfg->m_sprite_frame);
		}
		if (Info().m_v2 != -1) {
			CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v2]->m_id_num);
			if (cfg) m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 55 + 45 + 30 + 13, sY + adj_y + 55 + 180, cfg->m_sprite_frame);
		}
		if (Info().m_v3 != -1) {
			CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v3]->m_id_num);
			if (cfg) m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 55 + 90 + 30 + 13, sY + adj_y + 55 + 180, cfg->m_sprite_frame);
		}
		if (Info().m_v4 != -1) {
			CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v4]->m_id_num);
			if (cfg) m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 55 + 30 + 13, sY + adj_y + 100 + 180, cfg->m_sprite_frame);
		}
		if (Info().m_v5 != -1) {
			CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v5]->m_id_num);
			if (cfg) m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 55 + 45 + 30 + 13, sY + adj_y + 100 + 180, cfg->m_sprite_frame);
		}
		if (Info().m_v6 != -1) {
			CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v6]->m_id_num);
			if (cfg) m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 55 + 90 + 30 + 13, sY + adj_y + 100 + 180, cfg->m_sprite_frame);
		}

		put_aligned_string(sX, sX + size_x, sY + adj_y + 230 + 75, DRAW_DIALOGBOX_SKILLDLG15, GameColors::UILabel);
		put_aligned_string(sX, sX + size_x, sY + adj_y + 245 + 75, DRAW_DIALOGBOX_SKILLDLG16, GameColors::UILabel);
		put_aligned_string(sX, sX + size_x, sY + adj_y + 260 + 75, DRAW_DIALOGBOX_SKILLDLG17, GameColors::UILabel);

		if ((mouse_x >= sX + adj_x + 32) && (mouse_x <= sX + adj_x + 95) && (mouse_y >= sY + adj_y + 353) && (mouse_y <= sY + adj_y + 372))
			hb::shared::text::draw_text(GameFont::Bitmap1, sX + adj_x + 25, sY + adj_y + 330 + 23, "Back", hb::shared::text::TextStyle::with_highlight(GameColors::UIMagicBlue));
		else hb::shared::text::draw_text(GameFont::Bitmap1, sX + adj_x + 25, sY + adj_y + 330 + 23, "Back", hb::shared::text::TextStyle::with_highlight(GameColors::BmpBtnNormal));

		if ((mouse_x >= sX + adj_x + 160) && (mouse_x <= sX + adj_x + 255) && (mouse_y >= sY + adj_y + 353) && (mouse_y <= sY + adj_y + 372)) {
			if (Info().m_str[4] == 1)
				hb::shared::text::draw_text(GameFont::Bitmap1, sX + adj_x + 153, sY + adj_y + 330 + 23, "Manufacture", hb::shared::text::TextStyle::with_highlight(GameColors::UIMagicBlue));
			else hb::shared::text::draw_text(GameFont::Bitmap1, sX + adj_x + 153, sY + adj_y + 330 + 23, "Manufacture", hb::shared::text::TextStyle::with_highlight(GameColors::BmpBtnActive));
		}
		else {
			if (Info().m_str[4] == 1)
				hb::shared::text::draw_text(GameFont::Bitmap1, sX + adj_x + 153, sY + adj_y + 330 + 23, "Manufacture", hb::shared::text::TextStyle::with_highlight(GameColors::BmpBtnNormal));
			else hb::shared::text::draw_text(GameFont::Bitmap1, sX + adj_x + 153, sY + adj_y + 330 + 23, "Manufacture", hb::shared::text::TextStyle::with_highlight(GameColors::BmpBtnActive));
		}
	}
	else {
		put_aligned_string(sX, sX + size_x, sY + adj_y + 200 + 75, DRAW_DIALOGBOX_SKILLDLG18, GameColors::UILabel);
		put_aligned_string(sX, sX + size_x, sY + adj_y + 215 + 75, DRAW_DIALOGBOX_SKILLDLG19, GameColors::UILabel);
		put_aligned_string(sX, sX + size_x, sY + adj_y + 230 + 75, DRAW_DIALOGBOX_SKILLDLG20, GameColors::UILabel);
		if ((mouse_x >= sX + adj_x + 32) && (mouse_x <= sX + adj_x + 95) && (mouse_y >= sY + adj_y + 353) && (mouse_y <= sY + adj_y + 372))
			hb::shared::text::draw_text(GameFont::Bitmap1, sX + adj_x + 25, sY + adj_y + 330 + 23, "Back", hb::shared::text::TextStyle::with_highlight(GameColors::UIMagicBlue));
		else hb::shared::text::draw_text(GameFont::Bitmap1, sX + adj_x + 25, sY + adj_y + 330 + 23, "Back", hb::shared::text::TextStyle::with_highlight(GameColors::BmpBtnNormal));
	}
}

void DialogBox_Manufacture::draw_manufacture_in_progress(short sX, short sY)
{
	int adj_x = -1;
	int adj_y = -7;
	uint32_t time = m_game->m_cur_time;
	short size_x = Info().m_size_x;
	std::string temp;
	int loc;

	draw_new_dialog_box(InterfaceNdGame3, sX, sY, 0);
	draw_new_dialog_box(InterfaceNdText, sX, sY, 8);
	m_game->m_sprite[ItemPackPivotPoint + build_item_manager::get().get_display_list()[Info().m_str[0]]->m_sprite_handle]->draw(sX + adj_x + 62 + 5, sY + adj_y + 84 + 17, build_item_manager::get().get_display_list()[Info().m_str[0]]->m_sprite_frame);

	auto itemInfo4 = item_name_formatter::get().format(m_game->find_item_id_by_name(build_item_manager::get().get_display_list()[Info().m_str[0]]->m_name.c_str()),  0);
	temp = itemInfo4.name.c_str();
	put_string(sX + adj_x + 44 + 10 + 60, sY + adj_y + 55, temp.c_str(), GameColors::UIWhite);

	temp = std::format(DRAW_DIALOGBOX_SKILLDLG7, build_item_manager::get().get_display_list()[Info().m_str[0]]->m_skill_limit, build_item_manager::get().get_display_list()[Info().m_str[0]]->m_max_skill);
	put_string(sX + adj_x + 44 + 10 + 60, sY + adj_y + 55 + 2 * 15, temp.c_str(), GameColors::UILabel);
	put_string(sX + adj_x + 44 + 10 + 60, sY + adj_y + 55 + 3 * 15 + 5, DRAW_DIALOGBOX_SKILLDLG8, GameColors::UILabel);

	loc = 4;
	for (int elem = 1; elem <= 6; elem++)
	{
		if (build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_count[elem] != 0)
		{
			const char* elemName = nullptr;
			bool elemFlag = false;
			switch (elem) {
			case 1: elemName = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_name_1.c_str(); elemFlag = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_flag[1]; break;
			case 2: elemName = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_name_2.c_str(); elemFlag = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_flag[2]; break;
			case 3: elemName = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_name_3.c_str(); elemFlag = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_flag[3]; break;
			case 4: elemName = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_name_4.c_str(); elemFlag = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_flag[4]; break;
			case 5: elemName = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_name_5.c_str(); elemFlag = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_flag[5]; break;
			case 6: elemName = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_name_6.c_str(); elemFlag = build_item_manager::get().get_display_list()[Info().m_str[0]]->m_element_flag[6]; break;
			}
			auto itemInfo5 = item_name_formatter::get().format(m_game->find_item_id_by_name(elemName),  0);
			temp = itemInfo5.name.c_str();
			if (elemFlag)
				put_string(sX + adj_x + 44 + 20 + 60, sY + adj_y + 55 + loc * 15 + 5, temp.c_str(), GameColors::UILabel);
			else
				put_string(sX + adj_x + 44 + 20 + 60, sY + adj_y + 55 + loc * 15 + 5, temp.c_str(), GameColors::UIDisabledMed);
			loc++;
		}
	}

	// draw ingredient slots
	for (int slot = 0; slot < 6; slot++)
	{
		int slotX = (slot % 3) * 45;
		int slotY = (slot / 3) * 45;
		m_game->m_sprite[InterfaceAddInterface]->draw(sX + adj_x + 55 + 30 + slotX + 13, sY + adj_y + 55 + slotY + 180, 2);
	}

	// draw items in slots
	if (Info().m_v1 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v1]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 55 + 30 + 13, sY + adj_y + 55 + 180, cfg->m_sprite_frame);
	}
	if (Info().m_v2 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v2]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 55 + 45 + 30 + 13, sY + adj_y + 55 + 180, cfg->m_sprite_frame);
	}
	if (Info().m_v3 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v3]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 55 + 90 + 30 + 13, sY + adj_y + 55 + 180, cfg->m_sprite_frame);
	}
	if (Info().m_v4 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v4]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 55 + 30 + 13, sY + adj_y + 100 + 180, cfg->m_sprite_frame);
	}
	if (Info().m_v5 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v5]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 55 + 45 + 30 + 13, sY + adj_y + 100 + 180, cfg->m_sprite_frame);
	}
	if (Info().m_v6 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v6]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 55 + 90 + 30 + 13, sY + adj_y + 100 + 180, cfg->m_sprite_frame);
	}

	put_string(sX + adj_x + 33, sY + adj_y + 230 + 75, DRAW_DIALOGBOX_SKILLDLG29, GameColors::UILabel);
	put_string(sX + adj_x + 33, sY + adj_y + 245 + 75, DRAW_DIALOGBOX_SKILLDLG30, GameColors::UILabel);

	if ((time - Info().m_dw_t1) > 1000)
	{
		Info().m_dw_t1 = time;
		Info().m_str[1]++;
		if (Info().m_str[1] >= 7) Info().m_str[1] = 7;
	}

	if (Info().m_str[1] == 4)
	{
		m_game->send_command(MsgId::CommandCommon, CommonType::BuildItem, 0, 0, 0, 0, build_item_manager::get().get_display_list()[Info().m_str[0]]->m_name.c_str());
		Info().m_str[1]++;
	}
}

void DialogBox_Manufacture::draw_manufacture_done(short sX, short sY, short mouse_x, short mouse_y)
{
	int adj_x = -1;
	int adj_y = -7;
	uint32_t time = m_game->m_cur_time;
	std::string temp;

	draw_new_dialog_box(InterfaceNdGame3, sX, sY, 0);
	draw_new_dialog_box(InterfaceNdText, sX, sY, 8);
	m_game->m_sprite[ItemPackPivotPoint + build_item_manager::get().get_display_list()[Info().m_str[0]]->m_sprite_handle]->draw(sX + adj_x + 62 + 5, sY + adj_y + 84 + 17, build_item_manager::get().get_display_list()[Info().m_str[0]]->m_sprite_frame);

	auto itemInfo6 = item_name_formatter::get().format(m_game->find_item_id_by_name(build_item_manager::get().get_display_list()[Info().m_str[0]]->m_name.c_str()),  0);

	temp = itemInfo6.name.c_str();
	put_string(sX + adj_x + 44 + 10 + 60, sY + adj_y + 55, temp.c_str(), GameColors::UIWhite);

	if (Info().m_str[2] == 1) {
		put_string(sX + adj_x + 33 + 11, sY + adj_y + 200 - 45, DRAW_DIALOGBOX_SKILLDLG31, GameColors::UILabel);

		std::string resultBuf;
		if (static_cast<ItemType>(Info().m_v1) == ItemType::Material) {
			resultBuf = std::format(DRAW_DIALOGBOX_SKILLDLG32, Info().m_str[3]);
			put_string(sX + adj_x + 33 + 11, sY + adj_y + 215 - 45, resultBuf.c_str(), GameColors::UILabel);
		}
		else {
			resultBuf = std::format(DRAW_DIALOGBOX_SKILLDLG33, static_cast<int>(Info().m_str[3]) + 100);
			put_string(sX + adj_x + 33, sY + adj_y + 215 - 45, resultBuf.c_str(), GameColors::UILabel);
		}
	}
	else {
		put_string(sX + adj_x + 33 + 11, sY + adj_y + 200, DRAW_DIALOGBOX_SKILLDLG34, GameColors::UILabel);
	}

	if ((mouse_x >= sX + adj_x + 32) && (mouse_x <= sX + adj_x + 95) && (mouse_y >= sY + adj_y + 353) && (mouse_y <= sY + adj_y + 372))
		hb::shared::text::draw_text(GameFont::Bitmap1, sX + adj_x + 35, sY + adj_y + 330 + 23, "Back", hb::shared::text::TextStyle::with_highlight(GameColors::UIMagicBlue));
	else hb::shared::text::draw_text(GameFont::Bitmap1, sX + adj_x + 35, sY + adj_y + 330 + 23, "Back", hb::shared::text::TextStyle::with_highlight(GameColors::BmpBtnNormal));
}

void DialogBox_Manufacture::draw_crafting_waiting(short sX, short sY, short mouse_x, short mouse_y)
{
	int adj_x = 5;
	int adj_y = 8;
	uint32_t time = m_game->m_cur_time;

	m_game->m_sprite[InterfaceCrafting]->draw(sX, sY, 0);

	if (Info().m_v1 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v1]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 55 + (1 - (rand() % 3)), sY + adj_y + 55 + (1 - (rand() % 3)), cfg->m_sprite_frame);
	}
	if (Info().m_v2 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v2]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 65 + 45 + (1 - (rand() % 3)), sY + adj_y + 40 + (1 - (rand() % 3)), cfg->m_sprite_frame);
	}
	if (Info().m_v3 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v3]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 65 + 90 + (1 - (rand() % 3)), sY + adj_y + 55 + (1 - (rand() % 3)), cfg->m_sprite_frame);
	}
	if (Info().m_v4 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v4]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 65 + (1 - (rand() % 3)), sY + adj_y + 100 + (1 - (rand() % 3)), cfg->m_sprite_frame);
	}
	if (Info().m_v5 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v5]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 65 + 45 + (1 - (rand() % 3)), sY + adj_y + 115 + (1 - (rand() % 3)), cfg->m_sprite_frame);
	}
	if (Info().m_v6 != -1) {
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v6]->m_id_num);
		if (cfg) m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 75 + 90 + (1 - (rand() % 3)), sY + adj_y + 100 + (1 - (rand() % 3)), cfg->m_sprite_frame);
	}

	if ((mouse_x >= sX + adj_x + 60) && (mouse_x <= sX + adj_x + 153) && (mouse_y >= sY + adj_y + 175) && (mouse_y <= sY + adj_y + 195))
		hb::shared::text::draw_text(GameFont::Bitmap1, sX + adj_x + 60, sY + adj_y + 175, "Try Now!", hb::shared::text::TextStyle::with_highlight(GameColors::BmpBtnBlue));
	else hb::shared::text::draw_text(GameFont::Bitmap1, sX + adj_x + 60, sY + adj_y + 175, "Try Now!", hb::shared::text::TextStyle::with_highlight(GameColors::UIMagicBlue));
}

void DialogBox_Manufacture::draw_crafting_in_progress(short sX, short sY)
{
	int adj_x = 5;
	int adj_y = 8;
	uint32_t time = m_game->m_cur_time;

	m_game->m_sprite[InterfaceCrafting]->draw(sX, sY, 0);

	if (Info().m_v1 != -1)
	{
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v1]->m_id_num);
		if (cfg) {
			m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 55 + (1 - (rand() % 3)), sY + adj_y + 55 + (1 - (rand() % 3)), cfg->m_sprite_frame);
			if ((cfg->get_item_type() == ItemType::Equip) && (cfg->get_equip_pos() == EquipPos::Neck))
				m_game->m_contribution_price = 10;
		}
	}
	if (Info().m_v2 != -1)
	{
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v2]->m_id_num);
		if (cfg) {
			m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 65 + 45 + (1 - (rand() % 3)), sY + adj_y + 40 + (1 - (rand() % 3)), cfg->m_sprite_frame);
			if ((cfg->get_item_type() == ItemType::Equip) && (cfg->get_equip_pos() == EquipPos::Neck))
				m_game->m_contribution_price = 10;
		}
	}
	if (Info().m_v3 != -1)
	{
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v3]->m_id_num);
		if (cfg) {
			m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 65 + 90 + (1 - (rand() % 3)), sY + adj_y + 55 + (1 - (rand() % 3)), cfg->m_sprite_frame);
			if ((cfg->get_item_type() == ItemType::Equip) && (cfg->get_equip_pos() == EquipPos::Neck))
				m_game->m_contribution_price = 10;
		}
	}
	if (Info().m_v4 != -1)
	{
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v4]->m_id_num);
		if (cfg) {
			m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 65 + (1 - (rand() % 3)), sY + adj_y + 100 + (1 - (rand() % 3)), cfg->m_sprite_frame);
			if ((cfg->get_item_type() == ItemType::Equip) && (cfg->get_equip_pos() == EquipPos::Neck))
				m_game->m_contribution_price = 10;
		}
	}
	if (Info().m_v5 != -1)
	{
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v5]->m_id_num);
		if (cfg) {
			m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 65 + 45 + (1 - (rand() % 3)), sY + adj_y + 115 + (1 - (rand() % 3)), cfg->m_sprite_frame);
			if ((cfg->get_item_type() == ItemType::Equip) && (cfg->get_equip_pos() == EquipPos::Neck))
				m_game->m_contribution_price = 10;
		}
	}
	if (Info().m_v6 != -1)
	{
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_v6]->m_id_num);
		if (cfg) {
			m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + adj_x + 75 + 90 + (1 - (rand() % 3)), sY + adj_y + 100 + (1 - (rand() % 3)), cfg->m_sprite_frame);
			if ((cfg->get_item_type() == ItemType::Equip) && (cfg->get_equip_pos() == EquipPos::Neck))
				m_game->m_contribution_price = 10;
		}
	}

	hb::shared::text::draw_text(GameFont::Bitmap1, sX + adj_x + 60, sY + adj_y + 175, "Creating...", hb::shared::text::TextStyle::with_highlight(GameColors::BmpBtnRed));

	if ((time - Info().m_dw_t1) > 1000)
	{
		Info().m_dw_t1 = time;
		Info().m_str[1]++;
	}
	if (Info().m_str[1] >= 5)
	{
		m_game->send_command(MsgId::CommandCommon, CommonType::CraftItem, 0, 0, 0, 0, 0);
		m_game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::Manufacture);
		m_game->play_game_sound('E', 42, 0);
	}
}

bool DialogBox_Manufacture::on_click(short mouse_x, short mouse_y)
{
	short sX = Info().m_x;
	short sY = Info().m_y;
	int adj_x = 5;
	int adj_y = 8;

	switch (Info().m_mode) {
	case 1: // Alchemy
		if ((mouse_x >= sX + adj_x + 60) && (mouse_x <= sX + adj_x + 153) && (mouse_y >= sY + adj_y + 175) && (mouse_y <= sY + adj_y + 195))
		{
			Info().m_mode = 2;
			Info().m_str[0] = 1;
			Info().m_dw_t1 = m_game->m_cur_time;
			m_game->play_game_sound('E', 14, 5);
			m_game->add_event_list(DLGBOX_CLICK_SKILLDLG1, 10);
			m_game->play_game_sound('E', 41, 0);
			return true;
		}
		break;

	case 7: // Crafting
		if ((mouse_x >= sX + adj_x + 60) && (mouse_x <= sX + adj_x + 153) && (mouse_y >= sY + adj_y + 175) && (mouse_y <= sY + adj_y + 195))
		{
			if (Info().m_v1 == -1)
			{
				m_game->add_event_list(DLGBOX_CLICK_SKILLDLG2, 10);
				m_game->play_game_sound('E', 14, 5);
			}
			else
			{
				Info().m_mode = 8;
				Info().m_dw_t1 = m_game->m_cur_time;
				Info().m_str[1] = 1;
				m_game->play_game_sound('E', 14, 5);
				m_game->add_event_list(DLGBOX_CLICK_SKILLDLG3, 10);
				m_game->play_game_sound('E', 51, 0);
			}
			return true;
		}
		break;

	case 3: // Manufacture list
		adj_x = 5;
		adj_y = 8;
		for (int i = 0; i < 13; i++)
			if (build_item_manager::get().get_display_list()[i + Info().m_view] != 0)
			{
				if ((mouse_x >= sX + adj_x + 44) && (mouse_x <= sX + adj_x + 135 + 44) && (mouse_y >= sY + adj_y + 55 + i * 15) && (mouse_y <= sY + adj_y + 55 + 14 + i * 15)) {
					Info().m_mode = 4;
					Info().m_str[0] = i + Info().m_view;
					m_game->play_game_sound('E', 14, 5);
					return true;
				}
			}
		break;

	case 4: // Manufacture waiting
		adj_x = -1;
		adj_y = -7;
		if (build_item_manager::get().get_display_list()[Info().m_str[0]]->m_build_enabled == true)
		{
			if ((mouse_x >= sX + adj_x + 32) && (mouse_x <= sX + adj_x + 95) && (mouse_y >= sY + adj_y + 353) && (mouse_y <= sY + adj_y + 372)) {
				// Back
				reset_item_slots();
				Info().m_mode = 3;
				m_game->play_game_sound('E', 14, 5);
				return true;
			}

			if ((mouse_x >= sX + adj_x + 160) && (mouse_x <= sX + adj_x + 255) && (mouse_y >= sY + adj_y + 353) && (mouse_y <= sY + adj_y + 372))
			{
				// Manufacture
				if (Info().m_str[4] == 1)
				{
					Info().m_mode = 5;
					Info().m_str[1] = 0;
					Info().m_dw_t1 = m_game->m_cur_time;
					m_game->play_game_sound('E', 14, 5);
					m_game->play_game_sound('E', 44, 0);
				}
				return true;
			}
		}
		else
		{
			if ((mouse_x >= sX + adj_x + 32) && (mouse_x <= sX + adj_x + 95) && (mouse_y >= sY + adj_y + 353) && (mouse_y <= sY + adj_y + 372))
			{
				// Back
				reset_item_slots();
				Info().m_mode = 3;
				m_game->play_game_sound('E', 14, 5);
				return true;
			}
		}
		break;

	case 6: // Manufacture done
		adj_x = -1;
		adj_y = -7;
		if ((mouse_x >= sX + adj_x + 32) && (mouse_x <= sX + adj_x + 95) && (mouse_y >= sY + adj_y + 353) && (mouse_y <= sY + adj_y + 372)) {
			// Back
			reset_item_slots();
			Info().m_mode = 3;
			m_game->play_game_sound('E', 14, 5);
			return true;
		}
		break;
	}

	return false;
}

// Helper: Check if clicking on item in a manufacture slot and handle selection
bool DialogBox_Manufacture::check_slot_item_click(int slotIndex, int itemIdx, int drawX, int drawY, short mouse_x, short mouse_y)
{
	if (itemIdx == -1 || m_game->m_item_list[itemIdx] == nullptr)
		return false;

	CItem* cfg = m_game->get_item_config(m_game->m_item_list[itemIdx]->m_id_num);
	if (!cfg) return false;

	int spriteIdx = ItemPackPivotPoint + cfg->m_sprite;
	m_game->m_sprite[spriteIdx]->CalculateBounds(drawX, drawY, cfg->m_sprite_frame);
	auto bounds = m_game->m_sprite[spriteIdx]->GetBoundRect();

	if (mouse_x > bounds.left && mouse_x < bounds.right && mouse_y > bounds.top && mouse_y < bounds.bottom)
	{
		// clear the slot
		switch (slotIndex)
		{
		case 1: Info().m_v1 = -1; break;
		case 2: Info().m_v2 = -1; break;
		case 3: Info().m_v3 = -1; break;
		case 4: Info().m_v4 = -1; break;
		case 5: Info().m_v5 = -1; break;
		case 6: Info().m_v6 = -1; break;
		}
		m_game->m_is_item_disabled[itemIdx] = false;
		CursorTarget::set_selection(SelectedObjectType::Item, itemIdx, mouse_x - drawX, mouse_y - drawY);
		return true;
	}
	return false;
}

PressResult DialogBox_Manufacture::on_press(short mouse_x, short mouse_y)
{
	short sX = Info().m_x;
	short sY = Info().m_y;
	const int adj_x = 5;
	const int adj_y = 10;

	short array[7] = { 0 };
	array[1] = Info().m_v1;
	array[2] = Info().m_v2;
	array[3] = Info().m_v3;
	array[4] = Info().m_v4;
	array[5] = Info().m_v5;
	array[6] = Info().m_v6;

	switch (Info().m_mode)
	{
	case 1: // Alchemy waiting
		for (int i = 1; i <= 6; i++)
		{
			int itemDrawX, itemDrawY;
			switch (i)
			{
			case 1: itemDrawX = sX + adj_x + 55;          itemDrawY = sY + adj_y + 55; break;
			case 2: itemDrawX = sX + adj_x + 55 + 45 * 1; itemDrawY = sY + adj_y + 55; break;
			case 3: itemDrawX = sX + adj_x + 55 + 45 * 2; itemDrawY = sY + adj_y + 55; break;
			case 4: itemDrawX = sX + adj_x + 55;          itemDrawY = sY + adj_y + 100; break;
			case 5: itemDrawX = sX + adj_x + 55 + 45 * 1; itemDrawY = sY + adj_y + 100; break;
			case 6: itemDrawX = sX + adj_x + 55 + 45 * 2; itemDrawY = sY + adj_y + 100; break;
			default: continue;
			}
			if (check_slot_item_click(i, array[i], itemDrawX, itemDrawY, mouse_x, mouse_y))
				return PressResult::ItemSelected;
		}
		break;

	case 4: // Manufacture waiting
		for (int i = 1; i <= 6; i++)
		{
			int itemDrawX, itemDrawY;
			switch (i)
			{
			case 1: itemDrawX = sX + adj_x + 55 + 30 + 13;          itemDrawY = sY + adj_y + 55 + 180; break;
			case 2: itemDrawX = sX + adj_x + 55 + 45 * 1 + 30 + 13; itemDrawY = sY + adj_y + 55 + 180; break;
			case 3: itemDrawX = sX + adj_x + 55 + 45 * 2 + 30 + 13; itemDrawY = sY + adj_y + 55 + 180; break;
			case 4: itemDrawX = sX + adj_x + 55 + 30 + 13;          itemDrawY = sY + adj_y + 100 + 180; break;
			case 5: itemDrawX = sX + adj_x + 55 + 45 * 1 + 30 + 13; itemDrawY = sY + adj_y + 100 + 180; break;
			case 6: itemDrawX = sX + adj_x + 55 + 45 * 2 + 30 + 13; itemDrawY = sY + adj_y + 100 + 180; break;
			default: continue;
			}
			if (check_slot_item_click(i, array[i], itemDrawX, itemDrawY, mouse_x, mouse_y))
			{
				Info().m_str[4] = static_cast<char>(build_item_manager::get().validate_current_recipe());
				return PressResult::ItemSelected;
			}
		}
		break;

	case 7: // Crafting waiting
		for (int i = 1; i <= 6; i++)
		{
			int itemDrawX, itemDrawY;
			switch (i)
			{
			case 1: itemDrawX = sX + adj_x + 55;          itemDrawY = sY + adj_y + 55; break;
			case 2: itemDrawX = sX + adj_x + 65 + 45 * 1; itemDrawY = sY + adj_y + 40; break;
			case 3: itemDrawX = sX + adj_x + 65 + 45 * 2; itemDrawY = sY + adj_y + 55; break;
			case 4: itemDrawX = sX + adj_x + 65;          itemDrawY = sY + adj_y + 100; break;
			case 5: itemDrawX = sX + adj_x + 65 + 45 * 1; itemDrawY = sY + adj_y + 115; break;
			case 6: itemDrawX = sX + adj_x + 75 + 45 * 2; itemDrawY = sY + adj_y + 100; break;
			default: continue;
			}
			if (check_slot_item_click(i, array[i], itemDrawX, itemDrawY, mouse_x, mouse_y))
				return PressResult::ItemSelected;
		}
		break;
	}

	return PressResult::Normal;
}

bool DialogBox_Manufacture::try_add_item_to_slot(int item_id, bool updateBuildStatus)
{
	auto& info = Info();
	int* slots[] = { &info.m_v1, &info.m_v2, &info.m_v3, &info.m_v4, &info.m_v5, &info.m_v6 };

	for (int i = 0; i < 6; i++)
	{
		if (*slots[i] == -1)
		{
			*slots[i] = item_id;
			if (updateBuildStatus)
				info.m_str[4] = static_cast<char>(build_item_manager::get().validate_current_recipe());

			// Only disable non-stackable items (stackable consumables can be added multiple times)
			CItem* cfg = m_game->get_item_config(m_game->m_item_list[item_id]->m_id_num);
			if (!cfg || cfg->get_item_type() != ItemType::Consume ||
				m_game->m_item_list[item_id]->m_count <= 1)
			{
				m_game->m_is_item_disabled[item_id] = true;
			}
			return true;
		}
	}
	return false;
}

bool DialogBox_Manufacture::on_item_drop(short mouse_x, short mouse_y)
{
	if (m_game->m_player->m_Controller.get_command() < 0) return false;

	int item_id = CursorTarget::get_selected_id();
	if (item_id < 0 || item_id >= hb::shared::limits::MaxItems) return false;
	if (m_game->m_item_list[item_id] == nullptr) return false;
	if (m_game->m_is_item_disabled[item_id]) return false;
	CItem* cfg = m_game->get_item_config(m_game->m_item_list[item_id]->m_id_num);
	if (!cfg) return false;

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

	auto& info = Info();

	switch (info.m_mode) {
	case 1: // Alchemy
	{
		// Check consumable item count - can't add if all instances are already used
		if (cfg->get_item_type() == ItemType::Consume)
		{
			int consume_num = 0;
			if (info.m_v1 == item_id) consume_num++;
			if (info.m_v2 == item_id) consume_num++;
			if (info.m_v3 == item_id) consume_num++;
			if (info.m_v4 == item_id) consume_num++;
			if (info.m_v5 == item_id) consume_num++;
			if (info.m_v6 == item_id) consume_num++;
			if (consume_num >= static_cast<int>(m_game->m_item_list[item_id]->m_count)) return false;
		}

		// Only allow EAT, CONSUME, or NONE item types for alchemy
		if (cfg->get_item_type() != ItemType::Eat &&
			cfg->get_item_type() != ItemType::Consume &&
			cfg->get_item_type() != ItemType::None)
		{
			return false;
		}

		if (!try_add_item_to_slot(item_id, false))
			add_event_list(BITEMDROP_SKILLDIALOG4, 10);
		break;
	}

	case 4: // Manufacture
	{
		// Check consumable item count
		if (cfg->get_item_type() == ItemType::Consume)
		{
			int consume_num = 0;
			if (info.m_v1 == item_id) consume_num++;
			if (info.m_v2 == item_id) consume_num++;
			if (info.m_v3 == item_id) consume_num++;
			if (info.m_v4 == item_id) consume_num++;
			if (info.m_v5 == item_id) consume_num++;
			if (info.m_v6 == item_id) consume_num++;
			if (consume_num >= static_cast<int>(m_game->m_item_list[item_id]->m_count)) return false;
		}

		if (!try_add_item_to_slot(item_id, true))
			add_event_list(BITEMDROP_SKILLDIALOG4, 10);
		break;
	}

	case 7: // Crafting
	{
		// Only allow specific item types for crafting
		if (cfg->get_item_type() != ItemType::None &&      // Merien Stone
			cfg->get_item_type() != ItemType::Equip &&     // Necklaces, Rings
			cfg->get_item_type() != ItemType::Consume &&   // Stones
			cfg->get_item_type() != ItemType::Material)    // Craftwares
		{
			return false;
		}

		if (!try_add_item_to_slot(item_id, false))
			add_event_list(BITEMDROP_SKILLDIALOG4, 10);
		break;
	}
	}

	return true;
}

void DialogBox_Manufacture::reset_item_slots()
{
	if ((Info().m_v1 != -1) && (m_game->m_item_list[Info().m_v1] != 0))
		m_game->m_is_item_disabled[Info().m_v1] = false;
	if ((Info().m_v2 != -1) && (m_game->m_item_list[Info().m_v2] != 0))
		m_game->m_is_item_disabled[Info().m_v2] = false;
	if ((Info().m_v3 != -1) && (m_game->m_item_list[Info().m_v3] != 0))
		m_game->m_is_item_disabled[Info().m_v3] = false;
	if ((Info().m_v4 != -1) && (m_game->m_item_list[Info().m_v4] != 0))
		m_game->m_is_item_disabled[Info().m_v4] = false;
	if ((Info().m_v5 != -1) && (m_game->m_item_list[Info().m_v5] != 0))
		m_game->m_is_item_disabled[Info().m_v5] = false;
	if ((Info().m_v6 != -1) && (m_game->m_item_list[Info().m_v6] != 0))
		m_game->m_is_item_disabled[Info().m_v6] = false;

	Info().m_v1 = -1;
	Info().m_v2 = -1;
	Info().m_v3 = -1;
	Info().m_v4 = -1;
	Info().m_v5 = -1;
	Info().m_v6 = -1;
	Info().m_str[0] = 0;
	Info().m_str[1] = 0;
	Info().m_str[4] = 0;
}
