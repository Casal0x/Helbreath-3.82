#pragma once
#include "IDialogBox.h"

class DialogBox_GuildMenu : public IDialogBox
{
public:
	DialogBox_GuildMenu(CGame* game);
	~DialogBox_GuildMenu() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;

private:
	void DrawMode0_MainMenu(short sX, short sY, short size_x, short mouse_x, short mouse_y);
	void DrawMode1_CreateGuild(short sX, short sY, short size_x, short mouse_x, short mouse_y);
	void DrawMode5_DisbandConfirm(short sX, short sY, short size_x, short mouse_x, short mouse_y);
	void DrawMode9_AdmissionTicket(short sX, short sY, short size_x, short mouse_x, short mouse_y);
	void DrawMode11_SecessionTicket(short sX, short sY, short size_x, short mouse_x, short mouse_y);
	void DrawMode13_FightzoneSelect(short sX, short sY, short size_x, short mouse_x, short mouse_y);
	void DrawMode20_ConfirmCancel(short sX, short sY, short size_x, short mouse_x, short mouse_y);
	void draw_simple_message(short sX, short sY, short size_x, short mouse_x, short mouse_y, int mode);

	bool on_click_mode0(short sX, short sY, short mouse_x, short mouse_y);
	bool on_click_mode1(short sX, short sY, short mouse_x, short mouse_y);
	bool on_click_mode5(short sX, short sY, short mouse_x, short mouse_y);
	bool on_click_mode9(short sX, short sY, short mouse_x, short mouse_y);
	bool on_click_mode11(short sX, short sY, short mouse_x, short mouse_y);
	bool on_click_mode13(short sX, short sY, short mouse_x, short mouse_y);
	bool on_click_mode_ok_only(short sX, short sY, short mouse_x, short mouse_y);

	static constexpr int ADJX = -13;
	static constexpr int ADJY = 30;
};
