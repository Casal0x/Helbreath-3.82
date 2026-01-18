#!/usr/bin/env python3
"""
Replace all CSprite constructor calls with SpriteLib::Sprites::Create() calls.
"""
import re
import sys

def replace_csprite_calls(content):
    """
    Replace patterns like:
    new class CSprite(m_hPakFile, static_cast<DXC_ddraw*>(m_Renderer->GetNativeRenderer()), "pakName", index, alphaEffect)

    With:
    SpriteLib::Sprites::Create("pakName", index, alphaEffect)
    """
    # Pattern to match CSprite constructor calls
    # Handles various forms of the alpha parameter (true, false, or missing)
    pattern = r'new class CSprite\s*\(\s*m_hPakFile\s*,\s*static_cast<DXC_ddraw\*>\s*\(\s*m_Renderer->GetNativeRenderer\(\)\s*\)\s*,\s*"([^"]+)"\s*,\s*([^,)]+)\s*(?:,\s*([^)]+))?\s*\)'

    def replacement(match):
        pak_name = match.group(1)
        index = match.group(2).strip()
        alpha_effect = match.group(3)

        if alpha_effect is None:
            # No alpha parameter, use default (true)
            return f'SpriteLib::Sprites::Create("{pak_name}", {index})'
        else:
            alpha_effect = alpha_effect.strip()
            return f'SpriteLib::Sprites::Create("{pak_name}", {index}, {alpha_effect})'

    new_content = re.sub(pattern, replacement, content)
    return new_content

def main():
    if len(sys.argv) < 2:
        print("Usage: python replace_csprite.py <filename>")
        sys.exit(1)

    filename = sys.argv[1]

    with open(filename, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()

    new_content = replace_csprite_calls(content)

    if content != new_content:
        # Create backup
        backup_filename = filename + '.bak'
        with open(backup_filename, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"Backup created: {backup_filename}")

        # Write modified content
        with open(filename, 'w', encoding='utf-8') as f:
            f.write(new_content)
        print(f"Updated: {filename}")

        # Count replacements
        import_count = content.count('new class CSprite')
        new_count = new_content.count('new class CSprite')
        print(f"Replaced {import_count - new_count} CSprite constructor calls")
    else:
        print("No changes needed")

if __name__ == '__main__':
    main()
