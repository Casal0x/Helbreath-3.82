#include "DialogBox_ItemDropAmount.h"
#include "Game.h"
#include "GlobalDef.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"

DialogBox_ItemDropAmount::DialogBox_ItemDropAmount(CGame* pGame)
	: IDialogBox(DialogBoxId::ItemDropExternal, pGame)
{
	SetDefaultRect(0 + SCREENX, 0 + SCREENY, 215, 87);
}

void DialogBox_ItemDropAmount::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX = Info().sX;
	short sY = Info().sY;
	char cTxt[120], cStr1[64], cStr2[64], cStr3[64];

	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 5);

	switch (Info().cMode)
	{
	case 1:
		m_pGame->GetItemName(m_pGame->m_pItemList[Info().sView].get(), cStr1, cStr2, cStr3);

		if (strlen(Info().cStr) == 0)
			wsprintf(cTxt, DRAW_DIALOGBOX_QUERY_DROP_ITEM_AMOUNT1, cStr1);
		else
			wsprintf(cTxt, DRAW_DIALOGBOX_QUERY_DROP_ITEM_AMOUNT2, cStr1, Info().cStr);

		if (Info().sV3 < 1000)
			PutString(sX + 30, sY + 20, cTxt, GameColors::UILabel.ToColorRef());

		PutString(sX + 30, sY + 35, DRAW_DIALOGBOX_QUERY_DROP_ITEM_AMOUNT3, GameColors::UILabel.ToColorRef());

		if (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() != DialogBoxId::ItemDropExternal)
			TextLib::DrawText(GameFont::Default, sX + 40, sY + 57, m_pGame->m_cAmountString, TextLib::TextStyle::FromColorRef(GameColors::UIWhite.ToColorRef()));

		wsprintf(cTxt, "__________ (0 ~ %d)", m_pGame->m_pItemList[Info().sView]->m_dwCount);
		PutString(sX + 38, sY + 62, cTxt, GameColors::UILabel.ToColorRef());
		break;

	case 20:
		m_pGame->GetItemName(m_pGame->m_pItemList[Info().sView].get(), cStr1, cStr2, cStr3);

		if (strlen(Info().cStr) == 0)
			wsprintf(cTxt, DRAW_DIALOGBOX_QUERY_DROP_ITEM_AMOUNT1, cStr1);
		else
			wsprintf(cTxt, DRAW_DIALOGBOX_QUERY_DROP_ITEM_AMOUNT2, cStr1, Info().cStr);

		if (Info().sV3 < 1000)
			PutString(sX + 30, sY + 20, cTxt, GameColors::UILabel.ToColorRef());

		PutString(sX + 30, sY + 35, DRAW_DIALOGBOX_QUERY_DROP_ITEM_AMOUNT3, GameColors::UILabel.ToColorRef());
		TextLib::DrawText(GameFont::Default, sX + 40, sY + 57, m_pGame->m_cAmountString, TextLib::TextStyle::FromColorRef(GameColors::UIWhite.ToColorRef()));

		wsprintf(cTxt, "__________ (0 ~ %d)", m_pGame->m_pItemList[Info().sView]->m_dwCount);
		PutString(sX + 38, sY + 62, cTxt, GameColors::UILabel.ToColorRef());
		break;
	}
}

bool DialogBox_ItemDropAmount::OnClick(short msX, short msY)
{
	// Click handling for this dialog is done elsewhere (text input handling)
	return false;
}
