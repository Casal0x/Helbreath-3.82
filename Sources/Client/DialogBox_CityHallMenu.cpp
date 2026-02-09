#include "DialogBox_CityHallMenu.h"
#include "Game.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"

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

	m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 2);
	m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_TEXT, sX, sY, 18);

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
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 70, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU1, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
		else
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 70, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU1, TextLib::TextStyle::Color(GameColors::UIMagicBlue), TextLib::Align::TopCenter);
	}
	else
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 70, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU1, TextLib::TextStyle::Color(GameColors::UIDisabled), TextLib::Align::TopCenter);

	// Reward gold
	if (m_pGame->m_pPlayer->m_iRewardGold > 0)
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 95) && (msY < sY + 120))
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 95, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU4, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
		else
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 95, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU4, TextLib::TextStyle::Color(GameColors::UIMagicBlue), TextLib::Align::TopCenter);
	}
	else
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 95, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU4, TextLib::TextStyle::Color(GameColors::UIDisabled), TextLib::Align::TopCenter);

	// Hero's Items
	if ((m_pGame->m_pPlayer->m_iEnemyKillCount >= 100) && (m_pGame->m_pPlayer->m_iContribution >= 10))
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 120) && (msY < sY + 145))
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 120, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU8, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
		else
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 120, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU8, TextLib::TextStyle::Color(GameColors::UIMagicBlue), TextLib::Align::TopCenter);
	}
	else
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 120, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU8, TextLib::TextStyle::Color(GameColors::UIDisabled), TextLib::Align::TopCenter);

	// Cancel quest
	if (m_pGame->m_stQuest.sQuestType != 0)
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 145) && (msY < sY + 170))
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 145, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU11, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
		else
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 145, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU11, TextLib::TextStyle::Color(GameColors::UIMagicBlue), TextLib::Align::TopCenter);
	}
	else
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 145, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU11, TextLib::TextStyle::Color(GameColors::UIDisabled), TextLib::Align::TopCenter);

	// Change playmode
	if ((m_pGame->m_bIsCrusadeMode == false) && m_pGame->m_pPlayer->m_bCitizen && (m_pGame->m_pPlayer->m_iPKCount == 0))
	{
		if (m_pGame->m_pPlayer->m_bHunter == true)
		{
			if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 170) && (msY < sY + 195))
				TextLib::DrawTextAligned(GameFont::Default, sX, sY + 170, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU56, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
			else
				TextLib::DrawTextAligned(GameFont::Default, sX, sY + 170, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU56, TextLib::TextStyle::Color(GameColors::UIMagicBlue), TextLib::Align::TopCenter);
		}
		else if (m_pGame->m_pPlayer->m_iLevel < 100)
		{
			if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 170) && (msY < sY + 195))
				TextLib::DrawTextAligned(GameFont::Default, sX, sY + 170, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU56, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
			else
				TextLib::DrawTextAligned(GameFont::Default, sX, sY + 170, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU56, TextLib::TextStyle::Color(GameColors::UIMagicBlue), TextLib::Align::TopCenter);
		}
		else
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 170, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU56, TextLib::TextStyle::Color(GameColors::UIDisabled), TextLib::Align::TopCenter);
	}
	else
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 170, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU56, TextLib::TextStyle::Color(GameColors::UIDisabled), TextLib::Align::TopCenter);

	// Teleport menu
	if ((m_pGame->m_bIsCrusadeMode == false) && m_pGame->m_pPlayer->m_bCitizen && (m_pGame->m_pPlayer->m_iPKCount == 0))
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 195) && (msY < sY + 220))
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 195, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU69, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
		else
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 195, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU69, TextLib::TextStyle::Color(GameColors::UIMagicBlue), TextLib::Align::TopCenter);
	}
	else
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 195, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU69, TextLib::TextStyle::Color(GameColors::UIDisabled), TextLib::Align::TopCenter);

	// Change crusade role
	if (m_pGame->m_bIsCrusadeMode && m_pGame->m_pPlayer->m_bCitizen)
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 220) && (msY < sY + 220))
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 220, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU14, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
		else
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 220, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU14, TextLib::TextStyle::Color(GameColors::UIMagicBlue), TextLib::Align::TopCenter);
	}
	else
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 220, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU14, TextLib::TextStyle::Color(GameColors::UIDisabled), TextLib::Align::TopCenter);

	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 270, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU17, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
}

void DialogBox_CityHallMenu::DrawMode1_CitizenshipWarning(short sX, short sY, short szX, short msX, short msY)
{
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 80, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU18, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 95, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU19, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 110, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU20, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 125, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU21, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 140, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU22, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 155, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU23, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 170, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU24, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 200, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU25, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 215, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU26, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 230, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU27, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);

	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 19);
	else
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 18);

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 3);
	else
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 2);
}

void DialogBox_CityHallMenu::DrawMode2_OfferingCitizenship(short sX, short sY, short szX)
{
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 140, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU28, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
}

void DialogBox_CityHallMenu::DrawMode3_CitizenshipSuccess(short sX, short sY, short szX, short msX, short msY)
{
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 140, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU29, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
	else
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

void DialogBox_CityHallMenu::DrawMode4_CitizenshipFailed(short sX, short sY, short szX, short msX, short msY)
{
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 80, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU30, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 100, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU31, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 115, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENuint32_t, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
	else
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

void DialogBox_CityHallMenu::DrawMode5_RewardGold(short sX, short sY, short szX, short msX, short msY)
{
	char cTxt[120];

	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 125, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU33, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	std::snprintf(cTxt, sizeof(cTxt), DRAW_DIALOGBOX_CITYHALL_MENU34, m_pGame->m_pPlayer->m_iRewardGold);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 140, (sX + szX) - (sX), 15, cTxt, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 155, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU35, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);

	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 19);
	else
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 18);

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 3);
	else
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 2);
}

void DialogBox_CityHallMenu::DrawMode7_HeroItems(short sX, short sY, short szX, short msX, short msY)
{
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 60, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU46, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);

	// Hero's Cape (EK 300)
	if (m_pGame->m_pPlayer->m_iEnemyKillCount >= 300)
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 95) && (msY < sY + 110))
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 95, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU47, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
		else
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 95, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU47, TextLib::TextStyle::Color(GameColors::UIMagicBlue), TextLib::Align::TopCenter);
	}
	else
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 95, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU47, TextLib::TextStyle::Color(GameColors::UIDisabled), TextLib::Align::TopCenter);

	// Hero's Helm (EK 150 - Contrib 20)
	if ((m_pGame->m_pPlayer->m_iEnemyKillCount >= 150) && (m_pGame->m_pPlayer->m_iContribution >= 20))
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 125) && (msY < sY + 140))
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 125, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU48, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
		else
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 125, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU48, TextLib::TextStyle::Color(GameColors::UIMagicBlue), TextLib::Align::TopCenter);
	}
	else
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 125, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU48, TextLib::TextStyle::Color(GameColors::UIDisabled), TextLib::Align::TopCenter);

	// Hero's Cap (EK 100 - Contrib 20)
	if ((m_pGame->m_pPlayer->m_iEnemyKillCount >= 100) && (m_pGame->m_pPlayer->m_iContribution >= 20))
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 155) && (msY < sY + 170))
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 155, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU49, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
		else
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 155, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU49, TextLib::TextStyle::Color(GameColors::UIMagicBlue), TextLib::Align::TopCenter);
	}
	else
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 155, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU49, TextLib::TextStyle::Color(GameColors::UIDisabled), TextLib::Align::TopCenter);

	// Hero's Armor (EK 300 - Contrib 30)
	if ((m_pGame->m_pPlayer->m_iEnemyKillCount >= 300) && (m_pGame->m_pPlayer->m_iContribution >= 30))
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 185) && (msY < sY + 200))
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 185, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU50, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
		else
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 185, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU50, TextLib::TextStyle::Color(GameColors::UIMagicBlue), TextLib::Align::TopCenter);
	}
	else
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 185, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU50, TextLib::TextStyle::Color(GameColors::UIDisabled), TextLib::Align::TopCenter);

	// Hero's Robe (EK 200 - Contrib 20)
	if ((m_pGame->m_pPlayer->m_iEnemyKillCount >= 200) && (m_pGame->m_pPlayer->m_iContribution >= 20))
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 215) && (msY < sY + 230))
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 215, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU51, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
		else
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 215, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU51, TextLib::TextStyle::Color(GameColors::UIMagicBlue), TextLib::Align::TopCenter);
	}
	else
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 215, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU51, TextLib::TextStyle::Color(GameColors::UIDisabled), TextLib::Align::TopCenter);

	// Hero's Hauberk (EK 100 - Contrib 10)
	if ((m_pGame->m_pPlayer->m_iEnemyKillCount >= 100) && (m_pGame->m_pPlayer->m_iContribution >= 10))
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 245) && (msY < sY + 260))
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 245, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU52, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
		else
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 245, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU52, TextLib::TextStyle::Color(GameColors::UIMagicBlue), TextLib::Align::TopCenter);
	}
	else
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 245, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU52, TextLib::TextStyle::Color(GameColors::UIDisabled), TextLib::Align::TopCenter);

	// Hero's Leggings (EK 150 - Contrib 15)
	if ((m_pGame->m_pPlayer->m_iEnemyKillCount >= 150) && (m_pGame->m_pPlayer->m_iContribution >= 15))
	{
		if ((msX > sX + 35) && (msX < sX + 220) && (msY > sY + 275) && (msY < sY + 290))
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 275, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU53, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
		else
			TextLib::DrawTextAligned(GameFont::Default, sX, sY + 275, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU53, TextLib::TextStyle::Color(GameColors::UIMagicBlue), TextLib::Align::TopCenter);
	}
	else
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 275, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU53, TextLib::TextStyle::Color(GameColors::UIDisabled), TextLib::Align::TopCenter);
}

void DialogBox_CityHallMenu::DrawMode8_CancelQuest(short sX, short sY, short szX, short msX, short msY)
{
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 125, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU54, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 140, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU55, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);

	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 19);
	else
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 18);

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 3);
	else
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 2);
}

void DialogBox_CityHallMenu::DrawMode9_ChangePlayMode(short sX, short sY, short szX, short msX, short msY)
{
	if (m_pGame->m_pPlayer->m_bHunter)
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 53, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU57, TextLib::TextStyle::Color(GameColors::UIYellow), TextLib::Align::TopCenter);
	else
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 53, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU58, TextLib::TextStyle::Color(GameColors::UIYellow), TextLib::Align::TopCenter);

	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 78, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU59, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawText(GameFont::Default, sX + 35, sY + 108, DRAW_DIALOGBOX_CITYHALL_MENU60, TextLib::TextStyle::Color(GameColors::UIOrange));
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 125, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU61, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 140, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU62, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 155, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU63, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawText(GameFont::Default, sX + 35, sY + 177, DRAW_DIALOGBOX_CITYHALL_MENU64, TextLib::TextStyle::Color(GameColors::UIOrange));
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 194, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU65, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 209, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU66, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 224, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU67, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 252, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU68, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);

	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 19);
	else
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 18);

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 3);
	else
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 2);
}

void DialogBox_CityHallMenu::DrawMode10_TeleportMenu(short sX, short sY, short szX, short msX, short msY)
{
	char mapNameBuf[120];
	char teleportBuf[128];

	if (m_pGame->m_iTeleportMapCount > 0)
	{
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 50, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU69, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 80, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU70, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 95, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU71, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 110, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU72, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
		TextLib::DrawText(GameFont::Default, sX + 35, sY + 250, DRAW_DIALOGBOX_CITYHALL_MENU72_1, TextLib::TextStyle::WithShadow(GameColors::UILabel));

		for (int i = 0; i < m_pGame->m_iTeleportMapCount; i++)
		{
			std::memset(mapNameBuf, 0, sizeof(mapNameBuf));
			m_pGame->GetOfficialMapName(m_pGame->m_stTeleportList[i].mapname, mapNameBuf);
			snprintf(teleportBuf, sizeof(teleportBuf), DRAW_DIALOGBOX_CITYHALL_MENU77, mapNameBuf, m_pGame->m_stTeleportList[i].iCost);

			if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + 130 + i * 15) && (msY <= sY + 144 + i * 15))
				TextLib::DrawTextAligned(GameFont::Default, sX, sY + 130 + i * 15, (sX + szX) - (sX), 15, teleportBuf, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
			else
				TextLib::DrawTextAligned(GameFont::Default, sX, sY + 130 + i * 15, (sX + szX) - (sX), 15, teleportBuf, TextLib::TextStyle::Color(GameColors::UIMenuHighlight), TextLib::Align::TopCenter);
		}
	}
	else if (m_pGame->m_iTeleportMapCount == -1)
	{
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 125, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU73, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 150, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU74, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 175, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU75, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	}
	else
	{
		TextLib::DrawTextAligned(GameFont::Default, sX, sY + 175, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU76, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	}
}

void DialogBox_CityHallMenu::DrawMode11_HeroItemConfirm(short sX, short sY, short szX, short msX, short msY)
{
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 125, (sX + szX - 1) - (sX), 15, m_pGame->m_cTakeHeroItemName, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX + 1, sY + 125, (sX + szX) - (sX + 1), 15, m_pGame->m_cTakeHeroItemName, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
	TextLib::DrawTextAligned(GameFont::Default, sX, sY + 260, (sX + szX) - (sX), 15, DRAW_DIALOGBOX_CITYHALL_MENU46A, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);

	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 19);
	else
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 18);

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 3);
	else
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 2);
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
		m_pGame->m_iTeleportMapCount = -1;
		m_pGame->bSendCommand(MSGID_REQUEST_TELEPORT_LIST, 0, 0, 0, 0, 0, 0);
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
		m_pGame->bSendCommand(MSGID_REQUEST_CIVILRIGHT, DEF_MSGTYPE_CONFIRM, 0, 0, 0, 0, 0);
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
		m_pGame->bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQ_GETREWARDMONEY, 0, 0, 0, 0, 0);
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
		std::memset(m_pGame->m_cTakeHeroItemName, 0, sizeof(m_pGame->m_cTakeHeroItemName));
		memcpy(m_pGame->m_cTakeHeroItemName, DRAW_DIALOGBOX_CITYHALL_MENU47, strlen(DRAW_DIALOGBOX_CITYHALL_MENU47));
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
		std::memset(m_pGame->m_cTakeHeroItemName, 0, sizeof(m_pGame->m_cTakeHeroItemName));
		memcpy(m_pGame->m_cTakeHeroItemName, DRAW_DIALOGBOX_CITYHALL_MENU48, strlen(DRAW_DIALOGBOX_CITYHALL_MENU48));
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
		std::memset(m_pGame->m_cTakeHeroItemName, 0, sizeof(m_pGame->m_cTakeHeroItemName));
		memcpy(m_pGame->m_cTakeHeroItemName, DRAW_DIALOGBOX_CITYHALL_MENU49, strlen(DRAW_DIALOGBOX_CITYHALL_MENU49));
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
		std::memset(m_pGame->m_cTakeHeroItemName, 0, sizeof(m_pGame->m_cTakeHeroItemName));
		memcpy(m_pGame->m_cTakeHeroItemName, DRAW_DIALOGBOX_CITYHALL_MENU50, strlen(DRAW_DIALOGBOX_CITYHALL_MENU50));
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
		std::memset(m_pGame->m_cTakeHeroItemName, 0, sizeof(m_pGame->m_cTakeHeroItemName));
		memcpy(m_pGame->m_cTakeHeroItemName, DRAW_DIALOGBOX_CITYHALL_MENU51, strlen(DRAW_DIALOGBOX_CITYHALL_MENU51));
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
		std::memset(m_pGame->m_cTakeHeroItemName, 0, sizeof(m_pGame->m_cTakeHeroItemName));
		memcpy(m_pGame->m_cTakeHeroItemName, DRAW_DIALOGBOX_CITYHALL_MENU52, strlen(DRAW_DIALOGBOX_CITYHALL_MENU52));
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
		std::memset(m_pGame->m_cTakeHeroItemName, 0, sizeof(m_pGame->m_cTakeHeroItemName));
		memcpy(m_pGame->m_cTakeHeroItemName, DRAW_DIALOGBOX_CITYHALL_MENU53, strlen(DRAW_DIALOGBOX_CITYHALL_MENU53));
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
		m_pGame->bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQUEST_CANCELQUEST, 0, 0, 0, 0, 0);
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
		m_pGame->bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQUEST_HUNTMODE, 0, 0, 0, 0, 0);
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
	if (m_pGame->m_iTeleportMapCount > 0)
	{
		for (int i = 0; i < m_pGame->m_iTeleportMapCount; i++)
		{
			if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + 130 + i * 15) && (msY <= sY + 144 + i * 15))
			{
				m_pGame->bSendCommand(MSGID_REQUEST_CHARGED_TELEPORT, 0, 0, m_pGame->m_stTeleportList[i].iIndex, 0, 0, 0);
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
		m_pGame->bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQ_GETHEROMANTLE, 0, m_pGame->m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).sV1, 0, 0, 0);
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
