// TextLibExt.h: Game-side extensions for TextLib
//
// Includes TextLib.h and adds game-specific overloads.
// Game code should include this instead of TextLib.h directly.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "TextLib.h"
#include "CommonTypes.h"

// ============== TextLib Namespace Extensions ==============

namespace TextLib {

// DrawTextAligned overload accepting GameRectangle
inline void DrawTextAligned(int fontId, const GameRectangle& rect, const char* text,
                            const TextStyle& style, Align alignment = Align::TopLeft) {
	DrawTextAligned(fontId, rect.x, rect.y, rect.width, rect.height, text, style, alignment);
}

// Convenience overload with RGB values
inline void DrawTextAligned(int fontId, const GameRectangle& rect, const char* text,
                            uint8_t r, uint8_t g, uint8_t b, Align alignment = Align::TopLeft) {
	DrawTextAligned(fontId, rect, text, TextStyle::Color(r, g, b), alignment);
}

} // namespace TextLib
