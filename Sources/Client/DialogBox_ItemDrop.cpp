#include "DialogBox_ItemDrop.h"
#include "Game.h"
#include "ItemNameFormatter.h"
#include "lan_eng.h"
#include <string>

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
DialogBox_ItemDrop::DialogBox_ItemDrop(CGame* game)
	: IDialogBox(DialogBoxId::ItemDropConfirm, game)
{
	set_default_rect(0 , 0 , 270, 105);
}

void DialogBox_ItemDrop::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
	if (!m_game->ensure_item_configs_loaded()) return;
	short sX = Info().m_x;
	short sY = Info().m_y;
	std::string txt;


	draw_new_dialog_box(InterfaceNdGame1, sX, sY, 2);

	auto itemInfo = item_name_formatter::get().format(m_game->m_item_list[Info().m_view].get());

	if (Info().m_str[0] == '\0')
		txt = itemInfo.name.c_str();

	// Item name (green if special, blue otherwise)
	if (itemInfo.is_special)
	{
		put_string(sX + 35, sY + 20, txt.c_str(), GameColors::UIItemName_Special);
		put_string(sX + 36, sY + 20, txt.c_str(), GameColors::UIItemName_Special);
	}
	else
	{
		put_string(sX + 35, sY + 20, txt.c_str(), GameColors::UIMagicBlue);
		put_string(sX + 36, sY + 20, txt.c_str(), GameColors::UIMagicBlue);
	}

	// "Do you want to drop?" text
	put_string(sX + 35, sY + 36, DRAW_DIALOGBOX_ITEM_DROP1, GameColors::UIMagicBlue);
	put_string(sX + 36, sY + 36, DRAW_DIALOGBOX_ITEM_DROP1, GameColors::UIMagicBlue);

	// toggle option text
	if (m_game->m_item_drop)
	{
		// "Don't show this message again"
		if ((mouse_x >= sX + 35) && (mouse_x <= sX + 240) && (mouse_y >= sY + 80) && (mouse_y <= sY + 90))
		{
			put_string(sX + 35, sY + 80, DRAW_DIALOGBOX_ITEM_DROP2, GameColors::UIWhite);
			put_string(sX + 36, sY + 80, DRAW_DIALOGBOX_ITEM_DROP2, GameColors::UIWhite);
		}
		else
		{
			put_string(sX + 35, sY + 80, DRAW_DIALOGBOX_ITEM_DROP2, GameColors::UIMagicBlue);
			put_string(sX + 36, sY + 80, DRAW_DIALOGBOX_ITEM_DROP2, GameColors::UIMagicBlue);
		}
	}
	else
	{
		// "Show this message again"
		if ((mouse_x >= sX + 35) && (mouse_x <= sX + 240) && (mouse_y >= sY + 80) && (mouse_y <= sY + 90))
		{
			put_string(sX + 35, sY + 80, DRAW_DIALOGBOX_ITEM_DROP3, GameColors::UIWhite);
			put_string(sX + 36, sY + 80, DRAW_DIALOGBOX_ITEM_DROP3, GameColors::UIWhite);
		}
		else
		{
			put_string(sX + 35, sY + 80, DRAW_DIALOGBOX_ITEM_DROP3, GameColors::UIMagicBlue);
			put_string(sX + 36, sY + 80, DRAW_DIALOGBOX_ITEM_DROP3, GameColors::UIMagicBlue);
		}
	}

	// Yes button
	if ((mouse_x >= sX + 30) && (mouse_x <= sX + 30 + ui_layout::btn_size_x) && (mouse_y >= sY + 55) && (mouse_y <= sY + 55 + ui_layout::btn_size_y))
		draw_new_dialog_box(InterfaceNdButton, sX + 30, sY + 55, 19);
	else
		draw_new_dialog_box(InterfaceNdButton, sX + 30, sY + 55, 18);

	// No button
	if ((mouse_x >= sX + 170) && (mouse_x <= sX + 170 + ui_layout::btn_size_x) && (mouse_y >= sY + 55) && (mouse_y <= sY + 55 + ui_layout::btn_size_y))
		draw_new_dialog_box(InterfaceNdButton, sX + 170, sY + 55, 3);
	else
		draw_new_dialog_box(InterfaceNdButton, sX + 170, sY + 55, 2);
}

bool DialogBox_ItemDrop::on_click(short mouse_x, short mouse_y)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	if (m_game->m_player->m_Controller.get_command() < 0) return false;

	// Yes button - drop item
	if ((mouse_x >= sX + 30) && (mouse_x <= sX + 30 + ui_layout::btn_size_x) && (mouse_y >= sY + 55) && (mouse_y <= sY + 55 + ui_layout::btn_size_y))
	{
		Info().m_mode = 3;
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[Info().m_view]->m_id_num);
		if (cfg) send_command(MsgId::CommandCommon,
			CommonType::ItemDrop,
			0,
			Info().m_view,
			1,
			0,
			cfg->m_name);
		disable_this_dialog();
		return true;
	}

	// No button - cancel
	if ((mouse_x >= sX + 170) && (mouse_x <= sX + 170 + ui_layout::btn_size_x) && (mouse_y >= sY + 55) && (mouse_y <= sY + 55 + ui_layout::btn_size_y))
	{
		for (int i = 0; i < game_limits::max_sell_list; i++)
			m_game->m_is_item_disabled[i] = false;

		disable_this_dialog();
		return true;
	}

	// toggle "don't show again" option
	if ((mouse_x >= sX + 35) && (mouse_x <= sX + 240) && (mouse_y >= sY + 80) && (mouse_y <= sY + 90))
	{
		m_game->m_item_drop = !m_game->m_item_drop;
		return true;
	}

	return false;
}
