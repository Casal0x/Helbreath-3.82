"""F1: Remove #include "winmain.h" from server .cpp files that no longer need it."""
import re
from pathlib import Path

server_dir = Path(r"Z:\Helbreath-3.82\Sources\Server")

files = [
    "CmdBroadcast.cpp",
    "CmdGiveItem.cpp",
    "CmdHelp.cpp",
    "CmdReload.cpp",
    "CmdSetAdmin.cpp",
    "CmdSetCmdLevel.cpp",
    "CmdShowChat.cpp",
    "GameChatCommand.cpp",
    "ServerCommand.cpp",
]

for fname in files:
    path = server_dir / fname
    text = path.read_text(encoding="utf-8")
    new_text = re.sub(r'#include\s+"winmain\.h"\s*\n', '', text)
    if new_text != text:
        path.write_text(new_text, encoding="utf-8")
        print(f"  Removed #include \"winmain.h\" from {fname}")
    else:
        print(f"  No match in {fname}")
