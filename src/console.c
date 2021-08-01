#pragma once

#include "game.h"
#include "console.h"
void DrawConsole(Console *console, GameData *game_data) { 
                                    

    if(console->console_open < console->console_target) {
        console->console_open += 20;
    } else if(console->console_open > console->console_target) {
        console->console_open -= 20;
    }
    if (console->console_open) {
        u32 console_y = console->console_open;
        const u32 padx = 5;
        const u32 pady = 5;
        // History area
        UIPushQuad(game_data->ui_push_buffer, 0, 0, game_data->window_width, console_y - 20, (Vec4){0.1f, 0.1f, 0.1f, 0.8f});
        // Edit area
        UIPushQuad(game_data->ui_push_buffer, 0, console_y-20, game_data->window_width, 20, (Vec4){0.5f, 0.3f, 0.3f, 0.9f});
        UIPushText(game_data->ui_push_buffer, console->current_command, padx, console_y-pady, (Vec4){1.0f, 1.0f, 1.0f, 1.0f});
        // Caret
        UIPushQuad(game_data->ui_push_buffer, padx + (console->current_char*10.5f), console_y-20, 10, 20, (Vec4){0.9f, 0.9f, 0.9f, 1.0f});

    }    
}

void InputConsole(Console *console, GameInput *input) {
    char c = input->text_input;
    if(c) {
        if(c >= 32 && c <= 125) {      // Ascii letters
            if(console->current_char < 128){
                console->current_command[console->current_char] = c;
                console->current_char++;
                sLog("%d", console->current_char);
            }
        } else {
            switch(c) {
            case (-78): // ²
                console->console_target = 0;
                input->read_text_input = false;
                break;
            case (8): // backspace       
                if (console->current_char > 0) {
                    console->current_command[console->current_char] = '\0';
                    console->current_char--;
                }
                break;
            default : 
                sLog("%d", c);
                break;
            };
        }
        
        input->text_input = '\0';

    }
}
