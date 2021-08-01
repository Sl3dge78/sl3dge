#pragma once

typedef struct Console {

    u32 console_open;
    u32 console_target;
    const char *last_command;
    u32 current_char;
    char current_command[128];

} Console;             

void DrawConsole(Console *console, GameData *game_data);
 
