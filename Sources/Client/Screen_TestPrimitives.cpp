// Screen_TestPrimitives.cpp: Visual Test Scene for Primitive Drawing
//
// Renders labeled rows exercising every IRenderer primitive method.
// Layout adapts to logical resolution (640x480 or 800x600).
//
//////////////////////////////////////////////////////////////////////

#include "Screen_TestPrimitives.h"
#include "Game.h"
#include "GameModeManager.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include "IInput.h"
#include "ResolutionConfig.h"
#include <cmath>

Screen_TestPrimitives::Screen_TestPrimitives(CGame* pGame)
	: IGameScreen(pGame)
{
}

void Screen_TestPrimitives::on_initialize()
{
	GameModeManager::SetCurrentMode(GameMode::TestPrimitives);
}

void Screen_TestPrimitives::on_uninitialize()
{
}

void Screen_TestPrimitives::on_update()
{
	if (Input::Get() && Input::Get()->IsKeyPressed(KeyCode::Escape))
	{
		Window::Close();
		return;
	}
}

void Screen_TestPrimitives::on_render()
{
	m_pGame->m_Renderer->BeginFrame();

	auto& res = ResolutionConfig::Get();
	int W = res.LogicalWidth();
	int H = res.LogicalHeight();

	// Dark background
	m_pGame->m_Renderer->DrawRectFilled(0, 0, W, H, Color(20, 20, 30));

	TextLib::BeginBatch();

	RenderHeader();

	// Distribute 7 rows evenly across screen height below header
	int headerH = 30;
	int rowH = (H - headerH) / 7;

	int rowStarts[7];
	for (int i = 0; i < 7; i++)
		rowStarts[i] = headerH + rowH * i;

	// Draw divider lines between rows
	for (int i = 1; i < 7; i++)
	{
		int divY = rowStarts[i] - 1;
		m_pGame->m_Renderer->DrawLine(10, divY, W - 10, divY, Color(50, 50, 70), BlendMode::Alpha);
	}

	RenderPixelTests(rowStarts[0]);
	RenderLineAlphaTests(rowStarts[1]);
	RenderLineAdditiveTests(rowStarts[2]);
	RenderRectFilledTests(rowStarts[3]);
	RenderRectOutlineTests(rowStarts[4]);
	RenderRoundedRectFilledTests(rowStarts[5]);
	RenderRoundedRectOutlineTests(rowStarts[6]);

	TextLib::EndBatch();
}

void Screen_TestPrimitives::RenderHeader()
{
	auto& res = ResolutionConfig::Get();
	int W = res.LogicalWidth();

	TextLib::DrawText(GameFont::Default, W / 2 - 100, 5, "PRIMITIVE RENDERING TEST",
		TextLib::TextStyle::Color(Color(255, 220, 100)));
	TextLib::DrawText(GameFont::Default, W - 80, 5, "ESC=Quit",
		TextLib::TextStyle::Color(Color(160, 160, 160)));
}

// Row 1: DrawPixel
void Screen_TestPrimitives::RenderPixelTests(int y)
{
	auto& res = ResolutionConfig::Get();
	int W = res.LogicalWidth();
	int rowH = (res.LogicalHeight() - 30) / 7;

	TextLib::DrawText(GameFont::Default, 10, y + 2, "DrawPixel",
		TextLib::TextStyle::Color(Color(180, 180, 255)));

	// Section 1: Cross pattern (left third)
	int s1 = W / 6;
	int cy = y + rowH / 2;
	m_pGame->m_Renderer->DrawPixel(s1, cy - 2, Color::White());
	m_pGame->m_Renderer->DrawPixel(s1 - 2, cy, Color::White());
	m_pGame->m_Renderer->DrawPixel(s1, cy, Color::White());
	m_pGame->m_Renderer->DrawPixel(s1 + 2, cy, Color::White());
	m_pGame->m_Renderer->DrawPixel(s1, cy + 2, Color::White());

	// Section 2: Colored pixels (middle third)
	int s2 = W / 3;
	int spacing = W / 20;
	Color pixColors[] = {
		{255, 0, 0}, {0, 255, 0}, {0, 0, 255},
		{255, 255, 0}, {0, 255, 255}, {255, 0, 255}
	};
	for (int i = 0; i < 6; i++)
	{
		int px = s2 + i * spacing;
		// 3x3 block so it's visible
		for (int dy = -1; dy <= 1; dy++)
			for (int dx = -1; dx <= 1; dx++)
				m_pGame->m_Renderer->DrawPixel(px + dx, cy + dy, pixColors[i]);
	}

	// Section 3: Alpha pixels (right third)
	int s3 = W * 2 / 3;
	int alphaSpacing = W / 10;
	TextLib::DrawText(GameFont::Default, s3, y + 2, "Alpha: 255  192  128  64",
		TextLib::TextStyle::Color(Color(120, 120, 120)));
	for (int i = 0; i < 4; i++)
	{
		int px = s3 + i * alphaSpacing;
		uint8_t a = static_cast<uint8_t>(255 - i * 63);
		for (int dy = -2; dy <= 2; dy++)
			for (int dx = -2; dx <= 2; dx++)
				m_pGame->m_Renderer->DrawPixel(px + dx, cy + dy, Color(255, 255, 255, a));
	}
}

// Row 2: DrawLine — Alpha Blend
void Screen_TestPrimitives::RenderLineAlphaTests(int y)
{
	auto& res = ResolutionConfig::Get();
	int W = res.LogicalWidth();
	int rowH = (res.LogicalHeight() - 30) / 7;
	int contentH = rowH - 16;

	TextLib::DrawText(GameFont::Default, 10, y + 2, "DrawLine (Alpha)",
		TextLib::TextStyle::Color(Color(180, 180, 255)));

	// Section 1: Fan of 8 lines (left third)
	int cx = W / 6, cy = y + rowH / 2 + 4;
	int fanR = contentH / 2;
	Color fanColors[] = {
		{255, 0, 0}, {255, 128, 0}, {255, 255, 0}, {0, 255, 0},
		{0, 255, 255}, {0, 128, 255}, {128, 0, 255}, {255, 0, 255}
	};
	for (int i = 0; i < 8; i++)
	{
		float angle = (i * 3.14159f * 2.0f) / 8.0f;
		int ex = cx + static_cast<int>(fanR * 1.5f * cosf(angle));
		int ey = cy + static_cast<int>(fanR * sinf(angle));
		m_pGame->m_Renderer->DrawLine(cx, cy, ex, ey, fanColors[i], BlendMode::Alpha);
	}

	// Section 2: Lines with varying alpha (middle third)
	int lx = W / 3;
	int lineW = W / 5;
	TextLib::DrawText(GameFont::Default, lx, y + 2, "Alpha: 255  192  128  64",
		TextLib::TextStyle::Color(Color(120, 120, 120)));
	int lineSpacing = contentH / 4;
	m_pGame->m_Renderer->DrawLine(lx, y + 16 + lineSpacing * 0, lx + lineW, y + 16 + lineSpacing * 0, Color(255, 255, 0, 255), BlendMode::Alpha);
	m_pGame->m_Renderer->DrawLine(lx, y + 16 + lineSpacing * 1, lx + lineW, y + 16 + lineSpacing * 1, Color(255, 255, 0, 192), BlendMode::Alpha);
	m_pGame->m_Renderer->DrawLine(lx, y + 16 + lineSpacing * 2, lx + lineW, y + 16 + lineSpacing * 2, Color(255, 255, 0, 128), BlendMode::Alpha);
	m_pGame->m_Renderer->DrawLine(lx, y + 16 + lineSpacing * 3, lx + lineW, y + 16 + lineSpacing * 3, Color(255, 255, 0, 64), BlendMode::Alpha);

	// Section 3: Diagonal and vertical lines (right third)
	int dx = W * 2 / 3;
	TextLib::DrawText(GameFont::Default, dx, y + 2, "Diagonal / Vertical",
		TextLib::TextStyle::Color(Color(120, 120, 120)));
	m_pGame->m_Renderer->DrawLine(dx, y + 16, dx + contentH, y + 16 + contentH, Color(0, 255, 128), BlendMode::Alpha);
	int midX = dx + contentH + 20;
	m_pGame->m_Renderer->DrawLine(midX, y + 16, midX, y + 16 + contentH, Color(255, 128, 0), BlendMode::Alpha);
	m_pGame->m_Renderer->DrawLine(midX + 20, y + 16 + contentH, midX + 20 + contentH, y + 16, Color(128, 128, 255), BlendMode::Alpha);
}

// Row 3: DrawLine — Additive Blend
void Screen_TestPrimitives::RenderLineAdditiveTests(int y)
{
	auto& res = ResolutionConfig::Get();
	int W = res.LogicalWidth();
	int rowH = (res.LogicalHeight() - 30) / 7;
	int contentH = rowH - 16;

	TextLib::DrawText(GameFont::Default, 10, y + 2, "DrawLine (Additive)",
		TextLib::TextStyle::Color(Color(180, 180, 255)));

	// Section 1: Fan with additive blending (left third)
	int cx = W / 6, cy = y + rowH / 2 + 4;
	int fanR = contentH / 2;
	Color fanColors[] = {
		{255, 0, 0}, {255, 128, 0}, {255, 255, 0}, {0, 255, 0},
		{0, 255, 255}, {0, 128, 255}, {128, 0, 255}, {255, 0, 255}
	};
	for (int i = 0; i < 8; i++)
	{
		float angle = (i * 3.14159f * 2.0f) / 8.0f;
		int ex = cx + static_cast<int>(fanR * 1.5f * cosf(angle));
		int ey = cy + static_cast<int>(fanR * sinf(angle));
		m_pGame->m_Renderer->DrawLine(cx, cy, ex, ey, fanColors[i], BlendMode::Additive);
	}

	// Section 2: Overlapping lines (middle third)
	int lx = W / 3;
	int lineW = W / 5;
	TextLib::DrawText(GameFont::Default, lx, y + 2, "Overlapping (accumulation)",
		TextLib::TextStyle::Color(Color(120, 120, 120)));
	for (int i = 0; i < 6; i++)
	{
		m_pGame->m_Renderer->DrawLine(lx, y + 16 + i * (contentH / 6), lx + lineW, y + 16 + contentH - i * (contentH / 12),
			Color(80, 80, 120), BlendMode::Additive);
	}
	for (int i = 0; i < 6; i++)
	{
		m_pGame->m_Renderer->DrawLine(lx + lineW, y + 16 + i * (contentH / 6), lx, y + 16 + contentH - i * (contentH / 12),
			Color(120, 80, 80), BlendMode::Additive);
	}

	// Section 3: RGB additive overlap (right third)
	int rx = W * 2 / 3;
	int rgbW = W / 5;
	TextLib::DrawText(GameFont::Default, rx, y + 2, "RGB overlap",
		TextLib::TextStyle::Color(Color(120, 120, 120)));
	for (int i = 0; i < 5; i++)
	{
		m_pGame->m_Renderer->DrawLine(rx, y + 16 + contentH / 4 + i * 2, rx + rgbW, y + 16 + contentH / 2 + i * 2,
			Color(200, 0, 0), BlendMode::Additive);
		m_pGame->m_Renderer->DrawLine(rx, y + 16 + contentH * 3 / 4 + i * 2, rx + rgbW, y + 16 + contentH / 2 + i * 2,
			Color(0, 200, 0), BlendMode::Additive);
		m_pGame->m_Renderer->DrawLine(rx + rgbW / 3, y + 16 + i * 2, rx + rgbW * 2 / 3, y + 16 + contentH + i * 2,
			Color(0, 0, 200), BlendMode::Additive);
	}
}

// Row 4: DrawRectFilled
void Screen_TestPrimitives::RenderRectFilledTests(int y)
{
	auto& res = ResolutionConfig::Get();
	int W = res.LogicalWidth();
	int rowH = (res.LogicalHeight() - 30) / 7;
	int contentH = rowH - 16;
	int rectH = contentH - 14;

	TextLib::DrawText(GameFont::Default, 10, y + 2, "DrawRectFilled",
		TextLib::TextStyle::Color(Color(180, 180, 255)));

	// Section 1: Alpha blending over colored strip (left third)
	int s1 = W / 20;
	int rectW = (W / 3 - s1 - 20) / 4;
	int stripW = rectW * 4 + 30;
	m_pGame->m_Renderer->DrawRectFilled(s1, y + 16, stripW, rectH, Color(100, 60, 60));
	for (int i = 0; i < 4; i++)
	{
		uint8_t a = static_cast<uint8_t>(255 - i * 63);
		m_pGame->m_Renderer->DrawRectFilled(s1 + i * (rectW + 4), y + 16, rectW, rectH, Color(0, 0, 0, a));
	}
	TextLib::DrawText(GameFont::Default, s1, y + rowH - 14, "a=255  a=192  a=128  a=64",
		TextLib::TextStyle::Color(Color(120, 120, 120)));

	// Section 2: 4 colored filled rects (middle third)
	int s2 = W / 3 + 10;
	int cRectW = (W / 3 - 40) / 4;
	m_pGame->m_Renderer->DrawRectFilled(s2, y + 16, cRectW, rectH, Color(255, 0, 0));
	m_pGame->m_Renderer->DrawRectFilled(s2 + cRectW + 6, y + 16, cRectW, rectH, Color(0, 255, 0));
	m_pGame->m_Renderer->DrawRectFilled(s2 + (cRectW + 6) * 2, y + 16, cRectW, rectH, Color(0, 0, 255));
	m_pGame->m_Renderer->DrawRectFilled(s2 + (cRectW + 6) * 3, y + 16, cRectW, rectH, Color(255, 255, 0));

	// Section 3: Semi-transparent overlap (right third)
	int s3 = W * 2 / 3 + 10;
	int oRectW = W / 8;
	m_pGame->m_Renderer->DrawRectFilled(s3, y + 16, oRectW, rectH, Color(255, 0, 0));
	m_pGame->m_Renderer->DrawRectFilled(s3 + oRectW / 2, y + 16 + rectH / 4, oRectW, rectH, Color(0, 0, 255, 128));
	TextLib::DrawText(GameFont::Default, s3, y + rowH - 14, "alpha overlap",
		TextLib::TextStyle::Color(Color(120, 120, 120)));
}

// Row 5: DrawRectOutline
void Screen_TestPrimitives::RenderRectOutlineTests(int y)
{
	auto& res = ResolutionConfig::Get();
	int W = res.LogicalWidth();
	int rowH = (res.LogicalHeight() - 30) / 7;
	int contentH = rowH - 16;
	int rectH = contentH - 14;

	TextLib::DrawText(GameFont::Default, 10, y + 2, "DrawRectOutline",
		TextLib::TextStyle::Color(Color(180, 180, 255)));

	// Section 1: Three thicknesses (left quarter)
	int spacing = W / 12;
	int rectW = spacing - 10;
	int x0 = W / 20;
	m_pGame->m_Renderer->DrawRectOutline(x0, y + 16, rectW, rectH, Color::White(), 1);
	TextLib::DrawText(GameFont::Default, x0, y + rowH - 14, "t=1",
		TextLib::TextStyle::Color(Color(120, 120, 120)));

	m_pGame->m_Renderer->DrawRectOutline(x0 + spacing, y + 16, rectW, rectH, Color(0, 255, 0), 2);
	TextLib::DrawText(GameFont::Default, x0 + spacing, y + rowH - 14, "t=2",
		TextLib::TextStyle::Color(Color(120, 120, 120)));

	m_pGame->m_Renderer->DrawRectOutline(x0 + spacing * 2, y + 16, rectW, rectH, Color(100, 150, 255), 3);
	TextLib::DrawText(GameFont::Default, x0 + spacing * 2, y + rowH - 14, "t=3",
		TextLib::TextStyle::Color(Color(120, 120, 120)));

	// Section 2: Nested outlines (middle)
	int nx = W / 3 + W / 8;
	int nw = W / 6;
	m_pGame->m_Renderer->DrawRectOutline(nx, y + 16, nw, rectH, Color(255, 100, 100), 1);
	m_pGame->m_Renderer->DrawRectOutline(nx + nw / 6, y + 16 + rectH / 5, nw * 2 / 3, rectH * 3 / 5, Color(100, 255, 100), 1);
	m_pGame->m_Renderer->DrawRectOutline(nx + nw / 3, y + 16 + rectH * 2 / 5, nw / 3, rectH / 5, Color(100, 100, 255), 1);
	TextLib::DrawText(GameFont::Default, nx, y + rowH - 14, "nested",
		TextLib::TextStyle::Color(Color(120, 120, 120)));

	// Section 3: Alpha overlap (right third)
	int ax = W * 2 / 3 + 10;
	int aRectW = W / 7;
	m_pGame->m_Renderer->DrawRectOutline(ax, y + 16, aRectW, rectH, Color(255, 255, 0, 255), 2);
	m_pGame->m_Renderer->DrawRectOutline(ax + aRectW / 3, y + 16 + rectH / 6, aRectW, rectH, Color(255, 255, 0, 128), 2);
	TextLib::DrawText(GameFont::Default, ax, y + rowH - 14, "alpha overlap",
		TextLib::TextStyle::Color(Color(120, 120, 120)));
}

// Row 6: DrawRoundedRectFilled
void Screen_TestPrimitives::RenderRoundedRectFilledTests(int y)
{
	auto& res = ResolutionConfig::Get();
	int W = res.LogicalWidth();
	int rowH = (res.LogicalHeight() - 30) / 7;
	int contentH = rowH - 16;
	int rectH = contentH - 14;

	TextLib::DrawText(GameFont::Default, 10, y + 2, "DrawRoundedRectFilled",
		TextLib::TextStyle::Color(Color(180, 180, 255)));

	// Section 1: Varying radii (left 2/3)
	int spacing = W / 7;
	int rectW = spacing - 16;
	int x0 = W / 20;
	int radii[] = { 0, 5, 10, 17 };
	const char* labels[] = { "r=0", "r=5", "r=10", "r=17" };
	for (int i = 0; i < 4; i++)
	{
		int rx = x0 + i * spacing;
		m_pGame->m_Renderer->DrawRoundedRectFilled(rx, y + 16, rectW, rectH, radii[i], Color(200, 100, 50));
		TextLib::DrawText(GameFont::Default, rx, y + rowH - 14, labels[i],
			TextLib::TextStyle::Color(Color(120, 120, 120)));
	}

	// Section 2: Alpha overlap (right third)
	int ax = W * 2 / 3 + 10;
	int oRectW = W / 6;
	m_pGame->m_Renderer->DrawRoundedRectFilled(ax, y + 16, oRectW, rectH, 10, Color(0, 200, 255));
	m_pGame->m_Renderer->DrawRoundedRectFilled(ax + oRectW / 3, y + 16 + rectH / 4, oRectW, rectH, 10, Color(255, 100, 0, 128));
	TextLib::DrawText(GameFont::Default, ax, y + rowH - 14, "alpha overlap",
		TextLib::TextStyle::Color(Color(120, 120, 120)));
}

// Row 7: DrawRoundedRectOutline
void Screen_TestPrimitives::RenderRoundedRectOutlineTests(int y)
{
	auto& res = ResolutionConfig::Get();
	int W = res.LogicalWidth();
	int rowH = (res.LogicalHeight() - 30) / 7;
	int contentH = rowH - 16;
	int rectH = contentH - 14;

	TextLib::DrawText(GameFont::Default, 10, y + 2, "DrawRoundedRectOutline",
		TextLib::TextStyle::Color(Color(180, 180, 255)));

	// Section 1: Varying radius and thickness (left 2/3)
	int spacing = W / 6;
	int rectW = spacing - 16;
	int x0 = W / 20;

	m_pGame->m_Renderer->DrawRoundedRectOutline(x0, y + 16, rectW, rectH, 10, Color(0, 255, 200), 1);
	TextLib::DrawText(GameFont::Default, x0, y + rowH - 14, "r=10 t=1",
		TextLib::TextStyle::Color(Color(120, 120, 120)));

	m_pGame->m_Renderer->DrawRoundedRectOutline(x0 + spacing, y + 16, rectW, rectH, 10, Color(0, 255, 200), 2);
	TextLib::DrawText(GameFont::Default, x0 + spacing, y + rowH - 14, "r=10 t=2",
		TextLib::TextStyle::Color(Color(120, 120, 120)));

	m_pGame->m_Renderer->DrawRoundedRectOutline(x0 + spacing * 2, y + 16, rectW, rectH, 15, Color(0, 255, 200), 3);
	TextLib::DrawText(GameFont::Default, x0 + spacing * 2, y + rowH - 14, "r=15 t=3",
		TextLib::TextStyle::Color(Color(120, 120, 120)));

	// Section 2: RGB color variants (right third)
	int cx = W * 2 / 3 + 10;
	int cRectW = W / 8;
	int cSpacing = cRectW / 2;
	m_pGame->m_Renderer->DrawRoundedRectOutline(cx, y + 16, cRectW, rectH, 12, Color(255, 100, 100), 2);
	m_pGame->m_Renderer->DrawRoundedRectOutline(cx + cSpacing, y + 16, cRectW, rectH, 12, Color(100, 255, 100), 2);
	m_pGame->m_Renderer->DrawRoundedRectOutline(cx + cSpacing * 2, y + 16, cRectW, rectH, 12, Color(100, 100, 255), 2);
}
