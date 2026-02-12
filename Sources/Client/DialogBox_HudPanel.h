#pragma once
#include "IDialogBox.h"
#include "DialogBoxIDs.h"
#include "GlobalDef.h"

class DialogBox_HudPanel : public IDialogBox
{
public:
	DialogBox_HudPanel(CGame* pGame);
	~DialogBox_HudPanel() override = default;

	void OnDraw(short msX, short msY, short msZ, char cLB) override;
	bool OnClick(short msX, short msY) override;
	bool OnItemDrop(short msX, short msY) override;

private:
	// Y offset for resolution - shifts all Y positions when running at higher resolutions
	// At 640x480: offset = 0, At 800x600: offset = 120
	static int HudYOffset() { return LOGICAL_HEIGHT() - 480; }

	// X offset for resolution - centers the HUD when running at wider resolutions
	// At 640x480: offset = 0, At 800x600: offset = 80 (centered)
	static int HudXOffset() { return (LOGICAL_WIDTH() - 640) / 2; }

	// Bar dimensions
	static constexpr int HP_MP_BAR_WIDTH = 101;
	static constexpr int SP_BAR_WIDTH = 167;

	// Bar positions (base values for 640x480, add offsets at runtime)
	static constexpr int BASE_HP_BAR_X = 23;
	static constexpr int BASE_HP_BAR_Y = 437;
	static constexpr int BASE_MP_BAR_Y = 459;
	static constexpr int BASE_SP_BAR_X = 147;
	static constexpr int BASE_SP_BAR_Y = 434;
	// Runtime positions (X and Y offsets applied)
	static int HP_BAR_X() { return BASE_HP_BAR_X + HudXOffset(); }
	static int HP_BAR_Y() { return BASE_HP_BAR_Y + HudYOffset(); }
	static int MP_BAR_Y() { return BASE_MP_BAR_Y + HudYOffset(); }
	static int SP_BAR_X() { return BASE_SP_BAR_X + HudXOffset(); }
	static int SP_BAR_Y() { return BASE_SP_BAR_Y + HudYOffset(); }

	// HP/MP/SP number positions
	static constexpr int BASE_HP_NUM_X = 80;
	static constexpr int BASE_HP_NUM_Y = 441;
	static constexpr int BASE_MP_NUM_Y = 463;
	static constexpr int BASE_SP_NUM_X = 228;
	static constexpr int BASE_SP_NUM_Y = 435;

	static int HP_NUM_X() { return BASE_HP_NUM_X + HudXOffset(); }
	static int HP_NUM_Y() { return BASE_HP_NUM_Y + HudYOffset(); }
	static int MP_NUM_Y() { return BASE_MP_NUM_Y + HudYOffset(); }
	static int SP_NUM_X() { return BASE_SP_NUM_X + HudXOffset(); }
	static int SP_NUM_Y() { return BASE_SP_NUM_Y + HudYOffset(); }

	// Combat mode icon position (right side - needs X offset)
	static constexpr int BASE_COMBAT_ICON_X = 368;
	static constexpr int BASE_COMBAT_ICON_Y = 440;
	static int COMBAT_ICON_X() { return BASE_COMBAT_ICON_X + HudXOffset(); }
	static int COMBAT_ICON_Y() { return BASE_COMBAT_ICON_Y + HudYOffset(); }

	// Map message text position
	static constexpr int BASE_MAP_MSG_X1 = 140;
	static constexpr int BASE_MAP_MSG_X2 = 323;
	static constexpr int BASE_MAP_MSG_Y = 456;
	static int MAP_MSG_X1() { return BASE_MAP_MSG_X1 + HudXOffset(); }
	static int MAP_MSG_X2() { return BASE_MAP_MSG_X2 + HudXOffset(); }
	static int MAP_MSG_Y() { return BASE_MAP_MSG_Y + HudYOffset(); }

	// Button regions (Y is shared, right-side X needs offset)
	static constexpr int BASE_BTN_Y1 = 434;
	static constexpr int BASE_BTN_Y2 = 475;
	static int BTN_Y1() { return BASE_BTN_Y1 + HudYOffset(); }
	static int BTN_Y2() { return BASE_BTN_Y2 + HudYOffset(); }

	// Right-side buttons (need X offset for wider resolutions)
	static constexpr int BASE_BTN_CRUSADE_X1 = 322;
	static constexpr int BASE_BTN_CRUSADE_X2 = 355;
	static constexpr int BASE_BTN_COMBAT_X1 = 362;
	static constexpr int BASE_BTN_COMBAT_X2 = 404;
	static int BTN_CRUSADE_X1() { return BASE_BTN_CRUSADE_X1 + HudXOffset(); }
	static int BTN_CRUSADE_X2() { return BASE_BTN_CRUSADE_X2 + HudXOffset(); }
	static int BTN_COMBAT_X1() { return BASE_BTN_COMBAT_X1 + HudXOffset(); }
	static int BTN_COMBAT_X2() { return BASE_BTN_COMBAT_X2 + HudXOffset(); }

	// Toggle button info structure
	struct ToggleButtonInfo {
		int x1, x2;
		int spriteX;
		int spriteFrame;
		const char* tooltip;
		DialogBoxId::Type dialogId;
	};

	static const ToggleButtonInfo TOGGLE_BUTTONS[];
	static constexpr int TOGGLE_BUTTON_COUNT = 6;

	// Level up text position (class constants, not the global functions)
	static constexpr int LOCAL_LEVELUP_TEXT_X = 32;
	static constexpr int LOCAL_LEVELUP_TEXT_Y = 448;

	// Helper methods
	void DrawGaugeBars();
	void DrawIconButtons(short msX, short msY);
	void DrawStatusIcons(short msX, short msY);
	bool IsInButton(short msX, short msY, int x1, int x2) const;
	void ToggleDialogWithSound(DialogBoxId::Type dialogId);
};
