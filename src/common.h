/* date = April 25th 2021 0:44 pm */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdlib.h>

#include <SDL/SDL.h>

#define internal static;
#define global static;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t i64;
typedef int32_t i32;

#include "debug.cpp"
#include "math.cpp"

#endif //COMMON_H
