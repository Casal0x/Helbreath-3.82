// TextLibExt.h: Game-side extensions for TextLib
//
// Includes TextLib.h and adds game-specific overloads.
// Game code should include this instead of TextLib.h directly.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "TextLib.h"
#include "CommonTypes.h"

// ============== TextLib Namespace Extensions ==============

namespace hb::shared::text {

// DrawText overload accepting hb::shared::render::Color
inline void DrawText(int fontId, int x, int y, const char* text, const hb::shared::render::Color& color) {
	DrawText(fontId, x, y, text, TextStyle::Color(color));
}

// DrawTextAligned overload accepting hb::shared::geometry::GameRectangle
inline void DrawTextAligned(int fontId, const hb::shared::geometry::GameRectangle& rect, const char* text,
                            const TextStyle& style, Align alignment = Align::TopLeft) {
	DrawTextAligned(fontId, rect.x, rect.y, rect.width, rect.height, text, style, alignment);
}

// DrawTextAligned overload accepting hb::shared::geometry::GameRectangle + hb::shared::render::Color
inline void DrawTextAligned(int fontId, const hb::shared::geometry::GameRectangle& rect, const char* text,
                            const hb::shared::render::Color& color, Align alignment = Align::TopLeft) {
	DrawTextAligned(fontId, rect, text, TextStyle::Color(color), alignment);
}

// DrawTextAligned overload accepting hb::shared::render::Color
inline void DrawTextAligned(int fontId, int x, int y, int width, int height, const char* text,
                            const hb::shared::render::Color& color, Align alignment = Align::TopLeft) {
	DrawTextAligned(fontId, x, y, width, height, text, TextStyle::Color(color), alignment);
}

} // namespace hb::shared::text
