#pragma once

#include "sTypes.h"
#include "sLogging.h"

#if defined(__WIN32__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef struct PerfInfo {
    const char *name;
    i64 start_time;
    i64 accumulated_time;
    u32 calls;
} PerfInfo;

i64 clock_frequency = 0;
const u32 infos_size = 64;
u32 counter = 0;
PerfInfo infos[64] = {0};

void sInitPerf() {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    clock_frequency = frequency.QuadPart;
}

#define sBeginTimer(name) sBeginTimer_(name)
void sBeginTimer_(const char *name) {
    PerfInfo *info = 0;
    for(u32 i = 0; i < counter; i++) {
        if(strcmp(infos[i].name, name) == 0) {
            info = &infos[i];
        }
    }
    if(!info) {
        info = &infos[counter++];
        info->name = name;
    }
    LARGE_INTEGER start;
    QueryPerformanceCounter(&start);
    info->start_time = start.QuadPart;
}

#define sEndTimer(name) sEndTimer_(name)
void sEndTimer_(const char *name) {
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);

    PerfInfo *info = 0;
    for(u32 i = 0; i < counter; i++) {
        if(strcmp(infos[i].name, name) == 0) {
            info = &infos[i];
        }
    }
    if(!info) {
        sError("No performance counter start for %s", name);
        return;
    }

    i64 diff = end.QuadPart - info->start_time;
    diff *= 1000000;
    diff /= clock_frequency;
    info->accumulated_time += diff;
    info->calls++;
}

#define sDumpPerf() sDumpPerf_()
void sDumpPerf_() {
    for(u32 i = 0; i < counter; i++) {
        PerfInfo *info = &infos[i];

        f64 total_time = info->accumulated_time / 1000.0f;
        f64 average = total_time / (f32)info->calls;

        sLog("%s - Total time : %fms (Calls : %u, Avg : %f)",
             info->name,
             total_time,
             info->calls,
             average);
    }
}

#else

#endif
