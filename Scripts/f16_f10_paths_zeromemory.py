"""F16+F10: Replace backslash paths with forward slashes, ZeroMemory with std::memset."""
import re
from pathlib import Path

server_dir = Path(r"Z:\Helbreath-3.82\Sources\Server")

# F16: Directory prefix backslash replacements (source-level: \\ = one literal backslash)
path_prefixes = [
    ("Accounts\\\\", "Accounts/"),
    ("GameLogs\\\\", "GameLogs/"),
    ("GameConfigs\\\\", "GameConfigs/"),
    ("GameData\\\\", "GameData/"),
    ("Guilds\\\\", "Guilds/"),
    ("mapdata\\\\", "mapdata/"),
]

f16_files = [
    "AccountSqliteStore.cpp", "CmdShowChat.cpp", "Game.cpp",
    "GameChatCommand.cpp", "GuildManager.cpp", "LoginServer.cpp",
    "Map.cpp", "WarManager.cpp",
]

# Files with standalone "\\" path separator strings
standalone_backslash_files = ["GuildManager.cpp", "WarManager.cpp"]

# F10: ZeroMemory(ptr, size) → std::memset(ptr, 0, size)
zero_memory_re = re.compile(r'ZeroMemory\(([^,]+),\s*([^)]+)\)')
f10_files = ["Game.cpp", "EntityManager.cpp"]

all_files = sorted(set(f16_files + f10_files))

total_changes = 0
for fname in all_files:
    path = server_dir / fname
    text = path.read_text(encoding="utf-8")
    original = text
    changes = []

    # F16: Replace directory prefix backslashes
    if fname in f16_files:
        for old, new in path_prefixes:
            if old in text:
                count = text.count(old)
                text = text.replace(old, new)
                changes.append(f"  F16: '{old}' -> '{new}' ({count}x)")

    # F16: Replace standalone "\\" with "/" in specific files
    # After prefix replacements, only truly standalone "\\" remain
    if fname in standalone_backslash_files:
        standalone = "\"\\\\\""  # Python repr of the 4 source chars: " \ \ "
        replacement = '"/"'
        if standalone in text:
            count = text.count(standalone)
            text = text.replace(standalone, replacement)
            changes.append(f'  F16: standalone "\\\\" -> "/" ({count}x)')

    # F10: Replace ZeroMemory
    if fname in f10_files:
        matches = zero_memory_re.findall(text)
        if matches:
            text = zero_memory_re.sub(r'std::memset(\1, 0, \2)', text)
            changes.append(f"  F10: ZeroMemory -> std::memset ({len(matches)}x)")

    if text != original:
        path.write_text(text, encoding="utf-8")
        print(f"{fname}:")
        for c in changes:
            print(c)
        total_changes += len(changes)
    else:
        print(f"{fname}: no changes")

print(f"\nTotal: {total_changes} change groups applied")
