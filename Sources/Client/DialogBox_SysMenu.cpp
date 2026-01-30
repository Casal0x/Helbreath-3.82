#include "DialogBox_SysMenu.h"
#include "Game.h"
#include "GlobalDef.h"
#include "lan_eng.h"
#include "AudioManager.h"
#include "ConfigManager.h"
#include "IInput.h"
#include "RendererFactory.h"
#include "ITextRenderer.h"
#include <cstring>

// Content area constants
static const int CONTENT_X = 21;
static const int CONTENT_Y = 57;
static const int CONTENT_WIDTH = 297;
static const int CONTENT_HEIGHT = 234;

// Slider tracking
static bool s_bDraggingSoundSlider = false;
static bool s_bDraggingMusicSlider = false;

// 4:3 resolutions from 640x480 to 1440x1080
const Resolution DialogBox_SysMenu::s_Resolutions[] = {
	{ 640, 480 },
	{ 800, 600 },
	{ 1024, 768 },
	{ 1280, 960 },
	{ 1440, 1080 }
};

const int DialogBox_SysMenu::s_NumResolutions = sizeof(s_Resolutions) / sizeof(s_Resolutions[0]);

int DialogBox_SysMenu::GetCurrentResolutionIndex()
{
	int currentWidth = ConfigManager::Get().GetWindowWidth();
	int currentHeight = ConfigManager::Get().GetWindowHeight();

	for (int i = 0; i < s_NumResolutions; i++) {
		if (s_Resolutions[i].width == currentWidth && s_Resolutions[i].height == currentHeight) {
			return i;
		}
	}
	return GetNearestResolutionIndex(currentWidth, currentHeight);
}

int DialogBox_SysMenu::GetNearestResolutionIndex(int width, int height)
{
	int bestIndex = 0;
	int bestDiff = abs(s_Resolutions[0].width - width) + abs(s_Resolutions[0].height - height);

	for (int i = 1; i < s_NumResolutions; i++) {
		int diff = abs(s_Resolutions[i].width - width) + abs(s_Resolutions[i].height - height);
		if (diff < bestDiff) {
			bestDiff = diff;
			bestIndex = i;
		}
	}
	return bestIndex;
}

void DialogBox_SysMenu::CycleResolution()
{
	int currentIndex = GetCurrentResolutionIndex();
	int nextIndex = (currentIndex + 1) % s_NumResolutions;
	ApplyResolution(nextIndex);
}

void DialogBox_SysMenu::ApplyResolution(int index)
{
	if (index < 0 || index >= s_NumResolutions) return;

	int newWidth = s_Resolutions[index].width;
	int newHeight = s_Resolutions[index].height;

	ConfigManager::Get().SetWindowSize(newWidth, newHeight);
	ConfigManager::Get().Save();

	Window::SetSize(newWidth, newHeight, true);

	if (Renderer::Get())
		Renderer::Get()->ResizeBackBuffer(newWidth, newHeight);

	Input::Get()->SetWindowActive(true);
}

DialogBox_SysMenu::DialogBox_SysMenu(CGame* pGame)
	: IDialogBox(DialogBoxId::SystemMenu, pGame)
	, m_iActiveTab(TAB_GENERAL)
	, m_bFrameSizesInitialized(false)
	, m_iWideBoxWidth(0)
	, m_iWideBoxHeight(0)
	, m_iSmallBoxWidth(0)
	, m_iSmallBoxHeight(0)
{
	SetDefaultRect(237 + SCREENX, 67 + SCREENY, 331, 303);
}

void DialogBox_SysMenu::OnUpdate()
{
	// Cache frame dimensions on first update (sprites loaded by now)
	if (!m_bFrameSizesInitialized && m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON] != nullptr)
	{
		SpriteLib::SpriteRect wideRect = m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->GetFrameRect(78);
		SpriteLib::SpriteRect smallRect = m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->GetFrameRect(79);

		// Only use if we got valid dimensions (not from NullSprite)
		if (wideRect.width > 0 && wideRect.height > 0 && smallRect.width > 0 && smallRect.height > 0)
		{
			m_iWideBoxWidth = wideRect.width;
			m_iWideBoxHeight = wideRect.height;
			m_iSmallBoxWidth = smallRect.width;
			m_iSmallBoxHeight = smallRect.height;
			m_bFrameSizesInitialized = true;
		}
	}
}

void DialogBox_SysMenu::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX = Info().sX;
	short sY = Info().sY;

	// Draw dialog background
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME1, sX, sY, 0);
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_TEXT, sX, sY, 6);

	// Handle mouse scroll over dialog to cycle tabs
	if (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::SystemMenu && msZ != 0)
	{
		if (msZ > 0)
			m_iActiveTab = (m_iActiveTab - 1 + TAB_COUNT) % TAB_COUNT;
		else
			m_iActiveTab = (m_iActiveTab + 1) % TAB_COUNT;
	}

	DrawTabs(sX, sY, msX, msY);
	DrawTabContent(sX, sY, msX, msY, cLB);

	// Reset slider dragging when mouse button is released
	if (cLB == 0)
	{
		s_bDraggingSoundSlider = false;
		s_bDraggingMusicSlider = false;
		Info().bIsScrollSelected = false;
	}
}

void DialogBox_SysMenu::DrawTabs(short sX, short sY, short msX, short msY)
{
	SpriteLib::SpriteRect button_rect = m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->GetFrameRect(70);
	int btnY = sY + 33;

	const int tabFrames[TAB_COUNT][2] = {
		{70, 71},  // General
		{72, 73},  // Graphics
		{74, 76},  // Audio
		{75, 77}   // System
	};

	for (int i = 0; i < TAB_COUNT; i++)
	{
		int btnX = sX + 17 + (button_rect.width * i);
		bool bHovered = (msX >= btnX && msX < btnX + button_rect.width && msY >= btnY && msY < btnY + button_rect.height);
		bool bActive = (m_iActiveTab == i);

		int frameIndex = (bActive || bHovered) ? tabFrames[i][1] : tabFrames[i][0];
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, btnX, btnY, frameIndex);
	}
}

void DialogBox_SysMenu::DrawTabContent(short sX, short sY, short msX, short msY, char cLB)
{
	switch (m_iActiveTab)
	{
	case TAB_GENERAL:
		DrawGeneralTab(sX, sY, msX, msY);
		break;
	case TAB_GRAPHICS:
		DrawGraphicsTab(sX, sY, msX, msY);
		break;
	case TAB_AUDIO:
		DrawAudioTab(sX, sY, msX, msY, cLB);
		break;
	case TAB_SYSTEM:
		DrawSystemTab(sX, sY, msX, msY);
		break;
	}
}

void DialogBox_SysMenu::DrawToggle(int x, int y, bool bEnabled, short msX, short msY)
{
	// Draw toggle background box at y-2
	const int boxY = y - 2;
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, x, boxY, 79);

	// Use cached dimensions or fallback values
	const int boxWidth = m_bFrameSizesInitialized ? m_iSmallBoxWidth : 36;
	const int boxHeight = m_bFrameSizesInitialized ? m_iSmallBoxHeight : 16;

	bool bHover = (msX >= x && msX <= x + boxWidth && msY >= boxY && msY <= boxY + boxHeight);
	COLORREF color = (bEnabled || bHover) ? GameColors::UIWhite.ToColorRef() : GameColors::UIDisabled.ToColorRef();

	// Center text horizontally and vertically in the box
	const char* text = bEnabled ? DRAW_DIALOGBOX_SYSMENU_ON : DRAW_DIALOGBOX_SYSMENU_OFF;
	TextLib::TextMetrics metrics = TextLib::GetTextRenderer()->MeasureText(text);
	int textX = x + (boxWidth - metrics.width) / 2;
	int textY = boxY + (boxHeight - metrics.height) / 2;
	PutString(textX, textY, text, color);
}

bool DialogBox_SysMenu::IsInToggleArea(int x, int y, short msX, short msY)
{
	const int boxWidth = m_bFrameSizesInitialized ? m_iSmallBoxWidth : 36;
	const int boxHeight = m_bFrameSizesInitialized ? m_iSmallBoxHeight : 16;
	return (msX >= x && msX <= x + boxWidth && msY >= y - 2 && msY <= y - 2 + boxHeight);
}

// =============================================================================
// GENERAL TAB
// =============================================================================
void DialogBox_SysMenu::DrawGeneralTab(short sX, short sY, short msX, short msY)
{
	const int contentX = sX + CONTENT_X;
	const int contentY = sY + CONTENT_Y;
	const int contentBottom = contentY + CONTENT_HEIGHT;
	const int centerX = contentX + (CONTENT_WIDTH / 2);

	// Server name at top left
	PutString(contentX + 5, contentY + 5, UPDATE_SCREEN_ON_SELECT_CHARACTER36, GameColors::UILabel.ToColorRef());
	PutString(contentX + 6, contentY + 5, UPDATE_SCREEN_ON_SELECT_CHARACTER36, GameColors::UILabel.ToColorRef());

	// Current time centered below server name (MM/DD/YYYY HH:MM AM/PM)
	SYSTEMTIME SysTime;
	GetLocalTime(&SysTime);
	char timeBuf[32];
	int hour12 = SysTime.wHour % 12;
	if (hour12 == 0) hour12 = 12;
	const char* ampm = (SysTime.wHour < 12) ? "AM" : "PM";
	snprintf(timeBuf, sizeof(timeBuf), "%02d/%02d/%04d %d:%02d %s",
		SysTime.wMonth, SysTime.wDay, SysTime.wYear,
		hour12, SysTime.wMinute, ampm);

	int textWidth = TextLib::GetTextRenderer()->MeasureText(timeBuf).width;
	int timeX = centerX - (textWidth / 2);
	PutString(timeX, contentY + 25, timeBuf, GameColors::UILabel.ToColorRef());
	PutString(timeX + 1, contentY + 25, timeBuf, GameColors::UILabel.ToColorRef());

	// Buttons at bottom of content area
	int buttonY = contentBottom - 30;

	// Log-Out / Continue button (left side)
	if (m_pGame->m_cLogOutCount == -1) {
		bool bHover = (msX >= sX + DEF_LBTNPOSX && msX <= sX + DEF_LBTNPOSX + DEF_BTNSZX &&
			msY >= buttonY && msY <= buttonY + DEF_BTNSZY);
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_LBTNPOSX, buttonY, bHover ? 9 : 8);
	}
	else {
		bool bHover = (msX >= sX + DEF_LBTNPOSX && msX <= sX + DEF_LBTNPOSX + DEF_BTNSZX &&
			msY >= buttonY && msY <= buttonY + DEF_BTNSZY);
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_LBTNPOSX, buttonY, bHover ? 7 : 6);
	}

	// Restart button (right side, only when dead)
	if ((m_pGame->m_pPlayer->m_iHP <= 0) && (m_pGame->m_cRestartCount == -1))
	{
		bool bHover = (msX >= sX + DEF_RBTNPOSX && msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX &&
			msY >= buttonY && msY <= buttonY + DEF_BTNSZY);
		DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, buttonY, bHover ? 37 : 36);
	}
}

// =============================================================================
// GRAPHICS TAB
// =============================================================================
void DialogBox_SysMenu::DrawGraphicsTab(short sX, short sY, short msX, short msY)
{
	const int contentX = sX + CONTENT_X;
	const int contentY = sY + CONTENT_Y;
	const int contentRight = contentX + CONTENT_WIDTH;
	int lineY = contentY + 5;
	const int labelX = contentX + 5;

	// All boxes align to the same right edge
	const int rightMargin = 8;
	const int boxRightEdge = contentRight - rightMargin;

	// Use cached dimensions or fallback values
	const int wideBoxWidth = m_bFrameSizesInitialized ? m_iWideBoxWidth : 175;
	const int wideBoxHeight = m_bFrameSizesInitialized ? m_iWideBoxHeight : 16;

	// Wide boxes align to right edge, small boxes align to LEFT edge of wide boxes
	const int wideBoxX = boxRightEdge - wideBoxWidth;
	const int smallBoxX = wideBoxX;  // Left-align with wide box

	// Detail Level - single wide box with Low/Normal/High inside
	PutString(labelX, lineY, DRAW_DIALOGBOX_SYSMENU_DETAILLEVEL, GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, DRAW_DIALOGBOX_SYSMENU_DETAILLEVEL, GameColors::UILabel.ToColorRef());

	const int detailLevel = ConfigManager::Get().GetDetailLevel();
	const int boxY = lineY - 2;

	// Draw single wide background box
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, wideBoxX, boxY, 78);

	// Calculate text positions - evenly spaced inside the wide box (width / 3 regions)
	const int regionWidth = wideBoxWidth / 3;

	// Center text vertically in the box
	TextLib::TextMetrics lowMetrics = TextLib::GetTextRenderer()->MeasureText(DRAW_DIALOGBOX_SYSMENU_LOW);
	int textY = boxY + (wideBoxHeight - lowMetrics.height) / 2;

	// Center each option horizontally within its region
	TextLib::TextMetrics normMetrics = TextLib::GetTextRenderer()->MeasureText(DRAW_DIALOGBOX_SYSMENU_NORMAL);
	TextLib::TextMetrics highMetrics = TextLib::GetTextRenderer()->MeasureText(DRAW_DIALOGBOX_SYSMENU_HIGH);

	int lowX = wideBoxX + (regionWidth - lowMetrics.width) / 2;
	int normX = wideBoxX + regionWidth + (regionWidth - normMetrics.width) / 2;
	int highX = wideBoxX + (regionWidth * 2) + (regionWidth - highMetrics.width) / 2;

	bool lowHover = (msX >= wideBoxX && msX < wideBoxX + regionWidth && msY >= boxY && msY <= boxY + wideBoxHeight);
	bool normHover = (msX >= wideBoxX + regionWidth && msX < wideBoxX + (regionWidth * 2) && msY >= boxY && msY <= boxY + wideBoxHeight);
	bool highHover = (msX >= wideBoxX + (regionWidth * 2) && msX <= wideBoxX + wideBoxWidth && msY >= boxY && msY <= boxY + wideBoxHeight);

	PutString(lowX, textY, DRAW_DIALOGBOX_SYSMENU_LOW, (detailLevel == 0 || lowHover) ? GameColors::UIWhite.ToColorRef() : GameColors::UIDisabled.ToColorRef());
	PutString(normX, textY, DRAW_DIALOGBOX_SYSMENU_NORMAL, (detailLevel == 1 || normHover) ? GameColors::UIWhite.ToColorRef() : GameColors::UIDisabled.ToColorRef());
	PutString(highX, textY, DRAW_DIALOGBOX_SYSMENU_HIGH, (detailLevel == 2 || highHover) ? GameColors::UIWhite.ToColorRef() : GameColors::UIDisabled.ToColorRef());

	lineY += 18;

	// Dialog Transparency
	PutString(labelX, lineY, "Transparency:", GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, "Transparency:", GameColors::UILabel.ToColorRef());
	DrawToggle(smallBoxX, lineY, ConfigManager::Get().IsDialogTransparencyEnabled(), msX, msY);

	lineY += 18;

	// Guide Map
	PutString(labelX, lineY, DRAW_DIALOGBOX_SYSMENU_GUIDEMAP, GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, DRAW_DIALOGBOX_SYSMENU_GUIDEMAP, GameColors::UILabel.ToColorRef());
	DrawToggle(smallBoxX, lineY, m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::GuideMap), msX, msY);

	lineY += 18;

	// Show FPS
	PutString(labelX, lineY, "Show FPS:", GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, "Show FPS:", GameColors::UILabel.ToColorRef());
	DrawToggle(smallBoxX, lineY, ConfigManager::Get().IsShowFpsEnabled(), msX, msY);

	lineY += 18;

	// Show Latency
	PutString(labelX, lineY, "Show Latency:", GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, "Show Latency:", GameColors::UILabel.ToColorRef());
	DrawToggle(smallBoxX, lineY, ConfigManager::Get().IsShowLatencyEnabled(), msX, msY);

	lineY += 18;

	// Display Mode - wide box showing current mode (clicking toggles)
	PutString(labelX, lineY, "Display Mode:", GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, "Display Mode:", GameColors::UILabel.ToColorRef());

	const bool isFullscreen = m_pGame->m_Renderer->IsFullscreen();
	const int modeBoxY = lineY - 2;

	// Draw wide box for display mode
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, wideBoxX, modeBoxY, 78);

	bool modeHover = (msX >= wideBoxX && msX <= wideBoxX + wideBoxWidth && msY >= modeBoxY && msY <= modeBoxY + wideBoxHeight);
	COLORREF modeColor = modeHover ? GameColors::UIWhite.ToColorRef() : GameColors::UIDisabled.ToColorRef();

	// Show current mode text, centered horizontally and vertically
	const char* modeText = isFullscreen ? "Fullscreen" : "Windowed";
	TextLib::TextMetrics modeMetrics = TextLib::GetTextRenderer()->MeasureText(modeText);
	int modeTextX = wideBoxX + (wideBoxWidth - modeMetrics.width) / 2;
	int modeTextY = modeBoxY + (wideBoxHeight - modeMetrics.height) / 2;
	PutString(modeTextX, modeTextY, modeText, modeColor);

	lineY += 18;

	// Resolution - wide box with centered text
	PutString(labelX, lineY, "Resolution:", GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, "Resolution:", GameColors::UILabel.ToColorRef());

	int resWidth, resHeight;
	if (isFullscreen) {
		resWidth = GetSystemMetrics(SM_CXSCREEN);
		resHeight = GetSystemMetrics(SM_CYSCREEN);
	}
	else {
		int resIndex = GetCurrentResolutionIndex();
		resWidth = s_Resolutions[resIndex].width;
		resHeight = s_Resolutions[resIndex].height;
	}

	char resBuf[32];
	snprintf(resBuf, sizeof(resBuf), "%dx%d", resWidth, resHeight);

	// Draw wide background box
	const int resBoxY = lineY - 2;
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, wideBoxX, resBoxY, 78);

	bool resHover = !isFullscreen && (msX >= wideBoxX && msX <= wideBoxX + wideBoxWidth && msY >= resBoxY && msY <= resBoxY + wideBoxHeight);
	COLORREF resColor = isFullscreen ? GameColors::UIDisabled.ToColorRef() : (resHover ? GameColors::UIWhite.ToColorRef() : GameColors::UIDisabled.ToColorRef());

	// Center the resolution text horizontally and vertically in the box
	TextLib::TextMetrics resMetrics = TextLib::GetTextRenderer()->MeasureText(resBuf);
	int resTextX = wideBoxX + (wideBoxWidth - resMetrics.width) / 2;
	int resTextY = resBoxY + (wideBoxHeight - resMetrics.height) / 2;
	PutString(resTextX, resTextY, resBuf, resColor);
}

// =============================================================================
// AUDIO TAB
// =============================================================================
void DialogBox_SysMenu::DrawAudioTab(short sX, short sY, short msX, short msY, char cLB)
{
	const int contentX = sX + CONTENT_X;
	const int contentY = sY + CONTENT_Y;
	const int labelX = contentX + 5;
	const int toggleX = contentX + 120;
	const int sliderX = contentX + 50;
	const int sliderWidth = 100;

	// Sound section
	int lineY = contentY + 10;

	// Sound toggle
	PutString(labelX, lineY, DRAW_DIALOGBOX_SYSMENU_SOUND, GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, DRAW_DIALOGBOX_SYSMENU_SOUND, GameColors::UILabel.ToColorRef());

	if (AudioManager::Get().IsSoundAvailable()) {
		DrawToggle(toggleX, lineY, AudioManager::Get().IsSoundEnabled(), msX, msY);
	}
	else {
		PutString(toggleX, lineY, DRAW_DIALOGBOX_SYSMENU_DISABLED, GameColors::UIDisabled.ToColorRef());
	}

	lineY += 25;  // Extra spacing before volume section

	// Sound Volume label
	PutString(labelX, lineY, DRAW_DIALOGBOX_SYSMENU_SOUNDVOLUME, GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, DRAW_DIALOGBOX_SYSMENU_SOUNDVOLUME, GameColors::UILabel.ToColorRef());

	lineY += 15;

	// Sound Volume slider
	int soundVol = AudioManager::Get().GetSoundVolume();
	int soundSliderY = lineY;
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sliderX, soundSliderY + 5, 80);  // Slider track (dropped 5px)
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sliderX + soundVol, soundSliderY, 8);  // Slider knob

	// Handle sound volume slider - once dragging, only track X position
	if (s_bDraggingSoundSlider && cLB != 0)
	{
		int volume = msX - sliderX;
		if (volume > 100) volume = 100;
		if (volume < 0) volume = 0;
		AudioManager::Get().SetSoundVolume(volume);
	}

	lineY += 35;

	// Music section
	int musicToggleY = lineY;

	// Music toggle
	PutString(labelX, lineY, DRAW_DIALOGBOX_SYSMENU_MUSIC, GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, DRAW_DIALOGBOX_SYSMENU_MUSIC, GameColors::UILabel.ToColorRef());

	if (AudioManager::Get().IsSoundAvailable()) {
		DrawToggle(toggleX, lineY, AudioManager::Get().IsMusicEnabled(), msX, msY);
	}
	else {
		PutString(toggleX, lineY, DRAW_DIALOGBOX_SYSMENU_DISABLED, GameColors::UIDisabled.ToColorRef());
	}

	lineY += 25;  // Extra spacing before volume section

	// Music Volume label
	PutString(labelX, lineY, DRAW_DIALOGBOX_SYSMENU_MUSICVOLUME, GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, DRAW_DIALOGBOX_SYSMENU_MUSICVOLUME, GameColors::UILabel.ToColorRef());

	lineY += 15;

	// Music Volume slider
	int musicVol = AudioManager::Get().GetMusicVolume();
	int musicSliderY = lineY;
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sliderX, musicSliderY + 5, 80);  // Slider track (dropped 5px)
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sliderX + musicVol, musicSliderY, 8);  // Slider knob

	// Handle music volume slider - once dragging, only track X position
	if (s_bDraggingMusicSlider && cLB != 0)
	{
		int volume = msX - sliderX;
		if (volume > 100) volume = 100;
		if (volume < 0) volume = 0;
		AudioManager::Get().SetMusicVolume(volume);
	}
}

// =============================================================================
// SYSTEM TAB
// =============================================================================
void DialogBox_SysMenu::DrawSystemTab(short sX, short sY, short msX, short msY)
{
	const int contentX = sX + CONTENT_X;
	const int contentY = sY + CONTENT_Y;
	int lineY = contentY + 5;
	const int labelX = contentX + 5;
	const int valueX = contentX + 140;

	// Whisper toggle
	PutString(labelX, lineY, DRAW_DIALOGBOX_SYSMENU_WHISPER, GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, DRAW_DIALOGBOX_SYSMENU_WHISPER, GameColors::UILabel.ToColorRef());
	DrawToggle(valueX, lineY, m_pGame->m_bWhisper, msX, msY);

	lineY += 20;

	// Shout toggle
	PutString(labelX, lineY, DRAW_DIALOGBOX_SYSMENU_SHOUT, GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, DRAW_DIALOGBOX_SYSMENU_SHOUT, GameColors::UILabel.ToColorRef());
	DrawToggle(valueX, lineY, m_pGame->m_bShout, msX, msY);

	lineY += 20;

	// Running Mode toggle
	PutString(labelX, lineY, "Running Mode:", GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, "Running Mode:", GameColors::UILabel.ToColorRef());
	DrawToggle(valueX, lineY, ConfigManager::Get().IsRunningModeEnabled(), msX, msY);
}

// =============================================================================
// CLICK HANDLERS
// =============================================================================
bool DialogBox_SysMenu::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	// Check tab button clicks
	SpriteLib::SpriteRect button_rect = m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->GetFrameRect(70);
	int btnY = sY + 33;

	for (int i = 0; i < TAB_COUNT; i++)
	{
		int btnX = sX + 17 + (button_rect.width * i);
		if (msX >= btnX && msX < btnX + button_rect.width && msY >= btnY && msY < btnY + button_rect.height)
		{
			m_iActiveTab = i;
			PlaySoundEffect('E', 14, 5);
			return true;
		}
	}

	// Handle clicks for active tab content
	switch (m_iActiveTab)
	{
	case TAB_GENERAL:
		return OnClickGeneral(sX, sY, msX, msY);
	case TAB_GRAPHICS:
		return OnClickGraphics(sX, sY, msX, msY);
	case TAB_AUDIO:
		return OnClickAudio(sX, sY, msX, msY);
	case TAB_SYSTEM:
		return OnClickSystem(sX, sY, msX, msY);
	}

	return false;
}

PressResult DialogBox_SysMenu::OnPress(short msX, short msY)
{
	// Only claim scroll for Audio tab slider areas
	if (m_iActiveTab != TAB_AUDIO)
		return PressResult::Normal;

	short sX = Info().sX;
	short sY = Info().sY;
	const int contentX = sX + CONTENT_X;
	const int contentY = sY + CONTENT_Y;
	const int sliderX = contentX + 50;
	const int sliderWidth = 100;

	// Sound slider Y: contentY + 10 + 25 + 15 = contentY + 50
	int soundSliderY = contentY + 50;
	if ((msX >= sliderX) && (msX <= sliderX + sliderWidth + 10) &&
		(msY >= soundSliderY - 5) && (msY <= soundSliderY + 15))
	{
		s_bDraggingSoundSlider = true;
		Info().bIsScrollSelected = true;
		return PressResult::ScrollClaimed;
	}

	// Music slider Y: contentY + 50 + 35 + 25 + 15 = contentY + 125
	int musicSliderY = contentY + 125;
	if ((msX >= sliderX) && (msX <= sliderX + sliderWidth + 10) &&
		(msY >= musicSliderY - 5) && (msY <= musicSliderY + 15))
	{
		s_bDraggingMusicSlider = true;
		Info().bIsScrollSelected = true;
		return PressResult::ScrollClaimed;
	}

	return PressResult::Normal;
}

bool DialogBox_SysMenu::OnClickGeneral(short sX, short sY, short msX, short msY)
{
	const int contentY = sY + CONTENT_Y;
	const int contentBottom = contentY + CONTENT_HEIGHT;
	int buttonY = contentBottom - 30;

	// Log-Out / Continue button
	if (msX >= sX + DEF_LBTNPOSX && msX <= sX + DEF_LBTNPOSX + DEF_BTNSZX &&
		msY >= buttonY && msY <= buttonY + DEF_BTNSZY)
	{
		if (!m_pGame->m_bForceDisconn)
		{
			if (m_pGame->m_cLogOutCount == -1) {
				m_pGame->m_cLogOutCount = 11;
			}
			else {
				m_pGame->m_cLogOutCount = -1;
				AddEventList(DLGBOX_CLICK_SYSMENU2, 10);
				DisableThisDialog();
			}
			PlaySoundEffect('E', 14, 5);
			return true;
		}
	}

	// Restart button (only when dead)
	if ((m_pGame->m_pPlayer->m_iHP <= 0) && (m_pGame->m_cRestartCount == -1))
	{
		if (msX >= sX + DEF_RBTNPOSX && msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX &&
			msY >= buttonY && msY <= buttonY + DEF_BTNSZY)
		{
			m_pGame->m_cRestartCount = 5;
			m_pGame->m_dwRestartCountTime = GameClock::GetTimeMS();
			DisableThisDialog();
			char restartBuf[64];
			snprintf(restartBuf, sizeof(restartBuf), DLGBOX_CLICK_SYSMENU1, m_pGame->m_cRestartCount);
			AddEventList(restartBuf, 10);
			PlaySoundEffect('E', 14, 5);
			return true;
		}
	}

	return false;
}

bool DialogBox_SysMenu::OnClickGraphics(short sX, short sY, short msX, short msY)
{
	const int contentX = sX + CONTENT_X;
	const int contentY = sY + CONTENT_Y;
	const int contentRight = contentX + CONTENT_WIDTH;
	int lineY = contentY + 5;

	// Match draw positions - use fallback values if not initialized
	const int rightMargin = 8;
	const int boxRightEdge = contentRight - rightMargin;
	const int wideBoxWidth = m_bFrameSizesInitialized ? m_iWideBoxWidth : 175;
	const int wideBoxHeight = m_bFrameSizesInitialized ? m_iWideBoxHeight : 16;
	const int wideBoxX = boxRightEdge - wideBoxWidth;
	const int smallBoxX = wideBoxX;  // Left-align with wide box
	const int regionWidth = wideBoxWidth / 3;

	// Detail Level clicks (single wide box with three regions)
	const int boxY = lineY - 2;
	if (msY >= boxY && msY <= boxY + wideBoxHeight && msX >= wideBoxX && msX <= wideBoxX + wideBoxWidth) {
		// Low region
		if (msX < wideBoxX + regionWidth) {
			ConfigManager::Get().SetDetailLevel(0);
			AddEventList(NOTIFY_MSG_DETAIL_LEVEL_LOW, 10);
			PlaySoundEffect('E', 14, 5);
			return true;
		}
		// Normal region
		if (msX < wideBoxX + (regionWidth * 2)) {
			ConfigManager::Get().SetDetailLevel(1);
			AddEventList(NOTIFY_MSG_DETAIL_LEVEL_MEDIUM, 10);
			PlaySoundEffect('E', 14, 5);
			return true;
		}
		// High region
		ConfigManager::Get().SetDetailLevel(2);
		AddEventList(NOTIFY_MSG_DETAIL_LEVEL_HIGH, 10);
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	lineY += 18;

	// Dialog Transparency toggle
	if (IsInToggleArea(smallBoxX, lineY, msX, msY)) {
		bool enabled = ConfigManager::Get().IsDialogTransparencyEnabled();
		ConfigManager::Get().SetDialogTransparencyEnabled(!enabled);
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	lineY += 18;

	// Guide Map toggle
	if (IsInToggleArea(smallBoxX, lineY, msX, msY)) {
		if (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::GuideMap))
			DisableDialogBox(DialogBoxId::GuideMap);
		else
			EnableDialogBox(DialogBoxId::GuideMap, 0, 0, 0);
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	lineY += 18;

	// Show FPS toggle
	if (IsInToggleArea(smallBoxX, lineY, msX, msY)) {
		bool enabled = ConfigManager::Get().IsShowFpsEnabled();
		ConfigManager::Get().SetShowFpsEnabled(!enabled);
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	lineY += 18;

	// Show Latency toggle
	if (IsInToggleArea(smallBoxX, lineY, msX, msY)) {
		bool enabled = ConfigManager::Get().IsShowLatencyEnabled();
		ConfigManager::Get().SetShowLatencyEnabled(!enabled);
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	lineY += 18;

	// Display Mode toggle click (wide box, toggles between windowed/fullscreen)
	const int modeBoxY = lineY - 2;
	if (msX >= wideBoxX && msX <= wideBoxX + wideBoxWidth && msY >= modeBoxY && msY <= modeBoxY + wideBoxHeight) {
		bool isFullscreen = m_pGame->m_Renderer->IsFullscreen();
		m_pGame->m_Renderer->SetFullscreen(!isFullscreen);
		m_pGame->m_Renderer->ChangeDisplayMode(Window::GetHandle());
		Input::Get()->SetWindowActive(true);
		ConfigManager::Get().SetFullscreenEnabled(!isFullscreen);
		ConfigManager::Get().Save();
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	lineY += 18;

	// Resolution click (wide box, only in windowed mode)
	const int resBoxY = lineY - 2;
	bool isFullscreen = m_pGame->m_Renderer->IsFullscreen();
	if (!isFullscreen && msX >= wideBoxX && msX <= wideBoxX + wideBoxWidth && msY >= resBoxY && msY <= resBoxY + wideBoxHeight) {
		CycleResolution();
		PlaySoundEffect('E', 14, 5);
		AddEventList("Resolution changed.", 10);
		return true;
	}

	return false;
}

bool DialogBox_SysMenu::OnClickAudio(short sX, short sY, short msX, short msY)
{
	const int contentX = sX + CONTENT_X;
	const int contentY = sY + CONTENT_Y;
	const int toggleX = contentX + 120;

	// Sound toggle (lineY = contentY + 10)
	int lineY = contentY + 10;
	if (IsInToggleArea(toggleX, lineY, msX, msY)) {
		if (AudioManager::Get().IsSoundAvailable()) {
			if (AudioManager::Get().IsSoundEnabled()) {
				AudioManager::Get().StopSound(SoundType::Effect, 38);
				AudioManager::Get().SetSoundEnabled(false);
				AddEventList(NOTIFY_MSG_SOUND_OFF, 10);
			}
			else {
				AudioManager::Get().SetSoundEnabled(true);
				AddEventList(NOTIFY_MSG_SOUND_ON, 10);
			}
			PlaySoundEffect('E', 14, 5);
		}
		return true;
	}

	// Music toggle (lineY = contentY + 10 + 25 + 15 + 35 = contentY + 85)
	lineY = contentY + 85;
	if (IsInToggleArea(toggleX, lineY, msX, msY)) {
		if (AudioManager::Get().IsSoundAvailable()) {
			if (AudioManager::Get().IsMusicEnabled()) {
				AudioManager::Get().SetMusicEnabled(false);
				AddEventList(NOTIFY_MSG_MUSIC_OFF, 10);
				AudioManager::Get().StopMusic();
			}
			else {
				AudioManager::Get().SetMusicEnabled(true);
				AddEventList(NOTIFY_MSG_MUSIC_ON, 10);
				m_pGame->StartBGM();
			}
			PlaySoundEffect('E', 14, 5);
		}
		return true;
	}

	return false;
}

bool DialogBox_SysMenu::OnClickSystem(short sX, short sY, short msX, short msY)
{
	const int contentX = sX + CONTENT_X;
	const int contentY = sY + CONTENT_Y;
	int lineY = contentY + 5;
	const int valueX = contentX + 140;

	// Whisper toggle
	if (IsInToggleArea(valueX, lineY, msX, msY)) {
		if (m_pGame->m_bWhisper) {
			m_pGame->m_bWhisper = false;
			AddEventList(BCHECK_LOCAL_CHAT_COMMAND7, 10);
		}
		else {
			m_pGame->m_bWhisper = true;
			AddEventList(BCHECK_LOCAL_CHAT_COMMAND6, 10);
		}
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	lineY += 20;

	// Shout toggle
	if (IsInToggleArea(valueX, lineY, msX, msY)) {
		if (m_pGame->m_bShout) {
			m_pGame->m_bShout = false;
			AddEventList(BCHECK_LOCAL_CHAT_COMMAND9, 10);
		}
		else {
			m_pGame->m_bShout = true;
			AddEventList(BCHECK_LOCAL_CHAT_COMMAND8, 10);
		}
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	lineY += 20;

	// Running Mode toggle
	if (IsInToggleArea(valueX, lineY, msX, msY)) {
		bool enabled = ConfigManager::Get().IsRunningModeEnabled();
		ConfigManager::Get().SetRunningModeEnabled(!enabled);
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	return false;
}
