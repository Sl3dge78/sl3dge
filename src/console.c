#pragma once

#include "console.h"
#include "game.h"

global Console *global_console; // Set in GameLoad in game.c

void CommandExit(ConsoleArgs *args, GameData *game_data) {
    EventPush(&game_data->event_queue, EVENT_TYPE_QUIT);
    sLog("Exiting...");
}

void CommandFreeCam(ConsoleArgs *args, GameData *game_data) {
    game_data->is_free_cam = !game_data->is_free_cam;
    platform->SetCaptureMouse(!game_data->is_free_cam);
    sLog("Freecam is %s", (game_data->is_free_cam ? "on" : "off"));
}

void CommandRestart(ConsoleArgs *args, GameData *game_data) {
    EventPush(&game_data->event_queue, EVENT_TYPE_RESTART);
    sLog("Reloading the game");
}

void ConsoleInit(Console *console) {
    console->commands[0] = (ConsoleCommand){"exit", &CommandExit};
    console->commands[1] = (ConsoleCommand){"freecam", &CommandFreeCam};
    console->commands[2] = (ConsoleCommand){"restart", &CommandRestart};

    console->command_count = ARRAY_SIZE(console->commands);
}

void ConsoleCallCommand(ConsoleArgs *args, GameData *game_data) {
    for(u32 i = 0; i < global_console->command_count; ++i) {
        if(strcmp(*args[0], global_console->commands[i].command) == 0) {
            global_console->commands[i].function(args, game_data);
            return;
        }
    }
    sError("Unknown command %s", args[0]);
}

void ConsolePushHistory(const char *text, const Vec4 color, Console *console) {
    // shift everything one down
    for(u32 i = 31; i > 0; i--) {
        memcpy(&console->command_history[i], &console->command_history[i - 1], sizeof(ConsoleHistoryEntry));
    }

    memcpy(&console->command_history[0].command, text, 128);
    memcpy(&console->command_history[0].color, &color, sizeof(Vec4));
    console->history_size++;
    if(console->history_size > 32) {
        console->history_size = 32;
    }
}

DLL_EXPORT void ConsoleLogMessage(const char *message, const u8 level) {
    if(global_console) {
        Vec4 color;
        switch(level) {
        case(LOG_LEVEL_TRACE): color = (Vec4){0.5f, 0.5f, 0.5f, 1.0f}; break;
        case(LOG_LEVEL_LOG): color = (Vec4){0.85f, 0.85f, 0.85f, 1.0f}; break;
        case(LOG_LEVEL_WARN): color = (Vec4){235.f / 255.f, 195.f / 255.f, 52.f / 255.f, 1.0f}; break;
        case(LOG_LEVEL_ERROR): color = (Vec4){235.f / 255.f, 70.f / 255.f, 52.f / 255.f, 1.0f}; break;
        };

        ConsolePushHistory(message, color, global_console);
    }
}

internal void ConsoleParseMessage(const char *message, GameData *game_data) {
    // TEMPORARY : 5 args max

    const char *head = message;
    u32 length = strlen(message);

    length -= StringEatSpaces(&head, length);

    ConsoleArgs args;
    //char args[5][64];
    u32 argc = 0;
    while(argc < 5 && length > 0) {
        i32 val = StringFindFirstChar(head, length, ' ');
        if(val == -1) {
            StringCopyLength(args[argc], head, length);
            break;
        } else {
            StringCopyLength(args[argc], head, val);
            head += val + 1;
            length -= val + 1;
            argc++;
        }
    }

    ConsoleCallCommand(&args, game_data);
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
        UIPushQuad(global_renderer, 0, 0, global_renderer->width, console_y - 20, (Vec4){0.1f, 0.1f, 0.1f, 0.8f});
        // Edit area
        UIPushQuad(global_renderer, 0, console_y - 20, global_renderer->width, 20, (Vec4){0.5f, 0.3f, 0.3f, 0.9f});
        UIPushText(global_renderer, console->current_command, padx, console_y - pady, (Vec4){1.0f, 1.0f, 1.0f, 1.0f});
        // Caret
        UIPushQuad(global_renderer, padx + (console->current_char * 10.5f), console_y - 20, 1, 20, (Vec4){0.9f, 0.9f, 0.9f, 1.0f});
        u32 line_height = 20;

        i32 hist_y = console_y - 20 - pady;
        u32 i = 0;
        while(hist_y > 0) {
            UIPushText(global_renderer, console->command_history[i].command, padx, hist_y, console->command_history[i].color);
            ++i;
            hist_y -= line_height;
        }
    }
}

void InputConsole(Console *console, Input *input, GameData *game_data) {
    char c = input->text_input;
    if(c) {
        if(c >= 32 && c <= 125) { // Ascii letters
            if(console->current_char < 128) {
                console->current_command[console->current_char] = c;
                console->current_char++;
            }
        } else {
            switch(c) {
            case(8): // backspace
                if(console->current_char > 0) {
                    console->current_char--;
                    console->current_command[console->current_char] = '\0';
                }
                break;
            case(13): // return
                ConsoleLogMessage(console->current_command, LOG_LEVEL_LOG);
                ConsoleParseMessage(console->current_command, game_data);
                memset(console->current_command, '\0', 128);
                console->current_char = 0;
                console->history_browser = 0;
                break;
            default:
                //sLog("%d", c);
                break;
            };
        }

        input->text_input = '\0';
    }

    if(input->keyboard[SCANCODE_UP] && !input->old_keyboard[SCANCODE_UP]) {
        console->history_browser++;
        if(console->history_browser >= console->history_size) {
            console->history_browser = 0;
        }
        strcpy(console->current_command, console->command_history[console->history_browser].command);
        console->current_char = strlen(console->current_command);
    }
}
