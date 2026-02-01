#pragma once
#include "IDialogBox.h"

class DialogBox_ChangeStatsMajestic : public IDialogBox
{
public:
	DialogBox_ChangeStatsMajestic(CGame* pGame);
	~DialogBox_ChangeStatsMajestic() override = default;

	void OnDraw(short msX, short msY, short msZ, char cLB) override;
	bool OnClick(short msX, short msY) override;

private:
	void DrawStatRow(short sX, short sY, int iYOffset, const char* pLabel,
		int iCurrentStat, int16_t iPendingChange, short msX, short msY,
		int iArrowYOffset, bool bCanUndo, bool bCanReduce);
};
