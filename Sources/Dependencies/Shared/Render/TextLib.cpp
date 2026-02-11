// TextLib.cpp: Unified text rendering implementation
//
// This provides the single entry point for all text rendering.
// It delegates to ITextRenderer (TTF) or IBitmapFont based on Font type.
//////////////////////////////////////////////////////////////////////

#include "TextLib.h"
#include "BitmapFontFactory.h"
#include <cstring>
#include <algorithm>
#include <memory>
#include <vector>
#include <string>

namespace hb::shared::text {

// ============== Static Storage ==============

// TextLib owns the bitmap fonts - no need to store them elsewhere
static std::unique_ptr<IBitmapFont> s_ownedFonts[MAX_BITMAP_FONTS];

// ============== Bitmap Font Management ==============

void LoadBitmapFont(int fontId, hb::shared::sprite::ISprite* sprite, char firstChar, char lastChar,
                    int frameOffset, const FontSpacing& spacing)
{
	if (fontId < 0 || fontId >= MAX_BITMAP_FONTS)
		return;

	s_ownedFonts[fontId] = CreateBitmapFont(sprite, firstChar, lastChar, frameOffset, spacing);
}

void LoadBitmapFontDynamic(int fontId, hb::shared::sprite::ISprite* sprite, char firstChar, char lastChar,
                           int frameOffset)
{
	if (fontId < 0 || fontId >= MAX_BITMAP_FONTS)
		return;

	s_ownedFonts[fontId] = CreateBitmapFontDynamic(sprite, firstChar, lastChar, frameOffset);
}

bool IsBitmapFontLoaded(int fontId)
{
	if (fontId >= 0 && fontId < MAX_BITMAP_FONTS)
	{
		return s_ownedFonts[fontId] != nullptr;
	}
	return false;
}

IBitmapFont* GetBitmapFont(int fontId)
{
	if (fontId >= 0 && fontId < MAX_BITMAP_FONTS)
	{
		return s_ownedFonts[fontId].get();
	}
	return nullptr;
}

// ============== Internal Helpers ==============

static bool IsBitmapFont(int fontId)
{
	return fontId != FONT_ID_DEFAULT;
}

// Convert TextStyle to BitmapTextParams for bitmap font rendering
static BitmapTextParams StyleToBitmapParams(const TextStyle& style)
{
	BitmapTextParams params;
	if (style.alpha < 1.0f)
	{
		params = BitmapTextParams::ColorReplaceWithAlpha(style.color.r, style.color.g, style.color.b, style.alpha);
	}
	else if (style.shadow == ShadowStyle::Integrated)
	{
		params = BitmapTextParams::ColorReplaceWithShadow(style.color.r, style.color.g, style.color.b);
	}
	else
	{
		params = BitmapTextParams::ColorReplace(style.color.r, style.color.g, style.color.b);
	}
	params.useAdditive = style.useAdditive;
	return params;
}

// Calculate brightened highlight color
static void GetHighlightColor(const TextStyle& style, uint8_t& hr, uint8_t& hg, uint8_t& hb)
{
	hr = static_cast<uint8_t>(std::min(255, static_cast<int>(style.color.r) + 90));
	hg = static_cast<uint8_t>(std::min(255, static_cast<int>(style.color.g) + 55));
	hb = static_cast<uint8_t>(std::min(255, static_cast<int>(style.color.b) + 50));
}

// ============== Batching ==============

void BeginBatch()
{
	ITextRenderer* pRenderer = GetTextRenderer();
	if (pRenderer)
		pRenderer->BeginBatch();
}

void EndBatch()
{
	ITextRenderer* pRenderer = GetTextRenderer();
	if (pRenderer)
		pRenderer->EndBatch();
}

// ============== Text Rendering ==============

void DrawText(int fontId, int x, int y, const char* text, const TextStyle& style)
{
	if (!text || text[0] == '\0')
		return;

	if (IsBitmapFont(fontId))
	{
		// ============== Bitmap Font Rendering ==============
		IBitmapFont* pFont = GetBitmapFont(fontId);
		if (!pFont)
			return;

		// Handle shadow styles that require multiple draw calls
		switch (style.shadow)
		{
			case ShadowStyle::Highlight:
			{
				// Draw highlight first at +1,0 with brightened color
				uint8_t hr, hg, hb;
				GetHighlightColor(style, hr, hg, hb);
				pFont->DrawText(x + 1, y, text, BitmapTextParams::ColorReplace(hr, hg, hb));
				// Main text drawn below
				break;
			}
			case ShadowStyle::DropShadow:
			{
				// Draw shadow at +1,+1 in black
				pFont->DrawText(x + 1, y + 1, text, BitmapTextParams::ColorReplace(0, 0, 0));
				break;
			}
			case ShadowStyle::TwoPoint:
			{
				// 2-point shadow: 0,+1 and +1,+1 in black
				pFont->DrawText(x, y + 1, text, BitmapTextParams::ColorReplace(0, 0, 0));
				pFont->DrawText(x + 1, y + 1, text, BitmapTextParams::ColorReplace(0, 0, 0));
				break;
			}
			case ShadowStyle::ThreePoint:
			{
				// 3-point shadow: +1,+1, 0,+1, +1,0 in black
				pFont->DrawText(x + 1, y + 1, text, BitmapTextParams::ColorReplace(0, 0, 0));
				pFont->DrawText(x, y + 1, text, BitmapTextParams::ColorReplace(0, 0, 0));
				pFont->DrawText(x + 1, y, text, BitmapTextParams::ColorReplace(0, 0, 0));
				break;
			}
			default:
				// No pre-shadow needed for None or Integrated
				break;
		}

		// Draw main text
		pFont->DrawText(x, y, text, StyleToBitmapParams(style));
	}
	else
	{
		// ============== TTF Font Rendering ==============
		ITextRenderer* pRenderer = GetTextRenderer();
		if (!pRenderer)
			return;

		// Set font size if specified (ignored for bitmap fonts)
		if (style.fontSize > 0)
			pRenderer->SetFontSize(style.fontSize);

		// Handle shadow styles
		switch (style.shadow)
		{
			case ShadowStyle::ThreePoint:
			{
				// 3-point shadow: +1,+1, 0,+1, +1,0 in black
				pRenderer->DrawText(x + 1, y + 1, text, hb::shared::render::Color::Black());
				pRenderer->DrawText(x, y + 1, text, hb::shared::render::Color::Black());
				pRenderer->DrawText(x + 1, y, text, hb::shared::render::Color::Black());
				break;
			}
			case ShadowStyle::DropShadow:
			{
				// Simple drop shadow at +1,+1 in black
				pRenderer->DrawText(x + 1, y + 1, text, hb::shared::render::Color::Black());
				break;
			}
			case ShadowStyle::TwoPoint:
			{
				// 2-point shadow: 0,+1 and +1,+1 in black
				pRenderer->DrawText(x, y + 1, text, hb::shared::render::Color::Black());
				pRenderer->DrawText(x + 1, y + 1, text, hb::shared::render::Color::Black());
				break;
			}
			case ShadowStyle::Highlight:
			{
				// Highlight at +1,0 with brightened color
				uint8_t hr, hg, hb;
				GetHighlightColor(style, hr, hg, hb);
				pRenderer->DrawText(x + 1, y, text, hb::shared::render::Color(hr, hg, hb));
				break;
			}
			default:
				// None or Integrated - no pre-shadow for TTF
				break;
		}

		// Draw main text
		pRenderer->DrawText(x, y, text, style.color);
	}
}

// ============== Word Wrap (cached, 32-entry ring) ==============

static constexpr int WRAP_CACHE_SIZE = 32;

struct WrapCacheEntry {
	std::string text;
	int fontId = -1;
	int maxWidth = 0;
	std::vector<std::string> lines;
};

static WrapCacheEntry s_wrapCache[WRAP_CACHE_SIZE];
static int s_wrapCacheNext = 0;

static const WrapCacheEntry* FindWrapCache(const char* text, int fontId, int maxWidth)
{
	for (int i = 0; i < WRAP_CACHE_SIZE; i++)
	{
		const auto& e = s_wrapCache[i];
		if (e.fontId == fontId && e.maxWidth == maxWidth && e.text == text)
			return &e;
	}
	return nullptr;
}

static WrapCacheEntry& AllocWrapCache()
{
	WrapCacheEntry& slot = s_wrapCache[s_wrapCacheNext];
	s_wrapCacheNext = (s_wrapCacheNext + 1) % WRAP_CACHE_SIZE;
	return slot;
}

static void WordWrap(int fontId, const char* text, int maxWidth, std::vector<std::string>& outLines)
{
	if (!text || !text[0]) {
		outLines.emplace_back("");
		return;
	}

	// Check cache
	const WrapCacheEntry* cached = FindWrapCache(text, fontId, maxWidth);
	if (cached) {
		outLines = cached->lines;
		return;
	}

	std::string textStr(text);

	if (maxWidth <= 0) {
		outLines.push_back(textStr);
		auto& slot = AllocWrapCache();
		slot = {textStr, fontId, maxWidth, outLines};
		return;
	}

	// Check if entire text fits on one line
	TextMetrics metrics = MeasureText(fontId, text);
	if (metrics.width <= maxWidth) {
		outLines.push_back(textStr);
		auto& slot = AllocWrapCache();
		slot = {textStr, fontId, maxWidth, outLines};
		return;
	}

	const char* remaining = text;
	while (*remaining)
	{
		// Check if remaining fits
		if (MeasureText(fontId, remaining).width <= maxWidth) {
			outLines.emplace_back(remaining);
			break;
		}

		// Get max chars that fit
		int fitCount = GetFittingCharCount(fontId, remaining, maxWidth);
		if (fitCount <= 0) fitCount = 1;

		// Scan backwards for last space within fitCount
		int lastSpace = -1;
		for (int i = fitCount - 1; i >= 0; i--) {
			if (remaining[i] == ' ') {
				lastSpace = i;
				break;
			}
		}

		if (lastSpace > 0) {
			outLines.emplace_back(remaining, lastSpace);
			remaining += lastSpace + 1; // skip the space
		} else {
			outLines.emplace_back(remaining, fitCount);
			remaining += fitCount;
		}
	}

	if (outLines.empty())
		outLines.emplace_back("");

	// Store in cache
	auto& slot = AllocWrapCache();
	slot = {textStr, fontId, maxWidth, outLines};
}

// ============== Single-line aligned draw (internal) ==============

static void DrawTextAlignedSingleLine(int fontId, int rectX, int rectY, int rectWidth, int rectHeight,
                                       const char* text, const TextStyle& style, Align alignment)
{
	if (!text || text[0] == '\0')
		return;

	uint8_t hAlign = alignment & Align::HMask;
	uint8_t vAlign = alignment & Align::VMask;

	if (IsBitmapFont(fontId))
	{
		IBitmapFont* pFont = GetBitmapFont(fontId);
		if (!pFont)
			return;

		int textWidth = pFont->MeasureText(text);
		int textHeight = 16;

		int x = rectX;
		if (hAlign == Align::HCenter)
			x = rectX + (rectWidth - textWidth) / 2;
		else if (hAlign == Align::Right)
			x = rectX + rectWidth - textWidth;

		int y = rectY;
		if (vAlign == Align::VCenter)
			y = rectY + (rectHeight - textHeight) / 2;
		else if (vAlign == Align::Bottom)
			y = rectY + rectHeight - textHeight;

		switch (style.shadow)
		{
			case ShadowStyle::Highlight:
			{
				uint8_t hr, hg, hb;
				GetHighlightColor(style, hr, hg, hb);
				pFont->DrawText(x + 1, y, text, BitmapTextParams::ColorReplace(hr, hg, hb));
				break;
			}
			case ShadowStyle::DropShadow:
				pFont->DrawText(x + 1, y + 1, text, BitmapTextParams::ColorReplace(0, 0, 0));
				break;
			case ShadowStyle::ThreePoint:
				pFont->DrawText(x + 1, y + 1, text, BitmapTextParams::ColorReplace(0, 0, 0));
				pFont->DrawText(x, y + 1, text, BitmapTextParams::ColorReplace(0, 0, 0));
				pFont->DrawText(x + 1, y, text, BitmapTextParams::ColorReplace(0, 0, 0));
				break;
			default:
				break;
		}

		pFont->DrawText(x, y, text, StyleToBitmapParams(style));
	}
	else
	{
		ITextRenderer* pRenderer = GetTextRenderer();
		if (!pRenderer)
			return;

		if (style.fontSize > 0)
			pRenderer->SetFontSize(style.fontSize);

		switch (style.shadow)
		{
			case ShadowStyle::ThreePoint:
				pRenderer->DrawTextAligned(rectX + 1, rectY + 1, rectWidth, rectHeight, text, hb::shared::render::Color::Black(), alignment);
				pRenderer->DrawTextAligned(rectX, rectY + 1, rectWidth, rectHeight, text, hb::shared::render::Color::Black(), alignment);
				pRenderer->DrawTextAligned(rectX + 1, rectY, rectWidth, rectHeight, text, hb::shared::render::Color::Black(), alignment);
				break;
			case ShadowStyle::DropShadow:
				pRenderer->DrawTextAligned(rectX + 1, rectY + 1, rectWidth, rectHeight, text, hb::shared::render::Color::Black(), alignment);
				break;
			case ShadowStyle::Highlight:
			{
				uint8_t hr, hg, hb;
				GetHighlightColor(style, hr, hg, hb);
				pRenderer->DrawTextAligned(rectX + 1, rectY, rectWidth, rectHeight, text, hb::shared::render::Color(hr, hg, hb), alignment);
				break;
			}
			default:
				break;
		}

		pRenderer->DrawTextAligned(rectX, rectY, rectWidth, rectHeight, text, style.color, alignment);
	}
}

// ============== Public DrawTextAligned (single-line, no wrapping) ==============

void DrawTextAligned(int fontId, int rectX, int rectY, int rectWidth, int rectHeight, const char* text,
                     const TextStyle& style, Align alignment)
{
	DrawTextAlignedSingleLine(fontId, rectX, rectY, rectWidth, rectHeight, text, style, alignment);
}

// ============== Public DrawTextWrapped (word-wrap + multi-line) ==============

void DrawTextWrapped(int fontId, int rectX, int rectY, int rectWidth, int rectHeight, const char* text,
                     const TextStyle& style, Align alignment)
{
	if (!text || text[0] == '\0')
		return;

	// Word-wrap into lines (cached)
	std::vector<std::string> lines;
	WordWrap(fontId, text, rectWidth, lines);

	if (lines.size() <= 1) {
		DrawTextAlignedSingleLine(fontId, rectX, rectY, rectWidth, rectHeight, text, style, alignment);
		return;
	}

	// Multi-line layout
	int lineHeight = GetLineHeight(fontId);
	int totalTextHeight = static_cast<int>(lines.size()) * lineHeight;

	uint8_t vAlign = alignment & Align::VMask;
	Align lineAlign = static_cast<Align>((alignment & Align::HMask) | Align::Top);

	int startY = rectY;
	if (vAlign == Align::VCenter)
		startY = rectY + (rectHeight - totalTextHeight) / 2;
	else if (vAlign == Align::Bottom)
		startY = rectY + rectHeight - totalTextHeight;

	for (size_t i = 0; i < lines.size(); i++)
	{
		int lineY = startY + static_cast<int>(i) * lineHeight;
		DrawTextAlignedSingleLine(fontId, rectX, lineY, rectWidth, lineHeight,
		                          lines[i].c_str(), style, lineAlign);
	}
}

// ============== Text Measurement ==============

TextMetrics MeasureText(int fontId, const char* text)
{
	if (!text || text[0] == '\0')
		return {0, 0};

	if (IsBitmapFont(fontId))
	{
		IBitmapFont* pFont = GetBitmapFont(fontId);
		if (pFont)
		{
			// IBitmapFont::MeasureText returns width only, estimate height
			int width = pFont->MeasureText(text);
			return {width, 16}; // Approximate height for bitmap fonts
		}
	}
	else
	{
		ITextRenderer* pRenderer = GetTextRenderer();
		if (pRenderer)
			return pRenderer->MeasureText(text);
	}

	return {0, 0};
}

int GetFittingCharCount(int fontId, const char* text, int maxWidth)
{
	if (!text || text[0] == '\0')
		return 0;

	if (IsBitmapFont(fontId))
	{
		IBitmapFont* pFont = GetBitmapFont(fontId);
		if (!pFont)
			return 0;

		// Measure progressively until we exceed maxWidth
		int len = static_cast<int>(strlen(text));
		for (int i = len; i > 0; i--)
		{
			// Create substring and measure
			char temp[512];
			int copyLen = (i < 511) ? i : 511;
			memcpy(temp, text, copyLen);
			temp[copyLen] = '\0';

			int width = pFont->MeasureText(temp);
			if (width <= maxWidth)
				return i;
		}
		return 0;
	}
	else
	{
		ITextRenderer* pRenderer = GetTextRenderer();
		if (pRenderer)
			return pRenderer->GetFittingCharCount(text, maxWidth);
	}

	return 0;
}

int GetLineHeight(int fontId)
{
	if (IsBitmapFont(fontId))
		return 16; // Approximate height for bitmap fonts

	ITextRenderer* pRenderer = GetTextRenderer();
	if (pRenderer)
		return pRenderer->GetLineHeight();

	return 0;
}

int MeasureWrappedTextHeight(int fontId, const char* text, int maxWidth)
{
	if (!text || text[0] == '\0')
		return 0;

	std::vector<std::string> lines;
	WordWrap(fontId, text, maxWidth, lines);

	return static_cast<int>(lines.size()) * GetLineHeight(fontId);
}

} // namespace hb::shared::text
