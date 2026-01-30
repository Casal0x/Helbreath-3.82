// main_unix.cpp: Unix/Linux/macOS entry point for Helbreath Server
// Only compiled on non-Windows platforms
#ifndef _WIN32

#include "Game.h"
#include "Platform.h"
#include "UserMessages.h"
#include "Version.h"
#include "XSocket.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

// External logging functions
extern void PutLogList(char *cMsg);
extern void PutLogFileList(char *cMsg);

static bool G_bRunning = true;
static MMRESULT G_mmTimer = 0;

// Global variables required by Game.cpp and other modules
char G_cTxt[512];
char G_cData50000[50000];
HWND G_hWnd = 0; // Stub for Unix (no window handle)
bool G_bIsThread = true;
class CGame *G_pGame = nullptr;
class LoginServer *g_login = nullptr;
class XSocket *G_pListenSock = nullptr;
class XSocket *G_pLoginSock = nullptr;

// Logging functions
void PutLogList(char *cMsg) {
  if (cMsg == nullptr)
    return;
  printf("[LOG] %s\n", cMsg);
  fflush(stdout);
}

void PutLogFileList(char *cStr) {
  FILE *pFile;
  char cBuffer[512];
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);

  // Create directory if it doesn't exist (Unix-compatible)
  system("mkdir -p GameLogs");

  pFile = fopen("GameLogs/Main.log", "at");
  if (pFile == nullptr)
    return;

  snprintf(cBuffer, sizeof(cBuffer), "(%04d:%02d:%02d:%02d:%02d) - %s\n",
           timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
           timeinfo->tm_hour, timeinfo->tm_min, cStr);

  fwrite(cBuffer, 1, strlen(cBuffer), pFile);
  fclose(pFile);
}

void PutLogListLevel(int level, const char *cMsg) {
  if (cMsg == nullptr)
    return;
  printf("[LOG:%d] %s\n", level, cMsg);
  fflush(stdout);
}

void PutHackLogFileList(char *cStr) {
  FILE *pFile;
  char cBuffer[512];
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);

  system("mkdir -p GameLogs");
  pFile = fopen("GameLogs/HackEvents.log", "at");
  if (pFile == nullptr)
    return;

  snprintf(cBuffer, sizeof(cBuffer), "(%04d:%02d:%02d:%02d:%02d) - %s\n",
           timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
           timeinfo->tm_hour, timeinfo->tm_min, cStr);

  fwrite(cBuffer, 1, strlen(cBuffer), pFile);
  fclose(pFile);
}

void PutPvPLogFileList(char *cStr) {
  FILE *pFile;
  char cBuffer[512];
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);

  system("mkdir -p GameLogs");
  pFile = fopen("GameLogs/PvPEvents.log", "at");
  if (pFile == nullptr)
    return;

  snprintf(cBuffer, sizeof(cBuffer), "(%04d:%02d:%02d:%02d:%02d) - %s\n",
           timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
           timeinfo->tm_hour, timeinfo->tm_min, cStr);

  fwrite(cBuffer, 1, strlen(cBuffer), pFile);
  fclose(pFile);
}

void PutItemLogFileList(char *cStr) {
  FILE *pFile;
  char cBuffer[512];
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);

  system("mkdir -p GameLogs");
  pFile = fopen("GameLogs/ItemEvents.log", "at");
  if (pFile == nullptr)
    return;

  snprintf(cBuffer, sizeof(cBuffer), "(%04d:%02d:%02d:%02d:%02d) - %s\n",
           timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
           timeinfo->tm_hour, timeinfo->tm_min, cStr);

  fwrite(cBuffer, 1, strlen(cBuffer), pFile);
  fclose(pFile);
}

void PutLogEventFileList(char *cStr) {
  FILE *pFile;
  char cBuffer[512];
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);

  system("mkdir -p GameLogs");
  pFile = fopen("GameLogs/LogEvents.log", "at");
  if (pFile == nullptr)
    return;

  snprintf(cBuffer, sizeof(cBuffer), "(%04d:%02d:%02d:%02d:%02d) - %s\n",
           timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
           timeinfo->tm_hour, timeinfo->tm_min, cStr);

  fwrite(cBuffer, 1, strlen(cBuffer), pFile);
  fclose(pFile);
}

void PutAdminLogFileList(char *cStr) {
  FILE *pFile;
  char cBuffer[512];
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);

  system("mkdir -p GameLogs");
  pFile = fopen("GameLogs/AdminEvents.log", "at");
  if (pFile == nullptr)
    return;

  snprintf(cBuffer, sizeof(cBuffer), "(%04d:%02d:%02d:%02d:%02d) - %s\n",
           timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
           timeinfo->tm_hour, timeinfo->tm_min, cStr);

  fwrite(cBuffer, 1, strlen(cBuffer), pFile);
  fclose(pFile);
}

// Signal handler for graceful shutdown
void signal_handler(int sig) {
  char msg[128];
  snprintf(msg, sizeof(msg),
           "[INFO] Received signal %d, initiating shutdown...", sig);
  PutLogList(msg);
  G_bRunning = false;
}

// Poll all sockets for network events
void PollAllSockets() {
  if (G_pGame == nullptr)
    return;

  // Poll listen sockets for new connections
  if (G_pListenSock != nullptr) {
    int iRet = G_pListenSock->Poll();
    if (iRet == DEF_XSOCKEVENT_CONNECTIONESTABLISH) {
      G_pGame->bAccept(G_pListenSock);
    } else if (iRet < 0 && iRet != DEF_XSOCKEVENT_QUENEFULL) {
      char msg[256];
      snprintf(msg, sizeof(msg), "[ERROR] ListenSock Poll() error: %d", iRet);
      PutLogList(msg);
    }
  }

  if (G_pLoginSock != nullptr) {
    int iRet = G_pLoginSock->Poll();
    if (iRet == DEF_XSOCKEVENT_CONNECTIONESTABLISH) {
      G_pGame->bAcceptLogin(G_pLoginSock);
    } else if (iRet < 0 && iRet != DEF_XSOCKEVENT_QUENEFULL) {
      char msg[256];
      snprintf(msg, sizeof(msg), "[ERROR] LoginSock Poll() error: %d", iRet);
      PutLogList(msg);
    }
  }

  // Poll all game client sockets
  for (int i = 1; i < DEF_MAXCLIENTS; i++) {
    if (G_pGame->m_pClientList[i] != nullptr &&
        G_pGame->m_pClientList[i]->m_pXSock != nullptr) {
      G_pGame->OnClientSocketEvent(i);
    }
  }

  // Poll all login client sockets
  for (int i = 0; i < DEF_MAXCLIENTLOGINSOCK; i++) {
    if (G_pGame->_lclients[i] != nullptr &&
        G_pGame->_lclients[i]->_sock != nullptr) {
      G_pGame->OnLoginClientSocketEvent(i);
    }
  }
}

// Timer callback function
void timer_callback(void *userData) {
  (void)userData;
  if (G_pGame != nullptr) {
    G_pGame->OnTimer(0);
  }
}

// Main event loop (Unix version)
int EventLoop() {
  static uint32_t dwLastGameProcess = 0;
  static uint32_t dwLastTimerEvent = 0;

  printf("[INFO] Entering event loop...\n");

  while (G_bRunning) {
    if (G_pGame != nullptr) {
      uint32_t dwNow = GameClock::GetTimeMS();

      // Run game logic every 300ms (same as Windows timer)
      if (dwNow - dwLastGameProcess >= 300) {
        G_pGame->GameProcess();
        dwLastGameProcess = dwNow;
      }

      // Run timer events every 300ms (replaces Windows timer callback)
      if (dwNow - dwLastTimerEvent >= 300) {
        G_pGame->OnTimer(0);
        dwLastTimerEvent = dwNow;
      }

      // Poll all sockets for network events
      PollAllSockets();
    }

    // Small sleep to prevent 100% CPU usage (1ms)
    usleep(1000);
  }

  printf("[INFO] Event loop exiting...\n");
  return 0;
}

// Initialize server
bool Initialize() {
  printf("[INFO] Initializing socket subsystem...\n");
  if (!Platform::InitSockets()) {
    fprintf(stderr, "[ERROR] Failed to initialize socket subsystem!\n");
    return false;
  }

  printf("[INFO] Creating game instance...\n");
  G_pGame = new CGame(nullptr); // No HWND on Unix
  if (G_pGame->bInit() == false) {
    PutLogList("(!!!) Game initialization failed!");
    return false;
  }

  printf("[INFO] Starting timer (300ms interval)...\n");
  G_mmTimer = Platform::Timer::Start(300, timer_callback, nullptr);
  if (G_mmTimer == 0) {
    fprintf(stderr,
            "[WARNING] Failed to start timer, will use polling fallback\n");
  }

  printf("[INFO] Creating game socket (IP: %s, Port: %d)...\n",
         G_pGame->m_cGameListenIP, G_pGame->m_iGameListenPort);

  G_pListenSock = new XSocket(DEF_SERVERSOCKETBLOCKLIMIT);
  if (!G_pListenSock->bListen(G_pGame->m_cGameListenIP,
                              G_pGame->m_iGameListenPort)) {
    fprintf(stderr, "[ERROR] Failed to bind game socket!\n");
    return false;
  }

  printf("[INFO] Creating login socket (IP: %s, Port: %d)...\n",
         G_pGame->m_cLoginListenIP, G_pGame->m_iLoginListenPort);

  G_pLoginSock = new XSocket(DEF_SERVERSOCKETBLOCKLIMIT);
  if (!G_pLoginSock->bListen(G_pGame->m_cLoginListenIP,
                             G_pGame->m_iLoginListenPort)) {
    fprintf(stderr, "[ERROR] Failed to bind login socket!\n");
    return false;
  }

  printf("[INFO] Server initialization complete!\n");
  return true;
}

// Cleanup on shutdown
void OnDestroy() {
  printf("\n[INFO] Shutting down server...\n");

  if (G_pListenSock != nullptr) {
    delete G_pListenSock;
    G_pListenSock = nullptr;
  }

  if (G_pLoginSock != nullptr) {
    delete G_pLoginSock;
    G_pLoginSock = nullptr;
  }

  if (G_pGame != nullptr) {
    G_pGame->Quit();
    delete G_pGame;
    G_pGame = nullptr;
  }

  if (g_login != nullptr) {
    delete g_login;
    g_login = nullptr;
  }

  if (G_mmTimer != 0) {
    Platform::Timer::Stop(G_mmTimer);
    G_mmTimer = 0;
  }

  Platform::TermSockets();

  printf("[INFO] Server shutdown complete.\n");
}

// Main entry point
int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  // Get current time
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);

  // Print startup banner
  printf("\n");
  printf("====================================================================="
         "==\n");
  printf(
      "         HELBREATH GAME SERVER - UNIX/LINUX/MACOS BUILD             \n");
  printf("====================================================================="
         "==\n");
  printf("Version: %s.%s\n", DEF_UPPERVERSION, DEF_LOWERVERSION);
  printf("Build: %d\n", DEF_BUILDDATE);
  printf("Started: %d/%d/%d %02d:%02d:%02d\n", timeinfo->tm_mon + 1,
         timeinfo->tm_mday, timeinfo->tm_year + 1900, timeinfo->tm_hour,
         timeinfo->tm_min, timeinfo->tm_sec);
  printf("Platform: Unix/Linux/macOS\n");
  printf("====================================================================="
         "==\n\n");

  // Install signal handlers for graceful shutdown
  signal(SIGINT, signal_handler);  // Ctrl+C
  signal(SIGTERM, signal_handler); // kill command
  signal(SIGHUP, signal_handler);  // Terminal hangup

  printf("Initializing server...\n\n");

  // Initialize server
  if (!Initialize()) {
    fprintf(stderr, "\n[FATAL] Server initialization failed!\n");
    OnDestroy();
    return 1;
  }

  printf("\n");
  printf("====================================================================="
         "==\n");
  printf("                    SERVER IS NOW RUNNING                            "
         "\n");
  printf("====================================================================="
         "==\n");
  printf("Press Ctrl+C to shutdown gracefully\n");
  printf("====================================================================="
         "==\n\n");

  // Run event loop
  int result = EventLoop();

  // Cleanup
  OnDestroy();

  return result;
}

#endif // !_WIN32
