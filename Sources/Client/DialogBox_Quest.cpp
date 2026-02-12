#include "DialogBox_Quest.h"
#include "Game.h"
#include "lan_eng.h"
#include <format>
#include <string>
using namespace hb::client::sprite_id;

DialogBox_Quest::DialogBox_Quest(CGame* pGame)
	: IDialogBox(DialogBoxId::Quest, pGame)
{
	SetDefaultRect(0 , 0 , 258, 339);
}

void DialogBox_Quest::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX = Info().sX;
	short sY = Info().sY;
	short szX = Info().sSizeX;
	std::string cTxt;

	char cTemp[21];

	m_pGame->DrawNewDialogBox(InterfaceNdGame2, sX, sY, 2);
	m_pGame->DrawNewDialogBox(InterfaceNdText, sX, sY, 4);

	switch (Info().cMode) {
	case 1:
		switch (m_pGame->m_stQuest.sQuestType) {
		case 0:
			PutAlignedString(sX, sX + szX, sY + 50 + 115 - 30, DRAW_DIALOGBOX_QUEST1, GameColors::UILabel);
			break;

		case 1: // Hunt
			if (m_pGame->m_stQuest.bIsQuestCompleted == false)
				PutAlignedString(sX, sX + szX, sY + 50, DRAW_DIALOGBOX_QUEST2, GameColors::UILabel);
			else
				PutAlignedString(sX, sX + szX, sY + 50, DRAW_DIALOGBOX_QUEST3, GameColors::UILabel);

			cTxt = std::format("Rest Monster : {}", m_pGame->m_stQuest.sCurrentCount);
			PutAlignedString(sX, sX + szX, sY + 50 + 20, cTxt.c_str(), GameColors::UILabel);

			std::memset(cTemp, 0, sizeof(cTemp));
			switch (m_pGame->m_stQuest.sWho) {
			case 1:
			case 2:
			case 3: break;
			case 4: std::snprintf(cTemp, sizeof(cTemp), "%s", m_pGame->GetNpcConfigName(hb::shared::owner::William)); break;
			case 5:
			case 6:
			case 7: break;
			}
			cTxt = std::format(DRAW_DIALOGBOX_QUEST5, cTemp);
			PutAlignedString(sX, sX + szX, sY + 50 + 45, cTxt.c_str(), GameColors::UILabel);

			std::snprintf(cTemp, sizeof(cTemp), "%s", m_pGame->GetNpcConfigName(m_pGame->m_stQuest.sTargetType));
			cTxt = std::format(NPC_TALK_HANDLER16, m_pGame->m_stQuest.sTargetCount, cTemp);
			PutAlignedString(sX, sX + szX, sY + 50 + 60, cTxt.c_str(), GameColors::UILabel);

			if (m_pGame->m_stQuest.cTargetName.starts_with("NONE")) {
				cTxt = DRAW_DIALOGBOX_QUEST31;
				PutAlignedString(sX, sX + szX, sY + 50 + 75, cTxt.c_str(), GameColors::UILabel);
			}
			else {
				std::memset(cTemp, 0, sizeof(cTemp));
				m_pGame->GetOfficialMapName(m_pGame->m_stQuest.cTargetName.c_str(), cTemp);
				cTxt = std::format(DRAW_DIALOGBOX_QUEST32, cTemp);
				PutAlignedString(sX, sX + szX, sY + 50 + 75, cTxt.c_str(), GameColors::UILabel);

				if (m_pGame->m_stQuest.sX != 0) {
					cTxt = std::format(DRAW_DIALOGBOX_QUEST33, m_pGame->m_stQuest.sX, m_pGame->m_stQuest.sY, m_pGame->m_stQuest.sRange);
					PutAlignedString(sX, sX + szX, sY + 50 + 90, cTxt.c_str(), GameColors::UILabel);
				}
			}

			cTxt = std::format(DRAW_DIALOGBOX_QUEST34, m_pGame->m_stQuest.sContribution);
			PutAlignedString(sX, sX + szX, sY + 50 + 105, cTxt.c_str(), GameColors::UILabel);
			break;

		case 7:
			if (m_pGame->m_stQuest.bIsQuestCompleted == false)
				PutAlignedString(sX, sX + szX, sY + 50, DRAW_DIALOGBOX_QUEST26, GameColors::UILabel);
			else
				PutAlignedString(sX, sX + szX, sY + 50, DRAW_DIALOGBOX_QUEST27, GameColors::UILabel);

			std::memset(cTemp, 0, sizeof(cTemp));
			switch (m_pGame->m_stQuest.sWho) {
			case 1:
			case 2:
			case 3: break;
			case 4: std::snprintf(cTemp, sizeof(cTemp), "%s", m_pGame->GetNpcConfigName(hb::shared::owner::William)); break;
			case 5:
			case 6:
			case 7: break;
			}
			cTxt = std::format(DRAW_DIALOGBOX_QUEST29, cTemp);
			PutAlignedString(sX, sX + szX, sY + 50 + 45, cTxt.c_str(), GameColors::UILabel);

			PutAlignedString(sX, sX + szX, sY + 50 + 60, DRAW_DIALOGBOX_QUEST30, GameColors::UILabel);

			if (m_pGame->m_stQuest.cTargetName.starts_with("NONE")) {
				cTxt = DRAW_DIALOGBOX_QUEST31;
				PutAlignedString(sX, sX + szX, sY + 50 + 75, cTxt.c_str(), GameColors::UILabel);
			}
			else {
				std::memset(cTemp, 0, sizeof(cTemp));
				m_pGame->GetOfficialMapName(m_pGame->m_stQuest.cTargetName.c_str(), cTemp);
				cTxt = std::format(DRAW_DIALOGBOX_QUEST32, cTemp);
				PutAlignedString(sX, sX + szX, sY + 50 + 75, cTxt.c_str(), GameColors::UILabel);

				if (m_pGame->m_stQuest.sX != 0) {
					cTxt = std::format(DRAW_DIALOGBOX_QUEST33, m_pGame->m_stQuest.sX, m_pGame->m_stQuest.sY, m_pGame->m_stQuest.sRange);
					PutAlignedString(sX, sX + szX, sY + 50 + 90, cTxt.c_str(), GameColors::UILabel);
				}
			}

			cTxt = std::format(DRAW_DIALOGBOX_QUEST34, m_pGame->m_stQuest.sContribution);
			PutAlignedString(sX, sX + szX, sY + 50 + 105, cTxt.c_str(), GameColors::UILabel);
			break;
		}
		break;

	case 2:
		PutAlignedString(sX, sX + szX, sY + 50 + 115 - 30, DRAW_DIALOGBOX_QUEST35, GameColors::UILabel);
		break;
	}

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
	else
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

bool DialogBox_Quest::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
		m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::Quest);
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	return false;
}
