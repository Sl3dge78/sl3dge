#pragma once

#include "sTypes.h"

// Returns the position of the first iteration of char c in the string of max
// length length
i32 StringFindFirstChar(const char *string, const u32 length, const char c) {
    for(u32 i = 0; i < length; ++i) {
        if(string[i] == c)
            return i;
    }

    return -1;
}

// Looks for the first non space character into string
// Modifies the string to point at the first char and returs the amount of chars
// eaten
i32 StringEatSpaces(const char **string, const u32 length) {
    i32 nb = 0;
    while(**string == ' ') {
        nb++;
        (*string)++;
    }
    return nb;
}

// Copies source to destination. Stops if max_length is exceeded or '\0' has
// been reached. Adds a '\0' at the end
void StringCopyLength(char *destination, const char *source, u32 max_length) {
    u32 nb = 0;
    while(nb < max_length) {
        if(source[nb] == '\0')
            break;
        destination[nb] = source[nb];
        nb++;
    }
    destination[nb] = '\0';
}
