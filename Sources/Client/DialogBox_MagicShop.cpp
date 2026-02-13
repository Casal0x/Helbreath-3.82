#include "DialogBox_MagicShop.h"
#include "Game.h"
#include "IInput.h"
#include "Misc.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
DialogBox_MagicShop::DialogBox_MagicShop(CGame* pGame)
	: IDialogBox(DialogBoxId::MagicShop, pGame)
{
	SetDefaultRect(30 , 30 , 304, 328);
}

void DialogBox_MagicShop::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sX;
	short sY = m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sY;

	m_pGame->DrawNewDialogBox(InterfaceNdGame4, sX, sY, 1);
	m_pGame->DrawNewDialogBox(InterfaceNdText, sX, sY, 14);

	// Mouse wheel scrolling - read and consume input directly
	if (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::MagicShop)
	{
		short sWheelDelta = hb::shared::input::get_mouse_wheel_delta();
		if (sWheelDelta != 0)
		{
			if (sWheelDelta > 0)
				m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView--;
			if (sWheelDelta < 0)
				m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView++;
			// Consume the input
		}
	}

	// Clamp view (pages 0-9)
	if (m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView < 0)
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView = 9;
	if (m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView > 9)
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView = 0;

	// Column headers
	hb::shared::text::DrawText(GameFont::Default, sX - 20 + 60 - 17, sY - 35 + 90, DRAW_DIALOGBOX_MAGICSHOP11, hb::shared::text::TextStyle::Color(GameColors::UILabel)); // "Spell Name"
	hb::shared::text::DrawText(GameFont::Default, sX - 20 + 232 - 20, sY - 35 + 90, DRAW_DIALOGBOX_MAGICSHOP12, hb::shared::text::TextStyle::Color(GameColors::UILabel)); // "Req.Int"
	hb::shared::text::DrawText(GameFont::Default, sX - 20 + 270, sY - 35 + 90, DRAW_DIALOGBOX_MAGICSHOP13, hb::shared::text::TextStyle::Color(GameColors::UILabel)); // "Cost"

	DrawSpellList(sX, sY, msX, msY);
	DrawPageIndicator(sX, sY);

	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 275, sX + m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sSizeX - sX, 15, DRAW_DIALOGBOX_MAGICSHOP14, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
}

void DialogBox_MagicShop::DrawSpellList(short sX, short sY, short msX, short msY)
{
	int iCPivot = m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView * 10;
	int iYloc = 0;
	std::string cMana;

	char cTxt[120];

	for (int i = 0; i < 9; i++)
	{
		if ((m_pGame->m_pMagicCfgList[iCPivot + i] != nullptr) &&
			(m_pGame->m_pMagicCfgList[iCPivot + i]->m_bIsVisible))
		{
			std::snprintf(cTxt, sizeof(cTxt), "%s", m_pGame->m_pMagicCfgList[iCPivot + i]->m_cName.c_str());
			CMisc::ReplaceString(cTxt, '-', ' ');

			if (m_pGame->m_pPlayer->m_iMagicMastery[iCPivot + i] != 0)
			{
				// Already mastered - purple color
				hb::shared::text::DrawText(GameFont::Bitmap1, sX + 24, sY + 70 + iYloc, cTxt, hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicPurple));
				cMana = std::format("{:3}", m_pGame->m_pMagicCfgList[iCPivot + i]->m_sValue2);
				hb::shared::text::DrawText(GameFont::Bitmap1, sX + 200, sY + 70 + iYloc, cMana.c_str(), hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicPurple));
				cMana = std::format("{:3}", m_pGame->m_pMagicCfgList[iCPivot + i]->m_sValue3);
				hb::shared::text::DrawText(GameFont::Bitmap1, sX + 241, sY + 70 + iYloc, cMana.c_str(), hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicPurple));
			}
			else if ((msX >= sX + 24) && (msX <= sX + 24 + 135) &&
				(msY >= sY + 70 + iYloc) && (msY <= sY + 70 + 14 + iYloc))
			{
				// Hovering - white color
				hb::shared::text::DrawText(GameFont::Bitmap1, sX - 20 + 44, sY + 70 + iYloc, cTxt, hb::shared::text::TextStyle::WithHighlight(GameColors::UINearWhite));
				cMana = std::format("{:3}", m_pGame->m_pMagicCfgList[iCPivot + i]->m_sValue2);
				hb::shared::text::DrawText(GameFont::Bitmap1, sX - 20 + 220, sY + 70 + iYloc, cMana.c_str(), hb::shared::text::TextStyle::WithHighlight(GameColors::UINearWhite));
				cMana = std::format("{:3}", m_pGame->m_pMagicCfgList[iCPivot + i]->m_sValue3);
				hb::shared::text::DrawText(GameFont::Bitmap1, sX - 20 + 261, sY + 70 + iYloc, cMana.c_str(), hb::shared::text::TextStyle::WithHighlight(GameColors::UINearWhite));
			}
			else
			{
				hb::shared::text::DrawText(GameFont::Bitmap1, sX - 20 + 44, sY + 70 + iYloc, cTxt, hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicBlue));
				cMana = std::format("{:3}", m_pGame->m_pMagicCfgList[iCPivot + i]->m_sValue2);
				hb::shared::text::DrawText(GameFont::Bitmap1, sX - 20 + 220, sY + 70 + iYloc, cMana.c_str(), hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicBlue));
				cMana = std::format("{:3}", m_pGame->m_pMagicCfgList[iCPivot + i]->m_sValue3);
				hb::shared::text::DrawText(GameFont::Bitmap1, sX - 20 + 261, sY + 70 + iYloc, cMana.c_str(), hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicBlue));
			}
			iYloc += 18;
		}
	}
}

void DialogBox_MagicShop::DrawPageIndicator(short sX, short sY)
{
	uint32_t dwTime = m_pGame->m_dwCurTime;

	// Draw page number strip
	m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX + 55, sY + 250, 19);

	// Highlight current page
	short sView = m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView;
	switch (sView)
	{
	case 0: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX - 20 + 44 + 31, sY + 250, 20); break;
	case 1: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX - 20 + 57 + 31, sY + 250, 21); break;
	case 2: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX - 20 + 75 + 31, sY + 250, 22); break;
	case 3: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX - 20 + 100 + 31, sY + 250, 23); break;
	case 4: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX - 20 + 120 + 31, sY + 250, 24); break;
	case 5: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX - 20 + 135 + 31, sY + 250, 25); break;
	case 6: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX - 20 + 156 + 31, sY + 250, 26); break;
	case 7: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX - 20 + 183 + 31, sY + 250, 27); break;
	case 8: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX - 20 + 216 + 31, sY + 250, 28); break;
	case 9: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX - 20 + 236 + 31, sY + 250, 29); break;
	}
}

bool DialogBox_MagicShop::OnClick(short msX, short msY)
{
	short sX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sX;
	short sY = m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sY;

	if (HandleSpellClick(sX, sY, msX, msY))
		return true;

	HandlePageClick(sX, sY, msX, msY);
	return false;
}

bool DialogBox_MagicShop::HandleSpellClick(short sX, short sY, short msX, short msY)
{
	int iAdjX = -20;
	int iAdjY = -35;
	int iCPivot = m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView * 10;
	int iYloc = 0;

	for (int i = 0; i < 9; i++)
	{
		if ((m_pGame->m_pMagicCfgList[iCPivot + i] != nullptr) &&
			(m_pGame->m_pMagicCfgList[iCPivot + i]->m_bIsVisible))
		{
			if ((msX >= sX + iAdjX + 44) && (msX <= sX + iAdjX + 135 + 44) &&
				(msY >= sY + iAdjY + 70 + iYloc + 35) && (msY <= sY + iAdjY + 70 + 14 + iYloc + 35))
			{
				if (m_pGame->m_pPlayer->m_iMagicMastery[iCPivot + i] == 0)
				{
					m_pGame->bSendCommand(MsgId::CommandCommon, CommonType::ReqStudyMagic, 0, 0, 0, 0,
						m_pGame->m_pMagicCfgList[iCPivot + i]->m_cName.c_str());
					m_pGame->PlayGameSound('E', 14, 5);
				}
				return true;
			}
			iYloc += 18;
		}
	}
	return false;
}

void DialogBox_MagicShop::HandlePageClick(short sX, short sY, short msX, short msY)
{
	int iAdjX = -20;
	int iAdjY = -35;

	if ((msX >= sX + iAdjX + 42 + 31) && (msX <= sX + iAdjX + 50 + 31) &&
		(msY >= sY + iAdjY + 248 + 35) && (msY <= sY + iAdjY + 260 + 35))
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView = 0;

	if ((msX >= sX + iAdjX + 55 + 31) && (msX <= sX + iAdjX + 68 + 31) &&
		(msY >= sY + iAdjY + 248 + 35) && (msY <= sY + iAdjY + 260 + 35))
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView = 1;

	if ((msX >= sX + iAdjX + 73 + 31) && (msX <= sX + iAdjX + 93 + 31) &&
		(msY >= sY + iAdjY + 248 + 35) && (msY <= sY + iAdjY + 260 + 35))
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView = 2;

	if ((msX >= sX + iAdjX + 98 + 31) && (msX <= sX + iAdjX + 113 + 31) &&
		(msY >= sY + iAdjY + 248 + 35) && (msY <= sY + iAdjY + 260 + 35))
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView = 3;

	if ((msX >= sX + iAdjX + 118 + 31) && (msX <= sX + iAdjX + 129 + 31) &&
		(msY >= sY + iAdjY + 248 + 35) && (msY <= sY + iAdjY + 260 + 35))
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView = 4;

	if ((msX >= sX + iAdjX + 133 + 31) && (msX <= sX + iAdjX + 150 + 31) &&
		(msY >= sY + iAdjY + 248 + 35) && (msY <= sY + iAdjY + 260 + 35))
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView = 5;

	if ((msX >= sX + iAdjX + 154 + 31) && (msX <= sX + iAdjX + 177 + 31) &&
		(msY >= sY + iAdjY + 248 + 35) && (msY <= sY + iAdjY + 260 + 35))
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView = 6;

	if ((msX >= sX + iAdjX + 181 + 31) && (msX <= sX + iAdjX + 210 + 31) &&
		(msY >= sY + iAdjY + 248 + 35) && (msY <= sY + iAdjY + 260 + 35))
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView = 7;

	if ((msX >= sX + iAdjX + 214 + 31) && (msX <= sX + iAdjX + 230 + 31) &&
		(msY >= sY + iAdjY + 248 + 35) && (msY <= sY + iAdjY + 260 + 35))
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView = 8;

	if ((msX >= sX + iAdjX + 234 + 31) && (msX <= sX + iAdjX + 245 + 31) &&
		(msY >= sY + iAdjY + 248 + 35) && (msY <= sY + iAdjY + 260 + 35))
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::MagicShop).sView = 9;
}

