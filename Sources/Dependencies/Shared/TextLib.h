// TextLib.h: Unified text rendering API for Helbreath
//
// This is the single entry point for all text rendering in the game.
// The game should only use TextLib:: functions for text rendering.
// Engines provide ITextRenderer (TTF) and IBitmapFont implementations.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "ITextRenderer.h"
#include "IBitmapFont.h"
#include "BitmapFontFactory.h"
#include <algorithm>

namespace TextLib {

// ============== Font ID System ==============
// TextLib uses integer font IDs. The game defines its own enum that maps to these.
// Font ID 0 is reserved for the default TTF font.
// Bitmap fonts use IDs 1 and up.

constexpr int FONT_ID_DEFAULT = 0;      // TTF font (loaded from FONTS/default.ttf)
constexpr int MAX_BITMAP_FONTS = 16;    // Maximum number of bitmap fonts

// ============== Shadow Styles ==============

enum class ShadowStyle {
	None,           // No shadow
	Highlight,      // +1,0 offset with brightened color (PutString_SprFont style)
	TwoPoint,       // 0,+1 and +1,+1 offsets in black (PutString_SprFont3 style)
	ThreePoint,     // +1,+1, 0,+1, +1,0 offsets in black (PutString2 style)
	DropShadow,     // +1,+1 offset in black (simple drop shadow)
	Integrated      // Font handles shadow internally (PutString_SprFont2 style)
};

// ============== Text Style ==============

struct TextStyle
{
	uint8_t r = 255;
	uint8_t g = 255;
	uint8_t b = 255;
	float alpha = 1.0f;
	ShadowStyle shadow = ShadowStyle::None;
	int fontSize = 0;  // 0 = use default size, ignored for bitmap fonts
	bool useAdditive = false;  // Use additive blending for bright text on dark sprites

	// Default constructor
	constexpr TextStyle() = default;

	// RGB constructor
	constexpr TextStyle(uint8_t r_, uint8_t g_, uint8_t b_)
		: r(r_), g(g_), b(b_) {}

	// RGB + shadow constructor
	constexpr TextStyle(uint8_t r_, uint8_t g_, uint8_t b_, ShadowStyle s)
		: r(r_), g(g_), b(b_), shadow(s) {}

	// RGB + alpha constructor
	constexpr TextStyle(uint8_t r_, uint8_t g_, uint8_t b_, float a)
		: r(r_), g(g_), b(b_), alpha(a) {}

	// Full constructor
	constexpr TextStyle(uint8_t r_, uint8_t g_, uint8_t b_, float a, ShadowStyle s)
		: r(r_), g(g_), b(b_), alpha(a), shadow(s) {}

	// Full constructor with size
	constexpr TextStyle(uint8_t r_, uint8_t g_, uint8_t b_, float a, ShadowStyle s, int size)
		: r(r_), g(g_), b(b_), alpha(a), shadow(s), fontSize(size) {}

	// Full constructor with additive
	constexpr TextStyle(uint8_t r_, uint8_t g_, uint8_t b_, float a, ShadowStyle s, int size, bool additive)
		: r(r_), g(g_), b(b_), alpha(a), shadow(s), fontSize(size), useAdditive(additive) {}

	// ============== Factory Methods ==============

	// Simple color (most common)
	static constexpr TextStyle Color(uint8_t r, uint8_t g, uint8_t b) {
		return TextStyle(r, g, b);
	}

	// Color with 3-point shadow (like PutString2)
	static constexpr TextStyle WithShadow(uint8_t r, uint8_t g, uint8_t b) {
		return TextStyle(r, g, b, ShadowStyle::ThreePoint);
	}

	// Color with 2-point shadow (0,+1 and +1,+1) (like PutString_SprFont3)
	static constexpr TextStyle WithTwoPointShadow(uint8_t r, uint8_t g, uint8_t b) {
		return TextStyle(r, g, b, ShadowStyle::TwoPoint);
	}

	// Color with drop shadow (+1,+1)
	static constexpr TextStyle WithDropShadow(uint8_t r, uint8_t g, uint8_t b) {
		return TextStyle(r, g, b, ShadowStyle::DropShadow);
	}

	// Color with highlight effect (like PutString_SprFont)
	static constexpr TextStyle WithHighlight(uint8_t r, uint8_t g, uint8_t b) {
		return TextStyle(r, g, b, ShadowStyle::Highlight);
	}

	// Color with integrated shadow (like PutString_SprFont2)
	static constexpr TextStyle WithIntegratedShadow(uint8_t r, uint8_t g, uint8_t b) {
		return TextStyle(r, g, b, ShadowStyle::Integrated);
	}

	// Color with alpha transparency
	static constexpr TextStyle Transparent(uint8_t r, uint8_t g, uint8_t b, float alpha) {
		return TextStyle(r, g, b, alpha);
	}

	// From Windows COLORREF (0x00BBGGRR format) - for easy migration
	static TextStyle FromColorRef(uint32_t colorref) {
		return TextStyle(
			static_cast<uint8_t>(colorref & 0xFF),
			static_cast<uint8_t>((colorref >> 8) & 0xFF),
			static_cast<uint8_t>((colorref >> 16) & 0xFF)
		);
	}

	// ============== Conversion Methods ==============

	// Convert to Windows COLORREF
	uint32_t ToColorRef() const {
		return static_cast<uint32_t>(r) |
		       (static_cast<uint32_t>(g) << 8) |
		       (static_cast<uint32_t>(b) << 16);
	}

	// Create a copy with different shadow style
	constexpr TextStyle WithShadowStyle(ShadowStyle s) const {
		return TextStyle(r, g, b, alpha, s, fontSize, useAdditive);
	}

	// Create a copy with different alpha
	constexpr TextStyle WithAlpha(float a) const {
		return TextStyle(r, g, b, a, shadow, fontSize, useAdditive);
	}

	// Create a copy with different font size (ignored for bitmap fonts)
	constexpr TextStyle WithFontSize(int size) const {
		return TextStyle(r, g, b, alpha, shadow, size, useAdditive);
	}

	// Create a copy with additive blending enabled
	// Use this for bright text (damage numbers) that needs DDraw-like brightness
	constexpr TextStyle WithAdditive() const {
		return TextStyle(r, g, b, alpha, shadow, fontSize, true);
	}
};

// ============== Bitmap Font Management ==============

// Load and register a bitmap font with explicit character widths
// TextLib takes ownership of the font - no need to store it elsewhere
void LoadBitmapFont(int fontId, SpriteLib::ISprite* sprite, char firstChar, char lastChar,
                    int frameOffset, const FontSpacing& spacing);

// Load and register a bitmap font with dynamic spacing (width from sprite frames)
void LoadBitmapFontDynamic(int fontId, SpriteLib::ISprite* sprite, char firstChar, char lastChar,
                           int frameOffset);

// Check if a bitmap font is loaded
bool IsBitmapFontLoaded(int fontId);

// Get a registered bitmap font (for internal use)
IBitmapFont* GetBitmapFont(int fontId);

// ============== Text Rendering API ==============

// Draw text at position (x, y)
// fontId: FONT_ID_DEFAULT for TTF, or game-defined bitmap font ID
void DrawText(int fontId, int x, int y, const char* text, const TextStyle& style);

// Draw text aligned within a rectangle (x, y, width, height)
// Use Align flags: Align::Left, Align::HCenter, Align::Right, Align::Top, Align::VCenter, Align::Bottom
// Combine with bitwise OR: Align::HCenter | Align::VCenter, or use presets: Align::Center
void DrawTextAligned(int fontId, int x, int y, int width, int height, const char* text,
                     const TextStyle& style, Align alignment = Align::TopLeft);

// ============== Convenience Overloads ==============

// Simple RGB color (no style object needed for basic cases)
inline void DrawText(int fontId, int x, int y, const char* text, uint8_t r, uint8_t g, uint8_t b) {
	DrawText(fontId, x, y, text, TextStyle::Color(r, g, b));
}

inline void DrawTextAligned(int fontId, int x, int y, int width, int height, const char* text,
                            uint8_t r, uint8_t g, uint8_t b, Align alignment = Align::TopLeft) {
	DrawTextAligned(fontId, x, y, width, height, text, TextStyle::Color(r, g, b), alignment);
}

// ============== Text Measurement ==============

// Measure text dimensions
TextMetrics MeasureText(int fontId, const char* text);

// Get number of characters that fit within maxWidth pixels
int GetFittingCharCount(int fontId, const char* text, int maxWidth);

// ============== Batching ==============
// For DDraw, text rendering requires DC acquisition. Wrap multiple text calls
// in Begin/End for better performance. SFML ignores these (no-op).

void BeginBatch();
void EndBatch();

// RAII helper for automatic batch management
class ScopedBatch
{
public:
	ScopedBatch() { BeginBatch(); }
	~ScopedBatch() { EndBatch(); }

	// Non-copyable
	ScopedBatch(const ScopedBatch&) = delete;
	ScopedBatch& operator=(const ScopedBatch&) = delete;
};

} // namespace TextLib
