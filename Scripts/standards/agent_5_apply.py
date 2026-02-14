#!/usr/bin/env python3
"""Agent 5: TeleportManager + EventListManager + TextInputManager snake_case conversion.

Handles:
- TeleportEntry struct fields (declarations + member access)
- EventEntry struct fields (declarations + member access)
- TeleportManager methods (unique to this class)
- EventListManager methods (unique to this class)
- TextInputManager methods (unique to this class)
- Parameters/locals specific to these classes' methods and their forwarding chains

SKIPS (too broadly shared across codebase -- handled by other agents or a coordinator):
- SetGame, Reset, IsActive (shared method names on 10+ classes)
- pData, pGame (params used in 50+ files)
- dwTime as param/local (used in 20+ files)
"""
import glob, os, re

# ============================================================================
# PHASE 1: Global replacements safe to apply across all client files.
#
# Struct field accesses use dot-prefix patterns (e.g., .iIndex → .index).
# Method names are unique to the three assigned classes.
# Parameter names have been verified via grep to only appear in assigned
# files and their direct forwarding chains.
#
# ORDERING MATTERS: dot-prefixed struct field rules MUST come before
# standalone param/local rules (e.g., .cColor before cColor) because
# \bcColor\b would also match inside .cColor at the word boundary.
# ============================================================================
GLOBAL_REPLACEMENTS = [
    # --- TeleportEntry struct field access (dot-prefixed, safe) ---
    (r'\.iIndex\b', '.index', 'TeleportEntry .iIndex → .index'),
    (r'\.mapname\b', '.map_name', 'TeleportEntry .mapname → .map_name'),
    (r'\.iX\b', '.x', 'TeleportEntry .iX → .x'),
    (r'\.iY\b', '.y', 'TeleportEntry .iY → .y'),
    (r'\.iCost\b', '.cost', 'TeleportEntry .iCost → .cost'),

    # --- EventEntry struct field access (dot-prefixed, safe) ---
    # MUST come before standalone cColor/cTxt/dwTime rules below
    (r'\.dwTime\b', '.time', 'EventEntry .dwTime → .time'),
    (r'\.cColor\b', '.color', 'EventEntry .cColor → .color'),
    (r'\.cTxt\b', '.text', 'EventEntry .cTxt → .text'),

    # --- TeleportManager methods (all unique to TeleportManager) ---
    (r'\bHandleTeleportList\b', 'handle_teleport_list', 'TeleportManager::HandleTeleportList'),
    (r'\bHandleChargedTeleport\b', 'handle_charged_teleport', 'TeleportManager::HandleChargedTeleport'),
    (r'\bHandleHeldenianTeleportList\b', 'handle_heldenian_teleport_list', 'TeleportManager::HandleHeldenianTeleportList'),
    (r'\bGetMapCount\b', 'get_map_count', 'TeleportManager::GetMapCount'),
    (r'\bSetMapCount\b', 'set_map_count', 'TeleportManager::SetMapCount'),
    (r'\bGetList\b', 'get_list', 'TeleportManager::GetList'),
    (r'\bGetLocX\b', 'get_loc_x', 'TeleportManager::GetLocX'),
    (r'\bGetLocY\b', 'get_loc_y', 'TeleportManager::GetLocY'),
    (r'\bSetLocation\b', 'set_location', 'TeleportManager::SetLocation'),
    (r'\bGetMapName\b', 'get_map_name', 'TeleportManager::GetMapName'),
    (r'\bSetMapName\b', 'set_map_name', 'TeleportManager::SetMapName'),
    (r'\bIsRequested\b', 'is_requested', 'TeleportManager::IsRequested'),
    (r'\bSetRequested\b', 'set_requested', 'TeleportManager::SetRequested'),

    # --- EventListManager methods (unique to EventListManager) ---
    # AddEventTop must come BEFORE AddEvent to avoid partial match issues
    # (though \b prevents this, ordering is still defensive)
    (r'\bAddEventTop\b', 'add_event_top', 'EventListManager::AddEventTop'),
    (r'\bAddEvent\b', 'add_event', 'EventListManager::AddEvent'),
    (r'\bShowEvents\b', 'show_events', 'EventListManager::ShowEvents'),

    # --- TextInputManager methods (unique to TextInputManager) ---
    (r'\bStartInput\b', 'start_input', 'TextInputManager::StartInput'),
    (r'\bEndInput\b', 'end_input', 'TextInputManager::EndInput'),
    (r'\bClearInput\b', 'clear_input', 'TextInputManager::ClearInput'),
    (r'\bShowInput\b', 'show_input', 'TextInputManager::ShowInput'),
    (r'\bGetInputString\b', 'get_input_string', 'TextInputManager::GetInputString'),
    (r'\bHandleChar\b', 'handle_char', 'TextInputManager::HandleChar'),
    (r'\bGetCharKind\b', 'get_char_kind', 'TextInputManager::GetCharKind'),

    # --- Parameters unique to my files + their forwarding chain ---
    # maxLen: only in TextInputManager.h/cpp (2 files)
    (r'\bmaxLen\b', 'max_len', 'param maxLen → max_len'),
    # isHidden: only in TextInputManager.h/cpp (2 files)
    (r'\bisHidden\b', 'is_hidden', 'param isHidden → is_hidden'),
    # bDupAllow: EventListManager + AddEventList forwarding (8 files)
    (r'\bbDupAllow\b', 'allow_duplicates', 'param bDupAllow → allow_duplicates'),
    # pTxt: EventListManager + AddEventList forwarding (8 files, no other uses)
    (r'\bpTxt\b', 'text', 'param pTxt → text'),
    # cColor standalone (NOT .cColor which was already handled above):
    #   EventListManager params + AddEventList forwarding + NetworkMessages_Items locals
    #   All are correct renames per coding standard
    (r'\bcColor\b', 'color', 'param/local cColor → color'),

    # --- Locals unique to my files ---
    # sRejectReason: only in TeleportManager.cpp
    (r'\bsRejectReason\b', 'reject_reason', 'local sRejectReason → reject_reason'),
]

# ============================================================================
# PHASE 2: File-specific replacements for struct field DECLARATIONS.
#
# These patterns are too common to apply globally. For example:
#   "int iIndex" appears in BuildItemManager, FloatingTextManager, Game.h, etc.
#   "uint32_t dwTime" appears in 20+ files as param/local.
#   "std::string cTxt" appears in 50+ files as local variable.
#
# We only apply these in the specific header files where our structs are defined.
# ============================================================================
FILE_SPECIFIC = {
    'TeleportManager.h': [
        # TeleportEntry struct field declarations
        (r'\bint iIndex = 0\b', 'int index = 0', 'TeleportEntry::iIndex declaration'),
        (r'\bstd::string mapname\b', 'std::string map_name', 'TeleportEntry::mapname declaration'),
        (r'\bint iX = 0\b', 'int x = 0', 'TeleportEntry::iX declaration'),
        (r'\bint iY = 0\b', 'int y = 0', 'TeleportEntry::iY declaration'),
        (r'\bint iCost = 0\b', 'int cost = 0', 'TeleportEntry::iCost declaration'),
    ],
    'EventListManager.h': [
        # EventEntry struct field declarations
        (r'\buint32_t dwTime = 0\b', 'uint32_t time = 0', 'EventEntry::dwTime declaration'),
        # Note: "char cColor = 0" in the struct declaration will already be
        # handled by the global "\bcColor\b" → "color" rule, yielding
        # "char color = 0". That's correct for struct data fields (no m_ prefix).
        (r'\bstd::string cTxt\b', 'std::string text', 'EventEntry::cTxt declaration'),
    ],
    'Game.h': [
        # SellItemList anonymous struct: iIndex field is accessed via .iIndex
        # which our global rule renames to .index. Declaration must match.
        # Pattern uses tab+int to avoid matching other "int iIndex" in comments etc.
        (r'(\t\t)int iIndex;', r'\1int index;', 'SellItemList::iIndex declaration in Game.h'),
    ],
}

# ---------------------------------------------------------------------------
# Only process real source files, skip .bak_* files
# ---------------------------------------------------------------------------
files = sorted(
    f for f in
    glob.glob('Z:/Helbreath-3.82/Sources/Client/*.h') +
    glob.glob('Z:/Helbreath-3.82/Sources/Client/*.cpp')
    if '.bak_' not in os.path.basename(f)
)

total_changes = 0
for f in files:
    basename = os.path.basename(f)
    with open(f, 'r', encoding='utf-8', errors='replace') as fh:
        content = fh.read()
    original = content

    # Apply global replacements (order matters: dot-prefixed before standalone)
    for pattern, replacement, desc in GLOBAL_REPLACEMENTS:
        content = re.sub(pattern, replacement, content)

    # Apply file-specific replacements
    if basename in FILE_SPECIFIC:
        for pattern, replacement, desc in FILE_SPECIFIC[basename]:
            content = re.sub(pattern, replacement, content)

    if content != original:
        with open(f, 'w', encoding='utf-8', newline='') as fh:
            fh.write(content)
        total_changes += 1
        print(f'  Updated: {basename}')

print(f'\nAgent 5 done. {total_changes} file(s) updated.')
print(f'  {len(GLOBAL_REPLACEMENTS)} global rules + {sum(len(v) for v in FILE_SPECIFIC.values())} file-specific rules.')
