// Screen_Test.cpp: Text Rendering Test Screen Implementation
//
// Side-by-side comparison of legacy PutString* methods (left)
// and new TextLib::DrawText methods (right) for visual validation.
//
//////////////////////////////////////////////////////////////////////

#include "Screen_Test.h"
#include "Game.h"
#include "GameModeManager.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include "BitmapFontFactory.h"
#include "SpriteLoader.h"
#include "IInput.h"
#include "RendererFactory.h"

Screen_Test::Screen_Test(CGame* pGame)
	: IGameScreen(pGame)
{
}

void Screen_Test::on_initialize()
{
	GameModeManager::SetCurrentMode(GameMode::Test);
	m_scrollOffset = 0;

	// Preload bitmap fonts for testing (normally done in Screen_Loading)
	LoadBitmapFonts();
}

void Screen_Test::LoadBitmapFonts()
{
	// Skip if already loaded
	if (TextLib::IsBitmapFontLoaded(GameFont::Bitmap1))
		return;

	// Load sprfonts sprites
	SpriteLib::SpriteLoader::open_pak("sprfonts", [&](SpriteLib::SpriteLoader& loader) {
		m_pGame->m_pSprite[DEF_SPRID_INTERFACE_FONT1] = loader.get_sprite(0, false);
		m_pGame->m_pSprite[DEF_SPRID_INTERFACE_FONT2] = loader.get_sprite(1, false);
	});

	// Load interface2 sprites (for number font and SPRFONTS2)
	// ADDINTERFACE is sprite 0, SPRFONTS2 is sprite 1
	SpriteLib::SpriteLoader::open_pak("interface2", [&](SpriteLib::SpriteLoader& loader) {
		m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ADDINTERFACE] = loader.get_sprite(0, false);
		m_pGame->m_pSprite[DEF_SPRID_INTERFACE_SPRFONTS2] = loader.get_sprite(1, false);
	});

	// Create bitmap fonts from the loaded sprites
	// Font 1: Characters '!' (33) to 'z' (122)
	if (m_pGame->m_pSprite[DEF_SPRID_INTERFACE_FONT1])
	{
		TextLib::LoadBitmapFont(GameFont::Bitmap1,
			m_pGame->m_pSprite[DEF_SPRID_INTERFACE_FONT1].get(), '!', 'z', 0,
			GameFont::GetFontSpacing(GameFont::Bitmap1));
	}

	// Font 2: Characters ' ' (32) to '~' (126) - uses dynamic spacing
	if (m_pGame->m_pSprite[DEF_SPRID_INTERFACE_FONT2])
	{
		TextLib::LoadBitmapFontDynamic(GameFont::Bitmap2,
			m_pGame->m_pSprite[DEF_SPRID_INTERFACE_FONT2].get(), ' ', '~', 0);
	}

	// Number font: Digits '0' to '9', frame offset 6 in ADDINTERFACE sprite
	if (m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ADDINTERFACE])
	{
		TextLib::LoadBitmapFont(GameFont::Numbers,
			m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ADDINTERFACE].get(), '0', '9', 6,
			GameFont::GetFontSpacing(GameFont::Numbers));
	}

	// SPRFONTS2: Characters ' ' (32) to '~' (126), with 3 different sizes
	if (m_pGame->m_pSprite[DEF_SPRID_INTERFACE_SPRFONTS2])
	{
		for (int idx = 0; idx < 3; idx++)
		{
			int fontId = GameFont::SprFont3_0 + idx;
			TextLib::LoadBitmapFontDynamic(fontId,
				m_pGame->m_pSprite[DEF_SPRID_INTERFACE_SPRFONTS2].get(), ' ', '~', 95 * idx);
		}
	}
}

void Screen_Test::on_uninitialize()
{
}

void Screen_Test::on_update()
{
	// ESC to quit
	if (Input::Get() && Input::Get()->IsKeyPressed(VK_ESCAPE))
	{
		Window::Close();
		return;
	}

	// Scroll with arrow keys
	if (Input::Get())
	{
		if (Input::Get()->IsKeyPressed(VK_UP))
			m_scrollOffset = (m_scrollOffset > 0) ? m_scrollOffset - 1 : 0;
		if (Input::Get()->IsKeyPressed(VK_DOWN))
			m_scrollOffset++;
	}
}

void Screen_Test::on_render()
{
	m_pGame->m_Renderer->BeginFrame();

	// Fill with dark blue background for better shadow visibility
	int pitch = 0;
	uint16_t* pBackBuffer = m_pGame->m_Renderer->LockBackBuffer(&pitch);
	if (pBackBuffer)
	{
		// RGB(32, 32, 48) in RGB565: R=4, G=8, B=6 (scaled to 5-6-5 bits)
		uint16_t darkBlue = (4 << 11) | (8 << 5) | 6;
		for (int y = 0; y < 480; y++)
		{
			for (int x = 0; x < 640; x++)
			{
				pBackBuffer[x + y * pitch] = darkBlue;
			}
		}
		m_pGame->m_Renderer->UnlockBackBuffer();
	}

	RenderHeader();

	// Start text batch for efficiency
	TextLib::BeginBatch();

	int row = 0;

	// ============== TTF Font Tests ==============
	RenderTestRow(row++, "TTF: No Shadow",
	              &Screen_Test::Legacy_PutString_NoShadow,
	              &Screen_Test::New_DrawText_NoShadow);

	RenderTestRow(row++, "TTF: 3-Point Shadow (PutString cBG=1)",
	              &Screen_Test::Legacy_PutString_WithShadow,
	              &Screen_Test::New_DrawText_ThreePoint);

	RenderTestRow(row++, "TTF: 3-Point Shadow (PutString2)",
	              &Screen_Test::Legacy_PutString2,
	              &Screen_Test::New_DrawText_ThreePoint2);

	RenderTestRow(row++, "TTF: Centered (PutAlignedString)",
	              &Screen_Test::Legacy_PutAlignedString,
	              &Screen_Test::New_DrawTextCentered);

	// ============== Bitmap Font Tests ==============
	// Only run these if bitmap fonts are loaded
	if (TextLib::IsBitmapFontLoaded(GameFont::Bitmap1))
	{
		RenderTestRow(row++, "Bitmap1: Highlight (SprFont)",
		              &Screen_Test::Legacy_PutString_SprFont,
		              &Screen_Test::New_DrawText_Bitmap1_Highlight);

		RenderTestRow(row++, "Bitmap1: Raw Sprite Shadow (SprFont2)",
		              &Screen_Test::Legacy_PutString_SprFont2,
		              &Screen_Test::New_DrawText_Bitmap1_Integrated);
	}
	else
	{
		// Show placeholder if fonts not loaded
		int y = HEADER_HEIGHT + (row++ - m_scrollOffset) * ROW_HEIGHT + MENUY();
		TextLib::DrawText(GameFont::Default, LEFT_COLUMN_X + MENUX(), y, "Bitmap fonts not loaded - run through Loading screen first", TextLib::TextStyle::Color(255, 100, 100));
	}

	if (TextLib::IsBitmapFontLoaded(GameFont::Bitmap2))
	{
		RenderTestRow(row++, "Bitmap2: Drop Shadow (SprFont3)",
		              &Screen_Test::Legacy_PutString_SprFont3,
		              &Screen_Test::New_DrawText_Bitmap2_DropShadow);

		RenderTestRow(row++, "Bitmap2: Transparent (SprFont3 trans)",
		              &Screen_Test::Legacy_PutString_SprFont3_Trans,
		              &Screen_Test::New_DrawText_Bitmap2_Transparent);
	}

	if (TextLib::IsBitmapFontLoaded(GameFont::Numbers))
	{
		RenderTestRow(row++, "Numbers: SprNum",
		              &Screen_Test::Legacy_PutString_SprNum,
		              &Screen_Test::New_DrawText_Numbers);
	}

	// ============== Alignment Showcase ==============
	// Show only when scrolled down enough
	if (m_scrollOffset >= 5)
	{
		RenderAlignmentShowcase();
	}
	else
	{
		int displayRow = row - m_scrollOffset;
		if (displayRow >= 0 && displayRow <= 10)
		{
			int y = HEADER_HEIGHT + displayRow * ROW_HEIGHT + MENUY();
			TextLib::DrawText(GameFont::Default, LEFT_COLUMN_X + MENUX(), y, "Scroll down for Alignment Showcase...", TextLib::TextStyle::Color(100, 200, 255));
		}
	}

	TextLib::EndBatch();
}

void Screen_Test::RenderHeader()
{
	// Title
	TextLib::DrawText(GameFont::Default, MENUX() + 220, MENUY() + 5, "TEXTLIB TEST SCREEN", TextLib::TextStyle::Color(GameColors::UITopMsgYellow.r, GameColors::UITopMsgYellow.g, GameColors::UITopMsgYellow.b));

	// Instructions
	TextLib::DrawText(GameFont::Default, MENUX() + 10, MENUY() + 25, "ESC=Quit | UP/DOWN=Scroll", TextLib::TextStyle::Color(GameColors::InfoGrayLight.r, GameColors::InfoGrayLight.g, GameColors::InfoGrayLight.b));

	// Column headers
	TextLib::DrawText(GameFont::Default, LEFT_COLUMN_X + MENUX(), MENUY() + 45, "LEGACY (PutString*)", TextLib::TextStyle::Color(255, 150, 150));
	TextLib::DrawText(GameFont::Default, RIGHT_COLUMN_X + MENUX(), MENUY() + 45, "NEW (TextLib::DrawText)", TextLib::TextStyle::Color(150, 255, 150));

	// Divider line (visual separator)
	int midX = MENUX() + 320;
	for (int y = HEADER_HEIGHT; y < 480; y += 2)
	{
		m_pGame->m_Renderer->PutPixel(midX, MENUY() + y, 80, 80, 80);
	}
}

void Screen_Test::RenderTestRow(int row, const char* testName,
                                void (Screen_Test::*legacyFunc)(int x, int y),
                                void (Screen_Test::*newFunc)(int x, int y))
{
	int displayRow = row - m_scrollOffset;
	if (displayRow < 0 || displayRow > 10)
		return;  // Off screen

	int baseY = HEADER_HEIGHT + displayRow * ROW_HEIGHT + MENUY();

	// Test name label (small, gray)
	TextLib::DrawText(GameFont::Default, LEFT_COLUMN_X + MENUX(), baseY, testName, TextLib::TextStyle::Color(GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b));

	// Render legacy version on left
	(this->*legacyFunc)(LEFT_COLUMN_X + MENUX(), baseY + 16);

	// Render new version on right
	(this->*newFunc)(RIGHT_COLUMN_X + MENUX(), baseY + 16);
}

// ============== Legacy Test Functions ==============

void Screen_Test::Legacy_PutString_NoShadow(int x, int y)
{
	TextLib::DrawText(GameFont::Default, x, y, "Hello World", TextLib::TextStyle::Color(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b));
}

void Screen_Test::Legacy_PutString_WithShadow(int x, int y)
{
	// PutString with cBGtype=1 for 3-point shadow
	TextLib::DrawText(GameFont::Default, x, y, "Hello World", TextLib::TextStyle::WithShadow(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b));
}

void Screen_Test::Legacy_PutString2(int x, int y)
{
	TextLib::DrawText(GameFont::Default, x, y, "Hello World", TextLib::TextStyle::WithShadow(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b));
}

void Screen_Test::Legacy_PutAlignedString(int x, int y)
{
	// Centered between x and x+COLUMN_WIDTH
	TextLib::DrawTextAligned(GameFont::Default, x, y, x + COLUMN_WIDTH - x, 15, "Centered Text", TextLib::TextStyle::Color(255, 200, 100), TextLib::Align::TopCenter);
}

void Screen_Test::Legacy_PutString_SprFont(int x, int y)
{
	if (TextLib::IsBitmapFontLoaded(GameFont::Bitmap1))
	{
		TextLib::DrawText(GameFont::Bitmap1, x, y, "Hello World", TextLib::TextStyle::WithHighlight(GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b));
	}
}

void Screen_Test::Legacy_PutString_SprFont2(int x, int y)
{
	if (TextLib::IsBitmapFontLoaded(GameFont::Bitmap1))
	{
		TextLib::DrawText(GameFont::Bitmap1, x, y, "Hello World", TextLib::TextStyle::WithIntegratedShadow(255, 200, 100));
	}
}

void Screen_Test::Legacy_PutString_SprFont3(int x, int y)
{
	if (TextLib::IsBitmapFontLoaded(GameFont::Bitmap2))
	{
		TextLib::DrawText(GameFont::Bitmap2, x, y, "Hello World", TextLib::TextStyle::WithTwoPointShadow(100, 200, 255));
	}
}

void Screen_Test::Legacy_PutString_SprFont3_Trans(int x, int y)
{
	if (TextLib::IsBitmapFontLoaded(GameFont::Bitmap2))
	{
		TextLib::DrawText(GameFont::Bitmap2, x, y, "Transparent", TextLib::TextStyle::Color(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b).WithAlpha(0.7f));
	}
}

void Screen_Test::Legacy_PutString_SprNum(int x, int y)
{
	if (TextLib::IsBitmapFontLoaded(GameFont::Numbers))
	{
		// Shadow first
		TextLib::DrawText(GameFont::Numbers, x + 1, y + 1, "12345", TextLib::TextStyle::Color(GameColors::UIBlack.r, GameColors::UIBlack.g, GameColors::UIBlack.b));
		// Main text
		TextLib::DrawText(GameFont::Numbers, x, y, "12345", TextLib::TextStyle::Color(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b));
	}
}

// ============== New TextLib Functions ==============

void Screen_Test::New_DrawText_NoShadow(int x, int y)
{
	TextLib::DrawText(GameFont::Default, x, y, "Hello World",
	                  TextLib::TextStyle::Color(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b));
}

void Screen_Test::New_DrawText_ThreePoint(int x, int y)
{
	TextLib::DrawText(GameFont::Default, x, y, "Hello World",
	                  TextLib::TextStyle::WithShadow(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b));
}

void Screen_Test::New_DrawText_ThreePoint2(int x, int y)
{
	TextLib::DrawText(GameFont::Default, x, y, "Hello World",
	                  TextLib::TextStyle::WithShadow(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b));
}

void Screen_Test::New_DrawTextCentered(int x, int y)
{
	TextLib::DrawTextAligned(GameFont::Default, x, y, COLUMN_WIDTH, 20, "Centered Text",
	                         TextLib::TextStyle::Color(255, 200, 100), TextLib::Align::HCenter);
}

void Screen_Test::New_DrawText_Bitmap1_Highlight(int x, int y)
{
	TextLib::DrawText(GameFont::Bitmap1, x, y, "Hello World",
	                  TextLib::TextStyle::WithHighlight(GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b));
}

void Screen_Test::New_DrawText_Bitmap1_Integrated(int x, int y)
{
	TextLib::DrawText(GameFont::Bitmap1, x, y, "Hello World",
	                  TextLib::TextStyle::WithIntegratedShadow(255, 200, 100));
}

void Screen_Test::New_DrawText_Bitmap2_DropShadow(int x, int y)
{
	TextLib::DrawText(GameFont::Bitmap2, x, y, "Hello World",
	                  TextLib::TextStyle::WithDropShadow(100, 200, 255));
}

void Screen_Test::New_DrawText_Bitmap2_Transparent(int x, int y)
{
	TextLib::DrawText(GameFont::Bitmap2, x, y, "Transparent",
	                  TextLib::TextStyle::Transparent(255, 255, 255, 0.7f));
}

void Screen_Test::New_DrawText_Numbers(int x, int y)
{
	// With drop shadow like the legacy version
	TextLib::DrawText(GameFont::Numbers, x, y, "12345",
	                  TextLib::TextStyle::WithDropShadow(255, 255, 255));
}

// ============== Alignment Showcase ==============

void Screen_Test::DrawRectOutline(int x, int y, int width, int height, uint8_t r, uint8_t g, uint8_t b)
{
	// Draw rectangle outline using pixels
	for (int i = 0; i < width; i++)
	{
		m_pGame->m_Renderer->PutPixel(x + i, y, r, g, b);                  // Top
		m_pGame->m_Renderer->PutPixel(x + i, y + height - 1, r, g, b);     // Bottom
	}
	for (int i = 0; i < height; i++)
	{
		m_pGame->m_Renderer->PutPixel(x, y + i, r, g, b);                  // Left
		m_pGame->m_Renderer->PutPixel(x + width - 1, y + i, r, g, b);      // Right
	}
}

void Screen_Test::RenderAlignmentShowcase()
{
	// Title
	TextLib::DrawText(GameFont::Default, MENUX() + 180, MENUY() + 70, "DrawTextAligned with GameRectangle", TextLib::TextStyle::Color(GameColors::UITopMsgYellow.r, GameColors::UITopMsgYellow.g, GameColors::UITopMsgYellow.b));

	// Grid layout: 3 columns (Left, Center, Right) x 3 rows (Top, Middle, Bottom)
	constexpr int CELL_WIDTH = 180;
	constexpr int CELL_HEIGHT = 50;
	const int GRID_X = MENUX() + 50;
	const int GRID_Y = MENUY() + 100;
	constexpr int CELL_SPACING = 10;

	// Column labels
	const char* hLabels[] = { "Left", "Center", "Right" };
	const char* vLabels[] = { "Top", "Middle", "Bottom" };

	// Draw column headers
	for (int col = 0; col < 3; col++)
	{
		int x = GRID_X + col * (CELL_WIDTH + CELL_SPACING) + CELL_WIDTH / 2 - 20;
		TextLib::DrawText(GameFont::Default, x, GRID_Y - 20, hLabels[col], TextLib::TextStyle::Color(GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b));
	}

	// Draw row labels
	for (int row = 0; row < 3; row++)
	{
		int y = GRID_Y + row * (CELL_HEIGHT + CELL_SPACING) + CELL_HEIGHT / 2 - 8;
		TextLib::DrawText(GameFont::Default, GRID_X - 60, y, vLabels[row], TextLib::TextStyle::Color(GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b));
	}

	// Alignment combinations (use bitwise OR to combine)
	TextLib::Align alignments[3][3] = {
		{ TextLib::Align::TopLeft,    TextLib::Align::TopCenter,    TextLib::Align::TopRight },
		{ TextLib::Align::MiddleLeft, TextLib::Align::Center,       TextLib::Align::MiddleRight },
		{ TextLib::Align::BottomLeft, TextLib::Align::BottomCenter, TextLib::Align::BottomRight }
	};

	// Draw 3x3 grid of alignment examples
	for (int row = 0; row < 3; row++)
	{
		for (int col = 0; col < 3; col++)
		{
			int cellX = GRID_X + col * (CELL_WIDTH + CELL_SPACING);
			int cellY = GRID_Y + row * (CELL_HEIGHT + CELL_SPACING);

			// Draw rectangle outline to show bounds
			DrawRectOutline(cellX, cellY, CELL_WIDTH, CELL_HEIGHT, 100, 100, 100);

			// Create GameRectangle and draw aligned text
			GameRectangle rect(cellX, cellY, CELL_WIDTH, CELL_HEIGHT);
			TextLib::DrawTextAligned(GameFont::Default, rect, "Text",
			                         TextLib::TextStyle::Color(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b),
			                         alignments[row][col]);
		}
	}

	// Show code example
	int exampleY = GRID_Y + 3 * (CELL_HEIGHT + CELL_SPACING) + 20;
	TextLib::DrawText(GameFont::Default, GRID_X, exampleY, "Usage: GameRectangle rect(x, y, width, height);", TextLib::TextStyle::Color(150, 200, 150));
	TextLib::DrawText(GameFont::Default, GRID_X, exampleY + 16, "       TextLib::DrawTextAligned(fontId, rect, text, style, hAlign, vAlign);", TextLib::TextStyle::Color(150, 200, 150));
}
