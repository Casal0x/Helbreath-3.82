#include "DialogBox_Fishing.h"
#include "Game.h"
#include "ItemNameFormatter.h"
#include "GameFonts.h"
#include "GlobalDef.h"
#include "TextLibExt.h"
#include "lan_eng.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
DialogBox_Fishing::DialogBox_Fishing(CGame* pGame)
	: IDialogBox(DialogBoxId::Fishing, pGame)
{
	SetDefaultRect(193 , 241 , 263, 100);
}

void DialogBox_Fishing::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX = Info().sX;
	short sY = Info().sY;
	uint32_t dwTime = m_pGame->m_dwCurTime;
	std::string cTxt;

	DrawNewDialogBox(InterfaceNdGame1, sX, sY, 2);

	auto itemInfo = ItemNameFormatter::Get().Format(m_pGame->FindItemIdByName(Info().cStr),  0);

	switch (Info().cMode)
	{
	case 0:
		m_pGame->m_pSprite[ItemPackPivotPoint + Info().sV3]->Draw(sX + 18 + 35, sY + 18 + 17, Info().sV4);

		cTxt = itemInfo.name.c_str();
		PutString(sX + 98, sY + 14, cTxt.c_str(), GameColors::UIWhite);

		cTxt = std::format(DRAW_DIALOGBOX_FISHING1, Info().sV2);
		PutString(sX + 98, sY + 28, cTxt.c_str(), GameColors::UIBlack);

		PutString(sX + 97, sY + 43, DRAW_DIALOGBOX_FISHING2, GameColors::UIBlack);

		cTxt = std::format("{} %", Info().sV1);
		hb::shared::text::DrawText(GameFont::Bitmap1, sX + 157, sY + 40, cTxt.c_str(), hb::shared::text::TextStyle::WithHighlight(GameColors::BmpBtnFishRed));

		// "Try Now!" button
		if ((msX >= sX + 160) && (msX <= sX + 253) && (msY >= sY + 70) && (msY <= sY + 90))
			hb::shared::text::DrawText(GameFont::Bitmap1, sX + 160, sY + 70, "Try Now!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicBlue));
		else
			hb::shared::text::DrawText(GameFont::Bitmap1, sX + 160, sY + 70, "Try Now!", hb::shared::text::TextStyle::WithHighlight(GameColors::BmpBtnNormal));
		break;
	}
}

bool DialogBox_Fishing::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	switch (Info().cMode)
	{
	case 0:
		if ((msX >= sX + 160) && (msX <= sX + 253) && (msY >= sY + 70) && (msY <= sY + 90))
		{
			m_pGame->bSendCommand(MsgId::CommandCommon, CommonType::ReqGetFishThisTime, 0, 0, 0, 0, 0);
			m_pGame->AddEventList(DLGBOX_CLICK_FISH1, 10);
			DisableDialogBox(DialogBoxId::Fishing);
			m_pGame->PlayGameSound('E', 14, 5);
			return true;
		}
		break;
	}

	return false;
}
