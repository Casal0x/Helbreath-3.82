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
static bool s_bDraggingMasterSlider = false;
static bool s_bDraggingEffectsSlider = false;
static bool s_bDraggingAmbientSlider = false;
static bool s_bDraggingUISlider = false;
static bool s_bDraggingMusicSlider = false;

// 4:3 resolutions from 640x480 to 1920x1440
const Resolution DialogBox_SysMenu::s_Resolutions[] = {
	//{ 640, 480 },
	{ 800, 600 },
	{ 1024, 768 },
	{ 1280, 960 },
	{ 1440, 1080 },
	{ 1920, 1440 }
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
	SetDefaultRect(237 , 67 , 331, 303);
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

	// Save slider values to ConfigManager when drag ends (mouse released)
	if (cLB == 0)
	{
		if (s_bDraggingMasterSlider)
		{
			ConfigManager::Get().SetMasterVolume(AudioManager::Get().GetMasterVolume());
		}
		if (s_bDraggingEffectsSlider)
		{
			ConfigManager::Get().SetSoundVolume(AudioManager::Get().GetSoundVolume());
		}
		if (s_bDraggingAmbientSlider)
		{
			ConfigManager::Get().SetAmbientVolume(AudioManager::Get().GetAmbientVolume());
		}
		if (s_bDraggingUISlider)
		{
			ConfigManager::Get().SetUIVolume(AudioManager::Get().GetUIVolume());
		}
		if (s_bDraggingMusicSlider)
		{
			ConfigManager::Get().SetMusicVolume(AudioManager::Get().GetMusicVolume());
		}
		s_bDraggingMasterSlider = false;
		s_bDraggingEffectsSlider = false;
		s_bDraggingAmbientSlider = false;
		s_bDraggingUISlider = false;
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

	// Tile Grid (simple dark lines)
	PutString(labelX, lineY, "Tile Grid:", GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, "Tile Grid:", GameColors::UILabel.ToColorRef());
	DrawToggle(smallBoxX, lineY, ConfigManager::Get().IsTileGridEnabled(), msX, msY);

	lineY += 18;

	// Patching Grid (debug with zone colors)
	PutString(labelX, lineY, "Patching Grid:", GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, "Patching Grid:", GameColors::UILabel.ToColorRef());
	DrawToggle(smallBoxX, lineY, ConfigManager::Get().IsPatchingGridEnabled(), msX, msY);

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

	// Window Style - wide box showing Borderless/Bordered (disabled when fullscreen)
	PutString(labelX, lineY, "Window Style:", GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, "Window Style:", GameColors::UILabel.ToColorRef());

	const int styleBoxY = lineY - 2;
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, wideBoxX, styleBoxY, 78);

	bool styleHover = !isFullscreen && (msX >= wideBoxX && msX <= wideBoxX + wideBoxWidth && msY >= styleBoxY && msY <= styleBoxY + wideBoxHeight);
	COLORREF styleColor = isFullscreen ? GameColors::UIDisabled.ToColorRef() : (styleHover ? GameColors::UIWhite.ToColorRef() : GameColors::UIDisabled.ToColorRef());

	const char* styleText = ConfigManager::Get().IsBorderlessEnabled() ? "Borderless" : "Bordered";
	TextLib::TextMetrics styleMetrics = TextLib::GetTextRenderer()->MeasureText(styleText);
	int styleTextX = wideBoxX + (wideBoxWidth - styleMetrics.width) / 2;
	int styleTextY = styleBoxY + (wideBoxHeight - styleMetrics.height) / 2;
	PutString(styleTextX, styleTextY, styleText, styleColor);

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
	const int toggleX = contentX + 68;
	const int sliderX = contentX + 110;

	bool bAvailable = AudioManager::Get().IsSoundAvailable();

	// --- Master: [On/Off] ---slider--- ---
	int lineY = contentY + 8;

	PutString(labelX, lineY, "Master:", GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, "Master:", GameColors::UILabel.ToColorRef());

	if (bAvailable)
		DrawToggle(toggleX, lineY, AudioManager::Get().IsMasterEnabled(), msX, msY);
	else
		PutString(toggleX, lineY, DRAW_DIALOGBOX_SYSMENU_DISABLED, GameColors::UIDisabled.ToColorRef());

	int masterVol = AudioManager::Get().GetMasterVolume();
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sliderX, lineY + 5, 80);
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sliderX + masterVol, lineY, 8);

	if (s_bDraggingMasterSlider && cLB != 0)
	{
		int volume = msX - sliderX;
		if (volume > 100) volume = 100;
		if (volume < 0) volume = 0;
		AudioManager::Get().SetMasterVolume(volume);
	}

	// --- Effects: [On/Off] ---slider--- ---
	lineY = contentY + 52;

	PutString(labelX, lineY, "Effects:", GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, "Effects:", GameColors::UILabel.ToColorRef());

	if (bAvailable)
		DrawToggle(toggleX, lineY, AudioManager::Get().IsSoundEnabled(), msX, msY);
	else
		PutString(toggleX, lineY, DRAW_DIALOGBOX_SYSMENU_DISABLED, GameColors::UIDisabled.ToColorRef());

	int effectsVol = AudioManager::Get().GetSoundVolume();
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sliderX, lineY + 5, 80);
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sliderX + effectsVol, lineY, 8);

	if (s_bDraggingEffectsSlider && cLB != 0)
	{
		int volume = msX - sliderX;
		if (volume > 100) volume = 100;
		if (volume < 0) volume = 0;
		AudioManager::Get().SetSoundVolume(volume);
	}

	// --- Ambient: [On/Off] ---slider--- ---
	lineY = contentY + 92;

	PutString(labelX, lineY, "Ambient:", GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, "Ambient:", GameColors::UILabel.ToColorRef());

	if (bAvailable)
		DrawToggle(toggleX, lineY, AudioManager::Get().IsAmbientEnabled(), msX, msY);
	else
		PutString(toggleX, lineY, DRAW_DIALOGBOX_SYSMENU_DISABLED, GameColors::UIDisabled.ToColorRef());

	int ambientVol = AudioManager::Get().GetAmbientVolume();
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sliderX, lineY + 5, 80);
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sliderX + ambientVol, lineY, 8);

	if (s_bDraggingAmbientSlider && cLB != 0)
	{
		int volume = msX - sliderX;
		if (volume > 100) volume = 100;
		if (volume < 0) volume = 0;
		AudioManager::Get().SetAmbientVolume(volume);
	}

	// --- UI: [On/Off] ---slider--- ---
	lineY = contentY + 132;

	PutString(labelX, lineY, "UI:", GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, "UI:", GameColors::UILabel.ToColorRef());

	if (bAvailable)
		DrawToggle(toggleX, lineY, AudioManager::Get().IsUIEnabled(), msX, msY);
	else
		PutString(toggleX, lineY, DRAW_DIALOGBOX_SYSMENU_DISABLED, GameColors::UIDisabled.ToColorRef());

	int uiVol = AudioManager::Get().GetUIVolume();
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sliderX, lineY + 5, 80);
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sliderX + uiVol, lineY, 8);

	if (s_bDraggingUISlider && cLB != 0)
	{
		int volume = msX - sliderX;
		if (volume > 100) volume = 100;
		if (volume < 0) volume = 0;
		AudioManager::Get().SetUIVolume(volume);
	}

	// --- Music: [On/Off] ---slider--- ---
	lineY = contentY + 172;

	PutString(labelX, lineY, DRAW_DIALOGBOX_SYSMENU_MUSIC, GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, DRAW_DIALOGBOX_SYSMENU_MUSIC, GameColors::UILabel.ToColorRef());

	if (bAvailable)
		DrawToggle(toggleX, lineY, AudioManager::Get().IsMusicEnabled(), msX, msY);
	else
		PutString(toggleX, lineY, DRAW_DIALOGBOX_SYSMENU_DISABLED, GameColors::UIDisabled.ToColorRef());

	int musicVol = AudioManager::Get().GetMusicVolume();
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sliderX, lineY + 5, 80);
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sliderX + musicVol, lineY, 8);

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

	lineY += 20;

	// Capture Mouse toggle
	PutString(labelX, lineY, "Capture Mouse:", GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, "Capture Mouse:", GameColors::UILabel.ToColorRef());
	DrawToggle(valueX, lineY, ConfigManager::Get().IsMouseCaptureEnabled(), msX, msY);

	lineY += 20;

	// Patching Mode selector (3 options: Original / New / Shadow)
	PutString(labelX, lineY, "Patching:", GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, "Patching:", GameColors::UILabel.ToColorRef());

	const int patchMode = ConfigManager::Get().GetPatchingMode();
	const int patchBoxY = lineY - 2;
	const int patchBoxX = valueX - 40;  // Wider box needs to start earlier
	const int patchBoxWidth = 110;
	const int patchBoxHeight = 16;
	const int regionWidth = patchBoxWidth / 3;

	// Draw wide background box
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, patchBoxX, patchBoxY, 78);

	// Calculate text positions
	const char* labels[] = { "Original", "New", "Shadow" };
	for (int i = 0; i < 3; i++) {
		TextLib::TextMetrics metrics = TextLib::GetTextRenderer()->MeasureText(labels[i]);
		int textX = patchBoxX + (regionWidth * i) + (regionWidth - metrics.width) / 2;
		int textY = patchBoxY + (patchBoxHeight - metrics.height) / 2;
		bool bHover = (msX >= patchBoxX + regionWidth * i && msX < patchBoxX + regionWidth * (i + 1) &&
		               msY >= patchBoxY && msY <= patchBoxY + patchBoxHeight);
		COLORREF color = (patchMode == i || bHover) ? GameColors::UIWhite.ToColorRef() : GameColors::UIDisabled.ToColorRef();
		PutString(textX, textY, labels[i], color);
	}

	lineY += 20;

	// Quick Actions toggle (pickup during movement, 95% unlock, responsive stops)
	PutString(labelX, lineY, "Quick Actions:", GameColors::UILabel.ToColorRef());
	PutString(labelX + 1, lineY, "Quick Actions:", GameColors::UILabel.ToColorRef());
	DrawToggle(valueX, lineY, ConfigManager::Get().IsQuickActionsEnabled(), msX, msY);
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
	const int sliderX = contentX + 110;
	const int sliderWidth = 100;

	// Helper lambda for slider hit detection
	auto checkSlider = [&](int sliderY, bool& dragFlag) -> bool {
		if ((msX >= sliderX) && (msX <= sliderX + sliderWidth + 10) &&
			(msY >= sliderY - 5) && (msY <= sliderY + 15))
		{
			dragFlag = true;
			Info().bIsScrollSelected = true;
			return true;
		}
		return false;
	};

	// Master slider at contentY + 8
	if (checkSlider(contentY + 8, s_bDraggingMasterSlider))
		return PressResult::ScrollClaimed;

	// Effects slider at contentY + 52
	if (checkSlider(contentY + 52, s_bDraggingEffectsSlider))
		return PressResult::ScrollClaimed;

	// Ambient slider at contentY + 92
	if (checkSlider(contentY + 92, s_bDraggingAmbientSlider))
		return PressResult::ScrollClaimed;

	// UI slider at contentY + 132
	if (checkSlider(contentY + 132, s_bDraggingUISlider))
		return PressResult::ScrollClaimed;

	// Music slider at contentY + 172
	if (checkSlider(contentY + 172, s_bDraggingMusicSlider))
		return PressResult::ScrollClaimed;

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

	// Tile Grid toggle
	if (IsInToggleArea(smallBoxX, lineY, msX, msY)) {
		bool enabled = ConfigManager::Get().IsTileGridEnabled();
		ConfigManager::Get().SetTileGridEnabled(!enabled);
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	lineY += 18;

	// Patching Grid toggle
	if (IsInToggleArea(smallBoxX, lineY, msX, msY)) {
		bool enabled = ConfigManager::Get().IsPatchingGridEnabled();
		ConfigManager::Get().SetPatchingGridEnabled(!enabled);
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

	// Window Style toggle click (wide box, only in windowed mode)
	const int styleBoxY = lineY - 2;
	{
		bool isFullscreen = m_pGame->m_Renderer->IsFullscreen();
		if (!isFullscreen && msX >= wideBoxX && msX <= wideBoxX + wideBoxWidth && msY >= styleBoxY && msY <= styleBoxY + wideBoxHeight) {
			bool borderless = ConfigManager::Get().IsBorderlessEnabled();
			ConfigManager::Get().SetBorderlessEnabled(!borderless);
			Window::SetBorderless(!borderless);
			Input::Get()->SetWindowActive(true);
			ConfigManager::Get().Save();
			PlaySoundEffect('E', 14, 5);
			return true;
		}
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
	const int toggleX = contentX + 68;

	if (!AudioManager::Get().IsSoundAvailable())
		return false;

	// Master toggle (lineY = contentY + 8)
	int lineY = contentY + 8;
	if (IsInToggleArea(toggleX, lineY, msX, msY)) {
		bool enabled = AudioManager::Get().IsMasterEnabled();
		AudioManager::Get().SetMasterEnabled(!enabled);
		ConfigManager::Get().SetMasterEnabled(!enabled);
		ConfigManager::Get().Save();
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	// Effects toggle (lineY = contentY + 52)
	lineY = contentY + 52;
	if (IsInToggleArea(toggleX, lineY, msX, msY)) {
		bool enabled = AudioManager::Get().IsSoundEnabled();
		AudioManager::Get().SetSoundEnabled(!enabled);
		ConfigManager::Get().SetSoundEnabled(!enabled);
		ConfigManager::Get().Save();
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	// Ambient toggle (lineY = contentY + 92)
	lineY = contentY + 92;
	if (IsInToggleArea(toggleX, lineY, msX, msY)) {
		bool enabled = AudioManager::Get().IsAmbientEnabled();
		AudioManager::Get().SetAmbientEnabled(!enabled);
		ConfigManager::Get().SetAmbientEnabled(!enabled);
		ConfigManager::Get().Save();
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	// UI toggle (lineY = contentY + 132)
	lineY = contentY + 132;
	if (IsInToggleArea(toggleX, lineY, msX, msY)) {
		bool enabled = AudioManager::Get().IsUIEnabled();
		AudioManager::Get().SetUIEnabled(!enabled);
		ConfigManager::Get().SetUIEnabled(!enabled);
		ConfigManager::Get().Save();
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	// Music toggle (lineY = contentY + 172)
	lineY = contentY + 172;
	if (IsInToggleArea(toggleX, lineY, msX, msY)) {
		if (AudioManager::Get().IsMusicEnabled()) {
			AudioManager::Get().SetMusicEnabled(false);
			ConfigManager::Get().SetMusicEnabled(false);
			AudioManager::Get().StopMusic();
		}
		else {
			AudioManager::Get().SetMusicEnabled(true);
			ConfigManager::Get().SetMusicEnabled(true);
			m_pGame->StartBGM();
		}
		ConfigManager::Get().Save();
		PlaySoundEffect('E', 14, 5);
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

	lineY += 20;

	// Capture Mouse toggle
	if (IsInToggleArea(valueX, lineY, msX, msY)) {
		bool enabled = ConfigManager::Get().IsMouseCaptureEnabled();
		ConfigManager::Get().SetMouseCaptureEnabled(!enabled);
		Input::Get()->SetWindowActive(true);
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	lineY += 20;

	// Patching Mode selector (3 regions)
	{
		const int patchBoxY = lineY - 2;
		const int patchBoxX = valueX - 40;
		const int patchBoxWidth = 110;
		const int patchBoxHeight = 16;
		const int regionWidth = patchBoxWidth / 3;

		if (msY >= patchBoxY && msY <= patchBoxY + patchBoxHeight && msX >= patchBoxX && msX <= patchBoxX + patchBoxWidth) {
			int clickedRegion = (msX - patchBoxX) / regionWidth;
			if (clickedRegion > 2) clickedRegion = 2;
			ConfigManager::Get().SetPatchingMode(clickedRegion);
			char msg[64];
			const char* messages[] = {
				"Original patching (diagonal priority).",
				"New patching (asymmetric zones).",
				"Shadow patching (symmetric 2:1)."
			};
			strcpy(msg, messages[clickedRegion]);
			AddEventList(msg, 10);
			PlaySoundEffect('E', 14, 5);
			return true;
		}
	}

	lineY += 20;

	// Quick Actions toggle
	if (IsInToggleArea(valueX, lineY, msX, msY)) {
		bool enabled = ConfigManager::Get().IsQuickActionsEnabled();
		ConfigManager::Get().SetQuickActionsEnabled(!enabled);
		if (!enabled)
			AddEventList("Quick Actions enabled.", 10);
		else
			AddEventList("Quick Actions disabled.", 10);
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	return false;
}
