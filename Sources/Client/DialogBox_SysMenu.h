#pragma once
#include "IDialogBox.h"

// 4:3 resolution options
struct Resolution {
	int width;
	int height;
};

class DialogBox_SysMenu : public IDialogBox
{
public:
	// Tab indices
	enum Tab
	{
		TAB_GENERAL = 0,
		TAB_GRAPHICS,
		TAB_AUDIO,
		TAB_SYSTEM,
		TAB_COUNT
	};

	DialogBox_SysMenu(CGame* pGame);
	~DialogBox_SysMenu() override = default;

	void OnUpdate() override;
	void OnDraw(short msX, short msY, short msZ, char cLB) override;
	bool OnClick(short msX, short msY) override;
	PressResult OnPress(short msX, short msY) override;

	// Resolution management
	static const Resolution s_Resolutions[];
	static const int s_NumResolutions;
	static int GetCurrentResolutionIndex();
	static int GetNearestResolutionIndex(int width, int height);
	static void CycleResolution();
	static void ApplyResolution(int index);

private:
	// Tab drawing
	void DrawTabs(short sX, short sY, short msX, short msY);
	void DrawTabContent(short sX, short sY, short msX, short msY, char cLB);

	// Individual tab content
	void DrawGeneralTab(short sX, short sY, short msX, short msY);
	void DrawGraphicsTab(short sX, short sY, short msX, short msY);
	void DrawAudioTab(short sX, short sY, short msX, short msY, char cLB);
	void DrawSystemTab(short sX, short sY, short msX, short msY);

	// Click handlers for each tab
	bool OnClickGeneral(short sX, short sY, short msX, short msY);
	bool OnClickGraphics(short sX, short sY, short msX, short msY);
	bool OnClickAudio(short sX, short sY, short msX, short msY);
	bool OnClickSystem(short sX, short sY, short msX, short msY);

	// Helper to draw On/Off toggle
	void DrawToggle(int x, int y, bool bEnabled, short msX, short msY);
	bool IsInToggleArea(int x, int y, short msX, short msY);

	int m_iActiveTab;

	// Cached frame dimensions (initialized on first update)
	bool m_bFrameSizesInitialized;
	int m_iWideBoxWidth;   // Frame 78 width
	int m_iWideBoxHeight;  // Frame 78 height
	int m_iSmallBoxWidth;  // Frame 79 width
	int m_iSmallBoxHeight; // Frame 79 height
};
