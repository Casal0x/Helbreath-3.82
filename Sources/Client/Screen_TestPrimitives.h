// Screen_TestPrimitives.h: Visual Test Scene for Primitive Drawing
//
// Exercises every IRenderer primitive method:
// DrawPixel, DrawLine, DrawRectFilled, DrawRectOutline,
// DrawRoundedRectFilled, DrawRoundedRectOutline
//
// Press ESC to exit the game.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IGameScreen.h"

class Screen_TestPrimitives : public IGameScreen
{
public:
	SCREEN_TYPE(Screen_TestPrimitives)

	explicit Screen_TestPrimitives(CGame* pGame);
	~Screen_TestPrimitives() override = default;

	void on_initialize() override;
	void on_uninitialize() override;
	void on_update() override;
	void on_render() override;

private:
	void RenderHeader();
	void RenderPixelTests(int y);
	void RenderLineAlphaTests(int y);
	void RenderLineAdditiveTests(int y);
	void RenderRectFilledTests(int y);
	void RenderRectOutlineTests(int y);
	void RenderRoundedRectFilledTests(int y);
	void RenderRoundedRectOutlineTests(int y);
};
