#include "DialogBox_Resurrect.h"
#include "Game.h"

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
DialogBox_Resurrect::DialogBox_Resurrect(CGame* game)
	: IDialogBox(DialogBoxId::Resurrect, game)
{
	set_default_rect(185 , 100 , 270, 105);
}

void DialogBox_Resurrect::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	draw_new_dialog_box(InterfaceNdGame1, sX, sY, 2);

	put_string(sX + 50, sY + 20, "Someone intend to resurrect you.", GameColors::UIMagicBlue);
	put_string(sX + 80, sY + 35, "Will you revive here?", GameColors::UIMagicBlue);

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

bool DialogBox_Resurrect::on_click(short mouse_x, short mouse_y)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	// Yes button
	if ((mouse_x >= sX + 30) && (mouse_x <= sX + 30 + ui_layout::btn_size_x) && (mouse_y >= sY + 55) && (mouse_y <= sY + 55 + ui_layout::btn_size_y))
	{
		send_command(MsgId::RequestResurrectYes, 0, 0, 0, 0, 0, nullptr, 0);
		disable_this_dialog();
		return true;
	}

	// No button
	if ((mouse_x >= sX + 170) && (mouse_x <= sX + 170 + ui_layout::btn_size_x) && (mouse_y >= sY + 55) && (mouse_y <= sY + 55 + ui_layout::btn_size_y))
	{
		send_command(MsgId::RequestResurrectNo, 0, 0, 0, 0, 0, nullptr, 0);
		disable_this_dialog();
		return true;
	}

	return false;
}
