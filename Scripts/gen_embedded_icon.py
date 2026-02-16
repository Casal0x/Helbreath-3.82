#!/usr/bin/env python3
"""Generate embedded_icon.h from icon1.ico.

Converts the Windows ICO file to a 32x32 RGBA pixel array embedded as a
C++ constexpr header. This header is used on Linux/macOS to set the
window icon at runtime via SFML's setIcon().

Requires: pip install Pillow

Usage:
    python Scripts/gen_embedded_icon.py
"""

import os
import sys
import warnings

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
ICO_PATH = os.path.join(PROJECT_ROOT, "Sources", "Client", "icon1.ico")
OUTPUT_PATH = os.path.join(PROJECT_ROOT, "Sources", "Dependencies", "Shared", "embedded_icon.h")

ICON_SIZE = 32


def main():
    try:
        from PIL import Image
    except ImportError:
        print("ERROR: Pillow is required. Install with: pip install Pillow", file=sys.stderr)
        return 1

    warnings.filterwarnings("ignore")

    img = Image.open(ICO_PATH)
    icon = img.resize((ICON_SIZE, ICON_SIZE), Image.LANCZOS).convert("RGBA")
    pixels = icon.tobytes()

    lines = [
        "// Auto-generated from icon1.ico — do not edit manually",
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "namespace hb::embedded_icon",
        "{",
        "",
        f"constexpr unsigned int width = {ICON_SIZE};",
        f"constexpr unsigned int height = {ICON_SIZE};",
        "",
        f"// {ICON_SIZE}x{ICON_SIZE} RGBA pixel data ({len(pixels)} bytes)",
        "// clang-format off",
        "constexpr uint8_t pixels[] = {",
    ]

    for i in range(0, len(pixels), 16):
        chunk = pixels[i : i + 16]
        hex_vals = ", ".join(f"0x{b:02X}" for b in chunk)
        comma = "," if i + 16 < len(pixels) else ""
        lines.append(f"    {hex_vals}{comma}")

    lines.append("};")
    lines.append("// clang-format on")
    lines.append("")
    lines.append("} // namespace hb::embedded_icon")
    lines.append("")

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", newline="\n") as f:
        f.write("\n".join(lines))

    print(f"Generated {OUTPUT_PATH}: {ICON_SIZE}x{ICON_SIZE} RGBA ({len(pixels)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
