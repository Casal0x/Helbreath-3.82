"""F15+F17: Remove unnecessary #include <windows.h> from server source files.

Files that legitimately need windows.h (already platform-guarded) are skipped:
- ServerConsole.h  (HANDLE, DWORD in #ifdef _WIN32)
- CmdShowChat.h    (HANDLE in #ifdef _WIN32)
- PasswordHash.h   (BCrypt in #ifdef _WIN32)
"""

import os
import re
import subprocess
import sys

SERVER_DIR = r"Z:\Helbreath-3.82\Sources\Server"
BAK_PY = r"Z:\Helbreath-3.82\Scripts\bak.py"
PYTHON = r"C:\Python314\python.exe"

# Files that legitimately need windows.h (already properly guarded)
SKIP_FILES = {
    "ServerConsole.h",
    "CmdShowChat.h",
    "PasswordHash.h",
}

def find_files_with_windows_h():
    """Find all .h/.cpp files containing #include <windows.h>."""
    results = []
    for fname in sorted(os.listdir(SERVER_DIR)):
        if fname in SKIP_FILES:
            continue
        if not (fname.endswith(".h") or fname.endswith(".cpp")):
            continue
        fpath = os.path.join(SERVER_DIR, fname)
        with open(fpath, "r", encoding="utf-8-sig") as f:
            content = f.read()
        if "#include <windows.h>" in content:
            results.append((fname, fpath, content))
    return results


def remove_windows_include(content):
    """Remove #include <windows.h> and associated 'prevent winsock' comments."""
    lines = content.split("\n")
    out = []
    skip_next_blank = False
    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        # Skip the "prevent winsock" comment that precedes windows.h include
        if "prevent old winsock" in stripped.lower() or "must be before windows.h" in stripped.lower():
            # Skip this comment line, and the windows.h include that follows
            if i + 1 < len(lines) and "#include <windows.h>" in lines[i + 1]:
                i += 2
                # Also skip a blank line after the removed include
                if i < len(lines) and lines[i].strip() == "":
                    i += 1
                continue
            # If no windows.h follows, skip just the comment
            i += 1
            continue

        # Skip standalone #include <windows.h>
        if stripped == "#include <windows.h>":
            i += 1
            # Skip trailing blank line
            if i < len(lines) and lines[i].strip() == "":
                i += 1
            continue

        out.append(line)
        i += 1

    return "\n".join(out)


def main():
    files = find_files_with_windows_h()
    if not files:
        print("No files found with #include <windows.h> to remove.")
        return

    print(f"Found {len(files)} file(s) with #include <windows.h>:")
    for fname, _, _ in files:
        print(f"  {fname}")

    # Guard all files
    paths = [fpath for _, fpath, _ in files]
    cmd = [PYTHON, BAK_PY, "guard"] + paths
    result = subprocess.run(cmd, capture_output=True, text=True)
    print(result.stdout)
    if result.returncode != 0:
        print(f"Guard failed: {result.stderr}")
        sys.exit(1)

    # Process each file
    modified = 0
    for fname, fpath, content in files:
        new_content = remove_windows_include(content)
        if new_content != content:
            with open(fpath, "w", encoding="utf-8-sig" if content.startswith("\ufeff") else "utf-8") as f:
                f.write(new_content)
            modified += 1
            print(f"  Cleaned: {fname}")

    print(f"\nModified {modified} file(s).")


if __name__ == "__main__":
    main()
