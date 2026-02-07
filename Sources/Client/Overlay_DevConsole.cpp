// Overlay_DevConsole.cpp: Render shell for the in-game developer console
//
// Thin overlay that draws the DevConsole UI and routes input.
//
//////////////////////////////////////////////////////////////////////

#define _WINSOCKAPI_

#include "Overlay_DevConsole.h"
#include "DevConsole.h"
#include "Game.h"
#include "GameModeManager.h"
#include "IInput.h"
#include "CommonTypes.h"
#include "GlobalDef.h"

// Console layout constants (top half of screen)
static constexpr int CONSOLE_LEFT = 0;
static constexpr int CONSOLE_TOP = 0;
static constexpr int CONSOLE_BOTTOM = 240;
static constexpr float CONSOLE_ALPHA = 0.75f;

static constexpr int TEXT_MARGIN = 4;
static constexpr int LINE_HEIGHT = 13;
static constexpr int VISIBLE_LINES = 16;

static constexpr int TEXT_AREA_TOP = 2;
static constexpr int TEXT_AREA_BOTTOM = 209;
static constexpr int SEPARATOR_Y = 222;
static constexpr int INPUT_Y = 224;

// Colors
static constexpr Color COLOR_INPUT{0, 255, 0};      // Green

Overlay_DevConsole::Overlay_DevConsole(CGame* pGame)
	: IGameScreen(pGame)
{
}

void Overlay_DevConsole::on_initialize()
{
}

void Overlay_DevConsole::on_uninitialize()
{
}

void Overlay_DevConsole::on_update()
{
	// Escape closes the console
	if (Input::IsKeyPressed(KeyCode::Escape))
	{
		DevConsole::Get().Hide();
		clear_overlay();
		return;
	}

	// Mouse wheel scrolling within console area
	int wheelDelta = Input::GetMouseWheelDelta();
	if (wheelDelta != 0)
	{
		int mouseY = Input::GetMouseY();
		if (mouseY >= CONSOLE_TOP && mouseY <= CONSOLE_BOTTOM)
		{
			if (wheelDelta > 0)
				DevConsole::Get().ScrollUp(3);
			else
				DevConsole::Get().ScrollDown(3);
		}
	}
}

void Overlay_DevConsole::on_render()
{
	DevConsole& console = DevConsole::Get();
	IRenderer* pRenderer = m_pGame->m_Renderer;

	const int consoleRight = LOGICAL_WIDTH();

	// Draw 75% opaque black background over top half
	pRenderer->DrawRectFilled(CONSOLE_LEFT, CONSOLE_TOP, consoleRight - CONSOLE_LEFT, CONSOLE_BOTTOM - CONSOLE_TOP, Color::Black(static_cast<uint8_t>(CONSOLE_ALPHA * 255)));

	// Draw separator line
	pRenderer->DrawLine(TEXT_MARGIN, SEPARATOR_Y, consoleRight - TEXT_MARGIN, SEPARATOR_Y, Color(16, 32, 16, 204));

	// Begin text rendering
	pRenderer->BeginTextBatch();

	// Draw console lines (bottom-up from ring buffer)
	const DevConsole::ConsoleLine* lines = console.GetLines();
	int lineCount = console.GetLineCount();
	int writeIdx = console.GetWriteIndex();
	int scrollOff = console.GetScrollOffset();

	for (int i = 0; i < VISIBLE_LINES && i < lineCount; i++)
	{
		int lineIdx = i + scrollOff;
		if (lineIdx >= lineCount) break;

		// Ring buffer index: most recent is at (writeIdx - 1), going backwards
		int bufIdx = (writeIdx - 1 - lineIdx + DevConsole::MAX_LINES * 2) % DevConsole::MAX_LINES;

		int y = TEXT_AREA_BOTTOM - i * LINE_HEIGHT;
		if (y < TEXT_AREA_TOP) break;

		pRenderer->DrawText(TEXT_MARGIN, y, lines[bufIdx].text, lines[bufIdx].color);
	}

	// Draw input line with prompt
	char inputLine[272];
	uint32_t dwTime = GameClock::GetTimeMS();
	bool bCursorBlink = ((dwTime / 500) % 2) == 0;

	if (bCursorBlink)
		sprintf_s(inputLine, "> %s_", console.GetInputBuffer());
	else
		sprintf_s(inputLine, "> %s", console.GetInputBuffer());

	pRenderer->DrawText(TEXT_MARGIN, INPUT_Y, inputLine, COLOR_INPUT);

	pRenderer->EndTextBatch();
}
