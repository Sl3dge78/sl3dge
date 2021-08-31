#pragma once

#include <sl3dge-utils/sl3dge.h>

#define CONSOLE_HISTORY_MAX_SIZE 32

struct GameData;

typedef char ConsoleArgs[5][64];
typedef void ConsoleFunction(ConsoleArgs *args, struct GameData *game_data);

typedef struct ConsoleCommand {
    const char *command;
    ConsoleFunction *function;
} ConsoleCommand;

typedef struct ConsoleHistoryEntry {
    char command[128];
    Vec4 color;
} ConsoleHistoryEntry;

typedef struct Console {
    u32 console_open;
    u32 console_target;

    u32 current_char;
    char current_command[128];
    ConsoleHistoryEntry command_history[32];
    ConsoleCommand commands[3];
    u32 command_count;

    u32 history_browser;
    u32 history_size;
} Console;

void DrawConsole(Console *console, struct GameData *game_data);
