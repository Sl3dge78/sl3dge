#pragma once

typedef struct ConsoleEntry {
    const char command[128];
    struct ConsoleEntry *prev;
    struct ConsoleEntry *next;
} ConsoleEntry;

typedef struct ConsoleHistory {
    ConsoleEntry *first;
    ConsoleEntry *last;
    u32 size;
} ConsoleHistory;

#define CONSOLE_HISTORY_MAX_SIZE 32

typedef struct Console {
    u32 console_open;
    u32 console_target;
    const char *last_command;
    u32 current_char;
    char current_command[128];
    char command_history[32][128];
    // ConsoleHistory history;
} Console;

void DrawConsole(Console *console, GameData *game_data);

