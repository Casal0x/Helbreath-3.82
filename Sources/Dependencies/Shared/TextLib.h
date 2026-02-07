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
#include "PrimitiveTypes.h"
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
	::Color color{255, 255, 255};
	float alpha = 1.0f;
	ShadowStyle shadow = ShadowStyle::None;
	int fontSize = 0;  // 0 = use default size, ignored for bitmap fonts
	bool useAdditive = false;  // Use additive blending for bright text on dark sprites

	// Default constructor
	constexpr TextStyle() = default;

	// Color constructor
	constexpr TextStyle(const ::Color& c)
		: color(c) {}

	// Color + shadow constructor
	constexpr TextStyle(const ::Color& c, ShadowStyle s)
		: color(c), shadow(s) {}

	// Color + alpha constructor
	constexpr TextStyle(const ::Color& c, float a)
		: color(c), alpha(a) {}

	// Full constructor
	constexpr TextStyle(const ::Color& c, float a, ShadowStyle s)
		: color(c), alpha(a), shadow(s) {}

	// Full constructor with size
	constexpr TextStyle(const ::Color& c, float a, ShadowStyle s, int size)
		: color(c), alpha(a), shadow(s), fontSize(size) {}

	// Full constructor with additive
	constexpr TextStyle(const ::Color& c, float a, ShadowStyle s, int size, bool additive)
		: color(c), alpha(a), shadow(s), fontSize(size), useAdditive(additive) {}

	// ============== Factory Methods ==============

	// Simple color
	static constexpr TextStyle Color(const ::Color& c) {
		return TextStyle(c);
	}

	// Color with 3-point shadow (like PutString2)
	static constexpr TextStyle WithShadow(const ::Color& c) {
		return TextStyle(c, ShadowStyle::ThreePoint);
	}

	// Color with 2-point shadow (0,+1 and +1,+1) (like PutString_SprFont3)
	static constexpr TextStyle WithTwoPointShadow(const ::Color& c) {
		return TextStyle(c, ShadowStyle::TwoPoint);
	}

	// Color with drop shadow (+1,+1)
	static constexpr TextStyle WithDropShadow(const ::Color& c) {
		return TextStyle(c, ShadowStyle::DropShadow);
	}

	// Color with highlight effect (like PutString_SprFont)
	static constexpr TextStyle WithHighlight(const ::Color& c) {
		return TextStyle(c, ShadowStyle::Highlight);
	}

	// Color with integrated shadow (like PutString_SprFont2)
	static constexpr TextStyle WithIntegratedShadow(const ::Color& c) {
		return TextStyle(c, ShadowStyle::Integrated);
	}

	// Color with alpha transparency
	static constexpr TextStyle Transparent(const ::Color& c, float alpha) {
		return TextStyle(c, alpha);
	}

	// ============== Modifier Methods ==============

	// Create a copy with different shadow style
	constexpr TextStyle WithShadowStyle(ShadowStyle s) const {
		return TextStyle(color, alpha, s, fontSize, useAdditive);
	}

	// Create a copy with different alpha
	constexpr TextStyle WithAlpha(float a) const {
		return TextStyle(color, a, shadow, fontSize, useAdditive);
	}

	// Create a copy with different font size (ignored for bitmap fonts)
	constexpr TextStyle WithFontSize(int size) const {
		return TextStyle(color, alpha, shadow, size, useAdditive);
	}

	// Create a copy with additive blending enabled
	// Use this for bright text (damage numbers) that needs DDraw-like brightness
	constexpr TextStyle WithAdditive() const {
		return TextStyle(color, alpha, shadow, fontSize, true);
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
