#include "DialogBox_Noticement.h"
#include "Game.h"
#include "lan_eng.h"
#include <format>
#include <string>
using namespace hb::client::sprite_id;

DialogBox_Noticement::DialogBox_Noticement(CGame* game)
	: IDialogBox(DialogBoxId::Noticement, game)
{
	set_default_rect(162 , 40 , 315, 171);
}

void DialogBox_Noticement::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
	short sX = Info().m_x;
	short sY = Info().m_y;
	short size_x = Info().m_size_x;

	draw_new_dialog_box(InterfaceNdGame4, sX, sY, 2);

	switch (Info().m_mode)
	{
	case 1: // Server shutting down in X minutes
		{
			std::string msgBuf;
			if (Info().m_v1 != 0)
				msgBuf = std::format(DRAW_DIALOGBOX_NOTICEMSG1, Info().m_v1);
			else
				msgBuf = DRAW_DIALOGBOX_NOTICEMSG2;
			put_aligned_string(sX, sX + size_x, sY + 31, msgBuf.c_str(), GameColors::UINoticeRed);
		}
		put_aligned_string(sX, sX + size_x, sY + 48, DRAW_DIALOGBOX_NOTICEMSG3);
		put_aligned_string(sX, sX + size_x, sY + 65, DRAW_DIALOGBOX_NOTICEMSG4);
		put_aligned_string(sX, sX + size_x, sY + 82, DRAW_DIALOGBOX_NOTICEMSG5);
		put_aligned_string(sX, sX + size_x, sY + 99, DRAW_DIALOGBOX_NOTICEMSG6);
		break;

	case 2: // shutdown has started
		put_aligned_string(sX, sX + size_x, sY + 31, DRAW_DIALOGBOX_NOTICEMSG7, GameColors::UINoticeRed);
		put_aligned_string(sX, sX + size_x, sY + 48, DRAW_DIALOGBOX_NOTICEMSG8);
		put_aligned_string(sX, sX + size_x, sY + 65, DRAW_DIALOGBOX_NOTICEMSG9);
		put_aligned_string(sX, sX + size_x, sY + 82, DRAW_DIALOGBOX_NOTICEMSG10);
		put_aligned_string(sX, sX + size_x, sY + 99, DRAW_DIALOGBOX_NOTICEMSG11);
		break;
	}

	// OK button (same position for both modes)
	if ((mouse_x >= sX + 210) && (mouse_x <= sX + 210 + ui_layout::btn_size_x) && (mouse_y > sY + 127) && (mouse_y < sY + 127 + ui_layout::btn_size_y))
		draw_new_dialog_box(InterfaceNdButton, sX + 210, sY + 127, 1);
	else
		draw_new_dialog_box(InterfaceNdButton, sX + 210, sY + 127, 0);
}

bool DialogBox_Noticement::on_click(short mouse_x, short mouse_y)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	// OK button
	if ((mouse_x >= sX + 210) && (mouse_x <= sX + 210 + ui_layout::btn_size_x) && (mouse_y > sY + 127) && (mouse_y < sY + 127 + ui_layout::btn_size_y))
	{
		play_sound_effect('E', 14, 5);
		disable_this_dialog();
		return true;
	}

	return false;
}
