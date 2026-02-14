#!/usr/bin/env python3
"""
Phase E4: Server logging migration.

Replaces ALL old logging calls (PutLogList, PutLogListLevel, PutLogFileList,
PutHackLogFileList, PutPvPLogFileList, PutXSocketLogFileList, PutLogEventFileList)
with hb::logger calls.

Also handles:
- ItemLog::Get().* calls → hb::logger with channel
- ChatLog::Get().Write() calls → hb::logger with chat channel
- extern declarations removal
- #include additions
- Wmain.cpp: removes old function defs, adds logger init/shutdown
- WINMAIN.H: removes old declarations and LOG_LEVEL_* defines

Usage:
    python Scripts/phase_e4_server_logging.py [--dry-run]
"""

import re
import os
import sys

SERVER_DIR = "Sources/Server"
DRY_RUN = "--dry-run" in sys.argv
SURVEY_MODE = "--survey" in sys.argv

# ─────────────────────────────────────────────────────────────────────
# Prefix → logger function mapping
# ─────────────────────────────────────────────────────────────────────

# Ordered longest-first to avoid partial matches
PREFIX_MAP = [
    ("(!!!)",   "error"),
    ("(XXX)",   "error"),
    ("(X)",     "error"),
    ("(!)",     "log"),
    ("(*)",     "log"),
    ("[ERROR]", "error"),
    ("[WARNING]","warn"),
    ("[NOTICE]","log"),
    ("[INFO]",  "log"),
    # Custom prefixes found in code — treat as log level
    ("(SQLITE)","error"),
    ("(WARN)",  "warn"),
    ("(O)",     "log"),
]

# PutLogListLevel level → function
LEVEL_TO_FUNC = {
    "LOG_LEVEL_INFO":    "log",
    "LOG_LEVEL_NOTICE":  "log",
    "LOG_LEVEL_WARNING": "warn",
    "LOG_LEVEL_ERROR":   "error",
    "0": "log",
    "1": "log",
    "2": "warn",
    "3": "error",
}

# Legacy file functions → (channel, default_level_func)
LEGACY_FILE_FUNCS = {
    "PutLogFileList":       ("events",     "log"),
    "PutHackLogFileList":   ("security",   "warn"),
    "PutPvPLogFileList":    ("pvp",        "log"),
    "PutXSocketLogFileList":("network",    "log"),
    "PutLogEventFileList":  ("log_events", "log"),
    "PutXSocketLogList":    ("network",    "log"),
}

# ─────────────────────────────────────────────────────────────────────
# Extern declaration patterns to remove
# ─────────────────────────────────────────────────────────────────────

EXTERN_PATTERNS = [
    re.compile(r'^\s*extern\s+void\s+PutLogList\s*\(.*?\)\s*;', re.MULTILINE),
    re.compile(r'^\s*extern\s+void\s+PutLogListLevel\s*\(.*?\)\s*;', re.MULTILINE),
    re.compile(r'^\s*extern\s+void\s+PutLogFileList\s*\(.*?\)\s*;', re.MULTILINE),
    re.compile(r'^\s*extern\s+void\s+PutHackLogFileList\s*\(.*?\)\s*;', re.MULTILINE),
    re.compile(r'^\s*extern\s+void\s+PutPvPLogFileList\s*\(.*?\)\s*;', re.MULTILINE),
    re.compile(r'^\s*extern\s+void\s+PutXSocketLogFileList\s*\(.*?\)\s*;', re.MULTILINE),
    re.compile(r'^\s*extern\s+void\s+PutLogEventFileList\s*\(.*?\)\s*;', re.MULTILINE),
    # Also forward declarations without extern
    re.compile(r'^\s*void\s+PutLogFileList\s*\(.*?\)\s*;', re.MULTILINE),
    re.compile(r'^\s*void\s+PutHackLogFileList\s*\(.*?\)\s*;', re.MULTILINE),
    re.compile(r'^\s*void\s+PutPvPLogFileList\s*\(.*?\)\s*;', re.MULTILINE),
    re.compile(r'^\s*void\s+PutXSocketLogFileList\s*\(.*?\)\s*;', re.MULTILINE),
    re.compile(r'^\s*void\s+PutLogEventFileList\s*\(.*?\)\s*;', re.MULTILINE),
]

# ─────────────────────────────────────────────────────────────────────
# Format specifier conversion: printf → std::format
# ─────────────────────────────────────────────────────────────────────

def convert_printf_to_stdformat(fmt):
    """Convert a printf-style format string to std::format style.

    Examples:
        "Hello %s, count=%d"  → "Hello {}, count={}"
        "Value: 0x%08X"       → "Value: 0x{:08X}"
        "Ratio: %.2f%%"       → "Ratio: {:.2f}%"
    """
    result = []
    i = 0
    while i < len(fmt):
        if fmt[i] == '{':
            result.append('{{')
            i += 1
        elif fmt[i] == '}':
            result.append('}}')
            i += 1
        elif fmt[i] == '%':
            if i + 1 < len(fmt) and fmt[i + 1] == '%':
                result.append('%')
                i += 2
                continue
            # Parse printf format specifier
            j = i + 1
            # Flags: -, +, space, #, 0
            while j < len(fmt) and fmt[j] in '-+ #0':
                j += 1
            # Width (digits or *)
            width_start = j
            while j < len(fmt) and fmt[j].isdigit():
                j += 1
            width = fmt[width_start:j]
            # Precision
            precision = ""
            if j < len(fmt) and fmt[j] == '.':
                j += 1
                prec_start = j
                while j < len(fmt) and fmt[j].isdigit():
                    j += 1
                precision = fmt[prec_start:j]
            # Length modifier (h, hh, l, ll, z, j, t, L, q)
            while j < len(fmt) and fmt[j] in 'hlLzjtq':
                j += 1
            # Type character
            if j < len(fmt):
                type_char = fmt[j]
                j += 1

                # Check for leading-zero flag
                has_zero = '0' in fmt[i+1:width_start+1] if width_start > i+1 else False

                if type_char in ('d', 'i', 'u'):
                    result.append('{}')
                elif type_char == 's':
                    result.append('{}')
                elif type_char in ('X', 'x'):
                    if width and (has_zero or '0' in fmt[i+1:j]):
                        result.append('{:0' + width + type_char + '}')
                    elif width:
                        result.append('{:' + width + type_char + '}')
                    else:
                        result.append('{:' + type_char + '}')
                elif type_char == 'f':
                    if precision:
                        result.append('{:.' + precision + 'f}')
                    else:
                        result.append('{}')
                elif type_char == 'c':
                    result.append('{:c}')
                elif type_char == 'p':
                    result.append('{}')
                else:
                    # Unknown specifier, use generic
                    result.append('{}')
                i = j
            else:
                # Incomplete specifier at end
                result.append(fmt[i:])
                break
        else:
            result.append(fmt[i])
            i += 1
    return ''.join(result)

# ─────────────────────────────────────────────────────────────────────
# Prefix stripping and level detection
# ─────────────────────────────────────────────────────────────────────

def strip_prefix_and_get_level(msg):
    """Strip known prefix from a message string and return (stripped_msg, level_func).

    Returns (message_without_prefix, "log"|"warn"|"error").
    """
    stripped = msg.lstrip()

    for prefix, level in PREFIX_MAP:
        if stripped.startswith(prefix):
            stripped = stripped[len(prefix):].lstrip()
            # Check for CRITICAL ERROR override
            if level == "log" and "CRITICAL ERROR" in stripped:
                level = "error"
            return stripped, level

    # No known prefix — check content
    if "CRITICAL ERROR" in stripped:
        return stripped, "error"

    return stripped, "log"

def normalize_message_text(msg):
    """Normalize and modernize log message text.

    Removes decorative markers, embedded prefix markers, emoticons.
    Fixes known misspellings. Normalizes punctuation.
    """
    # Remove decorative markers
    msg = re.sub(r'@{3,}\s*', '', msg)
    msg = re.sub(r'={3,}\s*', '', msg)

    # Remove any remaining embedded prefix markers
    for prefix, _ in PREFIX_MAP:
        escaped = re.escape(prefix)
        msg = re.sub(escaped + r'\s*', '', msg)

    # Remove embedded text markers not in PREFIX_MAP
    msg = re.sub(r'\(T_T\)\s*', '', msg)
    msg = re.sub(r'\(\^o\^\)\s*', '', msg)
    msg = re.sub(r'\(HACK\?\)\s*', '', msg)
    msg = re.sub(r'\(TestLog\)\s*', '', msg)
    msg = re.sub(r'\[WARN\]\s*', '', msg)
    msg = re.sub(r'\[NETWARN\]\s*', '', msg)

    # Fix known misspellings
    msg = msg.replace('MsgQuene', 'message queue')
    msg = msg.replace('MANUALY', 'manually')
    msg = msg.replace('notmatch', 'mismatch')
    msg = msg.replace('Confirmcode', 'Confirm code')

    # Normalize excessive punctuation
    msg = re.sub(r'!{2,}', '', msg)
    msg = re.sub(r'\.{3,}', '', msg)

    # Clean up whitespace
    msg = re.sub(r'\s{2,}', ' ', msg)
    msg = msg.strip()

    # Convert printf specifiers to std::format BEFORE rewrite lookup
    # (so MESSAGE_REWRITES keys/values all use {} format)
    msg = convert_printf_to_stdformat(msg)

    # Apply manual rewrites from MESSAGE_REWRITES table
    msg = rewrite_message(msg)

    return msg

def higher_level(a, b):
    """Return the higher severity level function name."""
    ORDER = {"debug": 0, "log": 1, "warn": 2, "error": 3}
    return a if ORDER.get(a, 1) >= ORDER.get(b, 1) else b

# ─────────────────────────────────────────────────────────────────────
# snprintf argument extraction
# ─────────────────────────────────────────────────────────────────────

def extract_snprintf_parts(snprintf_text):
    """Extract (format_string, args_string) from a snprintf call text.

    Input: 'std::snprintf(buf, sizeof(buf), "fmt %s %d", arg1, arg2)'
    Returns: ('"fmt %s %d"', 'arg1, arg2') or None if can't parse.
    """
    # Find the format string (third argument)
    # Pattern: snprintf(BUF, SIZE, "FORMAT", ARGS...)
    # We need to find the opening quote of the format string

    # Remove the function call prefix up to the format string
    # Find all comma-separated args, skipping nested parens
    depth = 0
    args = []
    current = ""
    in_string = False
    escape_next = False

    # Find the opening paren
    paren_start = snprintf_text.find('(')
    if paren_start < 0:
        return None

    text = snprintf_text[paren_start + 1:]

    for ch in text:
        if escape_next:
            current += ch
            escape_next = False
            continue
        if ch == '\\' and in_string:
            current += ch
            escape_next = True
            continue
        if ch == '"' and not escape_next:
            in_string = not in_string
            current += ch
            continue
        if in_string:
            current += ch
            continue
        if ch == '(':
            depth += 1
            current += ch
        elif ch == ')':
            if depth == 0:
                if current.strip():
                    args.append(current.strip())
                break
            depth -= 1
            current += ch
        elif ch == ',' and depth == 0:
            args.append(current.strip())
            current = ""
        else:
            current += ch

    # args[0] = buffer, args[1] = size, args[2] = format string, args[3:] = format args
    if len(args) < 3:
        return None

    fmt_str = args[2]
    # The format string should be a quoted string
    if not fmt_str.startswith('"') or not fmt_str.endswith('"'):
        return None

    # Remove quotes
    fmt_str = fmt_str[1:-1]

    # Remaining args
    fmt_args = ", ".join(args[3:]) if len(args) > 3 else ""

    return (fmt_str, fmt_args)

# ─────────────────────────────────────────────────────────────────────
# Join multi-line statements
# ─────────────────────────────────────────────────────────────────────

def count_paren_balance(text, in_block_comment=False):
    """Count net paren balance in a line of C/C++ code.

    Correctly skips:
    - "string literals" (with \\ escapes)
    - 'c' character literals (with \\ escapes)
    - // line comments (everything after //)
    - /* ... */ block comments (including multi-line)

    Args:
        text: The line of code to analyze.
        in_block_comment: If True, we're already inside a /* */ comment
            from a previous line.

    Returns:
        (balance, still_in_block_comment) tuple.
    """
    balance = 0
    i = 0
    n = len(text)
    while i < n:
        # If inside a block comment, scan for closing */
        if in_block_comment:
            if text[i] == '*' and i + 1 < n and text[i + 1] == '/':
                in_block_comment = False
                i += 2
            else:
                i += 1
            continue

        ch = text[i]

        # Line comment — stop processing rest of line
        if ch == '/' and i + 1 < n and text[i + 1] == '/':
            break

        # Block comment start
        if ch == '/' and i + 1 < n and text[i + 1] == '*':
            in_block_comment = True
            i += 2
            continue

        # String literal
        if ch == '"':
            i += 1
            while i < n:
                if text[i] == '\\':
                    i += 2  # skip escaped char
                elif text[i] == '"':
                    i += 1
                    break
                else:
                    i += 1
            continue

        # Char literal
        if ch == "'":
            i += 1
            while i < n:
                if text[i] == '\\':
                    i += 2  # skip escaped char
                elif text[i] == "'":
                    i += 1
                    break
                else:
                    i += 1
            continue

        if ch == '(':
            balance += 1
        elif ch == ')':
            balance -= 1

        i += 1

    return balance, in_block_comment


def join_continuation_lines(lines):
    """Join lines that are continuations of multi-line statements.

    Tracks /* */ block comment state across lines so parens inside
    block comments are never counted.

    Returns list of (joined_line, original_line_indices).
    """
    result = []
    i = 0
    in_block_comment = False
    while i < len(lines):
        line = lines[i]
        indices = [i]

        # Check if this line starts a multi-line statement
        # (has an opening paren/bracket that isn't closed)
        stripped = line.rstrip()
        open_count, in_block_comment = count_paren_balance(stripped, in_block_comment)

        # If parens are unbalanced, join with next lines
        while open_count > 0 and i + 1 < len(lines):
            i += 1
            indices.append(i)
            next_line = lines[i].rstrip()
            line = line.rstrip() + " " + next_line.lstrip()
            delta, in_block_comment = count_paren_balance(next_line, in_block_comment)
            open_count += delta

        result.append((line, indices))
        i += 1

    return result

# ─────────────────────────────────────────────────────────────────────
# Main processing for a single file
# ─────────────────────────────────────────────────────────────────────

# Regex patterns for different call types
RE_PUTLOGLIST_LITERAL = re.compile(
    r'^(\s*)PutLogList\s*\(\s*(?:\(char\s*\*?\s*\))?\s*"((?:[^"\\]|\\.)*)"\s*\)\s*;',
)
RE_PUTLOGLIST_VAR = re.compile(
    r'^(\s*)PutLogList\s*\(\s*(\w+)\s*\)\s*;',
)
RE_PUTLOGLISTLEVEL_LITERAL = re.compile(
    r'^(\s*)PutLogListLevel\s*\(\s*(\w+)\s*,\s*(?:\(char\s*\*?\s*\))?\s*"((?:[^"\\]|\\.)*)"\s*\)\s*;',
)
RE_PUTLOGLISTLEVEL_VAR = re.compile(
    r'^(\s*)PutLogListLevel\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)\s*;',
)
RE_LEGACY_LITERAL = re.compile(
    r'^(\s*)(PutLogFileList|PutHackLogFileList|PutPvPLogFileList|PutXSocketLogFileList|PutLogEventFileList|PutXSocketLogList)\s*\(\s*(?:\(char\s*\*?\s*\))?\s*"((?:[^"\\]|\\.)*)"\s*\)\s*;',
)
RE_LEGACY_VAR = re.compile(
    r'^(\s*)(PutLogFileList|PutHackLogFileList|PutPvPLogFileList|PutXSocketLogFileList|PutLogEventFileList|PutXSocketLogList)\s*\(\s*(\w+)\s*\)\s*;',
)
RE_SNPRINTF = re.compile(
    r'^\s*(?:std::)?snprintf\s*\(\s*(\w+)\s*,',
)

# ItemLog patterns
RE_ITEMLOG = re.compile(
    r'^(\s*)ItemLog::Get\(\)\.(Log\w+)\s*\((.*)\)\s*;',
)
# ChatLog patterns
RE_CHATLOG = re.compile(
    r'^(\s*)ChatLog::Get\(\)\.(Write|Initialize|Shutdown)\s*\((.*)\)\s*;',
)

def find_preceding_snprintf(joined_lines, current_idx, var_name, max_lookback=15):
    """Search backwards from current_idx for a snprintf writing to var_name.

    Returns (format_string, args_string, snprintf_joined_index) or None.
    """
    for j in range(current_idx - 1, max(current_idx - max_lookback, -1), -1):
        jline, _ = joined_lines[j]
        m = RE_SNPRINTF.match(jline)
        if m and m.group(1) == var_name:
            parts = extract_snprintf_parts(jline)
            if parts:
                return parts[0], parts[1], j
    return None

def make_logger_call(indent, level_func, fmt_str, args_str, channel=None):
    """Generate a hb::logger call string."""
    # Strip trailing \n from format string
    while fmt_str.endswith("\\n"):
        fmt_str = fmt_str[:-2]
    fmt_str = fmt_str.rstrip()

    if channel:
        prefix = f"hb::logger::{level_func}<log_channel::{channel}>"
    else:
        prefix = f"hb::logger::{level_func}"

    if args_str:
        return f'{indent}{prefix}("{fmt_str}", {args_str});'
    else:
        return f'{indent}{prefix}("{fmt_str}");'

# ─────────────────────────────────────────────────────────────────────
# ItemLog replacement helpers
# ─────────────────────────────────────────────────────────────────────

# Map ItemLog method → (channel, format_template)
ITEMLOG_METHOD_MAP = {
    "LogDrop":     ("drops",      "{log_func}", "{name} IP({ip}) Drop {item_info} at {map}({x},{y})"),
    "LogPickup":   ("drops",      "{log_func}", "{name} IP({ip}) Get {item_info} at {map}({x},{y})"),
    "LogTrade":    ("trade",      "{log_func}", "{suspicious}{name} IP({ip}) Give {item_info} at {map}({x},{y}) -> {receiver}"),
    "LogExchange": ("trade",      "{log_func}", "{suspicious}{name} IP({ip}) Exchange {item_info} at {map}({x},{y}) -> {receiver}"),
    "LogShop":     ("shop",       "{log_func}", "{name} IP({ip}) {action} {item_info} at {map}({x},{y})"),
    "LogCraft":    ("crafting",   "{log_func}", "{name} IP({ip}) Make {item_info} at {map}({x},{y})"),
    "LogUpgrade":  ("upgrades",   "{log_func}", "{name} IP({ip}) Upgrade {success} {item_info} at {map}({x},{y})"),
    "LogBank":     ("bank",       "{log_func}", "{name} IP({ip}) {action} {item_info} at {map}({x},{y})"),
    "LogMisc":     ("items_misc", "{log_func}", "{name} IP({ip}) {action} {item_info} at {map}({x},{y})"),
}

def parse_itemlog_args(method, args_str):
    """Parse ItemLog method arguments and return replacement code pieces."""
    # Split args respecting nested parens
    args = split_args(args_str)
    if args is None:
        return None

    channel = ITEMLOG_METHOD_MAP[method][0]

    if method in ("LogDrop", "LogPickup", "LogCraft"):
        # (playerName, ip, mapName, sX, sY, pItem)
        if len(args) < 6:
            return None
        return {
            "channel": channel,
            "args": args,
            "pattern": "simple",  # name, ip, map, x, y, pItem
        }
    elif method in ("LogTrade", "LogExchange"):
        # (giverName, giverIP, receiverName, mapName, sX, sY, pItem)
        if len(args) < 7:
            return None
        return {
            "channel": channel,
            "args": args,
            "pattern": "trade",
        }
    elif method == "LogShop":
        # (action, playerName, ip, mapName, sX, sY, pItem)
        if len(args) < 7:
            return None
        return {
            "channel": channel,
            "args": args,
            "pattern": "shop",
        }
    elif method == "LogUpgrade":
        # (success, playerName, ip, mapName, sX, sY, pItem)
        if len(args) < 7:
            return None
        return {
            "channel": channel,
            "args": args,
            "pattern": "upgrade",
        }
    elif method in ("LogBank", "LogMisc"):
        # (action, playerName, ip, mapName, sX, sY, pItem)
        if len(args) < 7:
            return None
        return {
            "channel": channel,
            "args": args,
            "pattern": "action",
        }
    return None

def split_args(args_str):
    """Split comma-separated arguments respecting nested parens and strings."""
    args = []
    current = ""
    depth = 0
    in_str = False
    esc = False
    for ch in args_str:
        if esc:
            current += ch
            esc = False
            continue
        if ch == '\\':
            esc = True
            current += ch
            continue
        if ch == '"':
            in_str = not in_str
            current += ch
            continue
        if in_str:
            current += ch
            continue
        if ch == '(':
            depth += 1
            current += ch
        elif ch == ')':
            depth -= 1
            current += ch
        elif ch == ',' and depth == 0:
            args.append(current.strip())
            current = ""
        else:
            current += ch
    if current.strip():
        args.append(current.strip())
    return args

def generate_itemlog_replacement(indent, method, args_str):
    """Generate hb::logger call to replace an ItemLog call."""
    info = parse_itemlog_args(method, args_str)
    if info is None:
        return None

    ch = info["channel"]
    a = info["args"]

    if info["pattern"] == "simple":
        # (name, ip, map, x, y, pItem)
        action = {"LogDrop": "Drop", "LogPickup": "Get", "LogCraft": "Make"}[method]
        pItem = a[5]
        if pItem.strip() == "nullptr" or pItem.strip() == "0":
            item_expr = '"(null)"'
            return f'{indent}hb::logger::log<log_channel::{ch}>("{{}}" " IP({{}}) {action} (null) at {{}}({{}},{{}})", {a[0]}, {a[1]}, {a[3]}, {a[4]}, {a[5].replace(a[5], "").strip() or a[3]});'
        # Need format_item_info helper
        return f'{indent}hb::logger::log<log_channel::{ch}>("{{}} IP({{}}) {action} {{}} at {{}}({{}},{{}})", {a[0]}, {a[1]}, format_item_info({pItem}), {a[2]}, {a[3]}, {a[4]});'

    elif info["pattern"] == "trade":
        # (giver, giverIP, receiver, map, x, y, pItem)
        action = {"LogTrade": "Give", "LogExchange": "Exchange"}[method]
        pItem = a[6]
        return f'{indent}hb::logger::log<log_channel::{ch}>("{{}}{{}} IP({{}}) {action} {{}} at {{}}({{}},{{}}) -> {{}}", is_item_suspicious({pItem}) ? "[SUSPICIOUS] " : "", {a[0]}, {a[1]}, format_item_info({pItem}), {a[3]}, {a[4]}, {a[5]}, {a[2]});'

    elif info["pattern"] == "shop":
        # (action, name, ip, map, x, y, pItem)
        pItem = a[6]
        return f'{indent}hb::logger::log<log_channel::{ch}>("{{}} IP({{}}) {{}} {{}} at {{}}({{}},{{}})", {a[1]}, {a[2]}, {a[0]}, format_item_info({pItem}), {a[3]}, {a[4]}, {a[5]});'

    elif info["pattern"] == "upgrade":
        # (success, name, ip, map, x, y, pItem)
        pItem = a[6]
        return f'{indent}hb::logger::log<log_channel::{ch}>("{{}} IP({{}}) Upgrade {{}} {{}} at {{}}({{}},{{}})", {a[1]}, {a[2]}, {a[0]} ? "Success" : "Fail", format_item_info({pItem}), {a[3]}, {a[4]}, {a[5]});'

    elif info["pattern"] == "action":
        # (action, name, ip, map, x, y, pItem)
        pItem = a[6]
        if pItem.strip() == "nullptr" or pItem.strip() == "0":
            return f'{indent}hb::logger::log<log_channel::{ch}>("{{}} IP({{}}) {{}} (null) at {{}}({{}},{{}})", {a[1]}, {a[2]}, {a[0]}, {a[3]}, {a[4]}, {a[5]});'
        return f'{indent}hb::logger::log<log_channel::{ch}>("{{}} IP({{}}) {{}} {{}} at {{}}({{}},{{}})", {a[1]}, {a[2]}, {a[0]}, format_item_info({pItem}), {a[3]}, {a[4]}, {a[5]});'

    return None

# ─────────────────────────────────────────────────────────────────────
# ChatLog replacement
# ─────────────────────────────────────────────────────────────────────

CHAT_LABELS = {
    "0":  "Local",
    "1":  "Guild",
    "2":  "Global",
    "3":  "Alliance",
    "4":  "Party",
    "10": "Broadcast",
    "20": "Whisper",
}

# ─────────────────────────────────────────────────────────────────────
# Process one file
# ─────────────────────────────────────────────────────────────────────

def process_file(filepath):
    """Process a single server .cpp file. Returns (new_content, changes_count) or None."""
    with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
        original_lines = f.readlines()

    # Strip trailing newlines for processing, re-add later
    lines = [l.rstrip('\n').rstrip('\r') for l in original_lines]

    basename = os.path.basename(filepath)
    changes = 0
    lines_to_delete = set()  # Line indices to remove
    line_replacements = {}   # Line index → replacement string
    needs_log_include = False
    needs_channel_include = False
    needs_format_item_info = False  # Whether we need the item info helper

    # Join multi-line statements for analysis
    joined = join_continuation_lines(lines)

    # Pre-scan: mark lines that are (partially or fully) inside /* */ block comments
    # These must NEVER be modified by any regex
    in_block_comment_lines = set()
    in_bc = False
    for li, line in enumerate(lines):
        was_in_bc = in_bc  # True if line starts inside a block comment
        i = 0
        n = len(line)
        while i < n:
            if in_bc:
                if line[i] == '*' and i + 1 < n and line[i + 1] == '/':
                    in_bc = False
                    i += 2
                else:
                    i += 1
            else:
                if line[i] == '/' and i + 1 < n and line[i + 1] == '/':
                    break  # rest is line comment, not block
                if line[i] == '/' and i + 1 < n and line[i + 1] == '*':
                    in_bc = True
                    i += 2
                elif line[i] == '"':
                    i += 1
                    while i < n:
                        if line[i] == '\\':
                            i += 2
                        elif line[i] == '"':
                            i += 1
                            break
                        else:
                            i += 1
                elif line[i] == "'":
                    i += 1
                    while i < n:
                        if line[i] == '\\':
                            i += 2
                        elif line[i] == "'":
                            i += 1
                            break
                        else:
                            i += 1
                else:
                    i += 1
        # Mark any line that was inside a block comment at any point
        if was_in_bc or in_bc:
            in_block_comment_lines.add(li)

    # Track which joined-line indices have been consumed (for snprintf deletion)
    snprintf_consumed = set()

    # Also track PutLogList calls that are ALSO paired with a file function
    # (e.g., PutLogList(buf) followed by PutLogFileList(buf) — skip the PutLogList
    # because the channel call will aggregate to main log)
    paired_putloglist = set()

    # Pre-scan for paired patterns: PutLogList(var) immediately followed by PutLogFileList(var)
    for idx in range(len(joined) - 1):
        jline, jindices = joined[idx]
        next_jline, next_jindices = joined[idx + 1]

        m_pll = RE_PUTLOGLIST_VAR.match(jline)
        m_legacy = RE_LEGACY_VAR.match(next_jline)
        if m_pll and m_legacy:
            if m_pll.group(2) == m_legacy.group(3):
                # Same buffer — PutLogList is redundant (channel aggregates to main)
                for li in jindices:
                    paired_putloglist.add(li)

    # Also check reverse: PutLogFileList followed by PutLogList with same var
    for idx in range(len(joined) - 1):
        jline, jindices = joined[idx]
        next_jline, next_jindices = joined[idx + 1]

        m_legacy = RE_LEGACY_VAR.match(jline)
        m_pll = RE_PUTLOGLIST_VAR.match(next_jline)
        if m_legacy and m_pll:
            if m_legacy.group(3) == m_pll.group(2):
                for li in next_jindices:
                    paired_putloglist.add(li)

    # Process each joined line
    for idx, (jline, jindices) in enumerate(joined):
        primary_line = jindices[0]
        indent_match = re.match(r'^(\s*)', jline)
        indent = indent_match.group(1) if indent_match else "\t"

        # Skip lines already consumed or marked for deletion
        if any(li in lines_to_delete for li in jindices):
            continue

        # Skip lines inside /* */ block comments
        if primary_line in in_block_comment_lines:
            continue

        # ── PutLogList with literal string ──
        m = RE_PUTLOGLIST_LITERAL.match(jline)
        if m:
            indent = m.group(1)
            msg = m.group(2)

            # Skip blank/whitespace-only messages
            if not msg.strip() or msg.strip() in ('.', '..', '...', '....'):
                for li in jindices:
                    lines_to_delete.add(li)
                changes += 1
                continue

            stripped_msg, level_func = strip_prefix_and_get_level(msg)
            stripped_msg = normalize_message_text(stripped_msg)
            # Strip trailing \n
            while stripped_msg.endswith("\\n"):
                stripped_msg = stripped_msg[:-2]

            # normalize_message_text already escapes { } via convert_printf_to_stdformat
            replacement = f'{indent}hb::logger::{level_func}("{stripped_msg}");'
            line_replacements[primary_line] = replacement
            for li in jindices[1:]:
                lines_to_delete.add(li)
            needs_log_include = True
            changes += 1
            continue

        # ── PutLogList with variable ──
        m = RE_PUTLOGLIST_VAR.match(jline)
        if m:
            # Check if this is a paired call (redundant with file function)
            if primary_line in paired_putloglist:
                for li in jindices:
                    lines_to_delete.add(li)
                changes += 1
                continue

            indent = m.group(1)
            var_name = m.group(2)

            # Search for preceding snprintf
            result = find_preceding_snprintf(joined, idx, var_name)
            if result:
                fmt_str, args_str, snprintf_idx = result

                # Strip prefix and determine level
                stripped_fmt, level_func = strip_prefix_and_get_level(fmt_str)
                converted_fmt = normalize_message_text(stripped_fmt)
                # Strip trailing \n
                while converted_fmt.endswith("\\n"):
                    converted_fmt = converted_fmt[:-2]

                replacement = make_logger_call(indent, level_func, converted_fmt, args_str)
                line_replacements[primary_line] = replacement
                for li in jindices[1:]:
                    lines_to_delete.add(li)

                # Delete snprintf line(s) if not already consumed
                if snprintf_idx not in snprintf_consumed:
                    snprintf_consumed.add(snprintf_idx)
                    for li in joined[snprintf_idx][1]:
                        lines_to_delete.add(li)

                needs_log_include = True
                changes += 1
            else:
                # No snprintf found — passthrough the variable
                replacement = f'{indent}hb::logger::log("{{}}", {var_name});'
                line_replacements[primary_line] = replacement
                for li in jindices[1:]:
                    lines_to_delete.add(li)
                needs_log_include = True
                changes += 1
            continue

        # ── PutLogListLevel with literal ──
        m = RE_PUTLOGLISTLEVEL_LITERAL.match(jline)
        if m:
            indent = m.group(1)
            level_const = m.group(2)
            msg = m.group(3)

            level_from_const = LEVEL_TO_FUNC.get(level_const, "log")
            stripped_msg, level_from_prefix = strip_prefix_and_get_level(msg)
            level_func = higher_level(level_from_const, level_from_prefix)
            stripped_msg = normalize_message_text(stripped_msg)
            # Strip trailing \n
            while stripped_msg.endswith("\\n"):
                stripped_msg = stripped_msg[:-2]

            replacement = f'{indent}hb::logger::{level_func}("{stripped_msg}");'
            line_replacements[primary_line] = replacement
            for li in jindices[1:]:
                lines_to_delete.add(li)
            needs_log_include = True
            changes += 1
            continue

        # ── PutLogListLevel with variable ──
        m = RE_PUTLOGLISTLEVEL_VAR.match(jline)
        if m:
            indent = m.group(1)
            level_const = m.group(2)
            var_name = m.group(3)

            level_from_const = LEVEL_TO_FUNC.get(level_const, "log")

            result = find_preceding_snprintf(joined, idx, var_name)
            if result:
                fmt_str, args_str, snprintf_idx = result
                stripped_fmt, level_from_prefix = strip_prefix_and_get_level(fmt_str)
                level_func = higher_level(level_from_const, level_from_prefix)
                converted_fmt = normalize_message_text(stripped_fmt)
                while converted_fmt.endswith("\\n"):
                    converted_fmt = converted_fmt[:-2]

                replacement = make_logger_call(indent, level_func, converted_fmt, args_str)
                line_replacements[primary_line] = replacement
                for li in jindices[1:]:
                    lines_to_delete.add(li)

                if snprintf_idx not in snprintf_consumed:
                    snprintf_consumed.add(snprintf_idx)
                    for li in joined[snprintf_idx][1]:
                        lines_to_delete.add(li)

                needs_log_include = True
                changes += 1
            else:
                replacement = f'{indent}hb::logger::{level_from_const}("{{}}", {var_name});'
                line_replacements[primary_line] = replacement
                for li in jindices[1:]:
                    lines_to_delete.add(li)
                needs_log_include = True
                changes += 1
            continue

        # ── Legacy file functions with literal ──
        m = RE_LEGACY_LITERAL.match(jline)
        if m:
            indent = m.group(1)
            func_name = m.group(2)
            msg = m.group(3)

            channel, default_level = LEGACY_FILE_FUNCS.get(func_name, ("events", "log"))
            stripped_msg, level_func = strip_prefix_and_get_level(msg)
            stripped_msg = normalize_message_text(stripped_msg)
            # Override level if the channel has a specific default
            if level_func == "log" and default_level != "log":
                level_func = default_level

            while stripped_msg.endswith("\\n"):
                stripped_msg = stripped_msg[:-2]

            replacement = f'{indent}hb::logger::{level_func}<log_channel::{channel}>("{stripped_msg}");'
            line_replacements[primary_line] = replacement
            for li in jindices[1:]:
                lines_to_delete.add(li)
            needs_log_include = True
            needs_channel_include = True
            changes += 1
            continue

        # ── Legacy file functions with variable ──
        m = RE_LEGACY_VAR.match(jline)
        if m:
            indent = m.group(1)
            func_name = m.group(2)
            var_name = m.group(3)

            channel, default_level = LEGACY_FILE_FUNCS.get(func_name, ("events", "log"))

            result = find_preceding_snprintf(joined, idx, var_name)
            if result:
                fmt_str, args_str, snprintf_idx = result
                stripped_fmt, level_func = strip_prefix_and_get_level(fmt_str)
                converted_fmt = normalize_message_text(stripped_fmt)
                if level_func == "log" and default_level != "log":
                    level_func = default_level

                while converted_fmt.endswith("\\n"):
                    converted_fmt = converted_fmt[:-2]

                replacement = make_logger_call(indent, level_func, converted_fmt, args_str, channel)
                line_replacements[primary_line] = replacement
                for li in jindices[1:]:
                    lines_to_delete.add(li)

                if snprintf_idx not in snprintf_consumed:
                    snprintf_consumed.add(snprintf_idx)
                    for li in joined[snprintf_idx][1]:
                        lines_to_delete.add(li)

                needs_log_include = True
                needs_channel_include = True
                changes += 1
            else:
                replacement = f'{indent}hb::logger::{default_level}<log_channel::{channel}>("{{}}", {var_name});'
                line_replacements[primary_line] = replacement
                for li in jindices[1:]:
                    lines_to_delete.add(li)
                needs_log_include = True
                needs_channel_include = True
                changes += 1
            continue

        # ── ItemLog calls ──
        m = RE_ITEMLOG.match(jline)
        if m:
            indent = m.group(1)
            method = m.group(2)
            args_str = m.group(3)

            if method == "Initialize":
                # Remove initialization call
                for li in jindices:
                    lines_to_delete.add(li)
                changes += 1
                continue

            replacement = generate_itemlog_replacement(indent, method, args_str)
            if replacement:
                line_replacements[primary_line] = replacement
                for li in jindices[1:]:
                    lines_to_delete.add(li)
                needs_log_include = True
                needs_channel_include = True
                needs_format_item_info = True
                changes += 1
            continue

        # ── ChatLog calls ──
        m = RE_CHATLOG.match(jline)
        if m:
            indent = m.group(1)
            method = m.group(2)
            args_str = m.group(3)

            if method in ("Initialize", "Shutdown"):
                for li in jindices:
                    lines_to_delete.add(li)
                changes += 1
                continue

            if method == "Write":
                args = split_args(args_str)
                if args and len(args) >= 4:
                    chat_type = args[0].strip()
                    player = args[1].strip()
                    map_name = args[2].strip()
                    message = args[3].strip()
                    whisper_target = args[4].strip() if len(args) > 4 else "nullptr"

                    label = CHAT_LABELS.get(chat_type, None)

                    if chat_type == "20" or (whisper_target != "nullptr" and whisper_target != "0"):
                        if label:
                            replacement = f'{indent}hb::logger::log<log_channel::chat>("[{label}] {{}} -> {{}}: {{}}", {player}, {whisper_target}, {message});'
                        else:
                            replacement = f'{indent}hb::logger::log<log_channel::chat>("[type:{{}}] {{}} -> {{}}: {{}}", {chat_type}, {player}, {whisper_target}, {message});'
                    elif chat_type == "0":
                        replacement = f'{indent}hb::logger::log<log_channel::chat>("[Local] {{}} ({{}}): {{}}", {player}, {map_name}, {message});'
                    elif label:
                        replacement = f'{indent}hb::logger::log<log_channel::chat>("[{label}] {{}}: {{}}", {player}, {message});'
                    else:
                        replacement = f'{indent}hb::logger::log<log_channel::chat>("[type:{{}}] {{}}: {{}}", {chat_type}, {player}, {message});'

                    line_replacements[primary_line] = replacement
                    for li in jindices[1:]:
                        lines_to_delete.add(li)
                    needs_log_include = True
                    needs_channel_include = True
                    changes += 1
            continue

    # ── Build new file content ──
    new_lines = []
    for i, line in enumerate(lines):
        if i in lines_to_delete:
            continue
        if i in line_replacements:
            new_lines.append(line_replacements[i])
        else:
            new_lines.append(line)

    # ── Remove extern declarations ──
    content = '\n'.join(new_lines)
    for pat in EXTERN_PATTERNS:
        new_content = pat.sub('', content)
        if new_content != content:
            changes += 1
            content = new_content

    if changes == 0:
        return None

    # Clean up consecutive blank lines (more than 2 → 2)
    content = re.sub(r'\n{3,}', '\n\n', content)

    # ── Add includes ──
    if needs_log_include:
        # Find a good insertion point (after existing #include block)
        new_lines = content.split('\n')
        insert_idx = 0
        last_include = -1
        for i, line in enumerate(new_lines):
            if line.strip().startswith('#include'):
                last_include = i

        if last_include >= 0:
            insert_idx = last_include + 1

        includes_to_add = []
        if needs_log_include and '#include "Log.h"' not in content:
            includes_to_add.append('#include "Log.h"')
        if needs_channel_include and '#include "ServerLogChannels.h"' not in content:
            includes_to_add.append('#include "ServerLogChannels.h"')

        if includes_to_add:
            for inc in reversed(includes_to_add):
                new_lines.insert(insert_idx, inc)

        # Add using declaration if channel include was added
        if needs_channel_include and 'using hb::log_channel;' not in content:
            # Find insertion point after includes
            for i, line in enumerate(new_lines):
                if line.strip().startswith('#include'):
                    last_include = i
            if last_include >= 0:
                # Add after last include, with blank line
                new_lines.insert(last_include + 1 + len(includes_to_add), '')
                new_lines.insert(last_include + 2 + len(includes_to_add), 'using hb::log_channel;')

        content = '\n'.join(new_lines)

    # Remove old includes that are no longer needed
    if basename not in ("Wmain.cpp",):
        content = re.sub(r'^\s*#include\s+"ItemLog\.h"\s*\n', '', content, flags=re.MULTILINE)
        content = re.sub(r'^\s*#include\s+"ChatLog\.h"\s*\n', '', content, flags=re.MULTILINE)

    # Clean up again
    content = re.sub(r'\n{3,}', '\n\n', content)

    # Ensure file ends with newline
    if not content.endswith('\n'):
        content += '\n'

    return content, changes

# ─────────────────────────────────────────────────────────────────────
# Special: Wmain.cpp
# ─────────────────────────────────────────────────────────────────────

def process_wmain(filepath, content=None):
    """Special processing for Wmain.cpp:
    - Remove old logging function implementations
    - Remove old forward declarations
    - Add logger initialization/shutdown
    - Replace remaining PutLogList calls in Initialize/OnDestroy
    """
    if content is None:
        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()

    lines = content.split('\n')
    new_lines = []
    skip_until_next_function = False
    brace_depth = 0
    in_old_function = False

    # Set of old function signatures that mark the start of function bodies to delete
    OLD_FUNC_SIGS = [
        'void PutLogListLevel(',
        'void PutLogList(',
        'void PutXSocketLogList(',
        'void PutLogFileList(',
        'void PutHackLogFileList(',
        'void PutPvPLogFileList(',
        'void PutXSocketLogFileList(',
        'void PutLogEventFileList(',
    ]

    # Track if we're inside the anonymous namespace with old helpers
    in_anon_namespace = False
    anon_brace_depth = 0

    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        # Detect start of anonymous namespace containing old logging helpers
        if stripped == 'namespace' or stripped.startswith('namespace\t') or stripped.startswith('namespace '):
            # Check if this is the anonymous namespace with GetLevelName
            if stripped in ('namespace', 'namespace {') or (
                i + 1 < len(lines) and lines[i + 1].strip() == '{'):
                # Look ahead to see if GetLevelName is within ~5 lines
                lookahead = '\n'.join(lines[i:i+10])
                if 'GetLevelName' in lookahead:
                    in_anon_namespace = True
                    anon_brace_depth = 0
                    # Skip the namespace line and opening brace
                    if '{' in stripped:
                        anon_brace_depth = 1
                    i += 1
                    continue

        if in_anon_namespace:
            for ch in stripped:
                if ch == '{':
                    anon_brace_depth += 1
                elif ch == '}':
                    anon_brace_depth -= 1
            if anon_brace_depth <= 0:
                in_anon_namespace = False
            i += 1
            continue

        # Detect start of old function implementations
        is_old_func = False
        for sig in OLD_FUNC_SIGS:
            if sig in stripped:
                is_old_func = True
                break

        if is_old_func:
            in_old_function = True
            brace_depth = 0
            # Count braces on this line
            for ch in stripped:
                if ch == '{':
                    brace_depth += 1
                elif ch == '}':
                    brace_depth -= 1
            # If function body didn't start on this line, skip lines until we find it
            i += 1
            continue

        if in_old_function:
            for ch in stripped:
                if ch == '{':
                    brace_depth += 1
                elif ch == '}':
                    brace_depth -= 1
            if brace_depth <= 0 and '{' in ''.join(lines[max(0, i-20):i+1]):
                in_old_function = False
            i += 1
            continue

        # Skip comment blocks for removed functions
        if stripped.startswith('/*') and 'PutLogFileList' in stripped:
            # Skip until end of comment block
            while i < len(lines) and '*/' not in lines[i]:
                i += 1
            i += 1  # skip the */ line too
            continue

        # Remove forward declarations
        if stripped in (
            'void PutHackLogFileList(char* cStr);',
            'void PutPvPLogFileList(char* cStr);',
        ):
            i += 1
            continue

        # Remove old includes
        if stripped in ('#include "ChatLog.h"', '#include "ItemLog.h"'):
            i += 1
            continue

        # Remove UpdateScreen/OnPaint removed comments
        if stripped.startswith('// UpdateScreen()') and 'REMOVED' in stripped:
            i += 1
            continue
        if stripped.startswith('// OnPaint()') and 'REMOVED' in stripped:
            i += 1
            continue

        new_lines.append(line)
        i += 1

    content = '\n'.join(new_lines)

    # Add #include "Log.h" and "ServerLogChannels.h" after existing includes
    if '#include "Log.h"' not in content:
        content = content.replace(
            '#include "ConcurrentMsgQueue.h"\n',
            '#include "ConcurrentMsgQueue.h"\n#include "Log.h"\n#include "ServerLogChannels.h"\n'
        )

    # Add using declaration
    if 'using hb::log_channel;' not in content:
        content = content.replace(
            'using namespace hb::server::config;\n',
            'using namespace hb::server::config;\nusing hb::log_channel;\n'
        )

    # Replace Initialize() contents
    content = content.replace(
        'ChatLog::Get().Initialize();\n',
        ''
    )
    content = content.replace(
        '\tItemLog::Get().Initialize();\n',
        ''
    )

    # Replace PutLogList("(!!!) STOPPED!") call
    content = content.replace(
        'PutLogList("(!!!) STOPPED!");',
        'hb::logger::error("Server initialization failed");'
    )

    # Add logger initialization early in Initialize()
    content = content.replace(
        'G_pIOPool = new hb::shared::net::IOServicePool(4);',
        'hb::logger::initialize("GameLogs/");\n\n\tG_pIOPool = new hb::shared::net::IOServicePool(4);'
    )

    # Add logger shutdown in OnDestroy()
    if 'hb::logger::shutdown()' not in content:
        content = content.replace(
            'PostQuitMessage(0);',
            'hb::logger::shutdown();\n\tPostQuitMessage(0);'
        )

    # Clean up consecutive blank lines
    content = re.sub(r'\n{3,}', '\n\n', content)

    if not content.endswith('\n'):
        content += '\n'

    return content, 1

# ─────────────────────────────────────────────────────────────────────
# Special: WINMAIN.H
# ─────────────────────────────────────────────────────────────────────

def process_winmain_h(filepath):
    """Remove old logging declarations and LOG_LEVEL_* from WINMAIN.H."""
    with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()

    # Remove LOG_LEVEL_* defines
    content = re.sub(r'^// Log levels\n', '', content, flags=re.MULTILINE)
    content = re.sub(r'^#define LOG_LEVEL_\w+\s+\d+\n', '', content, flags=re.MULTILINE)

    # Remove function declarations
    content = re.sub(r'^void PutXSocketLogList\(char\* cMsg\);\n', '', content, flags=re.MULTILINE)
    content = re.sub(r'^void PutXSocketLogFileList\(char\* cMsg\);\n', '', content, flags=re.MULTILINE)
    content = re.sub(r'^void PutLogList\(char\* cMsg\);\n', '', content, flags=re.MULTILINE)
    content = re.sub(r'^void PutLogListLevel\(int level, const char\* cMsg\);\n', '', content, flags=re.MULTILINE)

    # Clean up blank lines
    content = re.sub(r'\n{3,}', '\n\n', content)

    return content, 1

# ─────────────────────────────────────────────────────────────────────
# Message rewrite table — old (after prefix strip) → new
# Populated via --survey mode output review.
# If a message is not in this table, normalize_message_text() is used.
# ─────────────────────────────────────────────────────────────────────

MESSAGE_REWRITES = {
    # ── Critical startup errors ──
    "CRITICAL ERROR! Cannot execute server! GameConfigs.db unavailable.":
        "Cannot start server: GameConfigs.db unavailable",
    "CRITICAL ERROR! Cannot execute server! GameConfigs.db missing configuration data.":
        "Cannot start server: GameConfigs.db missing configuration data",
    "CRITICAL ERROR! Cannot execute server! Program configs missing in GameConfigs.db.":
        "Cannot start server: program configs missing in GameConfigs.db",
    "CRITICAL ERROR! Cannot execute server! Settings configs missing in GameConfigs.db.":
        "Cannot start server: settings configs missing in GameConfigs.db",
    "CRITICAL ERROR! Cannot execute server! Banned list unavailable in GameConfigs.db.":
        "Cannot start server: banned list unavailable in GameConfigs.db",
    "CRITICAL ERROR! Cannot execute server! Item configs missing in GameConfigs.db.":
        "Cannot start server: item configs missing in GameConfigs.db",
    "CRITICAL ERROR! Cannot execute server! BuildItem configs missing in GameConfigs.db.":
        "Cannot start server: build item configs missing in GameConfigs.db",
    "CRITICAL ERROR! Cannot execute server! NPC configs missing in GameConfigs.db.":
        "Cannot start server: NPC configs missing in GameConfigs.db",
    "CRITICAL ERROR! Cannot execute server! Drop tables missing in GameConfigs.db.":
        "Cannot start server: drop tables missing in GameConfigs.db",
    "CRITICAL ERROR! Cannot execute server! Magic configs missing in GameConfigs.db.":
        "Cannot start server: magic configs missing in GameConfigs.db",
    "CRITICAL ERROR! Cannot execute server! Skill configs missing in GameConfigs.db.":
        "Cannot start server: skill configs missing in GameConfigs.db",
    "CRITICAL ERROR! Cannot execute server! Quest configs missing in GameConfigs.db.":
        "Cannot start server: quest configs missing in GameConfigs.db",
    "CRITICAL ERROR! Cannot execute server! Potion/Crafting configs missing in GameConfigs.db.":
        "Cannot start server: potion/crafting configs missing in GameConfigs.db",
    "WARNING! Crusade/Schedule configs missing in GameConfigs.db.":
        "Crusade/schedule configs missing in GameConfigs.db",
    "CRITICAL ERROR in message queue":
        "Critical error in message queue",
    "GameConfigs.db unavailable; cannot reload configs.":
        "GameConfigs.db unavailable, cannot reload configs",
    "CRITICAL ERROR: MapInfo.db not available!":
        "MapInfo.db not available",

    # ── Server lifecycle ──
    "GAME SERVER SHUTDOWN PROCESS BEGIN(by Local command)":
        "Server shutdown initiated (local command)",
    "GAME SERVER SHUTDOWN PROCESS COMPLETED! All players are disconnected.":
        "Server shutdown complete, all players disconnected",
    "Send server shutdown announcement1":
        "Sending shutdown announcement (phase 1)",
    "Send server shutdown announcement2":
        "Sending shutdown announcement (phase 2)",
    "Game Server Activated.":
        "Game server activated",
    "STOPPED!":
        "Server initialization failed",
    "RELOADING CONFIGS manually":
        "Reloading all configs",
    "Resume Crusade Mode":
        "Resuming crusade mode",
    "ManualEndCrusadeMode: side 0":
        "Manual end crusade mode: side 0",
    "Final Shutdown%d":
        "Final shutdown countdown: %d",
    "AUTO-SERVER-REBOOTING!":
        "Auto-rebooting server",

    # ── Connection/client messages ──
    "Closed Connection, not initialized":
        "Connection closed (not initialized)",
    "<%d> IP Banned: (%s)":
        "Client %d: IP banned (%s)",
    "<%d> Client Connected: (%s)":
        "Client %d: connected from %s",
    "Client %d Socket BLOCKED (send buffer full)":
        "Client %d: socket blocked (send buffer full)",
    "<%d> Confirm code mismatch!":
        "Client %d: confirm code mismatch",
    "<%d> Client Disconnected! MSGSIZETOOLARGE (%s)":
        "Client %d: disconnected, message too large (%s)",
    "<%d> Client Disconnected! SOCKETERROR (%s) WSA=%d LastMsg=0x%08X LastMsgAge=%dms LastMsgSize=%zu CharName=%s":
        "Client %d: disconnected, socket error (%s) WSA=%d last_msg=0x%08X age=%dms size=%zu char=%s",
    "<%d> Client Disconnected! SOCKETCLOSED (%s) WSA=%d TimeSinceLastPacket=%dms LastMsg=0x%08X LastMsgAge=%dms LastMsgSize=%zu CharName=%s":
        "Client %d: disconnected, socket closed (%s) WSA=%d idle=%dms last_msg=0x%08X age=%dms size=%zu char=%s",
    "<%d> Client Disconnected! CRITICALERROR (%s) WSA=%d LastMsg=0x%08X LastMsgAge=%dms LastMsgSize=%zu CharName=%s":
        "Client %d: disconnected, critical error (%s) WSA=%d last_msg=0x%08X age=%dms size=%zu char=%s",
    "<%d> Client Disconnected! Async error=%d (%s) CharName=%s":
        "Client %d: disconnected, async error=%d (%s) char=%s",
    "Client Timeout: %s":
        "Client timeout: %s",
    "<%d> InitPlayerData error - Socket error! Disconnected.":
        "Client %d: init player data failed (socket error), disconnected",
    "<%d> InitPlayerData - Socket error! Disconnected.":
        "Client %d: init player data socket error, disconnected",
    "<%d> Confirm code Login mismatch!":
        "Client %d: login confirm code mismatch",
    "Client (%s) connection closed!. Sniffer suspect!.":
        "Client '%s' connection closed, sniffer suspected",
    "<%d> Force disconnect account: CharName(%s) AccntName(%s) Count(%d)":
        "Client %d: force disconnect char=%s account=%s count=%d",

    # ── NPC/Entity messages ──
    "Not existing NPC creation request! (config_id=%d) Ignored.":
        "Invalid NPC creation request (config_id=%d), ignored",
    "[SPAWN] ERROR: No free entity slots! Used: %d/%d, ActiveList: %d, TotalEntities: %d":
        "No free entity slots: used=%d/%d active=%d total=%d",
    "Enemy Npc Killed by player! Construction: +%d WarContribution: +%d":
        "Enemy NPC killed by player, construction +%d, war contribution +%d",
    "Friendly Npc Killed by player! WarContribution: -%d":
        "Friendly NPC killed by player, war contribution -%d",
    "Enemy Npc Killed by Npc! Construct point +%d":
        "Enemy NPC killed by NPC, construction +%d",
    "Enemy Player Killed by Npc! Construction +%d":
        "Enemy player killed by NPC, construction +%d",
    "Enemy Player Killed by Player! Construction: +%d WarContribution +%d":
        "Enemy player killed by player, construction +%d, war contribution +%d",
    "Construction Complete! WarContribution: +%d":
        "Construction complete, war contribution +%d",
    "ManaStock down: %d":
        "Mana stock reduced: %d",
    "Aresden Tower Broken, Left TOWER %d":
        "Aresden tower destroyed, remaining: %d",
    "Elvine Tower Broken, Left TOWER %d":
        "Elvine tower destroyed, remaining: %d",

    # ── Unknown/invalid message handling ──
    "Unknown message received: (0x%.8X) PC(unknown).":
        "Unknown message: 0x%.8X (no player)",
    "Unknown message received! (0x%.8X)":
        "Unknown message: 0x%.8X",
    "Unknown login message received! (0x%.8X) Delete Client":
        "Unknown login message: 0x%.8X, disconnecting client",
    "Not existing character(%s) data request! Rejected!":
        "Non-existent character '%s' data request, rejected",
    "Non-existing player data received from Log server: CharName(%s)":
        "Non-existent player data from login server: %s",
    "Non-existing player data received from Log server(2): CharName(%s)":
        "Non-existent player data from login server: %s",
    "Character(%s) data error!":
        "Character '%s' data error",
    "Error! Account(%s)-Level(%d) password(or level) mismatch! Disconnect.":
        "Account '%s' level %d: password or level mismatch, disconnecting",

    # ── Guild messages ──
    "New guild(%s) creation success! : character(%s)":
        "Guild '%s' created by %s",
    "New guild(%s) creation Fail! : character(%s)":
        "Guild '%s' creation failed for %s",
    "Cannot create guild! Already guild member.: CharName(%s)":
        "Cannot create guild: already a guild member (%s)",
    "Cannot Disband guild! Not guildmaster.: CharName(%s)":
        "Cannot disband guild: not guildmaster (%s)",
    "Disband guild(%s) success! : character(%s)":
        "Guild '%s' disbanded by %s",
    "Disband guild(%s) Fail! : character(%s)":
        "Guild '%s' disband failed for %s",
    "Cannot create new guild - Already existing guild name: Name(%s)":
        "Cannot create guild: name already exists (%s)",
    "Cannot create new guild - cannot create file : Name(%s)":
        "Cannot create guild: file creation failed (%s)",
    "New guild created : Name(%s)":
        "Guild created: %s",
    "Disband Guild - Deleting guild file : Name(%s)":
        "Guild disbanded, deleting file: %s",

    # ── Player stat restoration ──
    "RestorePlayerCharacteristics(Minor) FAIL! Player(%s)-(%d/%d)":
        "Stat restoration (minor) failed: player=%s (%d/%d)",
    "RestorePlayerCharacteristics(Minor) SUCCESS! Player(%s)-(%d/%d)":
        "Stat restoration (minor) succeeded: player=%s (%d/%d)",
    "RestorePlayerCharacteristics(Over) FAIL! Player(%s)-(%d/%d)":
        "Stat restoration (overflow) failed: player=%s (%d/%d)",
    "RestorePlayerCharacteristics(Over) SUCCESS! Player(%s)-(%d/%d)":
        "Stat restoration (overflow) succeeded: player=%s (%d/%d)",

    # ── Player/map events ──
    "Player(%s) tries to enter unknown map : %s":
        "Player '%s' tried to enter unknown map: %s",
    "Char(%s)-Exit(%s)":
        "Player '%s' exited map '%s'",
    "Char(%s)-Enter(%s) Observer(%d)":
        "Player '%s' entered map '%s' (observer=%d)",
    "Notify Message list file not found!":
        "Notify message list file not found",
    "Reading Notify Message list file":
        "Reading notify message list file",
    "Majestic: Char(%s) cost(%d) Str(%d) Vit(%d) Dex(%d) Int(%d) Mag(%d) Chr(%d)":
        "Majestic stat upgrade: player=%s cost=%d STR=%d VIT=%d DEX=%d INT=%d MAG=%d CHR=%d",
    "Gail asked to create a wrong item!":
        "NPC craft request for invalid item",

    # ── Admin/security ──
    "SECURITY: Admin IP mismatch for account %s (expected %s, got %s)":
        "Admin IP mismatch for account %s (expected %s, got %s)",
    "Admin IP auto-set for account %s to %s":
        "Admin IP auto-set: account=%s ip=%s",
    "Admin list is full!":
        "Admin list is full",
    "Warning: Could not load admin config from GameConfigs.db. Admin list empty.":
        "Could not load admin config from GameConfigs.db, admin list empty",
    "Warning: Could not load command permissions from GameConfigs.db.":
        "Could not load command permissions from GameConfigs.db",
    "Warning: BuildItem '%s' has invalid item_id %d":
        "BuildItem '%s' has invalid item_id %d",
    "Shop data not configured - NPCs will not have shop inventories.":
        "Shop data not configured, NPCs will not have shop inventories",

    # ── Config reload ──
    "NPC config reload FAILED - GameConfigs.db unavailable":
        "NPC config reload failed: GameConfigs.db unavailable",
    "NPC config reload FAILED":
        "NPC config reload failed",
    "NPC configs reloaded successfully (new spawns will use updated data)":
        "NPC configs reloaded successfully",
    "Item config reload FAILED - GameConfigs.db unavailable":
        "Item config reload failed: GameConfigs.db unavailable",
    "Item config reload FAILED":
        "Item config reload failed",
    "Magic config reload FAILED - GameConfigs.db unavailable":
        "Magic config reload failed: GameConfigs.db unavailable",
    "Magic config reload FAILED":
        "Magic config reload failed",
    "Skill config reload FAILED - GameConfigs.db unavailable":
        "Skill config reload failed: GameConfigs.db unavailable",
    "Skill config reload FAILED":
        "Skill config reload failed",

    # ── SQLite messages (add consistent prefix) ──
    "Exec failed: %s":
        "SQLite exec failed: %s",
    "Open failed: %s":
        "SQLite open failed: %s",
    "Migrating item storage from names to IDs":
        "SQLite: migrating item storage from names to IDs",
    "Failed to load item mapping from GameConfigs.db":
        "SQLite: failed to load item mapping from GameConfigs.db",
    "Migration complete: %d items, %d bank items migrated (%d/%d skipped)":
        "SQLite: migration complete, %d items, %d bank items migrated (%d/%d skipped)",
    "Prepare account insert failed: %s":
        "SQLite: account insert prepare failed: %s",
    "Insert account failed: %s":
        "SQLite: account insert failed: %s",
    "Prepare character insert failed: %s":
        "SQLite: character insert prepare failed: %s",
    "Insert character failed: %s":
        "SQLite: character insert failed: %s",
    "SaveCharacterSnapshot: null db or client":
        "SQLite: save snapshot failed, null db or client",
    "SaveCharacterSnapshot failed at [%s]: %s":
        "SQLite: save snapshot failed at [%s]: %s",
    "SaveCharacterSnapshot: BEGIN failed":
        "SQLite: save snapshot BEGIN failed",
    "SaveCharacterSnapshot: upsert prepare failed: %s":
        "SQLite: save snapshot upsert prepare failed: %s",
    "SaveCharacterSnapshot: upsert bind failed at idx %d: %s":
        "SQLite: save snapshot upsert bind failed at idx %d: %s",
    "SaveCharacterSnapshot: upsert step failed (rc=%d): %s":
        "SQLite: save snapshot upsert step failed (rc=%d): %s",
    "SaveCharacterSnapshot: delete prepare failed: %s | SQL: %.100s":
        "SQLite: save snapshot delete prepare failed: %s | SQL: %.100s",
    "SaveCharacterSnapshot: delete step failed (rc=%d): %s | SQL: %.100s":
        "SQLite: save snapshot delete step failed (rc=%d): %s | SQL: %.100s",
    "SaveCharacterSnapshot: insert items prepare failed: %s":
        "SQLite: save snapshot item insert prepare failed: %s",
    "SaveCharacterSnapshot: insert item[%d] step failed (rc=%d): %s":
        "SQLite: save snapshot item[%d] step failed (rc=%d): %s",
    "SaveCharacterSnapshot: insert item[%d] bind failed":
        "SQLite: save snapshot item[%d] bind failed",
    "(MAPINFO-SQLITE) Exec failed: %s":
        "MapInfo SQLite exec failed: %s",
    "(MAPINFO-SQLITE) Open failed: %s":
        "MapInfo SQLite open failed: %s",
    "(MAPINFO-SQLITE) GetMapNames prepare failed: %s":
        "MapInfo SQLite: GetMapNames prepare failed: %s",
    "(MAPINFO-SQLITE) Failed to load base settings for map: %s":
        "MapInfo SQLite: failed to load base settings for map %s",

    # ── Party messages ──
    "PartyID:%d member:%d Out(Delete) Total:%d":
        "Party %d: member %d removed (deleted), total=%d",
    "Join Party Reject (1)":
        "Party join rejected (reason 1)",
    "Join Party Reject (2)":
        "Party join rejected (reason 2)",
    "Join Party Reject (3)":
        "Party join rejected (reason 3)",
    "Join Party Reject (4)":
        "Party join rejected (reason 4)",
    "Join Party Req: %s(%d) ID(%d) Stat(%d) ReqJoinH(%d) ReqJoinName(%s)":
        "Party join request: player=%s(%d) party=%d status=%d target=%d name=%s",
    "Party join reject(2) ClientH:%d ID:%d JoinName:%s":
        "Party join rejected: client=%d party=%d name=%s",
    "Request Create Party: %d":
        "Party create request: client=%d",
    "party Operation Result: Create(ClientH:%d PartyID:%d)":
        "Party created: client=%d party=%d",
    "party Operation Result: Delete(PartyID:%d)":
        "Party deleted: party=%d",
    "PartyID:%d member:%d Out(Clear) Total:%d":
        "Party %d: member %d left, total=%d",
    "Party Status 0: %s":
        "Party status 0: %s",
    "party Operation Result: Join(ClientH:%d PartyID:%d)":
        "Party joined: client=%d party=%d",
    "party Operation Result: Info(ClientH:%d Total:%d)":
        "Party info: client=%d total=%d",
    "party Operation Result: Dismiss(ClientH:%d PartyID:%d)":
        "Party dismissed: client=%d party=%d",
    "PartyID:%d member:%d New Total:%d":
        "Party %d: member %d added, total=%d",
    "Removing Party member(%s) by Server down":
        "Removing party member '%s' (server shutdown)",
    "Party Bug partyMember %d XXXXXXXXXX":
        "Party bug: member count %d exceeds limit",

    # ── Crusade/War ──
    "Automated crusade is being initiated!":
        "Automated crusade initiating",
    "Crusade Mode ON.":
        "Crusade mode enabled",
    "Crusade Mode OFF.":
        "Crusade mode disabled",
    "Beginning Meteor Strike Procedure":
        "Beginning meteor strike procedure",
    "MeteorStrikeHandler Error! MapIndex -1!":
        "Meteor strike error: map index is -1",
    "MeteorStrikeHandler Error! 0 Map!":
        "Meteor strike error: null map",
    "MeteorStrikeHandler Error! No Strike Points!":
        "Meteor strike error: no strike points",
    "No strike points!":
        "No strike points available",
    "Strike Point MapIndex: -1!":
        "Strike point error: map index is -1",
    "Automated apocalypse is concluded!":
        "Automated apocalypse concluded",
    "Apocalypse Mode OFF.":
        "Apocalypse mode disabled",
    "Apocalypse Mode ON.":
        "Apocalypse mode enabled",
    "Automated apocalypse is initiated!":
        "Automated apocalypse initiated",
    "Energy Sphere Created! (%d, %d)":
        "Energy sphere created at (%d, %d)",
    "HELDENIAN Start.":
        "Heldenian started",
    "HELDENIAN End. Result Report Failed":
        "Heldenian ended, result report failed",
    "HELDENIAN End. %d":
        "Heldenian ended, winner side: %d",

    # ── Hack detection (standardize format) ──
    "Logout Hack: (%s) Player: (%s) - disconnected within 10 seconds of most recent damage. Hack? Lag?":
        "Logout hack: IP=%s player=%s, disconnected within 10s of last damage",
    "Swing Hack: (%s) Player: (%s) - attacking at irregular rates. Gap: %ums, Min: %dms":
        "Swing hack: IP=%s player=%s, irregular attack rate (gap=%ums min=%dms)",
    "TSearch Fullswing Hack: (%s) Player: (%s) - dashing with only (%d) weapon skill.":
        "Fullswing hack: IP=%s player=%s, dashing with weapon skill %d",
    "Batch Swing Hack: (%s) Player: (%s) - 7 attacks in %ums, Min: %dms":
        "Batch swing hack: IP=%s player=%s, 7 attacks in %ums (min=%dms)",
    "Traveller Hack: (%s) Player: (%s) is a traveller and is greater than level 19.":
        "Traveller hack: IP=%s player=%s, traveller exceeds level 19",
    "Speed Hack: (%s) Player: (%s) - running too fast.":
        "Speed hack: IP=%s player=%s, running too fast",
    "TSearch Spell Hack: (%s) Player: (%s) - casting magic without precasting.":
        "Spell hack: IP=%s player=%s, casting without precast",
    "Speed Cast: (%s) Player: (%s) - casting magic at irregular rates.":
        "Speed cast: IP=%s player=%s, irregular casting rate",
    "3.51 Detection: (%s) Player: (%s) - Magic casting speed is too fast! Hack?":
        "Fast cast detection: IP=%s player=%s, magic casting too fast",
    "TSearch Slate Hack: (%s) Player: (%s) - creating slates without correct item!":
        "Slate hack: IP=%s player=%s, creating slates without required item",
    "Accessing crusade teleport: (%s) Player: (%s) - setting teleport location when crusade is disabled.":
        "Crusade teleport hack: IP=%s player=%s, setting teleport while crusade disabled",
    "Accessing crusade teleport: (%s) Player: (%s) - teleporting when not in a guild":
        "Crusade teleport hack: IP=%s player=%s, teleporting without guild",
    "Accessing Crusade Set Teleport:(%s) Player: (%s) - setting point when not a crusade.":
        "Crusade teleport hack: IP=%s player=%s, setting point outside crusade",
    "Accessing Crusade Set Teleport: (%s) Player: (%s) - setting point when not a guildmaster.":
        "Crusade teleport hack: IP=%s player=%s, setting point as non-guildmaster",
    "Packet Editing: (%s) Player: (%s) - has more than allowed skill points (%d).":
        "Packet editing: IP=%s player=%s, exceeds allowed skill points (%d)",

    # ── Misc events ──
    "Fightzone Dead Time: %d":
        "Fight zone dead time: %d",
    "PC(%s) Crafting (%s) Purity(%d)":
        "Player '%s' crafting '%s' purity=%d",
    "No Acc":
        "Account not found",
    "No Pass":
        "Password mismatch",
    "Account Login!":
        "Account login",
    "RIPH - cTxt: Char 0!":
        "RIPH: empty character name",
    "BlueBallEvent: SummonMob (%s)-(%d)":
        "BlueBall event: summoning '%s' x%d",
    "Reserve FIGHTZONETICKET : Char(%s) TICKENUMBER (%d)":
        "Fight zone ticket reserved: player=%s ticket=%d",
    "Get FIGHTZONETICKET : Char(%s) TICKENUMBER (%d)(%d)(%d)":
        "Fight zone ticket obtained: player=%s ticket=%d(%d)(%d)",
    "Get Flag : Char(%s) Flag-EK(%d) Player-EK(%d)":
        "Flag captured: player=%s flag_ek=%d player_ek=%d",
    "(skill:%d type:%d)plant(%s) Agriculture begin(%d,%d) sum(%d)!":
        "Agriculture: skill=%d type=%d plant=%s at (%d,%d) total=%d",
    "New command '/%s' added to permissions (default: level %d)":
        "Command '/%s' registered (default level: %d)",
}

# Auto-convert all keys and values from printf to std::format at load time.
# The dict source uses printf specifiers to match survey output for easy review,
# but at runtime everything uses {} format.
MESSAGE_REWRITES = {
    convert_printf_to_stdformat(k): convert_printf_to_stdformat(v)
    for k, v in MESSAGE_REWRITES.items()
}

def rewrite_message(msg):
    """Apply message rewrite if one exists, otherwise return as-is."""
    if msg in MESSAGE_REWRITES:
        return MESSAGE_REWRITES[msg]
    return msg

# ─────────────────────────────────────────────────────────────────────
# Survey mode — extract all messages for review
# ─────────────────────────────────────────────────────────────────────

def survey_file(filepath):
    """Extract all log messages from a file for survey output."""
    with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
        original_lines = f.readlines()

    lines = [l.rstrip('\n').rstrip('\r') for l in original_lines]
    basename = os.path.basename(filepath)
    joined = join_continuation_lines(lines)
    messages = []

    for idx, (jline, jindices) in enumerate(joined):
        line_no = jindices[0] + 1

        # Skip commented-out lines
        stripped = jline.lstrip()
        if stripped.startswith('//'):
            continue

        # ── PutLogList with literal ──
        m = RE_PUTLOGLIST_LITERAL.match(jline)
        if m:
            msg = m.group(2)
            if not msg.strip() or msg.strip() in ('.', '..', '...', '....', ' '):
                continue
            stripped_msg, level = strip_prefix_and_get_level(msg)
            stripped_msg = normalize_message_text(stripped_msg)
            messages.append({
                "file": basename, "line": line_no, "type": "PutLogList(literal)",
                "level": level, "original": msg, "stripped": stripped_msg, "has_args": False,
            })
            continue

        # ── PutLogList with variable (check for snprintf) ──
        m = RE_PUTLOGLIST_VAR.match(jline)
        if m:
            var_name = m.group(2)
            result = find_preceding_snprintf(joined, idx, var_name)
            if result:
                fmt_str, args_str, _ = result
                stripped_fmt, level = strip_prefix_and_get_level(fmt_str)
                stripped_fmt = normalize_message_text(stripped_fmt)
                messages.append({
                    "file": basename, "line": line_no, "type": "PutLogList(snprintf)",
                    "level": level, "original": fmt_str, "stripped": stripped_fmt,
                    "has_args": True, "args": args_str,
                })
            else:
                messages.append({
                    "file": basename, "line": line_no, "type": "PutLogList(var)",
                    "level": "log", "original": f"<{var_name}>", "stripped": f"<{var_name}>",
                    "has_args": False,
                })
            continue

        # ── PutLogListLevel with literal ──
        m = RE_PUTLOGLISTLEVEL_LITERAL.match(jline)
        if m:
            level_const = m.group(2)
            msg = m.group(3)
            level_from_const = LEVEL_TO_FUNC.get(level_const, "log")
            stripped_msg, level_from_prefix = strip_prefix_and_get_level(msg)
            level = higher_level(level_from_const, level_from_prefix)
            stripped_msg = normalize_message_text(stripped_msg)
            messages.append({
                "file": basename, "line": line_no, "type": f"PutLogListLevel({level_const},literal)",
                "level": level, "original": msg, "stripped": stripped_msg, "has_args": False,
            })
            continue

        # ── PutLogListLevel with variable ──
        m = RE_PUTLOGLISTLEVEL_VAR.match(jline)
        if m:
            level_const = m.group(2)
            var_name = m.group(3)
            level_from_const = LEVEL_TO_FUNC.get(level_const, "log")
            result = find_preceding_snprintf(joined, idx, var_name)
            if result:
                fmt_str, args_str, _ = result
                stripped_fmt, level_from_prefix = strip_prefix_and_get_level(fmt_str)
                level = higher_level(level_from_const, level_from_prefix)
                stripped_fmt = normalize_message_text(stripped_fmt)
                messages.append({
                    "file": basename, "line": line_no, "type": f"PutLogListLevel({level_const},snprintf)",
                    "level": level, "original": fmt_str, "stripped": stripped_fmt,
                    "has_args": True, "args": args_str,
                })
            continue

        # ── Legacy file functions with literal ──
        m = RE_LEGACY_LITERAL.match(jline)
        if m:
            func_name = m.group(2)
            msg = m.group(3)
            channel, default_level = LEGACY_FILE_FUNCS.get(func_name, ("events", "log"))
            stripped_msg, level = strip_prefix_and_get_level(msg)
            if level == "log" and default_level != "log":
                level = default_level
            stripped_msg = normalize_message_text(stripped_msg)
            messages.append({
                "file": basename, "line": line_no, "type": f"{func_name}(literal)",
                "level": level, "channel": channel, "original": msg,
                "stripped": stripped_msg, "has_args": False,
            })
            continue

        # ── Legacy file functions with variable ──
        m = RE_LEGACY_VAR.match(jline)
        if m:
            func_name = m.group(2)
            var_name = m.group(3)
            channel, default_level = LEGACY_FILE_FUNCS.get(func_name, ("events", "log"))
            result = find_preceding_snprintf(joined, idx, var_name)
            if result:
                fmt_str, args_str, _ = result
                stripped_fmt, level = strip_prefix_and_get_level(fmt_str)
                if level == "log" and default_level != "log":
                    level = default_level
                stripped_fmt = normalize_message_text(stripped_fmt)
                messages.append({
                    "file": basename, "line": line_no, "type": f"{func_name}(snprintf)",
                    "level": level, "channel": channel, "original": fmt_str,
                    "stripped": stripped_fmt, "has_args": True, "args": args_str,
                })
            continue

    return messages

def run_survey():
    """Run survey mode: extract and display all log messages for review."""
    os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

    all_messages = []
    server_files = []
    for f in sorted(os.listdir(SERVER_DIR)):
        if f.endswith('.cpp'):
            if f in ("ItemLog.cpp", "ChatLog.cpp"):
                continue
            server_files.append(os.path.join(SERVER_DIR, f))

    for filepath in server_files:
        messages = survey_file(filepath)
        all_messages.extend(messages)

    # Output in a structured format for review
    output_path = "Scripts/output/survey_messages.txt"
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(f"# Server Logging Survey — {len(all_messages)} messages found\n")
        f.write(f"# Format: LEVEL | FILE:LINE | TYPE | ORIGINAL → STRIPPED\n")
        f.write(f"# Review each STRIPPED message. Messages needing rewording should be\n")
        f.write(f"# added to MESSAGE_REWRITES dict in the script.\n")
        f.write(f"#\n")
        f.write(f"# Legend: [E]=error [W]=warn [L]=log\n\n")

        for msg in all_messages:
            level_tag = {"error": "E", "warn": "W", "log": "L", "debug": "D"}.get(msg["level"], "?")
            ch = f" ch={msg['channel']}" if 'channel' in msg else ""
            args = f" args=[{msg['args']}]" if msg.get('has_args') and msg.get('args') else ""

            f.write(f"[{level_tag}] {msg['file']}:{msg['line']} ({msg['type']}{ch})\n")
            f.write(f"  OLD: {msg['original']}\n")
            f.write(f"  NEW: {msg['stripped']}\n")
            if args:
                f.write(f"  {args}\n")
            f.write(f"\n")

    print(f"Survey complete: {len(all_messages)} messages found")
    print(f"Output: {output_path}")
    print(f"\nReview the output and add rewrites to MESSAGE_REWRITES dict.")

# ─────────────────────────────────────────────────────────────────────
# Main
# ─────────────────────────────────────────────────────────────────────

def main():
    os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

    if SURVEY_MODE:
        run_survey()
        return

    total_changes = 0
    files_modified = []
    files_skipped = []

    # Get all server .cpp files
    server_files = []
    for f in sorted(os.listdir(SERVER_DIR)):
        if f.endswith('.cpp') or f.endswith('.h'):
            server_files.append(os.path.join(SERVER_DIR, f))

    # Process WINMAIN.H specially
    winmain_h = os.path.join(SERVER_DIR, "WINMAIN.H")
    if os.path.exists(winmain_h):
        result = process_winmain_h(winmain_h)
        if result:
            content, count = result
            if DRY_RUN:
                print(f"  [DRY] {winmain_h}: {count} changes")
            else:
                with open(winmain_h, 'w', encoding='utf-8', newline='\n') as f:
                    f.write(content)
                print(f"  {winmain_h}: {count} changes")
            total_changes += count
            files_modified.append(winmain_h)

    # Process each server file
    for filepath in server_files:
        basename = os.path.basename(filepath)

        # Skip files that will be deleted
        if basename in ("ItemLog.cpp", "ItemLog.h", "ChatLog.cpp", "ChatLog.h"):
            continue

        # Skip non-source files
        if not basename.endswith('.cpp'):
            continue

        # Wmain.cpp gets special treatment
        if basename == "Wmain.cpp":
            # First do the standard processing (for any remaining PutLogList calls)
            result = process_file(filepath)
            processed_content = None
            if result:
                processed_content = result[0]

            result = process_wmain(filepath, processed_content)
            if result:
                content, count = result
                if DRY_RUN:
                    print(f"  [DRY] {filepath}: Wmain special ({count} changes)")
                else:
                    with open(filepath, 'w', encoding='utf-8', newline='\n') as f:
                        f.write(content)
                    print(f"  {filepath}: Wmain special")
                files_modified.append(filepath)
                total_changes += count
            continue

        result = process_file(filepath)
        if result:
            content, count = result
            if DRY_RUN:
                print(f"  [DRY] {filepath}: {count} changes")
            else:
                with open(filepath, 'w', encoding='utf-8', newline='\n') as f:
                    f.write(content)
                print(f"  {filepath}: {count} changes")
            total_changes += count
            files_modified.append(filepath)
        else:
            files_skipped.append(filepath)

    print(f"\n{'[DRY RUN] ' if DRY_RUN else ''}Summary:")
    print(f"  Files modified: {len(files_modified)}")
    print(f"  Total changes:  {total_changes}")
    print(f"  Files skipped:  {len(files_skipped)}")

    if not DRY_RUN:
        print("\nNext steps:")
        print("  1. Delete ItemLog.h, ItemLog.cpp, ChatLog.h, ChatLog.cpp")
        print("  2. Remove from Server.vcxproj and .filters")
        print("  3. Add format_item_info() and is_item_suspicious() helpers to ItemManager.cpp")
        print("  4. Complete Wmain.cpp cleanup (remove old function bodies)")
        print("  5. Build and verify")

if __name__ == "__main__":
    main()
