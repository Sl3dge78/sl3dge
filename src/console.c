#pragma once

#include "console.h"

#include "game.h"

void ConsolePushHistory(const char *text, ConsoleHistory *history) {
    ConsoleEntry *entry = sCalloc(1, sizeof(ConsoleEntry));
    memcpy((char *)entry->command, text, 128);
    entry->next = history->first;
    if (history->first) {
        history->first->prev = entry;
    }

    if (!history->last) {
        history->last = entry;
    }

    history->first = entry;

    if (history->size >= CONSOLE_HISTORY_MAX_SIZE) {
        ConsoleEntry *delete = history->last;
        history->last = delete->prev;
        sFree(delete);
    } else {
        history->size++;
        sLog("%d", history->size);
    }
}

void ConsoleDeleteHistory(ConsoleHistory *history) {
    ConsoleEntry *entry = history->first;
    while (entry) {
        ConsoleEntry *next = entry->next;
        sFree(entry);
        entry = next;
    }
}

void DrawConsole(Console *console, GameData *game_data) {
    if (console->console_open < console->console_target) {
        console->console_open += 20;
    } else if (console->console_open > console->console_target) {
        console->console_open -= 20;
    }
    if (console->console_open) {
        u32 console_y = console->console_open;
        const u32 padx = 5;
        const u32 pady = 5;
        // History area
        UIPushQuad(game_data->ui_push_buffer, 0, 0, game_data->window_width,
                   console_y - 20, (Vec4){0.1f, 0.1f, 0.1f, 0.8f});
        // Edit area
        UIPushQuad(game_data->ui_push_buffer, 0, console_y - 20,
                   game_data->window_width, 20, (Vec4){0.5f, 0.3f, 0.3f, 0.9f});
        UIPushText(game_data->ui_push_buffer, console->current_command, padx,
                   console_y - pady, (Vec4){1.0f, 1.0f, 1.0f, 1.0f});
        // Caret
        UIPushQuad(game_data->ui_push_buffer,
                   padx + (console->current_char * 10.5f), console_y - 20, 10,
                   20, (Vec4){0.9f, 0.9f, 0.9f, 1.0f});
        u32 line_height = 20;

        // How many lines can we display ?
        u32 hist_y = console_y - 20 - pady;

        ConsoleEntry *entry = console->history.first;
        while (entry && hist_y > 0) {
            UIPushText(game_data->ui_push_buffer, entry->command, padx, hist_y,
                       (Vec4){1.0f, 1.0f, 1.0f, 1.0f});
            entry = entry->next;
            hist_y -= line_height;
        }
    }
}

void InputConsole(Console *console, GameInput *input) {
    char c = input->text_input;
    if (c) {
        if (c >= 32 && c <= 125) {  // Ascii letters
            if (console->current_char < 128) {
                console->current_command[console->current_char] = c;
                console->current_char++;
            }
        } else {
            switch (c) {
                case (-78):  // ²
                    console->console_target = 0;
                    input->read_text_input = false;
                    break;
                case (8):  // backspace
                    if (console->current_char > 0) {
                        console->current_command[console->current_char] = '\0';
                        console->current_char--;
                    }
                    break;
                case (13):  // return
                    ConsolePushHistory(console->current_command,
                                       &console->history);
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
