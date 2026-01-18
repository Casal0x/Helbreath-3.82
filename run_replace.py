#!/usr/bin/env python3
"""Run replacement on all client files."""
import os
import glob
from replace_put_methods import replace_put_methods

def main():
    base_path = "Sources/Client"
    patterns = [
        "DialogBox_*.cpp",
        "Effect_*.cpp"
    ]

    files = []
    for pattern in patterns:
        files.extend(glob.glob(os.path.join(base_path, pattern)))

    total_replaced = 0
    for filepath in files:
        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()

        new_content = replace_put_methods(content)

        if content != new_content:
            put_count_before = content.count('->Put')
            put_count_after = new_content.count('->Put')
            replaced = put_count_before - put_count_after

            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(new_content)
            print(f"Updated: {filepath} ({replaced} replacements)")
            total_replaced += replaced

    print(f"\nTotal replaced: {total_replaced}")

if __name__ == '__main__':
    main()
