#!/usr/bin/env python3
"""Phase 5: SFMLEngine member variable renames
Target: 20 Hungarian/camelCase member vars across 7 header + 7 impl files.
All private, self-contained module — no external consumers.
Mode 2 justified: 14 files, mechanical word-boundary replacements.
"""

import re
import os
import sys

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SFML_DIR = os.path.join(BASE, "Sources", "SFMLEngine")

# All renames are m_-prefixed → unique, word-boundary safe.
# Sorted longest-first to prevent prefix clobbering.
RENAMES = [
    # SFMLWindow — compound names (longest first)
    ("m_bMouseCaptureEnabled", "m_mouse_capture_enabled"),
    ("m_bFullscreenStretch", "m_fullscreen_stretch"),  # SFMLRenderer + SFMLWindow
    ("m_windowedHeight", "m_windowed_height"),
    ("m_windowedWidth", "m_windowed_width"),
    ("m_pEventHandler", "m_event_handler"),
    ("m_iconResourceId", "m_icon_resource_id"),
    # SFMLRenderer
    ("m_fpsAccumulator", "m_fps_accumulator"),
    ("m_pRenderWindow", "m_render_window"),  # SFMLInput + SFMLRenderer
    ("m_bSkipFrame", "m_skip_frame"),
    ("m_deltaTime", "m_delta_time"),
    ("m_iFpsLimit", "m_fps_limit"),  # SFMLRenderer + SFMLWindow
    ("m_bVSync", "m_vsync"),  # SFMLRenderer + SFMLWindow
    # SFMLTextRenderer
    ("m_pBackBuffer", "m_back_buffer"),
    ("m_fontSize", "m_font_size"),
    # SFMLInput
    ("m_wheelDelta", "m_wheel_delta"),
    ("m_mouseX", "m_mouse_x"),
    ("m_mouseY", "m_mouse_y"),
    # SFMLSprite + SFMLSpriteFactory
    ("m_pRenderer", "m_renderer"),
    # SFMLBitmapFont
    ("m_pSprite", "m_sprite"),
    # SFMLInput + SFMLWindow
    ("m_hWnd", "m_handle"),
]


def collect_files():
    """Collect all .h and .cpp files under Sources/SFMLEngine/."""
    files = []
    for name in sorted(os.listdir(SFML_DIR)):
        if name.endswith((".h", ".cpp")):
            files.append(os.path.join(SFML_DIR, name))
    return files


def apply_renames(files, dry_run=False):
    """Apply word-boundary renames to all files."""
    patterns = [(re.compile(r"\b" + re.escape(old) + r"\b"), new)
                for old, new in RENAMES]

    modified_files = []
    total_changes = 0

    for fpath in files:
        with open(fpath, "r", encoding="utf-8") as f:
            content = f.read()

        original = content
        for pattern, new in patterns:
            content = pattern.sub(new, content)

        if content != original:
            if not dry_run:
                with open(fpath, "w", encoding="utf-8", newline="\n") as f:
                    f.write(content)
            # Count changed lines
            changes = sum(1 for a, b in zip(original.split("\n"), content.split("\n")) if a != b)
            total_changes += changes
            modified_files.append(fpath)
            print(f"  MOD  {os.path.basename(fpath)} ({changes} lines)")

    return modified_files, total_changes


def main():
    dry_run = "--dry-run" in sys.argv
    mode = "DRY RUN" if dry_run else "LIVE"
    print(f"Phase 5: SFMLEngine member renames [{mode}]")
    print(f"  Base: {SFML_DIR}")
    print()

    files = collect_files()
    print(f"Scanning {len(files)} SFMLEngine files...")
    print()

    mod_files, changes = apply_renames(files, dry_run)
    print()
    print(f"Summary: {len(mod_files)} files modified, ~{changes} lines changed")

    if dry_run:
        print("\n  Files that would be modified:")
        for f in mod_files:
            print(f"    {os.path.relpath(f, BASE)}")

    return 0 if mod_files else 1


if __name__ == "__main__":
    sys.exit(main())
