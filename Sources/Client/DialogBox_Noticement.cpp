#include "DialogBox_Noticement.h"
#include "Game.h"
#include "lan_eng.h"
using namespace hb::client::sprite_id;

DialogBox_Noticement::DialogBox_Noticement(CGame* pGame)
	: IDialogBox(DialogBoxId::Noticement, pGame)
{
	SetDefaultRect(162 , 40 , 315, 171);
}

void DialogBox_Noticement::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX = Info().sX;
	short sY = Info().sY;
	short szX = Info().sSizeX;

	DrawNewDialogBox(InterfaceNdGame4, sX, sY, 2);

	switch (Info().cMode)
	{
	case 1: // Server shutting down in X minutes
		{
			char msgBuf[128];
			if (Info().sV1 != 0)
				snprintf(msgBuf, sizeof(msgBuf), DRAW_DIALOGBOX_NOTICEMSG1, Info().sV1);
			else
				std::snprintf(msgBuf, sizeof(msgBuf), "%s", DRAW_DIALOGBOX_NOTICEMSG2);
			PutAlignedString(sX, sX + szX, sY + 31, msgBuf, GameColors::UINoticeRed);
		}
		PutAlignedString(sX, sX + szX, sY + 48, DRAW_DIALOGBOX_NOTICEMSG3);
		PutAlignedString(sX, sX + szX, sY + 65, DRAW_DIALOGBOX_NOTICEMSG4);
		PutAlignedString(sX, sX + szX, sY + 82, DRAW_DIALOGBOX_NOTICEMSG5);
		PutAlignedString(sX, sX + szX, sY + 99, DRAW_DIALOGBOX_NOTICEMSG6);
		break;

	case 2: // Shutdown has started
		PutAlignedString(sX, sX + szX, sY + 31, DRAW_DIALOGBOX_NOTICEMSG7, GameColors::UINoticeRed);
		PutAlignedString(sX, sX + szX, sY + 48, DRAW_DIALOGBOX_NOTICEMSG8);
		PutAlignedString(sX, sX + szX, sY + 65, DRAW_DIALOGBOX_NOTICEMSG9);
		PutAlignedString(sX, sX + szX, sY + 82, DRAW_DIALOGBOX_NOTICEMSG10);
		PutAlignedString(sX, sX + szX, sY + 99, DRAW_DIALOGBOX_NOTICEMSG11);
		break;
	}

	// OK button (same position for both modes)
	if ((msX >= sX + 210) && (msX <= sX + 210 + ui_layout::btn_size_x) && (msY > sY + 127) && (msY < sY + 127 + ui_layout::btn_size_y))
		DrawNewDialogBox(InterfaceNdButton, sX + 210, sY + 127, 1);
	else
		DrawNewDialogBox(InterfaceNdButton, sX + 210, sY + 127, 0);
}

bool DialogBox_Noticement::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	// OK button
	if ((msX >= sX + 210) && (msX <= sX + 210 + ui_layout::btn_size_x) && (msY > sY + 127) && (msY < sY + 127 + ui_layout::btn_size_y))
	{
		PlaySoundEffect('E', 14, 5);
		DisableThisDialog();
		return true;
	}

	return false;
}
