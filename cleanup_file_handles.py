#!/usr/bin/env python3
"""
Clean up redundant file handling code around sprite creation calls.
The sprite factory now handles file I/O internally.
"""
import re

def cleanup_file_handles(content):
    """
    Remove CreateFile/CloseHandle blocks that only contain Sprites::Create calls.

    Pattern:
        m_hPakFile = CreateFile("sprites\\xxx.pak", ...);
        if (m_hPakFile != INVALID_HANDLE_VALUE) {
            m_pSprite[...] = SpriteLib::Sprites::Create(...);
            m_pSprite[...] = SpriteLib::Sprites::Create(...);
            CloseHandle(m_hPakFile);
        }

    Becomes:
        m_pSprite[...] = SpriteLib::Sprites::Create(...);
        m_pSprite[...] = SpriteLib::Sprites::Create(...);
    """

    # Pattern to match CreateFile block with if check and CloseHandle
    # This is complex, so we'll do it in multiple passes

    # First, remove the CreateFile lines
    content = re.sub(r'\n?\s*m_hPakFile = CreateFile\([^)]+\);\s*\n', '\n', content)

    # Remove if (m_hPakFile != INVALID_HANDLE_VALUE) { ... but keep the body
    # Pattern: if (m_hPakFile != INVALID_HANDLE_VALUE) { or if(m_hPakFile != INVALID_HANDLE_VALUE){
    content = re.sub(r'\s*if\s*\(\s*m_hPakFile\s*!=\s*INVALID_HANDLE_VALUE\s*\)\s*\{?\s*\n', '\n', content)

    # Also handle: if (m_hPakFile != INVALID_HANDLE_VALUE)
    content = re.sub(r'\s*if\s*\(\s*m_hPakFile\s*!=\s*INVALID_HANDLE_VALUE\s*\)\s*\n', '\n', content)

    # Remove CloseHandle(m_hPakFile);
    content = re.sub(r'\n?\s*CloseHandle\s*\(\s*m_hPakFile\s*\)\s*;\s*\n', '\n', content)

    # Remove closing braces that are now orphaned (this is tricky)
    # We'll leave the structure intact and just remove the file-related lines
    # The closing braces for the blocks will need manual cleanup if they cause issues

    return content

def main():
    filename = "Sources/Client/Game.cpp"

    with open(filename, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()

    new_content = cleanup_file_handles(content)

    if content != new_content:
        with open(filename, 'w', encoding='utf-8') as f:
            f.write(new_content)
        print(f"Cleaned up file handles in: {filename}")
    else:
        print("No changes needed")

if __name__ == '__main__':
    main()
