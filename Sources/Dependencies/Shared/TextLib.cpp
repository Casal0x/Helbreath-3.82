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

namespace TextLib {

// ============== Static Storage ==============

// TextLib owns the bitmap fonts - no need to store them elsewhere
static std::unique_ptr<IBitmapFont> s_ownedFonts[MAX_BITMAP_FONTS];

// ============== Bitmap Font Management ==============

void LoadBitmapFont(int fontId, SpriteLib::ISprite* sprite, char firstChar, char lastChar,
                    int frameOffset, const FontSpacing& spacing)
{
	if (fontId < 0 || fontId >= MAX_BITMAP_FONTS)
		return;

	s_ownedFonts[fontId] = CreateBitmapFont(sprite, firstChar, lastChar, frameOffset, spacing);
}

void LoadBitmapFontDynamic(int fontId, SpriteLib::ISprite* sprite, char firstChar, char lastChar,
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
		params = BitmapTextParams::ColorReplaceWithAlpha(style.r, style.g, style.b, style.alpha);
	}
	else if (style.shadow == ShadowStyle::Integrated)
	{
		params = BitmapTextParams::ColorReplaceWithShadow(style.r, style.g, style.b);
	}
	else
	{
		params = BitmapTextParams::ColorReplace(style.r, style.g, style.b);
	}
	params.useAdditive = style.useAdditive;
	return params;
}

// Calculate brightened highlight color
static void GetHighlightColor(const TextStyle& style, uint8_t& hr, uint8_t& hg, uint8_t& hb)
{
	hr = static_cast<uint8_t>(std::min(255, static_cast<int>(style.r) + 90));
	hg = static_cast<uint8_t>(std::min(255, static_cast<int>(style.g) + 55));
	hb = static_cast<uint8_t>(std::min(255, static_cast<int>(style.b) + 50));
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

		uint32_t color = style.ToColorRef();

		// Handle shadow styles
		switch (style.shadow)
		{
			case ShadowStyle::ThreePoint:
			{
				// 3-point shadow: +1,+1, 0,+1, +1,0 in black
				uint32_t black = 0x00000000; // RGB(0,0,0)
				pRenderer->DrawText(x + 1, y + 1, text, black);
				pRenderer->DrawText(x, y + 1, text, black);
				pRenderer->DrawText(x + 1, y, text, black);
				break;
			}
			case ShadowStyle::DropShadow:
			{
				// Simple drop shadow at +1,+1 in black
				pRenderer->DrawText(x + 1, y + 1, text, 0x00000000);
				break;
			}
			case ShadowStyle::TwoPoint:
			{
				// 2-point shadow: 0,+1 and +1,+1 in black
				uint32_t black = 0x00000000;
				pRenderer->DrawText(x, y + 1, text, black);
				pRenderer->DrawText(x + 1, y + 1, text, black);
				break;
			}
			case ShadowStyle::Highlight:
			{
				// Highlight at +1,0 with brightened color
				uint8_t hr, hg, hb;
				GetHighlightColor(style, hr, hg, hb);
				uint32_t highlightColor = static_cast<uint32_t>(hr) |
				                          (static_cast<uint32_t>(hg) << 8) |
				                          (static_cast<uint32_t>(hb) << 16);
				pRenderer->DrawText(x + 1, y, text, highlightColor);
				break;
			}
			default:
				// None or Integrated - no pre-shadow for TTF
				break;
		}

		// Draw main text
		pRenderer->DrawText(x, y, text, color);
	}
}

void DrawTextAligned(int fontId, int rectX, int rectY, int rectWidth, int rectHeight, const char* text,
                     const TextStyle& style, Align alignment)
{
	if (!text || text[0] == '\0')
		return;

	// Extract horizontal and vertical components
	uint8_t hAlign = alignment & Align::HMask;
	uint8_t vAlign = alignment & Align::VMask;

	if (IsBitmapFont(fontId))
	{
		// ============== Bitmap Font Aligned ==============
		IBitmapFont* pFont = GetBitmapFont(fontId);
		if (!pFont)
			return;

		// Measure text for alignment calculations
		int textWidth = pFont->MeasureText(text);
		int textHeight = 16;  // Approximate height for bitmap fonts

		// Calculate X position based on horizontal alignment
		int x = rectX;
		if (hAlign == Align::HCenter)
			x = rectX + (rectWidth - textWidth) / 2;
		else if (hAlign == Align::Right)
			x = rectX + rectWidth - textWidth;

		// Calculate Y position based on vertical alignment
		int y = rectY;
		if (vAlign == Align::VCenter)
			y = rectY + (rectHeight - textHeight) / 2;
		else if (vAlign == Align::Bottom)
			y = rectY + rectHeight - textHeight;

		// Handle shadow styles
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
			{
				pFont->DrawText(x + 1, y + 1, text, BitmapTextParams::ColorReplace(0, 0, 0));
				break;
			}
			case ShadowStyle::ThreePoint:
			{
				pFont->DrawText(x + 1, y + 1, text, BitmapTextParams::ColorReplace(0, 0, 0));
				pFont->DrawText(x, y + 1, text, BitmapTextParams::ColorReplace(0, 0, 0));
				pFont->DrawText(x + 1, y, text, BitmapTextParams::ColorReplace(0, 0, 0));
				break;
			}
			default:
				break;
		}

		// Draw main text
		pFont->DrawText(x, y, text, StyleToBitmapParams(style));
	}
	else
	{
		// ============== TTF Font Aligned ==============
		ITextRenderer* pRenderer = GetTextRenderer();
		if (!pRenderer)
			return;

		// Set font size if specified
		if (style.fontSize > 0)
			pRenderer->SetFontSize(style.fontSize);

		uint32_t color = style.ToColorRef();

		// Handle shadow styles with offset rectangles
		switch (style.shadow)
		{
			case ShadowStyle::ThreePoint:
			{
				uint32_t black = 0x00000000;
				pRenderer->DrawTextAligned(rectX + 1, rectY + 1, rectWidth, rectHeight, text, black, alignment);
				pRenderer->DrawTextAligned(rectX, rectY + 1, rectWidth, rectHeight, text, black, alignment);
				pRenderer->DrawTextAligned(rectX + 1, rectY, rectWidth, rectHeight, text, black, alignment);
				break;
			}
			case ShadowStyle::DropShadow:
			{
				pRenderer->DrawTextAligned(rectX + 1, rectY + 1, rectWidth, rectHeight, text, 0x00000000, alignment);
				break;
			}
			case ShadowStyle::Highlight:
			{
				uint8_t hr, hg, hb;
				GetHighlightColor(style, hr, hg, hb);
				uint32_t highlightColor = static_cast<uint32_t>(hr) |
				                          (static_cast<uint32_t>(hg) << 8) |
				                          (static_cast<uint32_t>(hb) << 16);
				pRenderer->DrawTextAligned(rectX + 1, rectY, rectWidth, rectHeight, text, highlightColor, alignment);
				break;
			}
			default:
				break;
		}

		// Draw main aligned text
		pRenderer->DrawTextAligned(rectX, rectY, rectWidth, rectHeight, text, color, alignment);
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

} // namespace TextLib
