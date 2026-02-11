#include "DialogBox_WarningMsg.h"
#include "Game.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"

DialogBox_WarningMsg::DialogBox_WarningMsg(CGame* pGame)
	: IDialogBox(DialogBoxId::WarningBattleArea, pGame)
{
	SetDefaultRect(0 , 0 , 310, 170);
}

void DialogBox_WarningMsg::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX = Info().sX;
	short sY = Info().sY;

	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME4, sX, sY, 2);

	hb::shared::text::DrawText(GameFont::Default, sX + 63, sY + 35, DEF_MSG_WARNING1, hb::shared::text::TextStyle::WithShadow(GameColors::UIYellow));
	PutString(sX + 30, sY + 57, DEF_MSG_WARNING2, GameColors::UIOrange);
	PutString(sX + 30, sY + 74, DEF_MSG_WARNING3, GameColors::UIOrange);
	PutString(sX + 30, sY + 92, DEF_MSG_WARNING4, GameColors::UIOrange);
	PutString(sX + 30, sY + 110, DEF_MSG_WARNING5, GameColors::UIOrange);

	// OK button
	if ((msX >= sX + 122) && (msX <= sX + 125 + ui_layout::btn_size_x) && (msY >= sY + 127) && (msY <= sY + 127 + ui_layout::btn_size_y))
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + 122, sY + 127, 1);
	else
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + 122, sY + 127, 0);
}

bool DialogBox_WarningMsg::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	// OK button click
	if ((msX >= sX + 120) && (msX <= sX + 120 + ui_layout::btn_size_x) && (msY >= sY + 127) && (msY <= sY + 127 + ui_layout::btn_size_y))
	{
		DisableThisDialog();
		return true;
	}

	return false;
}
