"""F4+F5+F6+F11: Replace Win32 file I/O, FindFirstFile, GetFileAttributes, _mkdir with portable equivalents."""
import re
from pathlib import Path

server_dir = Path(r"Z:\Helbreath-3.82\Sources\Server")


def process_file(fname, replacements):
    """Apply a list of (old, new) string replacements to a file."""
    path = server_dir / fname
    text = path.read_text(encoding="utf-8")
    original = text
    changes = []
    for old, new, desc in replacements:
        if old in text:
            text = text.replace(old, new, 1)  # replace first occurrence only
            changes.append(f"  {desc}")
    if text != original:
        path.write_text(text, encoding="utf-8")
        print(f"{fname}:")
        for c in changes:
            print(c)
    else:
        print(f"{fname}: no changes")


def process_file_all(fname, replacements):
    """Apply a list of (old, new) string replacements to a file (replace all)."""
    path = server_dir / fname
    text = path.read_text(encoding="utf-8")
    original = text
    changes = []
    for old, new, desc in replacements:
        if old in text:
            count = text.count(old)
            text = text.replace(old, new)
            changes.append(f"  {desc} ({count}x)")
    if text != original:
        path.write_text(text, encoding="utf-8")
        print(f"{fname}:")
        for c in changes:
            print(c)
    else:
        print(f"{fname}: no changes")


# ===========================================================================
# F11: _mkdir → std::filesystem::create_directories
# ===========================================================================

# AccountSqliteStore.cpp: simple _mkdir, no #ifdef guard
process_file("AccountSqliteStore.cpp", [
    ('#include <direct.h>\n', '#include <filesystem>\n', "F11: <direct.h> -> <filesystem>"),
    ('    _mkdir("Accounts");', '    std::filesystem::create_directories("Accounts");', "F11: _mkdir -> create_directories"),
])

# GuildManager.cpp: _mkdir inside #ifdef _WIN32
path = server_dir / "GuildManager.cpp"
text = path.read_text(encoding="utf-8")
text = text.replace('#include <direct.h>\n', '#include <filesystem>\n')
# Replace #ifdef _WIN32 / _mkdir("Guilds"); / _mkdir(cDir); / #endif
text = text.replace(
    '#ifdef _WIN32\n\t_mkdir("Guilds");\n\t_mkdir(cDir);\n#endif',
    '\tstd::filesystem::create_directories(cDir);'
)
path.write_text(text, encoding="utf-8")
print("GuildManager.cpp:\n  F11: _mkdir -> create_directories (Guilds+cDir)")

# WarManager.cpp: three _mkdir blocks, each inside #ifdef _WIN32
path = server_dir / "WarManager.cpp"
text = path.read_text(encoding="utf-8")
text = text.replace('#include <direct.h>\n', '#include <filesystem>\n')
# All three patterns are identical: #ifdef _WIN32 / \t_mkdir("GameData"); / #endif
text = text.replace(
    '#ifdef _WIN32\n\t_mkdir("GameData");\n#endif',
    '\tstd::filesystem::create_directories("GameData");'
)
path.write_text(text, encoding="utf-8")
print("WarManager.cpp:\n  F11: _mkdir -> create_directories (3 blocks)")

# Client.cpp: just has #include <direct.h>, no _mkdir usage
path = server_dir / "Client.cpp"
text = path.read_text(encoding="utf-8")
if '#include <direct.h>' in text:
    text = text.replace('#include <direct.h>\n', '')
    path.write_text(text, encoding="utf-8")
    print("Client.cpp:\n  F11: removed unused #include <direct.h>")

# Game.cpp: has #include <direct.h> but _mkdir was already gone (was CreateDirectoryA in Wmain)
path = server_dir / "Game.cpp"
text = path.read_text(encoding="utf-8")
if '#include <direct.h>' in text:
    text = text.replace('#include <direct.h>\n', '#include <filesystem>\n')
    path.write_text(text, encoding="utf-8")
    print("Game.cpp:\n  F11: <direct.h> -> <filesystem>")

# ===========================================================================
# F4: CreateFile/ReadFile/GetFileSize/CloseHandle → fopen/fread/fclose
# ===========================================================================

# --- Game.cpp: 3 patterns ---
path = server_dir / "Game.cpp"
text = path.read_text(encoding="utf-8")

# Pattern 1: Guild file existence check (lines ~4242-4247)
# Just checking if file exists, dwFileSize unused after
text = text.replace(
    '\t\tHANDLE  hFile = CreateFile(cFn, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);\n'
    '\t\tauto dwFileSize = GetFileSize(hFile, 0);\n'
    '\n'
    '\t\tbGuildStatus = !(hFile == INVALID_HANDLE_VALUE);\n'
    '\t\tCloseHandle(hFile);',
    '\t\tbGuildStatus = std::filesystem::exists(cFn);'
)

# Also fix the remaining backslash in the guild path format string
text = text.replace(
    '"Guilds/AscII%d\\\\%s.txt"',
    '"Guilds/AscII%d/%s.txt"'
)
# Try the other form too (single escaped)
text = text.replace(
    'Guilds/AscII%d\\%s.txt',
    'Guilds/AscII%d/%s.txt'
)

# Pattern 2: bReadNotifyMsgListFile (lines ~9500-9510)
# Uses CreateFile just for file size, then fopen for actual reading
text = text.replace(
    '\tFILE* pFile;\n'
    '\tHANDLE hFile;\n'
    '\tuint32_t  dwFileSize;\n',
    '\tFILE* pFile;\n'
    '\tuint32_t  dwFileSize;\n'
)
text = text.replace(
    '\thFile = CreateFile(cFn, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);\n'
    '\tdwFileSize = GetFileSize(hFile, 0);\n'
    '\tif (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);\n'
    '\n'
    '\tpFile = fopen(cFn, "rt");',
    '\tstd::error_code ec;\n'
    '\tauto fsize = std::filesystem::file_size(cFn, ec);\n'
    '\tdwFileSize = ec ? 0 : static_cast<uint32_t>(fsize);\n'
    '\n'
    '\tpFile = fopen(cFn, "rt");'
)

# Pattern 3: RequestNoticementHandler (lines ~10142-10157)
# Full binary file read
text = text.replace(
    '\tDWORD lpNumberOfBytesRead;\n'
    '\n'
    '\tif (m_pClientList[iClientH] == 0) return;\n'
    '\n'
    '\tHANDLE hFile = CreateFile("GameConfigs/Noticement.txt", GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);\n'
    '\tuint32_t dwFileSize = GetFileSize(hFile, 0);\n'
    '\tif (dwFileSize == -1) {\n'
    '\t\treturn;\n'
    '\t}\n'
    '\n'
    '\tstd::memset(G_cData50000, 0, sizeof(G_cData50000));\n'
    '\n'
    '\tSetFilePointer(hFile, 0, 0, FILE_BEGIN);\n'
    '\n'
    '\tReadFile(hFile, G_cData50000 + sizeof(hb::net::PacketHeader), dwFileSize, &lpNumberOfBytesRead, 0);\n'
    '\tCloseHandle(hFile);',
    '\tif (m_pClientList[iClientH] == 0) return;\n'
    '\n'
    '\tFILE* pNotiFile = fopen("GameConfigs/Noticement.txt", "rb");\n'
    '\tif (!pNotiFile) return;\n'
    '\tfseek(pNotiFile, 0, SEEK_END);\n'
    '\tuint32_t dwFileSize = static_cast<uint32_t>(ftell(pNotiFile));\n'
    '\tfseek(pNotiFile, 0, SEEK_SET);\n'
    '\n'
    '\tstd::memset(G_cData50000, 0, sizeof(G_cData50000));\n'
    '\n'
    '\tfread(G_cData50000 + sizeof(hb::net::PacketHeader), 1, dwFileSize, pNotiFile);\n'
    '\tfclose(pNotiFile);'
)

path.write_text(text, encoding="utf-8")
print("Game.cpp:\n  F4: Guild exist check -> filesystem::exists\n  F4: bReadNotifyMsgListFile -> filesystem::file_size\n  F4: RequestNoticementHandler -> fopen/fread")

# --- Map.cpp: binary map file read ---
path = server_dir / "Map.cpp"
text = path.read_text(encoding="utf-8")

# Replace variable declarations
text = text.replace(
    '\tHANDLE hFile;\n'
    '\tchar  cMapFileName[256], cHeader[260], cTemp[100];\n'
    '\tDWORD dwFileSize, nRead;\n',
    '\tFILE* pMapFile;\n'
    '\tchar  cMapFileName[256], cHeader[260], cTemp[100];\n'
    '\tsize_t nRead;\n'
)

# Replace CreateFile + error check + GetFileSize
text = text.replace(
    '\thFile = CreateFile(cMapFileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);\n'
    '\tif (hFile == INVALID_HANDLE_VALUE) return false;\n'
    '\tdwFileSize = GetFileSize(hFile, 0);\n'
    '\n'
    '\tstd::memset(cHeader, 0, sizeof(cHeader));\n'
    '\tReadFile(hFile, (char*)cHeader, 256, &nRead, 0);',
    '\tpMapFile = fopen(cMapFileName, "rb");\n'
    '\tif (!pMapFile) return false;\n'
    '\n'
    '\tstd::memset(cHeader, 0, sizeof(cHeader));\n'
    '\tnRead = fread(cHeader, 1, 256, pMapFile);'
)

# Replace tile ReadFile
text = text.replace(
    '\t\t\tReadFile(hFile, (char*)cTemp, m_sTileDataSize, &nRead, 0);',
    '\t\t\tnRead = fread(cTemp, 1, m_sTileDataSize, pMapFile);'
)

# Replace CloseHandle
text = text.replace(
    '\tCloseHandle(hFile);\n\n\treturn true;',
    '\tfclose(pMapFile);\n\n\treturn true;'
)

path.write_text(text, encoding="utf-8")
print("Map.cpp:\n  F4: CreateFile/ReadFile/CloseHandle -> fopen/fread/fclose")

# --- WarManager.cpp: 3 identical "file size only" patterns ---
path = server_dir / "WarManager.cpp"
text = path.read_text(encoding="utf-8")

# All three functions have this pattern:
old_war_pattern = (
    '\tFILE* pFile;\n'
    '\tHANDLE hFile;\n'
    '\tuint32_t  dwFileSize;\n'
)
new_war_pattern = (
    '\tFILE* pFile;\n'
    '\tuint32_t  dwFileSize;\n'
)
text = text.replace(old_war_pattern, new_war_pattern)

old_war_io = (
    '\thFile = CreateFile(cFn, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);\n'
    '\tdwFileSize = GetFileSize(hFile, 0);\n'
    '\tif (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);\n'
    '\n'
    '\tpFile = fopen(cFn, "rt");'
)
new_war_io = (
    '\tstd::error_code ec;\n'
    '\tauto fsize = std::filesystem::file_size(cFn, ec);\n'
    '\tdwFileSize = ec ? 0 : static_cast<uint32_t>(fsize);\n'
    '\n'
    '\tpFile = fopen(cFn, "rt");'
)
text = text.replace(old_war_io, new_war_io)

path.write_text(text, encoding="utf-8")
print("WarManager.cpp:\n  F4: 3x CreateFile/GetFileSize -> filesystem::file_size")

# ===========================================================================
# F5: FindFirstFile → std::filesystem::directory_iterator
# ===========================================================================

path = server_dir / "AccountSqliteStore.cpp"
text = path.read_text(encoding="utf-8")

# Pattern appears 3 times. Each is a do-while loop with FindFirstFile/FindNextFile/FindClose.
# Replace the Windows block with std::filesystem::directory_iterator

# Common old pattern (appears 3 times with slight variations in the inner body):
# We'll replace the outer structure for each occurrence

# First: replace WIN32_FIND_DATA + FindFirstFile + loop structure
# Function 1: CharacterNameExistsInAnyAccount (~line 1931)
text = text.replace(
    '    WIN32_FIND_DATA findData;\n'
    '    HANDLE hFind = FindFirstFile("Accounts/*.db", &findData);\n'
    '\n'
    '    if (hFind == INVALID_HANDLE_VALUE) {\n'
    '        // No account databases found\n'
    '        return false;\n'
    '    }\n'
    '\n'
    '    bool found = false;\n'
    '\n'
    '    do {\n'
    '        // Skip directories\n'
    '        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {\n'
    '            continue;\n'
    '        }\n'
    '\n'
    '        // Build full path to database\n'
    '        char dbPath[MAX_PATH] = {};\n'
    '        std::snprintf(dbPath, sizeof(dbPath), "Accounts/%s", findData.cFileName);\n'
    '\n'
    '        // Open the database\n'
    '        sqlite3* db = nullptr;\n'
    '        if (sqlite3_open(dbPath, &db) != SQLITE_OK) {\n'
    '            if (db) sqlite3_close(db);\n'
    '            continue;\n'
    '        }\n'
    '\n'
    '        // Check if character name exists in this database\n'
    '        const char* sql = "SELECT 1 FROM characters WHERE character_name = ? COLLATE NOCASE LIMIT 1";\n'
    '        sqlite3_stmt* stmt = nullptr;\n'
    '\n'
    '        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {\n'
    '            if (sqlite3_bind_text(stmt, 1, characterName, -1, SQLITE_TRANSIENT) == SQLITE_OK) {\n'
    '                if (sqlite3_step(stmt) == SQLITE_ROW) {\n'
    '                    // Character name found in this database\n'
    '                    found = true;\n'
    '                }\n'
    '            }\n'
    '            sqlite3_finalize(stmt);\n'
    '        }\n'
    '\n'
    '        sqlite3_close(db);\n'
    '\n'
    '        if (found) {\n'
    '            break;  // No need to check remaining databases\n'
    '        }\n'
    '\n'
    '    } while (FindNextFile(hFind, &findData) != 0);\n'
    '\n'
    '    FindClose(hFind);\n'
    '\n'
    '    return found;',
    '    std::error_code ec;\n'
    '    bool found = false;\n'
    '\n'
    '    for (const auto& entry : std::filesystem::directory_iterator("Accounts", ec)) {\n'
    '        if (!entry.is_regular_file() || entry.path().extension() != ".db")\n'
    '            continue;\n'
    '\n'
    '        std::string dbPath = entry.path().string();\n'
    '\n'
    '        sqlite3* db = nullptr;\n'
    '        if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {\n'
    '            if (db) sqlite3_close(db);\n'
    '            continue;\n'
    '        }\n'
    '\n'
    '        const char* sql = "SELECT 1 FROM characters WHERE character_name = ? COLLATE NOCASE LIMIT 1";\n'
    '        sqlite3_stmt* stmt = nullptr;\n'
    '\n'
    '        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {\n'
    '            if (sqlite3_bind_text(stmt, 1, characterName, -1, SQLITE_TRANSIENT) == SQLITE_OK) {\n'
    '                if (sqlite3_step(stmt) == SQLITE_ROW) {\n'
    '                    found = true;\n'
    '                }\n'
    '            }\n'
    '            sqlite3_finalize(stmt);\n'
    '        }\n'
    '\n'
    '        sqlite3_close(db);\n'
    '        if (found) break;\n'
    '    }\n'
    '\n'
    '    return found;'
)

# Function 2: AccountNameExists (~line 1991)
text = text.replace(
    '    WIN32_FIND_DATA findData;\n'
    '    HANDLE hFind = FindFirstFile("Accounts/*.db", &findData);\n'
    '\n'
    '    if (hFind == INVALID_HANDLE_VALUE) {\n'
    '        // No account databases found\n'
    '        return false;\n'
    '    }\n'
    '\n'
    '    bool found = false;\n'
    '\n'
    '    do {\n'
    '        // Skip directories\n'
    '        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {\n'
    '            continue;\n'
    '        }\n'
    '\n'
    '        // Build full path to database\n'
    '        char dbPath[MAX_PATH] = {};\n'
    '        std::snprintf(dbPath, sizeof(dbPath), "Accounts/%s", findData.cFileName);\n'
    '\n'
    '        // Open the database\n'
    '        sqlite3* db = nullptr;\n'
    '        if (sqlite3_open(dbPath, &db) != SQLITE_OK) {\n'
    '            if (db) sqlite3_close(db);\n'
    '            continue;\n'
    '        }\n'
    '\n'
    '        // Check if account name exists in this database\n'
    '        const char* sql = "SELECT 1 FROM accounts WHERE account_name = ? COLLATE NOCASE LIMIT 1";\n'
    '        sqlite3_stmt* stmt = nullptr;\n'
    '\n'
    '        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {\n'
    '            if (sqlite3_bind_text(stmt, 1, accountName, -1, SQLITE_TRANSIENT) == SQLITE_OK) {\n'
    '                if (sqlite3_step(stmt) == SQLITE_ROW) {\n'
    '                    // Account name found in this database\n'
    '                    found = true;\n'
    '                }\n'
    '            }\n'
    '            sqlite3_finalize(stmt);\n'
    '        }\n'
    '\n'
    '        sqlite3_close(db);\n'
    '\n'
    '        if (found) {\n'
    '            break;  // No need to check remaining databases\n'
    '        }\n'
    '\n'
    '    } while (FindNextFile(hFind, &findData) != 0);\n'
    '\n'
    '    FindClose(hFind);\n'
    '\n'
    '    return found;',
    '    std::error_code ec;\n'
    '    bool found = false;\n'
    '\n'
    '    for (const auto& entry : std::filesystem::directory_iterator("Accounts", ec)) {\n'
    '        if (!entry.is_regular_file() || entry.path().extension() != ".db")\n'
    '            continue;\n'
    '\n'
    '        std::string dbPath = entry.path().string();\n'
    '\n'
    '        sqlite3* db = nullptr;\n'
    '        if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {\n'
    '            if (db) sqlite3_close(db);\n'
    '            continue;\n'
    '        }\n'
    '\n'
    '        const char* sql = "SELECT 1 FROM accounts WHERE account_name = ? COLLATE NOCASE LIMIT 1";\n'
    '        sqlite3_stmt* stmt = nullptr;\n'
    '\n'
    '        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {\n'
    '            if (sqlite3_bind_text(stmt, 1, accountName, -1, SQLITE_TRANSIENT) == SQLITE_OK) {\n'
    '                if (sqlite3_step(stmt) == SQLITE_ROW) {\n'
    '                    found = true;\n'
    '                }\n'
    '            }\n'
    '            sqlite3_finalize(stmt);\n'
    '        }\n'
    '\n'
    '        sqlite3_close(db);\n'
    '        if (found) break;\n'
    '    }\n'
    '\n'
    '    return found;'
)

# Function 3: ResolveCharacterToAccount (~line 2098)
text = text.replace(
    '    WIN32_FIND_DATA findData;\n'
    '    HANDLE hFind = FindFirstFile("Accounts/*.db", &findData);\n'
    '\n'
    '    if (hFind == INVALID_HANDLE_VALUE)\n'
    '        return false;\n'
    '\n'
    '    bool found = false;\n'
    '\n'
    '    do {\n'
    '        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)\n'
    '            continue;\n'
    '\n'
    '        char dbPath[MAX_PATH] = {};\n'
    '        std::snprintf(dbPath, sizeof(dbPath), "Accounts/%s", findData.cFileName);\n',
    '    std::error_code ec;\n'
    '    bool found = false;\n'
    '\n'
    '    for (const auto& entry : std::filesystem::directory_iterator("Accounts", ec)) {\n'
    '        if (!entry.is_regular_file() || entry.path().extension() != ".db")\n'
    '            continue;\n'
    '\n'
    '        std::string dbPathStr = entry.path().string();\n'
    '        const char* dbPath = dbPathStr.c_str();\n'
)
# Fix the end of the 3rd function's loop
text = text.replace(
    '    } while (FindNextFile(hFind, &findData) != 0);\n'
    '\n'
    '    FindClose(hFind);\n'
    '\n'
    '    return found;\n'
    '}',
    '    }\n'
    '\n'
    '    return found;\n'
    '}'
)

# Add #include <filesystem> if not present
if '<filesystem>' not in text:
    text = text.replace('#include <direct.h>\n', '#include <filesystem>\n')

path.write_text(text, encoding="utf-8")
print("AccountSqliteStore.cpp:\n  F5: 3x FindFirstFile -> directory_iterator\n  F11: <direct.h> -> <filesystem>")

# ===========================================================================
# F6: GetFileAttributes / GetModuleFileNameA → std::filesystem
# ===========================================================================

# --- GameConfigSqliteStore.cpp ---
path = server_dir / "GameConfigSqliteStore.cpp"
text = path.read_text(encoding="utf-8")

text = text.replace(
    '    std::string dbPath = "GameConfigs.db";\n'
    '    DWORD attrs = GetFileAttributes(dbPath.c_str());\n'
    '    if (attrs == INVALID_FILE_ATTRIBUTES) {\n'
    '        char modulePath[MAX_PATH] = {};\n'
    '        DWORD len = GetModuleFileNameA(nullptr, modulePath, MAX_PATH);\n'
    '        if (len > 0 && len < MAX_PATH) {\n'
    '            char* lastSlash = strrchr(modulePath, \'\\\\\');\n'
    '            if (lastSlash != nullptr) {\n'
    '                *(lastSlash + 1) = \'\\0\';\n'
    '                dbPath.assign(modulePath);\n'
    '                dbPath.append("GameConfigs.db");\n'
    '            }\n'
    '        }\n'
    '    }\n'
    '    outPath = dbPath;\n'
    '\n'
    '    bool created = false;\n'
    '    attrs = GetFileAttributes(dbPath.c_str());\n'
    '    if (attrs == INVALID_FILE_ATTRIBUTES) {\n'
    '        created = true;\n'
    '    }',
    '    std::string dbPath = "GameConfigs.db";\n'
    '    if (!std::filesystem::exists(dbPath)) {\n'
    '        auto exeDir = std::filesystem::current_path();\n'
    '        dbPath = (exeDir / "GameConfigs.db").string();\n'
    '    }\n'
    '    outPath = dbPath;\n'
    '\n'
    '    bool created = !std::filesystem::exists(dbPath);'
)

# Add #include <filesystem> if not present
if '<filesystem>' not in text:
    # Find a good place after existing includes
    text = text.replace('#include <windows.h>\n', '#include <filesystem>\n')

path.write_text(text, encoding="utf-8")
print("GameConfigSqliteStore.cpp:\n  F6: GetFileAttributes/GetModuleFileName -> std::filesystem")

# --- MapInfoSqliteStore.cpp ---
path = server_dir / "MapInfoSqliteStore.cpp"
text = path.read_text(encoding="utf-8")

text = text.replace(
    '\tstd::string dbPath = "MapInfo.db";\n'
    '\tDWORD attrs = GetFileAttributes(dbPath.c_str());\n'
    '\tif (attrs == INVALID_FILE_ATTRIBUTES) {\n'
    '\t\tchar modulePath[MAX_PATH] = {};\n'
    '\t\tDWORD len = GetModuleFileNameA(nullptr, modulePath, MAX_PATH);\n'
    '\t\tif (len > 0 && len < MAX_PATH) {\n'
    '\t\t\tchar* lastSlash = strrchr(modulePath, \'\\\\\');\n'
    '\t\t\tif (lastSlash != nullptr) {\n'
    '\t\t\t\t*(lastSlash + 1) = \'\\0\';\n'
    '\t\t\t\tdbPath.assign(modulePath);\n'
    '\t\t\t\tdbPath.append("MapInfo.db");\n'
    '\t\t\t}\n'
    '\t\t}\n'
    '\t}\n'
    '\toutPath = dbPath;\n'
    '\n'
    '\tbool created = false;\n'
    '\tattrs = GetFileAttributes(dbPath.c_str());\n'
    '\tif (attrs == INVALID_FILE_ATTRIBUTES) {\n'
    '\t\tcreated = true;\n'
    '\t}',
    '\tstd::string dbPath = "MapInfo.db";\n'
    '\tif (!std::filesystem::exists(dbPath)) {\n'
    '\t\tauto exeDir = std::filesystem::current_path();\n'
    '\t\tdbPath = (exeDir / "MapInfo.db").string();\n'
    '\t}\n'
    '\toutPath = dbPath;\n'
    '\n'
    '\tbool created = !std::filesystem::exists(dbPath);'
)

if '<filesystem>' not in text:
    text = text.replace('#include <windows.h>\n', '#include <filesystem>\n')

path.write_text(text, encoding="utf-8")
print("MapInfoSqliteStore.cpp:\n  F6: GetFileAttributes/GetModuleFileName -> std::filesystem")

# --- LoginServer.cpp ---
path = server_dir / "LoginServer.cpp"
text = path.read_text(encoding="utf-8")

text = text.replace(
    '\tchar dbPath[MAX_PATH] = {};\n'
    '\tstd::snprintf(dbPath, sizeof(dbPath), "Accounts/%s.db", lower);\n'
    '\tDWORD attrs = GetFileAttributes(dbPath);\n'
    '\treturn (attrs != INVALID_FILE_ATTRIBUTES) && ((attrs & FILE_ATTRIBUTE_DIRECTORY) == 0);',
    '\tchar dbPath[256] = {};\n'
    '\tstd::snprintf(dbPath, sizeof(dbPath), "Accounts/%s.db", lower);\n'
    '\treturn std::filesystem::exists(dbPath);'
)

if '<filesystem>' not in text:
    # Add after last #include
    lines = text.split('\n')
    last_inc = 0
    for i, line in enumerate(lines):
        if line.strip().startswith('#include'):
            last_inc = i
    lines.insert(last_inc + 1, '#include <filesystem>')
    text = '\n'.join(lines)

path.write_text(text, encoding="utf-8")
print("LoginServer.cpp:\n  F6: GetFileAttributes -> std::filesystem::exists")

# --- CmdShowChat.cpp: GetFullPathNameA → std::filesystem::absolute ---
# Note: CmdShowChat.cpp also has CreateProcess (F13) but we leave that for the next batch
path = server_dir / "CmdShowChat.cpp"
text = path.read_text(encoding="utf-8")

text = text.replace(
    '\tchar szLogPath[MAX_PATH];\n'
    '\tGetFullPathNameA("GameLogs/Chat.log", MAX_PATH, szLogPath, nullptr);',
    '\tstd::string logPath = std::filesystem::absolute("GameLogs/Chat.log").string();\n'
    '\tconst char* szLogPath = logPath.c_str();'
)

if '<filesystem>' not in text:
    text = text.replace('#include <windows.h>\n', '#include <windows.h>\n#include <filesystem>\n')

path.write_text(text, encoding="utf-8")
print("CmdShowChat.cpp:\n  F6: GetFullPathNameA -> std::filesystem::absolute")

print("\n=== All F4+F5+F6+F11 replacements complete ===")
