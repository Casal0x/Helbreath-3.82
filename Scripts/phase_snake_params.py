#!/usr/bin/env python3
"""Phase 9+10: Hungarian parameter and local variable renames.
Combined: rename all remaining Hungarian-prefixed identifiers in function
signatures and bodies across all source files.

Mode 2 justified: ~500+ unique names across ~200+ files.

Approach:
1. Scan all .h files for Hungarian-prefixed parameters
2. Also scan .cpp files for Hungarian-prefixed local variables
3. Auto-generate snake_case conversions by stripping prefixes
4. Sort longest-first, apply word-boundary renames across all files
5. Skip #include, #pragma, wire protocol (HB_PACKED) sections

Usage:
  --dry-run   Preview changes without modifying files
  --verify    Scan for collisions, keyword conflicts, shadowing issues
  --list      Print rename table only
"""

import re
import os
import sys
import argparse

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT_DIR = os.path.join(BASE, "Scripts", "output")

# ═══════════════════════════════════════════════════════════════════════════
# PREFIX STRIPPING RULES
# ═══════════════════════════════════════════════════════════════════════════
# Hungarian prefixes: i=int, b=bool, c=char, s=short, p=pointer,
#   dw=DWORD, w=WORD, f=float, d=double, sz=string-zero, st=struct
# After stripping prefix, convert PascalCase remainder to snake_case.

HUNGARIAN_PREFIX = re.compile(
    r'^(dw|sz|st|ms|is|po|'  # Two-char prefixes first
    r'[ibcspwfdr])'          # Single-char prefixes
    r'([A-Z])'               # Must be followed by uppercase
)

def strip_hungarian(name):
    """Strip Hungarian prefix and return (stripped, prefix)."""
    m = HUNGARIAN_PREFIX.match(name)
    if m:
        return m.group(2) + name[m.end():], m.group(1)
    return None, None

def pascal_to_snake(name):
    """Convert PascalCase to snake_case."""
    # Handle consecutive uppercase (like NPC, ID, HP, MP)
    s = re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1_\2', name)
    s = re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', s)
    return s.lower()

def hungarian_to_snake(name):
    """Convert Hungarian-prefixed name to snake_case."""
    stripped, prefix = strip_hungarian(name)
    if stripped is None:
        return None
    snake = pascal_to_snake(stripped)
    # Special handling for single-letter results
    if len(snake) == 1:
        return snake  # iX -> x, iY -> y, etc.
    return snake


# ═══════════════════════════════════════════════════════════════════════════
# EXCLUSION LIST — names that should NOT be renamed
# ═══════════════════════════════════════════════════════════════════════════
EXCLUDE_NAMES = {
    # Wire protocol field names (inside #pragma pack structs)
    # These are excluded by the HB_PACKED detection below

    # C++ keywords and standard library names that would be created
    'cInt', 'cFrom', 'cNew', 'cTrue', 'cFalse',
    'iA',  # too short, ambiguous

    # Names that are part of external APIs (Windows API struct fields)
    'dwExStyle', 'dwStyle',  # CreateWindowEx parameters
    'bKeyDown',              # KEY_EVENT_RECORD.bKeyDown (Windows Console API)
    'wVirtualKeyCode',       # KEY_EVENT_RECORD.wVirtualKeyCode (Windows Console API)

    # SFML API methods that look Hungarian due to 'is' prefix
    'isSmooth',              # sf::Texture::isSmooth()
    'isOpen',                # sf::RenderWindow::isOpen()

    # Data member → method name collisions (struct has both member and accessor)
    'bIsMoving',             # EntityMotion: collides with is_moving() method
    'bHasPending',           # EntityMotion: collides with has_pending() method

    # Local variable scope collisions
    'cSpacing',              # Screen_TestPrimitives: cSpacing→spacing collides with existing 'spacing'

    # Names that are already being used as-is in other contexts
    'pMsg',  # CMsg pointer pattern
}

# Names where the auto-generated snake_case would be wrong
OVERRIDE_RENAMES = {
    # Abbreviation expansions
    'iHP': 'hp',
    'iMP': 'mp',
    'iSP': 'sp',
    'iEK': 'ek',
    'iPK': 'pk',
    'iAP': 'ap',
    'iSTR': 'str_stat',
    'iDEX': 'dex',
    'iVIT': 'vit',
    'iINT': 'int_stat',
    'iMAG': 'mag',
    'iCHR': 'chr_stat',
    'sStr': 'str_text',  # avoid collision with str keyword
    'pError': 'error_acc',  # avoid shadowing local 'error' in Bresenham GetPoint/GetPoint2
}


def collect_hungarian_names():
    """Scan all source files for Hungarian-prefixed identifiers."""
    # Pattern to find Hungarian-prefixed identifiers in declarations
    # Matches: type prefix_Name or *prefix_Name
    ident_pattern = re.compile(r'\b([a-z]{1,2}[A-Z][A-Za-z0-9]*)\b')

    names = {}  # name -> count

    for d in ["Sources/Dependencies/Shared", "Sources/SFMLEngine",
              "Sources/Client", "Sources/Server"]:
        dirpath = os.path.join(BASE, d)
        if not os.path.isdir(dirpath):
            continue
        for root, dirs, fnames in os.walk(dirpath):
            for fname in sorted(fnames):
                if not fname.endswith((".h", ".cpp")):
                    continue
                fpath = os.path.join(root, fname)
                in_pragma_pack = False
                with open(fpath, "r", encoding="utf-8") as f:
                    for line in f:
                        stripped = line.lstrip()
                        # Track #pragma pack sections (wire protocol)
                        if 'HB_PACKED' in stripped or '#pragma pack' in stripped:
                            in_pragma_pack = True
                        if in_pragma_pack and stripped.startswith('};'):
                            in_pragma_pack = False
                            continue
                        if in_pragma_pack:
                            continue
                        if stripped.startswith(('#include', '#pragma', '//', '/*', '* ')):
                            continue

                        for m in ident_pattern.finditer(line):
                            name = m.group(1)
                            # Check it has a valid Hungarian prefix
                            sn = hungarian_to_snake(name)
                            if sn is not None and name not in EXCLUDE_NAMES:
                                names[name] = names.get(name, 0) + 1

    return names


CPP_KEYWORDS = {
    'int', 'float', 'double', 'char', 'bool', 'void', 'short', 'long',
    'auto', 'class', 'struct', 'enum', 'union', 'const', 'static',
    'new', 'delete', 'this', 'true', 'false', 'return', 'if', 'else',
    'for', 'while', 'do', 'switch', 'case', 'break', 'continue',
    'default', 'goto', 'try', 'catch', 'throw', 'namespace', 'using',
    'template', 'typename', 'virtual', 'override', 'final',
    'public', 'private', 'protected', 'friend', 'operator',
    'sizeof', 'alignof', 'decltype', 'typeid', 'register',
    'volatile', 'mutable', 'extern', 'inline', 'constexpr',
    'nullptr', 'from', 'not', 'and', 'or', 'xor', 'compl',
}

def build_rename_table(names):
    """Build the rename table from collected Hungarian names."""
    # First pass: collect all mappings and detect collisions
    all_mappings = {}  # name -> snake
    target_to_sources = {}  # snake -> [names]

    for name in sorted(names.keys()):
        if name in OVERRIDE_RENAMES:
            snake = OVERRIDE_RENAMES[name]
        else:
            snake = hungarian_to_snake(name)

        if snake is None or snake == name:
            continue

        # Skip if target is a C++ keyword
        if snake in CPP_KEYWORDS:
            print(f"  SKIP: '{name}' -> '{snake}' (C++ keyword)")
            continue

        # Skip very short names (single char after prefix, like iX, sY, bV)
        if len(name) <= 2:
            continue

        all_mappings[name] = snake
        if snake in target_to_sources:
            target_to_sources[snake].append(name)
        else:
            target_to_sources[snake] = [name]

    # Second pass: resolve collisions
    renames = []
    skipped_collisions = 0
    for name, snake in all_mappings.items():
        sources = target_to_sources[snake]
        if len(sources) > 1:
            # Collision: multiple Hungarian names map to same snake_case
            # For safety, skip ALL colliding names (they can be fixed manually)
            skipped_collisions += 1
            continue
        renames.append((name, snake))

    # Report
    collision_targets = {s: srcs for s, srcs in target_to_sources.items() if len(srcs) > 1}
    if collision_targets:
        print(f"  Skipped {skipped_collisions} names due to {len(collision_targets)} collision groups:")
        for snake, srcs in sorted(collision_targets.items()):
            print(f"    '{snake}' <- {srcs}")

    # Sort longest-first
    renames.sort(key=lambda x: -len(x[0]))
    return renames


def collect_files():
    """Collect all source files across all modules."""
    files = []
    for d in ["Sources/Dependencies/Shared", "Sources/SFMLEngine",
              "Sources/Client", "Sources/Server"]:
        dirpath = os.path.join(BASE, d)
        if not os.path.isdir(dirpath):
            continue
        for root, dirs, names in os.walk(dirpath):
            for name in sorted(names):
                if name.endswith((".h", ".cpp")):
                    files.append(os.path.join(root, name))
    return sorted(files)


def apply_renames(files, renames, dry_run=False):
    """Apply all renames to files."""
    patterns = [(re.compile(r"\b" + re.escape(old) + r"\b"), new)
                for old, new in renames]

    modified_files = []
    total_changes = 0

    for fpath in files:
        with open(fpath, "r", encoding="utf-8") as f:
            lines = f.readlines()

        new_lines = []
        file_changes = 0
        in_pragma_pack = False

        for line in lines:
            original_line = line
            stripped = line.lstrip()

            # Skip #include and #pragma lines
            if stripped.startswith("#include") or stripped.startswith("#pragma"):
                # Track pragma pack for wire protocol structs
                if 'HB_PACKED' in stripped or '#pragma pack' in stripped:
                    in_pragma_pack = True
                new_lines.append(line)
                continue

            # Skip wire protocol struct contents
            if in_pragma_pack:
                if stripped.startswith('};'):
                    in_pragma_pack = False
                new_lines.append(line)
                continue

            # Apply renames (word-boundary)
            for pattern, new in patterns:
                line = pattern.sub(new, line)

            if line != original_line:
                file_changes += 1
            new_lines.append(line)

        if file_changes > 0:
            if not dry_run:
                with open(fpath, "w", encoding="utf-8", newline="\n") as f:
                    f.writelines(new_lines)
            total_changes += file_changes
            modified_files.append(fpath)
            rel = os.path.relpath(fpath, BASE)
            print(f"  MOD  {rel} ({file_changes} lines)")

    return modified_files, total_changes


def run_verify(renames):
    """Verify mode: scan for collisions, keyword conflicts, and shadowing."""
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    log_path = os.path.join(OUTPUT_DIR, "phase_snake_params_verify.log")

    issues = []

    # 1. C++ keyword check (should already be filtered, but double-check)
    for old, new in renames:
        if new in CPP_KEYWORDS:
            issues.append(f"[C++ KEYWORD] '{old}' -> '{new}' — '{new}' is a C++ reserved keyword")

    # 2. Check for shadowing: target name already exists as a different identifier
    #    in the same files where the source name appears
    files = collect_files()
    print("  Checking for local variable shadowing...")
    shadow_checks = {}
    for old, new in renames:
        shadow_checks[new] = shadow_checks.get(new, [])
        shadow_checks[new].append(old)

    # Scan for cases where the target name already exists in source files
    target_pattern = re.compile(r'\b(' + '|'.join(
        re.escape(new) for _, new in renames
    ) + r')\b')

    existing_targets = set()
    for fpath in files:
        with open(fpath, "r", encoding="utf-8") as f:
            content = f.read()
        for m in target_pattern.finditer(content):
            existing_targets.add(m.group(1))

    for old, new in renames:
        if new in existing_targets:
            issues.append(f"[SHADOW RISK] '{old}' -> '{new}' — '{new}' already exists in codebase")

    # 3. Shared collision scan
    print("  Scanning Shared/ for collision risks...")
    shared_dir = os.path.join(BASE, "Sources/Dependencies/Shared")
    shared_pattern = re.compile(r'\b(enum|struct|class|constexpr|namespace)\b')
    for old, new in renames:
        old_re = re.compile(r'\b' + re.escape(old) + r'\b')
        for root, dirs, fnames in os.walk(shared_dir):
            for fname in fnames:
                if not fname.endswith(('.h',)):
                    continue
                fpath = os.path.join(root, fname)
                with open(fpath, "r", encoding="utf-8") as f:
                    for lineno, line in enumerate(f, 1):
                        if old_re.search(line) and shared_pattern.search(line):
                            rel = os.path.relpath(fpath, BASE)
                            issues.append(
                                f"[SHARED DECL] '{old}' -> '{new}' in {rel}:{lineno}"
                                f"\n    {line.rstrip()}"
                            )

    # Write log
    with open(log_path, "w", encoding="utf-8") as f:
        f.write(f"=== Verify: phase_snake_params.py ===\n\n")
        f.write(f"Total renames: {len(renames)}\n\n")
        if issues:
            f.write(f"--- Issues Found ({len(issues)}) ---\n\n")
            for issue in issues:
                f.write(f"{issue}\n\n")
        else:
            f.write("--- No issues found ---\n\n")
        f.write(f"\n{len(renames)} entries checked, {len(issues)} issues found\n")

    print(f"\n  Verify log: {os.path.relpath(log_path, BASE)}")
    print(f"  {len(renames)} entries checked, {len(issues)} issues found")

    # Print issues to stdout too
    if issues:
        print(f"\n  Issues:")
        for issue in issues[:50]:  # Cap at 50 for readability
            print(f"    {issue.split(chr(10))[0]}")
        if len(issues) > 50:
            print(f"    ... and {len(issues) - 50} more (see log)")

    return 0


def main():
    parser = argparse.ArgumentParser(description="Phase 9+10: Hungarian parameter/local renames")
    parser.add_argument("--dry-run", action="store_true",
                        help="Preview changes without modifying files")
    parser.add_argument("--verify", action="store_true",
                        help="Scan for collisions, keyword conflicts, shadowing")
    parser.add_argument("--list", action="store_true",
                        help="Print rename table only")
    args = parser.parse_args()

    mode = "DRY RUN" if args.dry_run else ("VERIFY" if args.verify else
           ("LIST" if args.list else "LIVE"))
    print(f"Phase 9+10: Hungarian parameter/local renames [{mode}]")
    print()

    print("Scanning for Hungarian-prefixed identifiers...")
    names = collect_hungarian_names()
    print(f"  Found {len(names)} unique Hungarian names ({sum(names.values())} total occurrences)")
    print()

    print("Building rename table...")
    renames = build_rename_table(names)
    print(f"  {len(renames)} renames generated")
    print()

    if args.list:
        for old, new in sorted(renames, key=lambda x: x[0]):
            print(f"  {old:30s} -> {new}")
        return 0

    if args.verify:
        return run_verify(renames)

    files = collect_files()
    print(f"Scanning {len(files)} source files...")
    print()

    mod_files, changes = apply_renames(files, renames, args.dry_run)
    print()
    print(f"Summary: {len(mod_files)} files modified, ~{changes} changes")

    return 0


if __name__ == "__main__":
    sys.exit(main())
