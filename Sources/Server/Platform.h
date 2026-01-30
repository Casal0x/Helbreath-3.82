// Platform.h: Cross-platform abstraction layer for Windows/Unix compatibility
// This header provides Unix equivalents for Windows types and functions
#pragma once

#ifdef _WIN32
// Windows builds use native headers - no abstraction needed
#define _WINSOCKAPI_ // Prevent winsock.h from loading
#include <mmsystem.h>
#include <process.h>
#include <windows.h>
#include <winsock2.h>
#else
// Unix/Linux/macOS platform abstraction
#include <arpa/inet.h>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Type compatibility layer - Windows types mapped to Unix equivalents
typedef int SOCKET;
typedef void *HWND;
typedef void *HANDLE;
typedef unsigned int UINT;
typedef unsigned long WPARAM;
typedef long LPARAM;
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef long LONG;
typedef int BOOL;
typedef unsigned long MMRESULT;
typedef void *HINSTANCE;
typedef int64_t __int64; // Windows 64-bit int

// POINT structure (Windows GDI)
typedef struct tagPOINT {
  int x;
  int y;
} POINT, *PPOINT, *LPPOINT;

// RECT structure (Windows GDI)
typedef struct tagRECT {
  int left;
  int top;
  int right;
  int bottom;
} RECT, *PRECT, *LPRECT;
typedef const RECT *LPCRECT;

// SYSTEMTIME structure
typedef struct tagSYSTEMTIME {
  unsigned short wYear;
  unsigned short wMonth;
  unsigned short wDayOfWeek;
  unsigned short wDay;
  unsigned short wHour;
  unsigned short wMinute;
  unsigned short wSecond;
  unsigned short wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME;

// WIN32_FIND_DATA structure (minimal)
typedef struct _WIN32_FIND_DATA {
  DWORD dwFileAttributes;
  char cFileName[260];
  // Unix-specific: internal state for directory traversal
  DIR* _dir;
  char _pattern[260];
  char _dirPath[260];
} WIN32_FIND_DATA;

// Socket constants
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)

// File/Path constants
#define MAX_PATH 260
#define INVALID_HANDLE_VALUE ((HANDLE) - 1)
#define INVALID_FILE_ATTRIBUTES ((DWORD) - 1)
#define FILE_ATTRIBUTE_DIRECTORY 0x10
#define GENERIC_READ 0x80000000L
#define GENERIC_WRITE 0x40000000L
#define OPEN_EXISTING 3
#define CREATE_ALWAYS 2

// Boolean constants (must be before inline functions)
#define FALSE 0
#define TRUE 1

// Virtual key codes (Windows keyboard constants)
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B
#define VK_HOME 0x24
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E

// Window messages
#define WM_USER 0x0400

// File seek constants
#define FILE_BEGIN 0
#define FILE_CURRENT 1
#define FILE_END 2

// Function macros - basic replacements
#define closesocket(s) close(s)
#define WSAGetLastError() errno
#define Sleep(ms) usleep((ms) * 1000)
#define GetModuleHandle(x) nullptr
#define GetLastError() errno
#define _mkdir(path) mkdir(path, 0755)
#define ZeroMemory(p, sz) memset((p), 0, (sz))
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define timeGetTime() Platform::GetTickCount()

// String functions
inline char *strcpy_s_impl(char *dst, size_t dstSize, const char *src) {
  strncpy(dst, src, dstSize - 1);
  dst[dstSize - 1] = '\0';
  return dst;
}
#define strcpy_s(dst, src) strcpy_s_impl(dst, sizeof(dst), src)
#define strtok_s(str, delim, context) strtok_r(str, delim, context)

// GetLocalTime implementation
inline void GetLocalTime(SYSTEMTIME *st) {
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);
  st->wYear = timeinfo->tm_year + 1900;
  st->wMonth = timeinfo->tm_mon + 1;
  st->wDayOfWeek = timeinfo->tm_wday;
  st->wDay = timeinfo->tm_mday;
  st->wHour = timeinfo->tm_hour;
  st->wMinute = timeinfo->tm_min;
  st->wSecond = timeinfo->tm_sec;
  st->wMilliseconds = 0;
}

// wsprintf replacement with snprintf
inline int wsprintf(char *buffer, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int result = vsnprintf(buffer, 1024, format, args);
  va_end(args);
  return result;
}

// File API implementations
inline HANDLE CreateFile(const char *filename, DWORD access, DWORD share,
                         void *security, DWORD creation, DWORD flags,
                         HANDLE templateFile) {
  (void)share;
  (void)security;
  (void)flags;
  (void)templateFile;

  int fd = -1;
  if (creation == OPEN_EXISTING) {
    fd = open(filename, (access & GENERIC_WRITE) ? O_RDWR : O_RDONLY);
  } else if (creation == CREATE_ALWAYS) {
    fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
  }

  return (fd == -1) ? INVALID_HANDLE_VALUE : (HANDLE)(intptr_t)fd;
}

inline HANDLE CreateFileA(const char *filename, DWORD access, DWORD share,
                          void *security, DWORD creation, DWORD flags,
                          HANDLE templateFile) {
  return CreateFile(filename, access, share, security, creation, flags,
                    templateFile);
}

inline DWORD GetFileSize(HANDLE hFile, DWORD *highSize) {
  if (highSize)
    *highSize = 0;
  if (hFile == INVALID_HANDLE_VALUE)
    return 0;

  int fd = (int)(intptr_t)hFile;
  struct stat st;
  if (fstat(fd, &st) != 0)
    return 0;
  return (DWORD)st.st_size;
}

inline BOOL ReadFile(HANDLE hFile, void *buffer, DWORD bytesToRead,
                     DWORD *bytesRead, void *overlapped) {
  (void)overlapped;
  if (hFile == INVALID_HANDLE_VALUE)
    return FALSE;

  int fd = (int)(intptr_t)hFile;
  ssize_t result = read(fd, buffer, bytesToRead);
  if (result < 0) {
    if (bytesRead)
      *bytesRead = 0;
    return FALSE;
  }
  if (bytesRead)
    *bytesRead = (DWORD)result;
  return TRUE;
}

inline BOOL WriteFile(HANDLE hFile, const void *buffer, DWORD bytesToWrite,
                      DWORD *bytesWritten, void *overlapped) {
  (void)overlapped;
  if (hFile == INVALID_HANDLE_VALUE)
    return FALSE;

  int fd = (int)(intptr_t)hFile;
  ssize_t result = write(fd, buffer, bytesToWrite);
  if (result < 0) {
    if (bytesWritten)
      *bytesWritten = 0;
    return FALSE;
  }
  if (bytesWritten)
    *bytesWritten = (DWORD)result;
  return TRUE;
}

inline BOOL CloseHandle(HANDLE hFile) {
  if (hFile == INVALID_HANDLE_VALUE)
    return FALSE;
  int fd = (int)(intptr_t)hFile;
  return close(fd) == 0 ? TRUE : FALSE;
}

// GetModuleFileName implementation
inline DWORD GetModuleFileNameA(HINSTANCE hModule, char *filename, DWORD size) {
  (void)hModule;
  if (getcwd(filename, size) != nullptr) {
    return strlen(filename);
  }
  return 0;
}

// DeleteFile implementation
inline BOOL DeleteFile(const char *filename) {
  return (unlink(filename) == 0) ? TRUE : FALSE;
}

// GetCursorPos stub (no-op - server has no GUI)
inline BOOL GetCursorPos(POINT *pt) {
  if (pt) {
    pt->x = 0;
    pt->y = 0;
  }
  return TRUE;
}

// SetRect implementation
inline BOOL SetRect(RECT *rect, int left, int top, int right, int bottom) {
  if (rect) {
    rect->left = left;
    rect->top = top;
    rect->right = right;
    rect->bottom = bottom;
    return TRUE;
  }
  return FALSE;
}

// InvalidateRect stub (no-op - server has no GUI)
inline BOOL InvalidateRect(HWND hwnd, const RECT *rect, BOOL erase) {
  (void)hwnd;
  (void)rect;
  (void)erase;
  return TRUE;
}

// SetFilePointer implementation
inline DWORD SetFilePointer(HANDLE hFile, LONG distance, LONG *distanceHigh,
                            DWORD moveMethod) {
  if (hFile == INVALID_HANDLE_VALUE)
    return 0xFFFFFFFF;
  if (distanceHigh)
    *distanceHigh = 0;

  int fd = (int)(intptr_t)hFile;
  int whence = (moveMethod == FILE_BEGIN)     ? SEEK_SET
               : (moveMethod == FILE_CURRENT) ? SEEK_CUR
                                              : SEEK_END;
  off_t result = lseek(fd, distance, whence);
  return (result == -1) ? 0xFFFFFFFF : (DWORD)result;
}

// GetFileAttributes implementation
inline DWORD GetFileAttributes(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0) {
    return INVALID_FILE_ATTRIBUTES;
  }
  return S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : 0;
}

// Helper function to match wildcard patterns (simple * matching)
inline bool MatchPattern(const char* filename, const char* pattern) {
  // Find the * in pattern
  const char* star = strchr(pattern, '*');
  if (!star) {
    // No wildcard, exact match
    return strcmp(filename, pattern) == 0;
  }

  // Match prefix before *
  size_t prefixLen = star - pattern;
  if (strncmp(filename, pattern, prefixLen) != 0) {
    return false;
  }

  // Match suffix after *
  const char* suffix = star + 1;
  size_t suffixLen = strlen(suffix);
  size_t filenameLen = strlen(filename);

  if (suffixLen > 0 && filenameLen >= suffixLen) {
    return strcmp(filename + filenameLen - suffixLen, suffix) == 0;
  }

  return suffixLen == 0;
}

// Forward declaration
inline BOOL FindNextFile(HANDLE hFind, WIN32_FIND_DATA *findData);

// FindFirstFile/FindNextFile implementations for Unix
inline HANDLE FindFirstFile(const char *path, WIN32_FIND_DATA *findData) {
  if (!path || !findData) {
    return INVALID_HANDLE_VALUE;
  }

  // Initialize internal state
  findData->_dir = nullptr;

  // Parse path into directory and pattern
  // Example: "Accounts\\*.db" -> dir="Accounts", pattern="*.db"
  char tempPath[MAX_PATH];
  strncpy(tempPath, path, sizeof(tempPath) - 1);
  tempPath[sizeof(tempPath) - 1] = '\0';

  // Replace backslashes with forward slashes
  for (char* p = tempPath; *p; ++p) {
    if (*p == '\\') *p = '/';
  }

  // Find last slash to separate directory from pattern
  char* lastSlash = strrchr(tempPath, '/');
  if (lastSlash) {
    *lastSlash = '\0';
    strncpy(findData->_dirPath, tempPath, sizeof(findData->_dirPath) - 1);
    strncpy(findData->_pattern, lastSlash + 1, sizeof(findData->_pattern) - 1);
  } else {
    // No directory, use current directory
    strcpy(findData->_dirPath, ".");
    strncpy(findData->_pattern, tempPath, sizeof(findData->_pattern) - 1);
  }

  findData->_dirPath[sizeof(findData->_dirPath) - 1] = '\0';
  findData->_pattern[sizeof(findData->_pattern) - 1] = '\0';

  // Open directory
  findData->_dir = opendir(findData->_dirPath);
  if (!findData->_dir) {
    return INVALID_HANDLE_VALUE;
  }

  // Find first matching file
  if (FindNextFile((HANDLE)findData, findData)) {
    return (HANDLE)findData;
  }

  closedir(findData->_dir);
  findData->_dir = nullptr;
  return INVALID_HANDLE_VALUE;
}

inline BOOL FindNextFile(HANDLE hFind, WIN32_FIND_DATA *findData) {
  if (hFind == INVALID_HANDLE_VALUE || !findData || !findData->_dir) {
    return FALSE;
  }

  struct dirent* entry;
  while ((entry = readdir(findData->_dir)) != nullptr) {
    // Skip . and ..
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    // Check if filename matches pattern
    if (MatchPattern(entry->d_name, findData->_pattern)) {
      strncpy(findData->cFileName, entry->d_name, sizeof(findData->cFileName) - 1);
      findData->cFileName[sizeof(findData->cFileName) - 1] = '\0';

      // Get file attributes
      char fullPath[MAX_PATH];
      snprintf(fullPath, sizeof(fullPath), "%s/%s", findData->_dirPath, entry->d_name);

      struct stat st;
      if (stat(fullPath, &st) == 0) {
        findData->dwFileAttributes = S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : 0;
      } else {
        findData->dwFileAttributes = 0;
      }

      return TRUE;
    }
  }

  return FALSE;
}

inline BOOL FindClose(HANDLE hFind) {
  if (hFind == INVALID_HANDLE_VALUE) {
    return TRUE;
  }

  WIN32_FIND_DATA* findData = (WIN32_FIND_DATA*)hFind;
  if (findData->_dir) {
    closedir(findData->_dir);
    findData->_dir = nullptr;
  }

  return TRUE;
}

// Message/Window stubs (no-op on Unix - server has no GUI)
#define PostQuitMessage(x)
#define PostMessage(hwnd, msg, wp, lp)
#define SendMessage(hwnd, msg, wp, lp) 0
#define PeekMessage(msg, hwnd, min, max, remove) false
#define GetMessage(msg, hwnd, min, max) false
#define TranslateMessage(msg)
#define DispatchMessage(msg)
#define DefWindowProc(hwnd, msg, wp, lp) 0

// Event/Handle stubs
typedef void *WSAEVENT;
#define WSA_INVALID_EVENT nullptr
#define WAIT_OBJECT_0 0
#define FALSE 0
#define TRUE 1

// Threading stubs
#define _endthread() pthread_exit(nullptr)
#define __stdcall

// Window message constants (not used on Unix but defined for compatibility)
#define WM_DESTROY 0x0002
#define WM_CLOSE 0x0010
#define PM_NOREMOVE 0

// Dummy MSG structure
struct MSG {
  HWND hwnd;
  UINT message;
  WPARAM wParam;
  LPARAM lParam;
};

#endif // _WIN32

// Cross-platform socket initialization
namespace Platform {
bool InitSockets();
void TermSockets();
uint32_t GetTickCount(); // Cross-platform implementation

// Timer functionality (platform-specific implementation)
class Timer {
public:
  static MMRESULT Start(uint32_t intervalMs, void (*callback)(void *),
                        void *userData);
  static void Stop(MMRESULT timerId);
};
} // namespace Platform

// Log levels (cross-platform)
#define LOG_LEVEL_ERROR 3
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_NOTICE 1

// External function declarations (defined in Wmain.cpp)
extern void PutLogList(char *msg);
extern void PutLogFileList(char *msg);
extern void PutLogListLevel(int level, const char *msg);
extern char G_cTxt[512]; // Global text buffer
