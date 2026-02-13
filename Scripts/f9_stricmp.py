"""F9: Replace _stricmp/_strnicmp with hb_stricmp/hb_strnicmp and add #include "StringCompat.h"."""
import re
from pathlib import Path

server_dir = Path(r"Z:\Helbreath-3.82\Sources\Server")

files = [
    "Client.h",
    "CmdGiveItem.cpp",
    "CmdHelp.cpp",
    "CmdReload.cpp",
    "CmdSetAdmin.cpp",
    "EntityManager.cpp",
    "Game.cpp",
    "GameChatCommand.cpp",
    "GameCmdBlock.cpp",
    "GameCmdGM.cpp",
    "GameCmdUnblock.cpp",
    "GameCmdWhisper.cpp",
    "GameConfigSqliteStore.cpp",
    "GuildManager.cpp",
    "ItemManager.cpp",
    "LoginServer.cpp",
    "PartyManager.cpp",
    "ServerCommand.cpp",
    "WarManager.cpp",
]

total_replacements = 0

for fname in files:
    path = server_dir / fname
    text = path.read_text(encoding="utf-8")
    original = text

    # Count replacements
    stricmp_count = text.count("_stricmp(")  # careful: _strnicmp also contains _stricmp
    strnicmp_count = text.count("_strnicmp(")
    # Adjust: _stricmp count includes _strnicmp matches if prefixed by _strn
    stricmp_count -= strnicmp_count  # remove double-counted _strnicmp

    # Replace _strnicmp FIRST (longer match), then _stricmp
    text = text.replace("_strnicmp(", "hb_strnicmp(")
    text = text.replace("_stricmp(", "hb_stricmp(")

    # Add #include "StringCompat.h" if not already present and changes were made
    if text != original and '"StringCompat.h"' not in text:
        # Find a good insertion point: after the last #include line in the header block
        lines = text.split('\n')
        last_include_idx = -1
        for i, line in enumerate(lines):
            if line.strip().startswith('#include'):
                last_include_idx = i
            # Stop searching after we hit a non-include, non-blank, non-comment line
            # past the first include block
            elif last_include_idx >= 0 and line.strip() and not line.strip().startswith('//'):
                break
        if last_include_idx >= 0:
            lines.insert(last_include_idx + 1, '#include "StringCompat.h"')
            text = '\n'.join(lines)

    if text != original:
        path.write_text(text, encoding="utf-8")
        total = stricmp_count + strnicmp_count
        total_replacements += total
        parts = []
        if stricmp_count > 0:
            parts.append(f"_stricmp({stricmp_count})")
        if strnicmp_count > 0:
            parts.append(f"_strnicmp({strnicmp_count})")
        print(f"  {fname}: {' + '.join(parts)} = {total} replacements")
    else:
        print(f"  {fname}: no changes")

print(f"\nTotal: {total_replacements} replacements across {len(files)} files")
