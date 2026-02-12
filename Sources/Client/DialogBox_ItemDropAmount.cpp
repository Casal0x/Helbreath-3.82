#include "DialogBox_ItemDropAmount.h"
#include "Game.h"
#include "ItemNameFormatter.h"
#include "GlobalDef.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include <format>
#include <string>
using namespace hb::client::sprite_id;

DialogBox_ItemDropAmount::DialogBox_ItemDropAmount(CGame* pGame)
	: IDialogBox(DialogBoxId::ItemDropExternal, pGame)
{
	SetDefaultRect(0 , 0 , 215, 87);
}

void DialogBox_ItemDropAmount::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX = Info().sX;
	short sY = Info().sY;
	std::string cTxt;


	DrawNewDialogBox(InterfaceNdGame2, sX, sY, 5);

	switch (Info().cMode)
	{
	case 1:
	{
		auto itemInfo = ItemNameFormatter::Get().Format(m_pGame->m_pItemList[Info().sView].get());

		if (strlen(Info().cStr) == 0)
			cTxt = std::format(DRAW_DIALOGBOX_QUERY_DROP_ITEM_AMOUNT1, itemInfo.name.c_str());
		else
			cTxt = std::format(DRAW_DIALOGBOX_QUERY_DROP_ITEM_AMOUNT2, itemInfo.name.c_str(), Info().cStr);

		if (Info().sV3 < 1000)
			PutString(sX + 30, sY + 20, cTxt.c_str(), GameColors::UILabel);

		PutString(sX + 30, sY + 35, DRAW_DIALOGBOX_QUERY_DROP_ITEM_AMOUNT3, GameColors::UILabel);

		if (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() != DialogBoxId::ItemDropExternal)
			hb::shared::text::DrawText(GameFont::Default, sX + 40, sY + 57, m_pGame->m_cAmountString, hb::shared::text::TextStyle::Color(GameColors::UIWhite));

		cTxt = std::format("__________ (0 ~ {})", m_pGame->m_pItemList[Info().sView]->m_dwCount);
		PutString(sX + 38, sY + 62, cTxt.c_str(), GameColors::UILabel);
		break;
	}

	case 20:
	{
		auto itemInfo2 = ItemNameFormatter::Get().Format(m_pGame->m_pItemList[Info().sView].get());

		if (strlen(Info().cStr) == 0)
			cTxt = std::format(DRAW_DIALOGBOX_QUERY_DROP_ITEM_AMOUNT1, itemInfo2.name.c_str());
		else
			cTxt = std::format(DRAW_DIALOGBOX_QUERY_DROP_ITEM_AMOUNT2, itemInfo2.name.c_str(), Info().cStr);

		if (Info().sV3 < 1000)
			PutString(sX + 30, sY + 20, cTxt.c_str(), GameColors::UILabel);

		PutString(sX + 30, sY + 35, DRAW_DIALOGBOX_QUERY_DROP_ITEM_AMOUNT3, GameColors::UILabel);
		hb::shared::text::DrawText(GameFont::Default, sX + 40, sY + 57, m_pGame->m_cAmountString, hb::shared::text::TextStyle::Color(GameColors::UIWhite));

		cTxt = std::format("__________ (0 ~ {})", m_pGame->m_pItemList[Info().sView]->m_dwCount);
		PutString(sX + 38, sY + 62, cTxt.c_str(), GameColors::UILabel);
		break;
	}
	}
}

bool DialogBox_ItemDropAmount::OnClick(short msX, short msY)
{
	// Click handling for this dialog is done elsewhere (text input handling)
	return false;
}
