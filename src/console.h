#pragma once

#include <sl3dge-utils/sl3dge.h>

#define CONSOLE_HISTORY_MAX_SIZE 32

typedef struct ConsoleHistoryEntry {
    char command[128];
    Vec4 color;
} ConsoleHistoryEntry;

typedef struct Console {
    u32 console_open;
    u32 console_target;
    const char *last_command;
    u32 current_char;
    char current_command[128];
    ConsoleHistoryEntry command_history[32];
} Console;

void DrawConsole(Console *console, GameData *game_data);

