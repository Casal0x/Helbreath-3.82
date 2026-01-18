#!/usr/bin/env python3
"""
Replace ->m_rcBound accesses with ->GetBoundRect() calls.
"""
import re
import sys

def replace_bound_rect(content):
    """Replace sprite->m_rcBound.member with sprite->GetBoundRect().member"""

    # Pattern to match ->m_rcBound.member
    # sprite->m_rcBound.top  becomes  sprite->GetBoundRect().top
    content = re.sub(
        r'->m_rcBound\.',
        r'->GetBoundRect().',
        content
    )

    return content

def main():
    if len(sys.argv) < 2:
        print("Usage: python replace_bound_rect.py <filename>")
        sys.exit(1)

    filename = sys.argv[1]

    with open(filename, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()

    new_content = replace_bound_rect(content)

    if content != new_content:
        count_before = content.count('->m_rcBound')
        count_after = new_content.count('->m_rcBound')

        with open(filename, 'w', encoding='utf-8') as f:
            f.write(new_content)
        print(f"Updated: {filename}")
        print(f"Replaced {count_before - count_after} m_rcBound accesses")
    else:
        print("No changes needed")

if __name__ == '__main__':
    main()
