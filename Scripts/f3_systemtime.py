"""F3: Replace SYSTEMTIME/GetLocalTime with hb::time::local_time."""
import re
from pathlib import Path

server_dir = Path(r"Z:\Helbreath-3.82\Sources\Server")

files = [
    "AccountSqliteStore.cpp",
    "CraftingManager.cpp",
    "EntityManager.cpp",
    "Game.cpp",
    "Game.h",
    "GameChatCommand.cpp",
    "GuildManager.cpp",
    "ItemManager.cpp",
    "LoginServer.cpp",
    "WarManager.cpp",
]

# Field name mappings
field_map = {
    ".wYear": ".year",
    ".wMonth": ".month",
    ".wDay": ".day",
    ".wHour": ".hour",
    ".wMinute": ".minute",
    ".wSecond": ".second",
    ".wDayOfWeek": ".day_of_week",  # only in comments
}

total_changes = 0

for fname in files:
    path = server_dir / fname
    text = path.read_text(encoding="utf-8")
    original = text
    changes = []

    # 1. Replace SYSTEMTIME variable declarations (non-commented)
    #    Pattern: "SYSTEMTIME varname;" on its own or with other declarations
    count = len(re.findall(r'^\s*SYSTEMTIME\s+\w+\s*;', text, re.MULTILINE))
    text = re.sub(
        r'^(\s*)SYSTEMTIME\s+(\w+)\s*;',
        r'\1hb::time::local_time \2{};',
        text,
        flags=re.MULTILINE,
    )
    if count > 0:
        changes.append(f"  SYSTEMTIME decl -> hb::time::local_time ({count}x)")

    # 2. Replace SYSTEMTIME in member declarations (Game.h)
    if "SYSTEMTIME m_" in text:
        count = text.count("SYSTEMTIME m_")
        text = text.replace("SYSTEMTIME m_", "hb::time::local_time m_")
        changes.append(f"  SYSTEMTIME member -> hb::time::local_time ({count}x)")

    # 3. Replace GetLocalTime(&var);
    count = len(re.findall(r'GetLocalTime\(\s*&\s*\w+\s*\)', text))
    text = re.sub(
        r'GetLocalTime\(\s*&\s*(\w+)\s*\)\s*;',
        r'\1 = hb::time::local_time::now();',
        text,
    )
    if count > 0:
        changes.append(f"  GetLocalTime -> local_time::now() ({count}x)")

    # 4. Replace field accesses
    for old, new in field_map.items():
        if old in text:
            count = text.count(old)
            text = text.replace(old, new)
            changes.append(f"  {old} -> {new} ({count}x)")

    # 5. Replace FormatTimestamp function definitions
    # AccountSqliteStore.cpp has it inside anonymous namespace (indented)
    # LoginServer.cpp has it at file scope
    fmt_def_re = re.compile(
        r'^\s*void\s+FormatTimestamp\s*\(\s*const\s+(?:SYSTEMTIME|hb::time::local_time)\s*&.*?\n\s*\{.*?\n.*?\n\s*\}\s*\n',
        re.MULTILINE | re.DOTALL,
    )
    if fmt_def_re.search(text):
        text = fmt_def_re.sub('', text)
        changes.append("  Deleted FormatTimestamp definition")

    # 6. Replace FormatTimestamp calls with hb::time::format_timestamp
    if "FormatTimestamp(" in text:
        count = text.count("FormatTimestamp(")
        text = text.replace("FormatTimestamp(", "hb::time::format_timestamp(")
        changes.append(f"  FormatTimestamp -> hb::time::format_timestamp ({count}x)")

    # 7. Delete commented-out SYSTEMTIME/GetLocalTime lines (dead code cleanup)
    lines = text.split('\n')
    new_lines = []
    for line in lines:
        stripped = line.strip()
        # Skip lines that are purely commented-out SYSTEMTIME or GetLocalTime
        if stripped in ('//SYSTEMTIME SysTime;', '//int iMapside, iMapside2;'):
            changes.append(f"  Deleted commented line: {stripped}")
            continue
        if stripped == '//GetLocalTime(&m_MaxUserSysTime);':
            changes.append(f"  Deleted commented line: {stripped}")
            continue
        new_lines.append(line)
    text = '\n'.join(new_lines)

    # 8. Add #include "TimeUtils.h" if not already present and changes were made
    if text != original and '"TimeUtils.h"' not in text:
        lines = text.split('\n')
        last_include_idx = -1
        for i, line in enumerate(lines):
            if line.strip().startswith('#include'):
                last_include_idx = i
            elif last_include_idx >= 0 and line.strip() and not line.strip().startswith('//'):
                break
        if last_include_idx >= 0:
            lines.insert(last_include_idx + 1, '#include "TimeUtils.h"')
            text = '\n'.join(lines)
            changes.append("  Added #include \"TimeUtils.h\"")

    if text != original:
        path.write_text(text, encoding="utf-8")
        print(f"{fname}:")
        for c in changes:
            print(c)
        total_changes += 1
    else:
        print(f"{fname}: no changes")

print(f"\nModified {total_changes} files")
