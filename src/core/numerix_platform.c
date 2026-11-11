#include "numerix_internal.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
double numerix_platform_monotonic_seconds(void)
{
    LARGE_INTEGER counter;
    LARGE_INTEGER frequency;
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart == 0 ||
        !QueryPerformanceCounter(&counter)) return 0.0;
    return (double)counter.QuadPart / (double)frequency.QuadPart;
}
#else
#include <time.h>
double numerix_platform_monotonic_seconds(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return 0.0;
    return (double)now.tv_sec + (double)now.tv_nsec / 1000000000.0;
}
#endif
