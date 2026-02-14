#pragma once
#include "IDialogBox.h"

class DialogBox_CityHallMenu : public IDialogBox
{
public:
	DialogBox_CityHallMenu(CGame* game);
	~DialogBox_CityHallMenu() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;

private:
	void DrawMode0_MainMenu(short sX, short sY, short size_x, short mouse_x, short mouse_y);
	void DrawMode1_CitizenshipWarning(short sX, short sY, short size_x, short mouse_x, short mouse_y);
	void DrawMode2_OfferingCitizenship(short sX, short sY, short size_x);
	void DrawMode3_CitizenshipSuccess(short sX, short sY, short size_x, short mouse_x, short mouse_y);
	void DrawMode4_CitizenshipFailed(short sX, short sY, short size_x, short mouse_x, short mouse_y);
	void DrawMode5_RewardGold(short sX, short sY, short size_x, short mouse_x, short mouse_y);
	void DrawMode7_HeroItems(short sX, short sY, short size_x, short mouse_x, short mouse_y);
	void DrawMode8_CancelQuest(short sX, short sY, short size_x, short mouse_x, short mouse_y);
	void DrawMode9_ChangePlayMode(short sX, short sY, short size_x, short mouse_x, short mouse_y);
	void DrawMode10_TeleportMenu(short sX, short sY, short size_x, short mouse_x, short mouse_y);
	void DrawMode11_HeroItemConfirm(short sX, short sY, short size_x, short mouse_x, short mouse_y);

	bool on_click_mode0(short sX, short sY, short mouse_x, short mouse_y);
	bool on_click_mode1(short sX, short sY, short mouse_x, short mouse_y);
	bool OnClickMode3_4(short sX, short sY, short mouse_x, short mouse_y);
	bool on_click_mode5(short sX, short sY, short mouse_x, short mouse_y);
	bool on_click_mode7(short sX, short sY, short mouse_x, short mouse_y);
	bool on_click_mode8(short sX, short sY, short mouse_x, short mouse_y);
	bool on_click_mode9(short sX, short sY, short mouse_x, short mouse_y);
	bool on_click_mode10(short sX, short sY, short mouse_x, short mouse_y);
	bool on_click_mode11(short sX, short sY, short mouse_x, short mouse_y);
	std::string m_cTakeHeroItemName;
};
