#include "DialogBox_CityHallMenu.h"
#include "Game.h"
#include "TeleportManager.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::client::net;
using namespace hb::client::sprite_id;
DialogBox_CityHallMenu::DialogBox_CityHallMenu(CGame* pGame)
	: IDialogBox(DialogBoxId::CityHallMenu, pGame)
{
	SetDefaultRect(337 , 57 , 258, 339);
}

void DialogBox_CityHallMenu::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).sX;
	short sY = m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).sY;
	short szX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).sSizeX;

	m_pGame->DrawNewDialogBox(InterfaceNdGame2, sX, sY, 2);
	m_pGame->DrawNewDialogBox(InterfaceNdText, sX, sY, 18);

	switch (m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode)
	{
	case 0:  DrawMode0_MainMenu(sX, sY, szX, msX, msY); break;
	case 1:  DrawMode1_CitizenshipWarning(sX, sY, szX, msX, msY); break;
	case 2:  DrawMode2_OfferingCitizenship(sX, sY, szX); break;
	case 3:  DrawMode3_CitizenshipSuccess(sX, sY, szX, msX, msY); break;
	case 4:  DrawMode4_CitizenshipFailed(sX, sY, szX, msX, msY); break;
	case 5:  DrawMode5_RewardGold(sX, sY, szX, msX, msY); break;
	case 7:  DrawMode7_HeroItems(sX, sY, szX, msX, msY); break;
	case 8:  DrawMode8_CancelQuest(sX, sY, szX, msX, msY); break;
	case 9:  DrawMode9_ChangePlayMode(sX, sY, szX, msX, msY); break;
	case 10: DrawMode10_TeleportMenu(sX, sY, szX, msX, msY); break;
	case 11: DrawMode11_HeroItemConfirm(sX, sY, szX, msX, msY); break;
	}
}

void DialogBox_CityHallMenu::DrawMode0_MainMenu(short sX, short sY, short szX, short msX, short msY)
{
	// Citizenship request
	if (m_pGame->m_pPlayer->m_bCitizen == false)
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 70) && (msY < sY + 95))
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 70, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU1, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
		else
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 70, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU1, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
	}
	else
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 70, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU1, hb::shared::text::TextStyle::Color(GameColors::UIDisabled), hb::shared::text::Align::TopCenter);

	// Reward gold
	if (m_pGame->m_pPlayer->m_iRewardGold > 0)
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 95) && (msY < sY + 120))
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 95, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU4, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
		else
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 95, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU4, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
	}
	else
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 95, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU4, hb::shared::text::TextStyle::Color(GameColors::UIDisabled), hb::shared::text::Align::TopCenter);

	// Hero's Items
	if ((m_pGame->m_pPlayer->m_iEnemyKillCount >= 100) && (m_pGame->m_pPlayer->m_iContribution >= 10))
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 120) && (msY < sY + 145))
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 120, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU8, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
		else
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 120, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU8, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
	}
	else
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 120, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU8, hb::shared::text::TextStyle::Color(GameColors::UIDisabled), hb::shared::text::Align::TopCenter);

	// Cancel quest
	if (m_pGame->m_stQuest.sQuestType != 0)
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 145) && (msY < sY + 170))
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 145, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU11, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
		else
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 145, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU11, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
	}
	else
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 145, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU11, hb::shared::text::TextStyle::Color(GameColors::UIDisabled), hb::shared::text::Align::TopCenter);

	// Change playmode
	if ((m_pGame->m_bIsCrusadeMode == false) && m_pGame->m_pPlayer->m_bCitizen && (m_pGame->m_pPlayer->m_iPKCount == 0))
	{
		if (m_pGame->m_pPlayer->m_bHunter == true)
		{
			if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 170) && (msY < sY + 195))
				hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 170, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU56, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
			else
				hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 170, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU56, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
		}
		else if (m_pGame->m_pPlayer->m_iLevel < 100)
		{
			if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 170) && (msY < sY + 195))
				hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 170, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU56, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
			else
				hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 170, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU56, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
		}
		else
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 170, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU56, hb::shared::text::TextStyle::Color(GameColors::UIDisabled), hb::shared::text::Align::TopCenter);
	}
	else
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 170, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU56, hb::shared::text::TextStyle::Color(GameColors::UIDisabled), hb::shared::text::Align::TopCenter);

	// Teleport menu
	if ((m_pGame->m_bIsCrusadeMode == false) && m_pGame->m_pPlayer->m_bCitizen && (m_pGame->m_pPlayer->m_iPKCount == 0))
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 195) && (msY < sY + 220))
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 195, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU69, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
		else
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 195, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU69, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
	}
	else
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 195, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU69, hb::shared::text::TextStyle::Color(GameColors::UIDisabled), hb::shared::text::Align::TopCenter);

	// Change crusade role
	if (m_pGame->m_bIsCrusadeMode && m_pGame->m_pPlayer->m_bCitizen)
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 220) && (msY < sY + 220))
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 220, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU14, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
		else
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 220, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU14, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
	}
	else
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 220, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU14, hb::shared::text::TextStyle::Color(GameColors::UIDisabled), hb::shared::text::Align::TopCenter);

	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 270, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU17, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
}

void DialogBox_CityHallMenu::DrawMode1_CitizenshipWarning(short sX, short sY, short szX, short msX, short msY)
{
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 80, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU18, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 95, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU19, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 110, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU20, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 125, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU21, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 140, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU22, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 155, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU23, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 170, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU24, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 200, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU25, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 215, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU26, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 230, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU27, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);

	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 19);
	else
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 18);

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 3);
	else
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 2);
}

void DialogBox_CityHallMenu::DrawMode2_OfferingCitizenship(short sX, short sY, short szX)
{
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 140, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU28, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
}

void DialogBox_CityHallMenu::DrawMode3_CitizenshipSuccess(short sX, short sY, short szX, short msX, short msY)
{
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 140, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU29, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
	else
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

void DialogBox_CityHallMenu::DrawMode4_CitizenshipFailed(short sX, short sY, short szX, short msX, short msY)
{
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 80, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU30, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 100, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU31, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 115, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENuint32_t, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
	else
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

void DialogBox_CityHallMenu::DrawMode5_RewardGold(short sX, short sY, short szX, short msX, short msY)
{
	std::string cTxt;

	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 125, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU33, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	cTxt = std::format(DRAW_DIALOGBOX_CITYHALL_MENU34, m_pGame->m_pPlayer->m_iRewardGold);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 140, (sX + szX) - (sX), 15, cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 155, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU35, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);

	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 19);
	else
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 18);

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 3);
	else
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 2);
}

void DialogBox_CityHallMenu::DrawMode7_HeroItems(short sX, short sY, short szX, short msX, short msY)
{
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 60, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU46, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);

	// Hero's Cape (EK 300)
	if (m_pGame->m_pPlayer->m_iEnemyKillCount >= 300)
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 95) && (msY < sY + 110))
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 95, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU47, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
		else
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 95, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU47, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
	}
	else
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 95, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU47, hb::shared::text::TextStyle::Color(GameColors::UIDisabled), hb::shared::text::Align::TopCenter);

	// Hero's Helm (EK 150 - Contrib 20)
	if ((m_pGame->m_pPlayer->m_iEnemyKillCount >= 150) && (m_pGame->m_pPlayer->m_iContribution >= 20))
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 125) && (msY < sY + 140))
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 125, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU48, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
		else
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 125, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU48, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
	}
	else
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 125, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU48, hb::shared::text::TextStyle::Color(GameColors::UIDisabled), hb::shared::text::Align::TopCenter);

	// Hero's Cap (EK 100 - Contrib 20)
	if ((m_pGame->m_pPlayer->m_iEnemyKillCount >= 100) && (m_pGame->m_pPlayer->m_iContribution >= 20))
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 155) && (msY < sY + 170))
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 155, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU49, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
		else
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 155, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU49, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
	}
	else
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 155, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU49, hb::shared::text::TextStyle::Color(GameColors::UIDisabled), hb::shared::text::Align::TopCenter);

	// Hero's Armor (EK 300 - Contrib 30)
	if ((m_pGame->m_pPlayer->m_iEnemyKillCount >= 300) && (m_pGame->m_pPlayer->m_iContribution >= 30))
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 185) && (msY < sY + 200))
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 185, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU50, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
		else
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 185, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU50, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
	}
	else
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 185, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU50, hb::shared::text::TextStyle::Color(GameColors::UIDisabled), hb::shared::text::Align::TopCenter);

	// Hero's Robe (EK 200 - Contrib 20)
	if ((m_pGame->m_pPlayer->m_iEnemyKillCount >= 200) && (m_pGame->m_pPlayer->m_iContribution >= 20))
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 215) && (msY < sY + 230))
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 215, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU51, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
		else
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 215, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU51, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
	}
	else
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 215, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU51, hb::shared::text::TextStyle::Color(GameColors::UIDisabled), hb::shared::text::Align::TopCenter);

	// Hero's Hauberk (EK 100 - Contrib 10)
	if ((m_pGame->m_pPlayer->m_iEnemyKillCount >= 100) && (m_pGame->m_pPlayer->m_iContribution >= 10))
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 245) && (msY < sY + 260))
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 245, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU52, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
		else
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 245, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU52, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
	}
	else
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 245, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU52, hb::shared::text::TextStyle::Color(GameColors::UIDisabled), hb::shared::text::Align::TopCenter);

	// Hero's Leggings (EK 150 - Contrib 15)
	if ((m_pGame->m_pPlayer->m_iEnemyKillCount >= 150) && (m_pGame->m_pPlayer->m_iContribution >= 15))
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 275) && (msY < sY + 290))
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 275, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU53, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
		else
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 275, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU53, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
	}
	else
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 275, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU53, hb::shared::text::TextStyle::Color(GameColors::UIDisabled), hb::shared::text::Align::TopCenter);
}

void DialogBox_CityHallMenu::DrawMode8_CancelQuest(short sX, short sY, short szX, short msX, short msY)
{
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 125, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU54, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 140, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU55, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);

	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 19);
	else
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 18);

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 3);
	else
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 2);
}

void DialogBox_CityHallMenu::DrawMode9_ChangePlayMode(short sX, short sY, short szX, short msX, short msY)
{
	if (m_pGame->m_pPlayer->m_bHunter)
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 53, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU57, hb::shared::text::TextStyle::Color(GameColors::UIYellow), hb::shared::text::Align::TopCenter);
	else
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 53, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU58, hb::shared::text::TextStyle::Color(GameColors::UIYellow), hb::shared::text::Align::TopCenter);

	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 78, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU59, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawText(GameFont::Default, sX + 35, sY + 108, DRAW_DIALOGBOX_CITYHALL_MENU60, hb::shared::text::TextStyle::Color(GameColors::UIOrange));
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 125, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU61, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 140, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU62, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 155, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU63, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawText(GameFont::Default, sX + 35, sY + 177, DRAW_DIALOGBOX_CITYHALL_MENU64, hb::shared::text::TextStyle::Color(GameColors::UIOrange));
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 194, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU65, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 209, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU66, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 224, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU67, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 252, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU68, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);

	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 19);
	else
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 18);

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 3);
	else
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 2);
}

void DialogBox_CityHallMenu::DrawMode10_TeleportMenu(short sX, short sY, short szX, short msX, short msY)
{
	char mapNameBuf[120];
	std::string teleportBuf;

	if (TeleportManager::Get().GetMapCount() > 0)
	{
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 50, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU69, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 80, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU70, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 95, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU71, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 110, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU72, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
		hb::shared::text::DrawText(GameFont::Default, sX + 35, sY + 250, DRAW_DIALOGBOX_CITYHALL_MENU72_1, hb::shared::text::TextStyle::WithShadow(GameColors::UILabel));

		for (int i = 0; i < TeleportManager::Get().GetMapCount(); i++)
		{
			std::memset(mapNameBuf, 0, sizeof(mapNameBuf));
			m_pGame->GetOfficialMapName(TeleportManager::Get().GetList()[i].mapname.c_str(), mapNameBuf);
			teleportBuf = std::format(DRAW_DIALOGBOX_CITYHALL_MENU77, mapNameBuf, TeleportManager::Get().GetList()[i].iCost);

			if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + 130 + i * 15) && (msY <= sY + 144 + i * 15))
				hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 130 + i * 15, (sX + szX) - (sX), 15, teleportBuf.c_str(), hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
			else
				hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 130 + i * 15, (sX + szX) - (sX), 15, teleportBuf.c_str(), hb::shared::text::TextStyle::Color(GameColors::UIMenuHighlight), hb::shared::text::Align::TopCenter);
		}
	}
	else if (TeleportManager::Get().GetMapCount() == -1)
	{
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 125, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU73, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 150, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU74, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 175, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU75, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	}
	else
	{
		hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 175, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU76, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	}
}

void DialogBox_CityHallMenu::DrawMode11_HeroItemConfirm(short sX, short sY, short szX, short msX, short msY)
{
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 125, (sX + szX - 1) - (sX), 15, m_cTakeHeroItemName.c_str(), hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX + 1, sY + 125, (sX + szX) - (sX + 1), 15, m_cTakeHeroItemName.c_str(), hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
	hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 260, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU46A, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);

	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 19);
	else
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 18);

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 3);
	else
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 2);
}

bool DialogBox_CityHallMenu::OnClick(short msX, short msY)
{
	short sX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).sX;
	short sY = m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).sY;

	switch (m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode)
	{
	case 0:  return OnClickMode0(sX, sY, msX, msY);
	case 1:  return OnClickMode1(sX, sY, msX, msY);
	case 3:
	case 4:  return OnClickMode3_4(sX, sY, msX, msY);
	case 5:  return OnClickMode5(sX, sY, msX, msY);
	case 7:  return OnClickMode7(sX, sY, msX, msY);
	case 8:  return OnClickMode8(sX, sY, msX, msY);
	case 9:  return OnClickMode9(sX, sY, msX, msY);
	case 10: return OnClickMode10(sX, sY, msX, msY);
	case 11: return OnClickMode11(sX, sY, msX, msY);
	}
	return false;
}

bool DialogBox_CityHallMenu::OnClickMode0(short sX, short sY, short msX, short msY)
{
	// Citizenship request
	if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 70) && (msY < sY + 95))
	{
		if (m_pGame->m_pPlayer->m_bCitizen == true) return false;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 1;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	// Reward gold
	if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 95) && (msY < sY + 120))
	{
		if (m_pGame->m_pPlayer->m_iRewardGold <= 0) return false;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 5;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	// Hero items
	if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 120) && (msY < sY + 145))
	{
		if (m_pGame->m_pPlayer->m_iEnemyKillCount < 100) return false;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 7;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	// Cancel quest
	if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 145) && (msY < sY + 170))
	{
		if (m_pGame->m_stQuest.sQuestType == 0) return false;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 8;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	// Change playmode
	if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 170) && (msY < sY + 195))
	{
		if (m_pGame->m_bIsCrusadeMode) return false;
		if (m_pGame->m_pPlayer->m_iPKCount != 0) return false;
		if (m_pGame->m_pPlayer->m_bCitizen == false) return false;
		if ((m_pGame->m_pPlayer->m_iLevel > 100) && (m_pGame->m_pPlayer->m_bHunter == false)) return false;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 9;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	// Teleport menu
	if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 195) && (msY < sY + 220))
	{
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 10;
		TeleportManager::Get().SetMapCount(-1);
		m_pGame->bSendCommand(ClientMsgId::RequestTeleportList, 0, 0, 0, 0, 0, 0);
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	// Crusade job
	if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 220) && (msY < sY + 245))
	{
		if (m_pGame->m_bIsCrusadeMode == false) return false;
		m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::CrusadeJob, 1, 0, 0);
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	return false;
}

bool DialogBox_CityHallMenu::OnClickMode1(short sX, short sY, short msX, short msY)
{
	// Yes button
	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		m_pGame->bSendCommand(MsgId::RequestCivilRight, MsgType::Confirm, 0, 0, 0, 0, 0);
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 2;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	// No button
	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 0;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	return false;
}

bool DialogBox_CityHallMenu::OnClickMode3_4(short sX, short sY, short msX, short msY)
{
	// OK button
	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 0;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}
	return false;
}

bool DialogBox_CityHallMenu::OnClickMode5(short sX, short sY, short msX, short msY)
{
	// Yes button
	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		m_pGame->bSendCommand(MsgId::CommandCommon, CommonType::ReqGetRewardMoney, 0, 0, 0, 0, 0);
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 0;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	// No button
	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 0;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	return false;
}

bool DialogBox_CityHallMenu::OnClickMode7(short sX, short sY, short msX, short msY)
{
	int iReqHeroItemID = 0;

	// Hero's Cape
	if ((msX >= sX + 35) && (msX <= sX + 220) && (msY >= sY + 95) && (msY <= sY + 110))
	{
		if (m_pGame->m_pPlayer->m_bAresden == true) iReqHeroItemID = 400;
		else iReqHeroItemID = 401;
		m_cTakeHeroItemName = DRAW_DIALOGBOX_CITYHALL_MENU47;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 11;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).sV1 = iReqHeroItemID;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	// Hero's Helm
	if ((msX >= sX + 35) && (msX <= sX + 220) && (msY >= sY + 125) && (msY <= sY + 140))
	{
		if ((m_pGame->m_pPlayer->m_bAresden == true) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 1)) iReqHeroItemID = 403;
		if ((m_pGame->m_pPlayer->m_bAresden == true) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 2)) iReqHeroItemID = 404;
		if ((m_pGame->m_pPlayer->m_bAresden == false) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 1)) iReqHeroItemID = 405;
		if ((m_pGame->m_pPlayer->m_bAresden == false) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 2)) iReqHeroItemID = 406;
		m_cTakeHeroItemName = DRAW_DIALOGBOX_CITYHALL_MENU48;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 11;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).sV1 = iReqHeroItemID;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	// Hero's Cap
	if ((msX >= sX + 35) && (msX <= sX + 220) && (msY >= sY + 155) && (msY <= sY + 170))
	{
		if ((m_pGame->m_pPlayer->m_bAresden == true) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 1)) iReqHeroItemID = 407;
		if ((m_pGame->m_pPlayer->m_bAresden == true) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 2)) iReqHeroItemID = 408;
		if ((m_pGame->m_pPlayer->m_bAresden == false) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 1)) iReqHeroItemID = 409;
		if ((m_pGame->m_pPlayer->m_bAresden == false) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 2)) iReqHeroItemID = 410;
		m_cTakeHeroItemName = DRAW_DIALOGBOX_CITYHALL_MENU49;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 11;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).sV1 = iReqHeroItemID;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	// Hero's Armor
	if ((msX >= sX + 35) && (msX <= sX + 220) && (msY >= sY + 185) && (msY <= sY + 200))
	{
		if ((m_pGame->m_pPlayer->m_bAresden == true) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 1)) iReqHeroItemID = 411;
		if ((m_pGame->m_pPlayer->m_bAresden == true) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 2)) iReqHeroItemID = 412;
		if ((m_pGame->m_pPlayer->m_bAresden == false) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 1)) iReqHeroItemID = 413;
		if ((m_pGame->m_pPlayer->m_bAresden == false) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 2)) iReqHeroItemID = 414;
		m_cTakeHeroItemName = DRAW_DIALOGBOX_CITYHALL_MENU50;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 11;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).sV1 = iReqHeroItemID;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	// Hero's Robe
	if ((msX >= sX + 35) && (msX <= sX + 220) && (msY >= sY + 215) && (msY <= sY + 230))
	{
		if ((m_pGame->m_pPlayer->m_bAresden == true) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 1)) iReqHeroItemID = 415;
		if ((m_pGame->m_pPlayer->m_bAresden == true) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 2)) iReqHeroItemID = 416;
		if ((m_pGame->m_pPlayer->m_bAresden == false) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 1)) iReqHeroItemID = 417;
		if ((m_pGame->m_pPlayer->m_bAresden == false) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 2)) iReqHeroItemID = 418;
		m_cTakeHeroItemName = DRAW_DIALOGBOX_CITYHALL_MENU51;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 11;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).sV1 = iReqHeroItemID;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	// Hero's Hauberk
	if ((msX >= sX + 35) && (msX <= sX + 220) && (msY >= sY + 245) && (msY <= sY + 260))
	{
		if ((m_pGame->m_pPlayer->m_bAresden == true) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 1)) iReqHeroItemID = 419;
		if ((m_pGame->m_pPlayer->m_bAresden == true) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 2)) iReqHeroItemID = 420;
		if ((m_pGame->m_pPlayer->m_bAresden == false) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 1)) iReqHeroItemID = 421;
		if ((m_pGame->m_pPlayer->m_bAresden == false) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 2)) iReqHeroItemID = 422;
		m_cTakeHeroItemName = DRAW_DIALOGBOX_CITYHALL_MENU52;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 11;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).sV1 = iReqHeroItemID;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	// Hero's Leggings
	if ((msX >= sX + 35) && (msX <= sX + 220) && (msY >= sY + 275) && (msY <= sY + 290))
	{
		if ((m_pGame->m_pPlayer->m_bAresden == true) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 1)) iReqHeroItemID = 423;
		if ((m_pGame->m_pPlayer->m_bAresden == true) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 2)) iReqHeroItemID = 424;
		if ((m_pGame->m_pPlayer->m_bAresden == false) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 1)) iReqHeroItemID = 425;
		if ((m_pGame->m_pPlayer->m_bAresden == false) && (m_pGame->m_pCharList[m_pGame->m_cCurFocus - 1]->m_sSex == 2)) iReqHeroItemID = 426;
		m_cTakeHeroItemName = DRAW_DIALOGBOX_CITYHALL_MENU53;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 11;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).sV1 = iReqHeroItemID;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	return false;
}

bool DialogBox_CityHallMenu::OnClickMode8(short sX, short sY, short msX, short msY)
{
	// Yes button
	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		m_pGame->bSendCommand(MsgId::CommandCommon, CommonType::RequestCancelQuest, 0, 0, 0, 0, 0);
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 0;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	// No button
	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 0;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	return false;
}

bool DialogBox_CityHallMenu::OnClickMode9(short sX, short sY, short msX, short msY)
{
	// Yes button
	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		m_pGame->bSendCommand(MsgId::CommandCommon, CommonType::RequestHuntMode, 0, 0, 0, 0, 0);
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 0;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	// No button
	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 0;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	return false;
}

bool DialogBox_CityHallMenu::OnClickMode10(short sX, short sY, short msX, short msY)
{
	if (TeleportManager::Get().GetMapCount() > 0)
	{
		for (int i = 0; i < TeleportManager::Get().GetMapCount(); i++)
		{
			if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + 130 + i * 15) && (msY <= sY + 144 + i * 15))
			{
				m_pGame->bSendCommand(ClientMsgId::RequestChargedTeleport, 0, 0, TeleportManager::Get().GetList()[i].iIndex, 0, 0, 0);
				m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::CityHallMenu);
				return true;
			}
		}
	}
	return false;
}

bool DialogBox_CityHallMenu::OnClickMode11(short sX, short sY, short msX, short msY)
{
	// Yes button
	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		m_pGame->bSendCommand(MsgId::CommandCommon, CommonType::ReqGetHeroMantle, 0, m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).sV1, 0, 0, 0);
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 0;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	// No button
	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 7;
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	return false;
}
