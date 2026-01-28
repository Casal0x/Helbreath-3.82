#include "DialogBox_SysMenu.h"
#include "Game.h"
#include "GlobalDef.h"

DialogBox_SysMenu::DialogBox_SysMenu(CGame* pGame)
	: IDialogBox(DialogBoxId::SystemMenu, pGame)
{
	SetDefaultRect(237 + SCREENX, 67 + SCREENY, 331, 303);
}

void DialogBox_SysMenu::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX = Info().sX;
	short sY = Info().sY;

	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME1, sX, sY, 0);
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_TEXT, sX, sY, 6);

	// Reset scroll selection when mouse button is released
	// This prevents drag from getting stuck if user clicks in old slider regions
	if (cLB == 0)
	{
		Info().bIsScrollSelected = false;
	}
}

bool DialogBox_SysMenu::OnClick(short msX, short msY)
{
	return false;
}
