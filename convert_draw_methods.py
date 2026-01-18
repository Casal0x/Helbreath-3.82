#!/usr/bin/env python3
"""
Convert old CSprite Draw* method calls to new ISprite Draw() with DrawParams.
"""
import re
import sys
import glob
import os

def convert_draw_methods(content):
    """Convert old CSprite Draw* methods to ISprite Draw() with DrawParams."""

    # DrawAlpha50NoColorKey(x, y, frame) -> Draw(x, y, frame, DrawParams::Alpha(0.5f) with useColorKey=false)
    content = re.sub(
        r'->DrawAlpha50NoColorKey\(([^,]+),\s*([^,]+),\s*([^)]+)\)',
        r'->Draw(\1, \2, \3, SpriteLib::DrawParams{0.5f, 0, 0, 0, false})',
        content
    )

    # DrawAlpha70NoColorKey(x, y, frame)
    content = re.sub(
        r'->DrawAlpha70NoColorKey\(([^,]+),\s*([^,]+),\s*([^)]+)\)',
        r'->Draw(\1, \2, \3, SpriteLib::DrawParams{0.7f, 0, 0, 0, false})',
        content
    )

    # DrawAlpha25NoColorKey(x, y, frame)
    content = re.sub(
        r'->DrawAlpha25NoColorKey\(([^,]+),\s*([^,]+),\s*([^)]+)\)',
        r'->Draw(\1, \2, \3, SpriteLib::DrawParams{0.25f, 0, 0, 0, false})',
        content
    )

    # DrawAlpha50(x, y, frame)
    content = re.sub(
        r'->DrawAlpha50\(([^,]+),\s*([^,]+),\s*([^)]+)\)',
        r'->Draw(\1, \2, \3, SpriteLib::DrawParams::Alpha(0.5f))',
        content
    )

    # DrawAlpha70(x, y, frame)
    content = re.sub(
        r'->DrawAlpha70\(([^,]+),\s*([^,]+),\s*([^)]+)\)',
        r'->Draw(\1, \2, \3, SpriteLib::DrawParams::Alpha(0.7f))',
        content
    )

    # DrawAlpha25(x, y, frame)
    content = re.sub(
        r'->DrawAlpha25\(([^,]+),\s*([^,]+),\s*([^)]+)\)',
        r'->Draw(\1, \2, \3, SpriteLib::DrawParams::Alpha(0.25f))',
        content
    )

    # DrawTintedAlpha(x, y, frame, r, g, b, alpha) -> Draw with TintedAlpha
    content = re.sub(
        r'->DrawTintedAlpha\(([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^)]+)\)',
        r'->Draw(\1, \2, \3, SpriteLib::DrawParams::TintedAlpha(\4, \5, \6, \7))',
        content
    )

    # DrawAlpha(x, y, frame, alpha) -> Draw with Alpha
    content = re.sub(
        r'->DrawAlpha\(([^,]+),\s*([^,]+),\s*([^,]+),\s*([^)]+)\)',
        r'->Draw(\1, \2, \3, SpriteLib::DrawParams::Alpha(\4))',
        content
    )

    # DrawFade(x, y, frame) -> Draw with Fade
    content = re.sub(
        r'->DrawFade\(([^,]+),\s*([^,]+),\s*([^)]+)\)',
        r'->Draw(\1, \2, \3, SpriteLib::DrawParams::Fade())',
        content
    )

    # DrawFast(x, y, frame) -> Draw(x, y, frame)
    # This should already have been handled but catch remaining
    content = re.sub(
        r'->DrawFast\(([^,]+),\s*([^,]+),\s*([^)]+)\)',
        r'->Draw(\1, \2, \3)',
        content
    )

    # _GetSpriteRect(frame) -> GetFrameRect(frame)
    content = re.sub(
        r'->_GetSpriteRect\(',
        r'->GetFrameRect(',
        content
    )

    # _iCloseSprite() -> Unload()
    content = re.sub(
        r'->_iCloseSprite\(\)',
        r'->Unload()',
        content
    )

    # iRestore() -> Restore()
    content = re.sub(
        r'->iRestore\(\)',
        r'->Restore()',
        content
    )

    return content

def main():
    base_path = "Sources/Client"
    files = glob.glob(os.path.join(base_path, "*.cpp"))

    total_fixed = 0
    for filepath in files:
        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()

        new_content = convert_draw_methods(content)

        if content != new_content:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(new_content)
            print(f"Fixed: {filepath}")
            total_fixed += 1

    print(f"\nTotal files fixed: {total_fixed}")

if __name__ == '__main__':
    main()
