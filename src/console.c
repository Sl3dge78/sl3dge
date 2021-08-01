#pragma once

#include "console.h"

#include "game.h"

global Console *global_console;

void ConsolePushHistory(const char *text, char command_history[32][128]) {
    // shift everything one down
    for(u32 i = 31; i > 0; i--) {
        memcpy(command_history[i], command_history[i - 1], 128);
    }

    memcpy(command_history[0], text, 128);
}

DLL_EXPORT void ConsoleLogMessage(const char *message, const u8 level) {
    if(global_console)
        ConsolePushHistory(message, global_console->command_history);
}

void DrawConsole(Console *console, GameData *game_data) {
    if(console->console_open < console->console_target) {
        console->console_open += 20;
    } else if(console->console_open > console->console_target) {
        console->console_open -= 20;
    }
    if(console->console_open) {
        u32 console_y = console->console_open;
        const u32 padx = 5;
        const u32 pady = 5;
        // History area
        UIPushQuad(game_data->ui_push_buffer, 0, 0, game_data->window_width, console_y - 20, (Vec4){0.1f, 0.1f, 0.1f, 0.8f});
        // Edit area
        UIPushQuad(game_data->ui_push_buffer, 0, console_y - 20, game_data->window_width, 20, (Vec4){0.5f, 0.3f, 0.3f, 0.9f});
        UIPushText(game_data->ui_push_buffer, console->current_command, padx, console_y - pady, (Vec4){1.0f, 1.0f, 1.0f, 1.0f});
        // Caret
        UIPushQuad(game_data->ui_push_buffer, padx + (console->current_char * 10.5f), console_y - 20, 10, 20, (Vec4){0.9f, 0.9f, 0.9f, 1.0f});
        u32 line_height = 20;

        i32 hist_y = console_y - 20 - pady;
        u32 i = 0;
        while(hist_y > 0) {
            UIPushText(game_data->ui_push_buffer, console->command_history[i], padx, hist_y, (Vec4){0.8f, 0.8f, 0.8f, 1.0f});
            ++i;
            hist_y -= line_height;
        }

        /*
        ConsoleEntry *entry = console->history.first;
        while(entry && hist_y > 0) {
            UIPushText(game_data->ui_push_buffer, entry->command, padx, hist_y, (Vec4){1.0f, 1.0f, 1.0f, 1.0f});
            entry = entry->next;
            hist_y -= line_height;
        }
        */
    }
}

void InputConsole(Console *console, GameInput *input) {
    char c = input->text_input;
    if(c) {
        if(c >= 32 && c <= 125) { // Ascii letters
            if(console->current_char < 128) {
                console->current_command[console->current_char] = c;
                console->current_char++;
            }
        } else {
            switch(c) {
            case(-78): // ²
                console->console_target = 0;
                input->read_text_input = false;
                break;
            case(8): // backspace
                if(console->current_char > 0) {
                    console->current_command[console->current_char] = '\0';
                    console->current_char--;
                }
                break;
            case(13): // return

                ConsolePushHistory(console->current_command, console->command_history);
                memset(console->current_command, '\0', 128);
                console->current_char = 0;
                break;
            default:
                sLog("%d", c);
                break;
            };
        }

        input->text_input = '\0';
    }
}
