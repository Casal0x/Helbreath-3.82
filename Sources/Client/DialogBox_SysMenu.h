#pragma once
#include "IDialogBox.h"

class DialogBox_SysMenu : public IDialogBox
{
public:
	DialogBox_SysMenu(CGame* pGame);
	~DialogBox_SysMenu() override = default;

	void OnDraw(short msX, short msY, short msZ, char cLB) override;
	bool OnClick(short msX, short msY) override;

private:
};
