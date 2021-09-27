#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "sTypes.h"

#define MAX_LOG_LENGTH 1024

#ifdef DEBUG
// Adds a new line char at the end
#define sTrace(message, ...) sLogOutputLine(LOG_LEVEL_TRACE, message, __VA_ARGS__)
#define sLog(message, ...) sLogOutputLine(LOG_LEVEL_LOG, message, __VA_ARGS__)
#define sWarn(message, ...) sLogOutputLine(LOG_LEVEL_WARN, message, __VA_ARGS__)
#define sError(message, ...) sLogOutputLine(LOG_LEVEL_ERROR, message, __VA_ARGS__)

// Raw (no new line added)
#define srTrace(message, ...) sLogOutput(LOG_LEVEL_TRACE, message, __VA_ARGS__)
#define srLog(message, ...) sLogOutput(LOG_LEVEL_LOG, message, __VA_ARGS__)
#define srWarn(message, ...) sLogOutput(LOG_LEVEL_WARN, message, __VA_ARGS__)
#define srError(message, ...) sLogOutput(LOG_LEVEL_ERROR, message, __VA_ARGS__)
#else
#define sTrace(message, ...)
#define sLog(message, ...)
#define sWarn(message, ...)
#define sError(message, ...)
#endif

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)                                                                       \
(byte & 0x80 ? '1' : '0'), (byte & 0x40 ? '1' : '0'), (byte & 0x20 ? '1' : '0'),               \
(byte & 0x10 ? '1' : '0'), (byte & 0x08 ? '1' : '0'), (byte & 0x04 ? '1' : '0'),           \
(byte & 0x02 ? '1' : '0'), (byte & 0x01 ? '1' : '0')

typedef enum LogLevel {
    LOG_LEVEL_TRACE,
    LOG_LEVEL_LOG,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_NONE = LOG_LEVEL_LOG,
} LogLevel;

global LogLevel LOG_LEVEL = LOG_LEVEL_LOG;

enum LogColor { LOG_COLOR_WHITE, LOG_COLOR_RED, LOG_COLOR_YELLOW, LOG_COLOR_GREY, LOG_COLOR_GREEN };

typedef void sLogCallback_t(const char *message, const u8 level);
typedef sLogCallback_t *PFN_LogCallback;
sLogCallback_t DefaultLog;

void sLogSetCallback(PFN_LogCallback cb);
void sLogLevel(LogLevel level);
void sLogOutputLine(u8 level, const char *fmt, ...);
void sLogOutput(u8 level, const char *fmt, ...);

typedef void LogColorCallback_t(enum LogColor color);
typedef LogColorCallback_t *PFN_LogColorCallback;
LogColorCallback_t DefaultLogSetColor;

void sLogSetColor(enum LogColor color);
void DefaultLogSetColor(enum LogColor color);

global PFN_LogCallback callback = &DefaultLog;
global PFN_LogColorCallback color_callback = &DefaultLogSetColor;

void DefaultLog(const char *message, const u8 level) {
    /*
        switch(level) {
        case LOG_LEVEL_ERROR: printf("\033[0;31m"); break;
        case LOG_LEVEL_WARN: printf("\033[0;33m"); break;
        case LOG_LEVEL_LOG: printf("\033[0m"); break;
        case LOG_LEVEL_TRACE: printf("\033[1;30m"); break;
        }
      */
    printf("%s", message);
    //    printf("\033[0m");
}

void sLogSetCallback(PFN_LogCallback cb) {
    callback = cb;
}

void sLogLevel(LogLevel level) {
    LOG_LEVEL = level;
}

// Outputs string format to console. Adds a new line.
void sLogOutputLine(u8 level, const char *fmt, ...) {
    if(level < LOG_LEVEL)
        return;
    va_list args;
    va_start(args, fmt);
    char buffer[MAX_LOG_LENGTH];
    vsnprintf(buffer, MAX_LOG_LENGTH, fmt, args);
    va_end(args);
    
    strncat(buffer, "\n\0", 2);
    callback(buffer, level);
}

void sLogOutput(u8 level, const char *fmt, ...) {
    if(level < LOG_LEVEL)
        return;
    va_list args;
    va_start(args, fmt);
    char buffer[MAX_LOG_LENGTH];
    vsnprintf(buffer, MAX_LOG_LENGTH, fmt, args);
    va_end(args);
    
    strncat(buffer, "\0", 1);
    
    callback(buffer, level);
}

void sLogSetColor(enum LogColor color) {
    color_callback(color);
}

void DefaultLogSetColor(enum LogColor color) {
    switch(color) {
        case(LOG_COLOR_WHITE): printf("\033[0m"); break;
        case(LOG_COLOR_RED): printf("\033[0;31m"); break;
        case(LOG_COLOR_YELLOW): printf("\033[0;33m"); break;
        case(LOG_COLOR_GREY): printf("\033[1;30m"); break;
        case(LOG_COLOR_GREEN): printf("\033[0;32m"); break;
    }
}
