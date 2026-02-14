#include "DialogBox_ConfirmExchange.h"
#include "Game.h"

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
DialogBox_ConfirmExchange::DialogBox_ConfirmExchange(CGame* game)
	: IDialogBox(DialogBoxId::ConfirmExchange, game)
{
	set_default_rect(285 , 200 , 270, 105);
}

void DialogBox_ConfirmExchange::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	draw_new_dialog_box(InterfaceNdGame1, sX, sY, 2);

	switch (Info().m_mode)
	{
	case 1: // Question
		put_string(sX + 35, sY + 30, "Do you really want to exchange?", GameColors::UIMagicBlue);
		put_string(sX + 36, sY + 30, "Do you really want to exchange?", GameColors::UIMagicBlue);

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
		break;

	case 2: // Waiting for response
		put_string(sX + 45, sY + 36, "Waiting for response...", GameColors::UIMagicBlue);
		put_string(sX + 46, sY + 36, "Waiting for response...", GameColors::UIMagicBlue);
		break;
	}
}

bool DialogBox_ConfirmExchange::on_click(short mouse_x, short mouse_y)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	switch (Info().m_mode)
	{
	case 1: // Not yet confirmed the exchange
		// Yes button
		if ((mouse_x >= sX + 30) && (mouse_x <= sX + 30 + ui_layout::btn_size_x) && (mouse_y >= sY + 55) && (mouse_y <= sY + 55 + ui_layout::btn_size_y))
		{
			if ((m_game->m_dialog_box_exchange_info[0].v1 != -1) && (m_game->m_dialog_box_exchange_info[4].v1 != -1))
			{
				send_command(MsgId::CommandCommon, CommonType::confirm_exchange_item, 0,
					m_game->m_dialog_box_exchange_info[0].v1,  // ItemID
					m_game->m_dialog_box_exchange_info[0].v3,  // Amount
					0, 0);
				play_sound_effect('E', 14, 5);
				m_game->m_dialog_box_manager.Info(DialogBoxId::Exchange).m_mode = 2;
				Info().m_mode = 2;
			}
			return true;
		}

		// No button
		if ((mouse_x >= sX + 170) && (mouse_x <= sX + 170 + ui_layout::btn_size_x) && (mouse_y >= sY + 55) && (mouse_y <= sY + 55 + ui_layout::btn_size_y))
		{
			disable_this_dialog();
			disable_dialog_box(DialogBoxId::Exchange);
			disable_dialog_box(DialogBoxId::Map);
			send_command(MsgId::CommandCommon, CommonType::cancel_exchange_item, 0, 0, 0, 0, 0);
			play_sound_effect('E', 14, 5);
			return true;
		}
		break;

	case 2: // Waiting for other side to confirm
		break;
	}

	return false;
}
