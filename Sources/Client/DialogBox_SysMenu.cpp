#include "DialogBox_SysMenu.h"
#include "Game.h"
#include "GlobalDef.h"

DialogBox_SysMenu::DialogBox_SysMenu(CGame* pGame)
	: IDialogBox(DialogBoxId::SystemMenu, pGame)
{
	SetDefaultRect(237 + SCREENX, 67 + SCREENY, 331, 303);
}

void DialogBox_SysMenu::OnUpdate()
{
	short sX = Info().sX;
	short sY = Info().sY;

}

void DialogBox_SysMenu::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX = Info().sX;
	short sY = Info().sY;

	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME1, sX, sY, 0);
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_TEXT, sX, sY, 6);

	// General Tab button
	int btnX = sX + 15;
	int btnY = sY + 33;
	SpriteLib::SpriteRect button_rect = m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->GetFrameRect(70);

	if (msX >= btnX && msX < btnX + button_rect.width && msY >= btnY && msY < btnY + button_rect.height)
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, btnX, btnY, 71);
	else
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, btnX, btnY, 70);

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
