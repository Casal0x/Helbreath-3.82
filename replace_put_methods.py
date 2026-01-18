#!/usr/bin/env python3
"""
Replace Put* sprite method calls with Draw* methods for ISprite interface.

Method mapping:
- PutSpriteFast(x, y, frame, ...) -> DrawFast(x, y, frame)
- PutSprite(x, y, frame, ...) -> Draw(x, y, frame) or DrawFastNoColorKey(x, y, frame)
- PutTransSprite(x, y, frame, ...) -> DrawAlpha(x, y, frame, alpha)
- PutTransSprite70(x, y, frame, ...) -> DrawAlpha70(x, y, frame)
- PutTransSprite50(x, y, frame, ...) -> DrawAlpha50(x, y, frame)
- PutTransSprite25(x, y, frame, ...) -> DrawAlpha25(x, y, frame)
- PutTransSprite2(x, y, frame, ...) -> DrawAlpha25(x, y, frame)  // same as 25%
- PutTransSprite70_NoColorKey(x, y, frame) -> DrawAlpha70NoColorKey(x, y, frame)
- PutTransSprite50_NoColorKey(x, y, frame) -> DrawAlpha50NoColorKey(x, y, frame)
- PutTransSprite25_NoColorKey(x, y, frame) -> DrawAlpha25NoColorKey(x, y, frame)
- PutSpriteRGB(x, y, frame, r, g, b, time) -> DrawTinted(x, y, frame, r, g, b)
- PutSpriteRGBNoColorKey(x, y, frame, r, g, b, time) -> DrawTintedNoColorKey(...) // need to add
- PutShadowSprite(x, y, frame, ...) -> DrawShadow(x, y, frame)
- PutFadeSprite(x, y, frame, ...) -> DrawFade(x, y, frame)
"""
import re
import sys

def replace_put_methods(content):
    """Replace Put* method calls with Draw* equivalents."""

    # PutSpriteFastNoColorKeyDst(dest, x, y, frame, time) -> DrawToSurface(dest, x, y, frame, DrawParams::NoColorKey())
    content = re.sub(
        r'->PutSpriteFastNoColorKeyDst\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*[^)]+\s*\)',
        r'->DrawToSurface(\1, \2, \3, \4, SpriteLib::DrawParams::NoColorKey())',
        content
    )

    # PutSpriteFastNoColorKey(x, y, frame, time) -> DrawFastNoColorKey(x, y, frame)
    content = re.sub(
        r'->PutSpriteFastNoColorKey\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*[^)]+\s*\)',
        r'->DrawFastNoColorKey(\1, \2, \3)',
        content
    )
    content = re.sub(
        r'->PutSpriteFastNoColorKey\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*\)',
        r'->DrawFastNoColorKey(\1, \2, \3)',
        content
    )

    # PutSpriteFast(x, y, frame, dest) -> DrawFast(x, y, frame) or DrawToSurface(dest, x, y, frame)
    # Most common pattern - just use DrawFast
    content = re.sub(
        r'->PutSpriteFast\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*,\s*[^)]+\s*\)',
        r'->DrawFast(\1, \2, \3)',
        content
    )
    content = re.sub(
        r'->PutSpriteFast\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*\)',
        r'->DrawFast(\1, \2, \3)',
        content
    )

    # PutTransSprite70_NoColorKey -> DrawAlpha70NoColorKey
    content = re.sub(
        r'->PutTransSprite70_NoColorKey\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*\)',
        r'->DrawAlpha70NoColorKey(\1, \2, \3)',
        content
    )
    content = re.sub(
        r'->PutTransSprite70_NoColorKey\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*[^)]+\s*\)',
        r'->DrawAlpha70NoColorKey(\1, \2, \3)',
        content
    )

    # PutTransSprite50_NoColorKey -> DrawAlpha50NoColorKey
    content = re.sub(
        r'->PutTransSprite50_NoColorKey\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*\)',
        r'->DrawAlpha50NoColorKey(\1, \2, \3)',
        content
    )
    content = re.sub(
        r'->PutTransSprite50_NoColorKey\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*[^)]+\s*\)',
        r'->DrawAlpha50NoColorKey(\1, \2, \3)',
        content
    )

    # PutTransSprite25_NoColorKey -> DrawAlpha25NoColorKey
    content = re.sub(
        r'->PutTransSprite25_NoColorKey\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*\)',
        r'->DrawAlpha25NoColorKey(\1, \2, \3)',
        content
    )
    content = re.sub(
        r'->PutTransSprite25_NoColorKey\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*[^)]+\s*\)',
        r'->DrawAlpha25NoColorKey(\1, \2, \3)',
        content
    )

    # PutTransSprite70 -> DrawAlpha70 (must be after NoColorKey versions)
    content = re.sub(
        r'->PutTransSprite70\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*,\s*[^)]+\s*\)',
        r'->DrawAlpha70(\1, \2, \3)',
        content
    )
    content = re.sub(
        r'->PutTransSprite70\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*\)',
        r'->DrawAlpha70(\1, \2, \3)',
        content
    )

    # PutTransSprite50 -> DrawAlpha50
    content = re.sub(
        r'->PutTransSprite50\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*,\s*[^)]+\s*\)',
        r'->DrawAlpha50(\1, \2, \3)',
        content
    )
    content = re.sub(
        r'->PutTransSprite50\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*\)',
        r'->DrawAlpha50(\1, \2, \3)',
        content
    )

    # PutTransSprite25 -> DrawAlpha25
    content = re.sub(
        r'->PutTransSprite25\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*,\s*[^)]+\s*\)',
        r'->DrawAlpha25(\1, \2, \3)',
        content
    )
    content = re.sub(
        r'->PutTransSprite25\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*\)',
        r'->DrawAlpha25(\1, \2, \3)',
        content
    )

    # PutTransSprite2 -> DrawAlpha25 (same transparency level)
    content = re.sub(
        r'->PutTransSprite2\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*,\s*[^)]+\s*\)',
        r'->DrawAlpha25(\1, \2, \3)',
        content
    )
    content = re.sub(
        r'->PutTransSprite2\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*\)',
        r'->DrawAlpha25(\1, \2, \3)',
        content
    )

    # PutTransSprite -> DrawAlpha (generic, variable alpha - keep last parameter as alpha)
    # PutTransSprite(x, y, frame, time) where time affects alpha
    content = re.sub(
        r'->PutTransSprite\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^)]+)\s*\)',
        r'->DrawAlpha(\1, \2, \3, \4 / 100.0f)',
        content
    )
    content = re.sub(
        r'->PutTransSprite\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*\)',
        r'->DrawAlpha(\1, \2, \3, 0.5f)',
        content
    )

    # PutSpriteRGB(x, y, frame, r, g, b, time) -> DrawTinted(x, y, frame, r, g, b)
    content = re.sub(
        r'->PutSpriteRGB\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*[^)]+\s*\)',
        r'->DrawTinted(\1, \2, \3, \4, \5, \6)',
        content
    )
    content = re.sub(
        r'->PutSpriteRGB\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*\)',
        r'->DrawTinted(\1, \2, \3, \4, \5, \6)',
        content
    )

    # PutShadowSprite -> DrawShadow
    content = re.sub(
        r'->PutShadowSprite\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*,\s*[^)]+\s*\)',
        r'->DrawShadow(\1, \2, \3)',
        content
    )
    content = re.sub(
        r'->PutShadowSprite\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*\)',
        r'->DrawShadow(\1, \2, \3)',
        content
    )

    # PutFadeSprite -> DrawFade
    content = re.sub(
        r'->PutFadeSprite\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*,\s*[^)]+\s*\)',
        r'->DrawFade(\1, \2, \3)',
        content
    )
    content = re.sub(
        r'->PutFadeSprite\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*\)',
        r'->DrawFade(\1, \2, \3)',
        content
    )

    # PutSprite with destination surface (rare)
    # For now, just convert to DrawFast and handle special cases manually

    # PutTransSpriteRGB(x, y, frame, r, g, b, time) -> DrawTintedAlpha(x, y, frame, r, g, b, alpha)
    # Note: time parameter in old code affected alpha, but for compatibility just use DrawTinted
    content = re.sub(
        r'->PutTransSpriteRGB\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*[^)]+\s*\)',
        r'->DrawTintedAlpha(\1, \2, \3, \4, \5, \6, 0.7f)',
        content
    )

    # PutSpriteFastWidth(x, y, frame, width, time) -> DrawWidth(x, y, frame, width)
    content = re.sub(
        r'->PutSpriteFastWidth\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*[^)]+\s*\)',
        r'->DrawWidth(\1, \2, \3, \4)',
        content
    )
    content = re.sub(
        r'->PutSpriteFastWidth\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*\)',
        r'->DrawWidth(\1, \2, \3, \4)',
        content
    )

    # PutShadowSpriteClip(x, y, frame, time) -> DrawShadow(x, y, frame)
    content = re.sub(
        r'->PutShadowSpriteClip\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*,\s*[^)]+\s*\)',
        r'->DrawShadow(\1, \2, \3)',
        content
    )
    content = re.sub(
        r'->PutShadowSpriteClip\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*\)',
        r'->DrawShadow(\1, \2, \3)',
        content
    )

    # PutRevTransSprite(x, y, frame, time) -> DrawAlpha with reverse blend (use DrawAlpha70 as fallback)
    content = re.sub(
        r'->PutRevTransSprite\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*,\s*[^)]+\s*\)',
        r'->DrawAlpha70(\1, \2, \3)',
        content
    )

    # PutShiftSpriteFast(x, y, shiftX, shiftY, frame) -> DrawShifted(x, y, shiftX, shiftY, frame)
    content = re.sub(
        r'->PutShiftSpriteFast\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*,\s*[^)]+\s*\)',
        r'->DrawShifted(\1, \2, \3, \4, \5)',
        content
    )
    content = re.sub(
        r'->PutShiftSpriteFast\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*\)',
        r'->DrawShifted(\1, \2, \3, \4, \5)',
        content
    )

    # PutShiftTransSprite2(x, y, shiftX, shiftY, frame, time) -> DrawShifted + alpha
    # For now, map to DrawShifted with Alpha25 params
    content = re.sub(
        r'->PutShiftTransSprite2\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*[^)]+\s*\)',
        r'->DrawShifted(\1, \2, \3, \4, \5, SpriteLib::DrawParams::Alpha(0.25f))',
        content
    )

    # PutTransSprite_NoColorKey(x, y, frame, time) -> DrawAlphaNoColorKey
    content = re.sub(
        r'->PutTransSprite_NoColorKey\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*[^)]+\s*\)',
        r'->DrawAlpha50NoColorKey(\1, \2, \3)',
        content
    )
    content = re.sub(
        r'->PutTransSprite_NoColorKey\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,)]+)\s*\)',
        r'->DrawAlpha50NoColorKey(\1, \2, \3)',
        content
    )

    return content

def main():
    if len(sys.argv) < 2:
        print("Usage: python replace_put_methods.py <filename>")
        sys.exit(1)

    filename = sys.argv[1]

    with open(filename, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()

    new_content = replace_put_methods(content)

    if content != new_content:
        # Count changes
        put_count_before = content.count('->Put')
        put_count_after = new_content.count('->Put')

        with open(filename, 'w', encoding='utf-8') as f:
            f.write(new_content)
        print(f"Updated: {filename}")
        print(f"Replaced {put_count_before - put_count_after} Put* calls")
    else:
        print("No changes needed")

if __name__ == '__main__':
    main()
