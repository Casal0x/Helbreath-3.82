#include "DialogBox_RepairAll.h"
#include "Game.h"
#include "IInput.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
DialogBox_RepairAll::DialogBox_RepairAll(CGame* game)
	: IDialogBox(DialogBoxId::RepairAll, game)
{
	set_default_rect(337 , 57 , 258, 339);
}

void DialogBox_RepairAll::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
	if (!m_game->ensure_item_configs_loaded()) return;
	short sX = Info().m_x;
	short sY = Info().m_y;
	short size_x = Info().m_size_x;
	std::string txt;
	int total_lines, pointer_loc;
	double d1, d2, d3;

	draw_new_dialog_box(InterfaceNdGame2, sX, sY, 2);
	draw_new_dialog_box(InterfaceNdText, sX, sY, 10);

	for (int i = 0; i < 15; i++)
	{
		if ((i + Info().m_view) < m_game->totalItemRepair)
		{
			CItem* cfg = m_game->get_item_config(m_game->m_item_list[m_game->m_repair_all[i + Info().m_view].index]->m_id_num);
			txt = std::format("{} - Cost: {}", cfg ? cfg->m_name : "Unknown", m_game->m_repair_all[i + Info().m_view].price);

			put_string(sX + 30, sY + 45 + i * 15, txt.c_str(), GameColors::UIBlack);
			m_game->m_is_item_disabled[m_game->m_repair_all[i + Info().m_view].index] = true;
		}
	}

	total_lines = m_game->totalItemRepair;
	if (total_lines > 15)
	{
		d1 = static_cast<double>(Info().m_view);
		d2 = static_cast<double>(total_lines - 15);
		d3 = (274.0f * d1) / d2;
		pointer_loc = static_cast<int>(d3);
	}
	else
	{
		pointer_loc = 0;
	}

	if (total_lines > 15)
	{
		draw_new_dialog_box(InterfaceNdGame2, sX, sY, 1);
		draw_new_dialog_box(InterfaceNdGame2, sX + 242, sY + pointer_loc + 35, 7);
	}

	// Mouse wheel scrolling
	if (total_lines > 15)
	{
		if (m_game->m_dialog_box_manager.get_top_dialog_box_index() == DialogBoxId::RepairAll && z != 0)
		{
			if (z > 0) Info().m_view--;
			if (z < 0) Info().m_view++;
		}

		if (Info().m_view < 0)
			Info().m_view = 0;

		if (total_lines > 15 && Info().m_view > total_lines - 15)
			Info().m_view = total_lines - 15;
	}

	if (m_game->totalItemRepair > 0)
	{
		// Repair button
		if ((mouse_x >= sX + ui_layout::left_btn_x) && (mouse_x <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 43);
		else
			draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 42);

		// Cancel button
		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 17);
		else
			draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 16);

		// Total cost
		txt = std::format("Total cost : {}", m_game->totalPrice);
		put_string(sX + 30, sY + 270, txt.c_str(), GameColors::UIBlack);
	}
	else
	{
		// No items to repair
		put_aligned_string(sX, sX + size_x, sY + 140, "There are no items to repair.", GameColors::UIBlack);

		// Cancel button only
		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 17);
		else
			draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 16);
	}
}

bool DialogBox_RepairAll::on_click(short mouse_x, short mouse_y)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	for (int i = 0; i < 15; i++)
	{
		if ((i + Info().m_view) < m_game->totalItemRepair)
		{
			// Repair button
			if ((mouse_x >= sX + 30) && (mouse_x <= sX + 30 + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			{
				send_command(MsgId::CommandCommon, CommonType::ReqRepairAllConfirm, 0, 0, 0, 0, 0);
				disable_this_dialog();
				return true;
			}

			// Cancel button
			if ((mouse_x >= sX + 154) && (mouse_x <= sX + 154 + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			{
				disable_this_dialog();
				return true;
			}
		}
		else
		{
			// Cancel button (no items)
			if ((mouse_x >= sX + 154) && (mouse_x <= sX + 154 + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			{
				disable_this_dialog();
				return true;
			}
		}
	}

	return false;
}

