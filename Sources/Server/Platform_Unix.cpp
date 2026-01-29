// Platform_Unix.cpp: Unix/Linux/macOS platform-specific implementations
#ifndef _WIN32

#include "Platform.h"
#include <cstring>
#include <sys/time.h>
#include <unistd.h>

// Initialize socket subsystem on Unix (no-op, sockets always available)
namespace Platform {
bool InitSockets() {
  return true; // Unix sockets don't need initialization
}

void TermSockets() {
  // No cleanup needed on Unix
}

// Get millisecond tick count (Unix equivalent of Windows GetTickCount)
uint32_t GetTickCount() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

// Timer class - simplified stubs for Unix (not used in demo)
MMRESULT Timer::Start(uint32_t intervalMs, void (*callback)(void *),
                      void *userData) {
  static int id = 1;
  (void)intervalMs;
  (void)callback;
  (void)userData;
  // Stub implementation - would need pthread_create for real timers
  return id++;
}

void Timer::Stop(MMRESULT timerId) {
  (void)timerId;
  // Stub - no-op
}
} // namespace Platform

#endif // !_WIN32
