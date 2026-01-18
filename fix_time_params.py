#!/usr/bin/env python3
"""
Remove trailing time parameters from Draw* method calls.
"""
import re
import sys
import glob
import os

def fix_time_params(content):
    """Remove trailing time parameters from Draw* method calls."""

    # Pattern: Draw*(x, y, frame, timeVar) -> Draw*(x, y, frame)
    # Where timeVar is: dwTime, m_dwCurTime, m_pGame->m_dwCurTime, G_dwGlobalTime
    time_vars = r'(?:dwTime|m_dwCurTime|m_pGame->m_dwCurTime|G_dwGlobalTime|m_pGame->G_dwGlobalTime)'

    # DrawFast(x, y, frame, time) -> DrawFast(x, y, frame)
    content = re.sub(
        r'(->DrawFast\([^,]+,[^,]+,[^,]+),\s*' + time_vars + r'\)',
        r'\1)',
        content
    )

    # DrawTinted(x, y, frame, r, g, b, time) -> DrawTinted(x, y, frame, r, g, b)
    # Already handled - has 6 params

    # DrawAlpha(x, y, frame, time / 100.0f) - already correctly converted

    # DrawWidth(x, y, frame, width, time) -> DrawWidth(x, y, frame, width)
    content = re.sub(
        r'(->DrawWidth\([^,]+,[^,]+,[^,]+,[^,]+),\s*' + time_vars + r'\)',
        r'\1)',
        content
    )

    # DrawShadow(x, y, frame, time) -> DrawShadow(x, y, frame)
    content = re.sub(
        r'(->DrawShadow\([^,]+,[^,]+,[^,]+),\s*' + time_vars + r'\)',
        r'\1)',
        content
    )

    # DrawFastNoColorKey same
    content = re.sub(
        r'(->DrawFastNoColorKey\([^,]+,[^,]+,[^,]+),\s*' + time_vars + r'\)',
        r'\1)',
        content
    )

    # DrawAlpha*(x, y, frame, time) -> DrawAlpha*(x, y, frame)
    content = re.sub(
        r'(->DrawAlpha\d*\([^,]+,[^,]+,[^,]+),\s*' + time_vars + r'\)',
        r'\1)',
        content
    )
    content = re.sub(
        r'(->DrawAlpha\d*NoColorKey\([^,]+,[^,]+,[^,]+),\s*' + time_vars + r'\)',
        r'\1)',
        content
    )

    # DrawTintedAlpha(x, y, frame, r, g, b, alpha, time) - shouldn't exist
    # but handle any remaining time params

    # Fix _bCheckCollison -> CheckCollision
    content = content.replace('->_bCheckCollison(', '->CheckCollision(')

    return content

def main():
    base_path = "Sources/Client"
    files = glob.glob(os.path.join(base_path, "*.cpp"))

    total_fixed = 0
    for filepath in files:
        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()

        new_content = fix_time_params(content)

        if content != new_content:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(new_content)
            print(f"Fixed: {filepath}")
            total_fixed += 1

    print(f"\nTotal files fixed: {total_fixed}")

if __name__ == '__main__':
    main()
