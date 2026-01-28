#pragma once

// Benchmark.h - Lightweight performance profiling macros and debug console
// Outputs to debug console window when allocated

#include <windows.h>
#include <stdio.h>
#include <psapi.h>

// ============================================================================
// Debug Console
// ============================================================================
//
// Call DebugConsole::Allocate() to create a console window for debug output.
// Call DebugConsole::Deallocate() to close it.
// All printf/BENCHMARK output will appear in the console when allocated.
//
// Example:
//    DebugConsole::Allocate();
//    printf("Debug message\n");
//    DebugConsole::PrintMemory("Checkpoint");
//    DebugConsole::Deallocate();
//

class DebugConsole {
private:
    static bool s_bAllocated;
    static FILE* s_pOut;
    static FILE* s_pErr;
    static SIZE_T s_lastWorkingSet;

public:
    static void Allocate()
    {
        if (s_bAllocated) return;

        if (AllocConsole())
        {
            freopen_s(&s_pOut, "CONOUT$", "w", stdout);
            freopen_s(&s_pErr, "CONOUT$", "w", stderr);

            SetConsoleTitleA("Helbreath Debug Console");

            // Enable ANSI colors (Windows 10+)
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD dwMode = 0;
            GetConsoleMode(hOut, &dwMode);
            SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

            s_bAllocated = true;
            s_lastWorkingSet = 0;
            printf("=== Helbreath Debug Console ===\n\n");
        }
    }

    static void Deallocate()
    {
        if (!s_bAllocated) return;

        if (s_pOut) fclose(s_pOut);
        if (s_pErr) fclose(s_pErr);
        s_pOut = nullptr;
        s_pErr = nullptr;

        FreeConsole();
        s_bAllocated = false;
    }

    static bool IsAllocated() { return s_bAllocated; }

    static void PrintMemory(const char* pLabel)
    {
        if (!s_bAllocated) return;

        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
        {
            double dWorkingMB = pmc.WorkingSetSize / (1024.0 * 1024.0);
            double dPrivateMB = pmc.PrivateUsage / (1024.0 * 1024.0);
            double dPeakMB = pmc.PeakWorkingSetSize / (1024.0 * 1024.0);

            if (s_lastWorkingSet > 0)
            {
                double dDeltaMB = (double)((LONGLONG)pmc.WorkingSetSize - (LONGLONG)s_lastWorkingSet) / (1024.0 * 1024.0);
                const char* pSign = (dDeltaMB >= 0) ? "+" : "";
                printf("[MEMORY] %-20s Working: %6.2f MB | Private: %6.2f MB | Peak: %6.2f MB (%s%.2f MB)\n",
                    pLabel, dWorkingMB, dPrivateMB, dPeakMB, pSign, dDeltaMB);
            }
            else
            {
                printf("[MEMORY] %-20s Working: %6.2f MB | Private: %6.2f MB | Peak: %6.2f MB\n",
                    pLabel, dWorkingMB, dPrivateMB, dPeakMB);
            }

            s_lastWorkingSet = pmc.WorkingSetSize;
        }
    }

    static void ResetMemoryDelta()
    {
        s_lastWorkingSet = 0;
    }
};

// Static member initialization
__declspec(selectany) bool DebugConsole::s_bAllocated = false;
__declspec(selectany) FILE* DebugConsole::s_pOut = nullptr;
__declspec(selectany) FILE* DebugConsole::s_pErr = nullptr;
__declspec(selectany) SIZE_T DebugConsole::s_lastWorkingSet = 0;

// ============================================================================
// Usage Examples:
// ============================================================================
//
// 1. Scope-based timing (automatic start/end with 1-second averaging):
//    {
//        BENCHMARK_SCOPE("DrawObjects");
//        DrawObjects(x, y);  // Measured and averaged over 1 second
//    }
//    // Output example: [BENCHMARK] DrawObjects: 2.5 ms/call (60 calls, 150 ms total in 1s)
//
// 2. Manual timing:
//    BENCHMARK_START(render);
//    RenderScene();
//    BENCHMARK_END(render, "RenderScene");
//
// 3. Quick one-liner:
//    BENCHMARK_STMT("Update", m_pMapData->iObjectFrameCounter());
//
// ============================================================================

#ifdef _DEBUG

// Internal helper class for scope-based benchmarks with 1-second averaging
class _BenchmarkScope {
private:
    LARGE_INTEGER m_liStart;
    LARGE_INTEGER m_liFreq;
    const char* m_pName;

    // Static state for accumulating measurements over 1 second
    struct BenchmarkSlot {
        const char* pName;
        LONGLONG liAccumTicks;
        int iCallCount;
        DWORD dwLastOutputTime;
    };
    static BenchmarkSlot s_slots[64];
    static int s_iSlotCount;

    int FindOrCreateSlot(const char* pName) {
        // Find existing slot
        for (int i = 0; i < s_iSlotCount; i++) {
            if (strcmp(s_slots[i].pName, pName) == 0) {
                return i;
            }
        }

        // Create new slot
        if (s_iSlotCount < 64) {
            int idx = s_iSlotCount++;
            s_slots[idx].pName = pName;
            s_slots[idx].liAccumTicks = 0;
            s_slots[idx].iCallCount = 0;
            s_slots[idx].dwLastOutputTime = timeGetTime();
            return idx;
        }
        return -1;
    }

public:
    _BenchmarkScope(const char* pName)
        : m_pName(pName)
    {
        QueryPerformanceFrequency(&m_liFreq);
        QueryPerformanceCounter(&m_liStart);
    }

    ~_BenchmarkScope()
    {
        LARGE_INTEGER liEnd;
        QueryPerformanceCounter(&liEnd);

        LONGLONG elapsed = liEnd.QuadPart - m_liStart.QuadPart;

        int slot = FindOrCreateSlot(m_pName);
        if (slot < 0) return; // Slot table full

        // Accumulate
        s_slots[slot].liAccumTicks += elapsed;
        s_slots[slot].iCallCount++;

        // Check if 1 second has passed
        DWORD dwNow = timeGetTime();
        if ((dwNow - s_slots[slot].dwLastOutputTime) >= 1000) {
            // Calculate average
            double dAvgMs = (s_slots[slot].liAccumTicks * 1000.0) / (m_liFreq.QuadPart * s_slots[slot].iCallCount);
            double dAvgUs = (s_slots[slot].liAccumTicks * 1000000.0) / (m_liFreq.QuadPart * s_slots[slot].iCallCount);
            double dTotalMs = (s_slots[slot].liAccumTicks * 1000.0) / m_liFreq.QuadPart;

            char buffer[256];
            if (dAvgMs >= 1.0) {
                sprintf_s(buffer, "[BENCHMARK] %s: %.3f ms/call (%d calls, %.1f ms total in 1s)\n",
                         m_pName, dAvgMs, s_slots[slot].iCallCount, dTotalMs);
            } else {
                sprintf_s(buffer, "[BENCHMARK] %s: %.1f μs/call (%d calls, %.3f ms total in 1s)\n",
                         m_pName, dAvgUs, s_slots[slot].iCallCount, dTotalMs);
            }
            printf(buffer);

            // Reset for next second
            s_slots[slot].liAccumTicks = 0;
            s_slots[slot].iCallCount = 0;
            s_slots[slot].dwLastOutputTime = dwNow;
        }
    }
};

// Static members initialization
__declspec(selectany) _BenchmarkScope::BenchmarkSlot _BenchmarkScope::s_slots[64];
__declspec(selectany) int _BenchmarkScope::s_iSlotCount = 0;

// Scope-based benchmark (RAII - automatic timing)
// Automatically accumulates over 1 second and outputs average
#define BENCHMARK_SCOPE(name) \
    _BenchmarkScope __benchmark_##__LINE__(name)

// Manual start/end benchmarks
#define BENCHMARK_START(var) \
    LARGE_INTEGER __bench_start_##var, __bench_freq_##var; \
    QueryPerformanceFrequency(&__bench_freq_##var); \
    QueryPerformanceCounter(&__bench_start_##var);

#define BENCHMARK_END(var, name) \
    { \
        LARGE_INTEGER __bench_end_##var; \
        QueryPerformanceCounter(&__bench_end_##var); \
        LONGLONG __elapsed_##var = __bench_end_##var.QuadPart - __bench_start_##var.QuadPart; \
        double __ms_##var = (__elapsed_##var * 1000.0) / __bench_freq_##var.QuadPart; \
        double __us_##var = (__elapsed_##var * 1000000.0) / __bench_freq_##var.QuadPart; \
        char __buffer_##var[256]; \
        if (__ms_##var >= 1.0) { \
            sprintf_s(__buffer_##var, "[BENCHMARK] %s: %.3f ms\n", name, __ms_##var); \
        } else { \
            sprintf_s(__buffer_##var, "[BENCHMARK] %s: %.1f μs\n", name, __us_##var); \
        } \
        printf(__buffer_##var); \
    }

// Quick one-liner for measuring a single statement
#define BENCHMARK_STMT(name, stmt) \
    { \
        BENCHMARK_SCOPE(name); \
        stmt; \
    }

#else
// Release mode - all benchmarks compile to nothing
#define BENCHMARK_SCOPE(name)
#define BENCHMARK_START(var)
#define BENCHMARK_END(var, name)
#define BENCHMARK_STMT(name, stmt) stmt
#endif
