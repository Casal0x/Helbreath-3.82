#!/usr/bin/env python3
"""
Convert Put* sprite method calls directly to ISprite Draw() with DrawParams.
Uses function to find balanced parentheses for correct argument parsing.
"""
import re
import sys
import glob
import os

def find_call_args(content, start_pos):
    """Find arguments of a function call starting at the opening paren.
    Returns (args_list, end_pos) where end_pos is after the closing paren."""
    if content[start_pos] != '(':
        return None, start_pos

    depth = 1
    pos = start_pos + 1
    args = []
    current_arg_start = pos

    while pos < len(content) and depth > 0:
        ch = content[pos]
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
            if depth == 0:
                # End of call - capture last arg
                arg = content[current_arg_start:pos].strip()
                if arg:
                    args.append(arg)
        elif ch == ',' and depth == 1:
            # Argument separator at our level
            arg = content[current_arg_start:pos].strip()
            args.append(arg)
            current_arg_start = pos + 1
        pos += 1

    return args, pos

def convert_put_call(content, method_name, replacement_func):
    """Replace all calls to method_name with the result of replacement_func(args)."""
    pattern = r'->' + re.escape(method_name) + r'\s*\('
    result = []
    last_end = 0

    for match in re.finditer(pattern, content):
        result.append(content[last_end:match.start()])

        # Find the opening paren
        paren_pos = match.end() - 1
        args, end_pos = find_call_args(content, paren_pos)

        if args is not None:
            replacement = replacement_func(args)
            result.append(replacement)
            last_end = end_pos
        else:
            # Couldn't parse, keep original
            result.append(match.group())
            last_end = match.end()

    result.append(content[last_end:])
    return ''.join(result)

def convert_put_to_draw(content):
    """Convert Put* method calls to Draw() with DrawParams."""

    # PutSpriteFast(x, y, frame, [time]) -> Draw(x, y, frame)
    def put_sprite_fast(args):
        if len(args) >= 3:
            return f'->Draw({args[0]}, {args[1]}, {args[2]})'
        return f'->Draw({", ".join(args)})'
    content = convert_put_call(content, 'PutSpriteFast', put_sprite_fast)

    # PutSpriteFastNoColorKey -> Draw with NoColorKey
    def put_sprite_fast_nck(args):
        if len(args) >= 3:
            return f'->Draw({args[0]}, {args[1]}, {args[2]}, SpriteLib::DrawParams::NoColorKey())'
        return f'->Draw({", ".join(args)}, SpriteLib::DrawParams::NoColorKey())'
    content = convert_put_call(content, 'PutSpriteFastNoColorKey', put_sprite_fast_nck)

    # PutTransSprite70_NoColorKey -> Draw with alpha 0.7f and no colorkey
    def put_trans70_nck(args):
        if len(args) >= 3:
            return f'->Draw({args[0]}, {args[1]}, {args[2]}, SpriteLib::DrawParams{{0.7f, 0, 0, 0, false}})'
        return f'->Draw({", ".join(args[:3])}, SpriteLib::DrawParams{{0.7f, 0, 0, 0, false}})'
    content = convert_put_call(content, 'PutTransSprite70_NoColorKey', put_trans70_nck)

    # PutTransSprite50_NoColorKey
    def put_trans50_nck(args):
        if len(args) >= 3:
            return f'->Draw({args[0]}, {args[1]}, {args[2]}, SpriteLib::DrawParams{{0.5f, 0, 0, 0, false}})'
        return f'->Draw({", ".join(args[:3])}, SpriteLib::DrawParams{{0.5f, 0, 0, 0, false}})'
    content = convert_put_call(content, 'PutTransSprite50_NoColorKey', put_trans50_nck)

    # PutTransSprite25_NoColorKey
    def put_trans25_nck(args):
        if len(args) >= 3:
            return f'->Draw({args[0]}, {args[1]}, {args[2]}, SpriteLib::DrawParams{{0.25f, 0, 0, 0, false}})'
        return f'->Draw({", ".join(args[:3])}, SpriteLib::DrawParams{{0.25f, 0, 0, 0, false}})'
    content = convert_put_call(content, 'PutTransSprite25_NoColorKey', put_trans25_nck)

    # PutTransSprite70 -> Draw with alpha 0.7f
    def put_trans70(args):
        if len(args) >= 3:
            return f'->Draw({args[0]}, {args[1]}, {args[2]}, SpriteLib::DrawParams::Alpha(0.7f))'
        return f'->Draw({", ".join(args[:3])}, SpriteLib::DrawParams::Alpha(0.7f))'
    content = convert_put_call(content, 'PutTransSprite70', put_trans70)

    # PutTransSprite50 -> Draw with alpha 0.5f
    def put_trans50(args):
        if len(args) >= 3:
            return f'->Draw({args[0]}, {args[1]}, {args[2]}, SpriteLib::DrawParams::Alpha(0.5f))'
        return f'->Draw({", ".join(args[:3])}, SpriteLib::DrawParams::Alpha(0.5f))'
    content = convert_put_call(content, 'PutTransSprite50', put_trans50)

    # PutTransSprite25 -> Draw with alpha 0.25f
    def put_trans25(args):
        if len(args) >= 3:
            return f'->Draw({args[0]}, {args[1]}, {args[2]}, SpriteLib::DrawParams::Alpha(0.25f))'
        return f'->Draw({", ".join(args[:3])}, SpriteLib::DrawParams::Alpha(0.25f))'
    content = convert_put_call(content, 'PutTransSprite25', put_trans25)

    # PutTransSprite2 -> Draw with alpha 0.25f
    def put_trans2(args):
        if len(args) >= 3:
            return f'->Draw({args[0]}, {args[1]}, {args[2]}, SpriteLib::DrawParams::Alpha(0.25f))'
        return f'->Draw({", ".join(args[:3])}, SpriteLib::DrawParams::Alpha(0.25f))'
    content = convert_put_call(content, 'PutTransSprite2', put_trans2)

    # PutTransSprite_NoColorKey -> Draw with alpha 0.5f and no colorkey
    def put_trans_nck(args):
        if len(args) >= 3:
            return f'->Draw({args[0]}, {args[1]}, {args[2]}, SpriteLib::DrawParams{{0.5f, 0, 0, 0, false}})'
        return f'->Draw({", ".join(args[:3])}, SpriteLib::DrawParams{{0.5f, 0, 0, 0, false}})'
    content = convert_put_call(content, 'PutTransSprite_NoColorKey', put_trans_nck)

    # PutTransSprite(x, y, frame, time) -> Draw with alpha 0.5f
    def put_trans(args):
        if len(args) >= 3:
            return f'->Draw({args[0]}, {args[1]}, {args[2]}, SpriteLib::DrawParams::Alpha(0.5f))'
        return f'->Draw({", ".join(args[:3])}, SpriteLib::DrawParams::Alpha(0.5f))'
    content = convert_put_call(content, 'PutTransSprite', put_trans)

    # PutSpriteRGB(x, y, frame, r, g, b, time) -> Draw with Tint
    def put_rgb(args):
        if len(args) >= 6:
            return f'->Draw({args[0]}, {args[1]}, {args[2]}, SpriteLib::DrawParams::Tint({args[3]}, {args[4]}, {args[5]}))'
        return f'->Draw({", ".join(args[:3])})'
    content = convert_put_call(content, 'PutSpriteRGB', put_rgb)

    # PutTransSpriteRGB(x, y, frame, r, g, b, time) -> Draw with TintedAlpha
    def put_trans_rgb(args):
        if len(args) >= 6:
            return f'->Draw({args[0]}, {args[1]}, {args[2]}, SpriteLib::DrawParams::TintedAlpha({args[3]}, {args[4]}, {args[5]}, 0.7f))'
        return f'->Draw({", ".join(args[:3])})'
    content = convert_put_call(content, 'PutTransSpriteRGB', put_trans_rgb)

    # PutShadowSprite -> Draw with Shadow
    def put_shadow(args):
        if len(args) >= 3:
            return f'->Draw({args[0]}, {args[1]}, {args[2]}, SpriteLib::DrawParams::Shadow())'
        return f'->Draw({", ".join(args[:3])}, SpriteLib::DrawParams::Shadow())'
    content = convert_put_call(content, 'PutShadowSprite', put_shadow)

    # PutShadowSpriteClip -> Draw with Shadow
    def put_shadow_clip(args):
        if len(args) >= 3:
            return f'->Draw({args[0]}, {args[1]}, {args[2]}, SpriteLib::DrawParams::Shadow())'
        return f'->Draw({", ".join(args[:3])}, SpriteLib::DrawParams::Shadow())'
    content = convert_put_call(content, 'PutShadowSpriteClip', put_shadow_clip)

    # PutFadeSprite -> Draw with Fade
    def put_fade(args):
        if len(args) >= 3:
            return f'->Draw({args[0]}, {args[1]}, {args[2]}, SpriteLib::DrawParams::Fade())'
        return f'->Draw({", ".join(args[:3])}, SpriteLib::DrawParams::Fade())'
    content = convert_put_call(content, 'PutFadeSprite', put_fade)

    # PutRevTransSprite -> Draw with alpha 0.7f
    def put_rev_trans(args):
        if len(args) >= 3:
            return f'->Draw({args[0]}, {args[1]}, {args[2]}, SpriteLib::DrawParams::Alpha(0.7f))'
        return f'->Draw({", ".join(args[:3])}, SpriteLib::DrawParams::Alpha(0.7f))'
    content = convert_put_call(content, 'PutRevTransSprite', put_rev_trans)

    # PutSpriteFastWidth(x, y, frame, width, time) -> DrawWidth(x, y, frame, width)
    def put_width(args):
        if len(args) >= 4:
            return f'->DrawWidth({args[0]}, {args[1]}, {args[2]}, {args[3]})'
        return f'->DrawWidth({", ".join(args[:4])})'
    content = convert_put_call(content, 'PutSpriteFastWidth', put_width)

    # PutShiftSpriteFast -> DrawShifted
    def put_shift(args):
        if len(args) >= 5:
            return f'->DrawShifted({args[0]}, {args[1]}, {args[2]}, {args[3]}, {args[4]})'
        return f'->DrawShifted({", ".join(args[:5])})'
    content = convert_put_call(content, 'PutShiftSpriteFast', put_shift)

    # PutShiftTransSprite2 -> DrawShifted with alpha
    def put_shift_trans(args):
        if len(args) >= 5:
            return f'->DrawShifted({args[0]}, {args[1]}, {args[2]}, {args[3]}, {args[4]}, SpriteLib::DrawParams::Alpha(0.25f))'
        return f'->DrawShifted({", ".join(args[:5])}, SpriteLib::DrawParams::Alpha(0.25f))'
    content = convert_put_call(content, 'PutShiftTransSprite2', put_shift_trans)

    # PutSpriteFastNoColorKeyDst(dest, x, y, frame, time) -> DrawToSurface(dest, x, y, frame, NoColorKey)
    def put_nck_dst(args):
        if len(args) >= 4:
            return f'->DrawToSurface({args[0]}, {args[1]}, {args[2]}, {args[3]}, SpriteLib::DrawParams::NoColorKey())'
        return f'->DrawToSurface({", ".join(args[:4])}, SpriteLib::DrawParams::NoColorKey())'
    content = convert_put_call(content, 'PutSpriteFastNoColorKeyDst', put_nck_dst)

    # Other method renames
    content = content.replace('->_GetSpriteRect(', '->GetFrameRect(')
    content = content.replace('->_iCloseSprite()', '->Unload()')
    content = content.replace('->iRestore()', '->Restore()')
    content = content.replace('->_bCheckCollison(', '->CheckCollision(')

    return content

def main():
    base_path = "Sources/Client"
    files = glob.glob(os.path.join(base_path, "*.cpp"))

    total_fixed = 0
    for filepath in files:
        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()

        new_content = convert_put_to_draw(content)

        if content != new_content:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(new_content)
            print(f"Fixed: {filepath}")
            total_fixed += 1

    print(f"\nTotal files fixed: {total_fixed}")

if __name__ == '__main__':
    main()
