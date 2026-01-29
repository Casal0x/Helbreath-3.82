// Screen_Test.h: Text Rendering Test Screen
//
// Side-by-side comparison of legacy PutString* methods (left)
// and new TextLib::DrawText methods (right) for visual validation.
//
// Press ESC to exit the game.
// Press UP/DOWN to scroll through test cases.
// Press 1-9 to jump to specific test sections.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IGameScreen.h"

class Screen_Test : public IGameScreen
{
public:
	SCREEN_TYPE(Screen_Test)

	explicit Screen_Test(CGame* pGame);
	~Screen_Test() override = default;

	void on_initialize() override;
	void on_uninitialize() override;
	void on_update() override;
	void on_render() override;

private:
	// Load bitmap fonts for testing (normally done in Screen_Loading)
	void LoadBitmapFonts();

	// Render the header with instructions
	void RenderHeader();

	// Render a single test row with legacy on left, new on right
	void RenderTestRow(int row, const char* testName,
	                   void (Screen_Test::*legacyFunc)(int x, int y),
	                   void (Screen_Test::*newFunc)(int x, int y));

	// ============== Legacy Test Functions (Left Side) ==============
	void Legacy_PutString_NoShadow(int x, int y);
	void Legacy_PutString_WithShadow(int x, int y);
	void Legacy_PutString2(int x, int y);
	void Legacy_PutAlignedString(int x, int y);
	void Legacy_PutString_SprFont(int x, int y);
	void Legacy_PutString_SprFont2(int x, int y);
	void Legacy_PutString_SprFont3(int x, int y);
	void Legacy_PutString_SprFont3_Trans(int x, int y);
	void Legacy_PutString_SprNum(int x, int y);

	// ============== New TextLib Functions (Right Side) ==============
	void New_DrawText_NoShadow(int x, int y);
	void New_DrawText_ThreePoint(int x, int y);
	void New_DrawText_ThreePoint2(int x, int y);
	void New_DrawTextCentered(int x, int y);
	void New_DrawText_Bitmap1_Highlight(int x, int y);
	void New_DrawText_Bitmap1_Integrated(int x, int y);
	void New_DrawText_Bitmap2_DropShadow(int x, int y);
	void New_DrawText_Bitmap2_Transparent(int x, int y);
	void New_DrawText_Numbers(int x, int y);

	// ============== Alignment Showcase ==============
	void RenderAlignmentShowcase();
	void DrawRectOutline(int x, int y, int width, int height, uint8_t r, uint8_t g, uint8_t b);

	int m_scrollOffset = 0;
	static constexpr int ROW_HEIGHT = 40;
	static constexpr int HEADER_HEIGHT = 60;
	static constexpr int LEFT_COLUMN_X = 20;
	static constexpr int RIGHT_COLUMN_X = 340;
	static constexpr int COLUMN_WIDTH = 300;
};
