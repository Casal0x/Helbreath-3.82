#include "DialogBox_RepairAll.h"
#include "Game.h"
#include "IInput.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
DialogBox_RepairAll::DialogBox_RepairAll(CGame* pGame)
	: IDialogBox(DialogBoxId::RepairAll, pGame)
{
	SetDefaultRect(337 , 57 , 258, 339);
}

void DialogBox_RepairAll::OnDraw(short msX, short msY, short msZ, char cLB)
{
	if (!m_pGame->EnsureItemConfigsLoaded()) return;
	short sX = Info().sX;
	short sY = Info().sY;
	short szX = Info().sSizeX;
	std::string cTxt;
	int iTotalLines, iPointerLoc;
	double d1, d2, d3;

	DrawNewDialogBox(InterfaceNdGame2, sX, sY, 2);
	DrawNewDialogBox(InterfaceNdText, sX, sY, 10);

	for (int i = 0; i < 15; i++)
	{
		if ((i + Info().sView) < m_pGame->totalItemRepair)
		{
			CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[m_pGame->m_stRepairAll[i + Info().sView].index]->m_sIDnum);
			cTxt = std::format("{} - Cost: {}", pCfg ? pCfg->m_cName : "Unknown", m_pGame->m_stRepairAll[i + Info().sView].price);

			PutString(sX + 30, sY + 45 + i * 15, cTxt.c_str(), GameColors::UIBlack);
			m_pGame->m_bIsItemDisabled[m_pGame->m_stRepairAll[i + Info().sView].index] = true;
		}
	}

	iTotalLines = m_pGame->totalItemRepair;
	if (iTotalLines > 15)
	{
		d1 = (double)Info().sView;
		d2 = (double)(iTotalLines - 15);
		d3 = (274.0f * d1) / d2;
		iPointerLoc = (int)d3;
	}
	else
	{
		iPointerLoc = 0;
	}

	if (iTotalLines > 15)
	{
		DrawNewDialogBox(InterfaceNdGame2, sX, sY, 1);
		DrawNewDialogBox(InterfaceNdGame2, sX + 242, sY + iPointerLoc + 35, 7);
	}

	// Mouse wheel scrolling
	if (iTotalLines > 15)
	{
		if (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::RepairAll && msZ != 0)
		{
			if (msZ > 0) Info().sView--;
			if (msZ < 0) Info().sView++;
		}

		if (Info().sView < 0)
			Info().sView = 0;

		if (iTotalLines > 15 && Info().sView > iTotalLines - 15)
			Info().sView = iTotalLines - 15;
	}

	if (m_pGame->totalItemRepair > 0)
	{
		// Repair button
		if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 43);
		else
			DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 42);

		// Cancel button
		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 17);
		else
			DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 16);

		// Total cost
		cTxt = std::format("Total cost : {}", m_pGame->totalPrice);
		PutString(sX + 30, sY + 270, cTxt.c_str(), GameColors::UIBlack);
	}
	else
	{
		// No items to repair
		PutAlignedString(sX, sX + szX, sY + 140, "There are no items to repair.", GameColors::UIBlack);

		// Cancel button only
		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 17);
		else
			DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 16);
	}
}

bool DialogBox_RepairAll::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	for (int i = 0; i < 15; i++)
	{
		if ((i + Info().sView) < m_pGame->totalItemRepair)
		{
			// Repair button
			if ((msX >= sX + 30) && (msX <= sX + 30 + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			{
				bSendCommand(MsgId::CommandCommon, CommonType::ReqRepairAllConfirm, 0, 0, 0, 0, 0);
				DisableThisDialog();
				return true;
			}

			// Cancel button
			if ((msX >= sX + 154) && (msX <= sX + 154 + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			{
				DisableThisDialog();
				return true;
			}
		}
		else
		{
			// Cancel button (no items)
			if ((msX >= sX + 154) && (msX <= sX + 154 + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			{
				DisableThisDialog();
				return true;
			}
		}
	}

	return false;
}

