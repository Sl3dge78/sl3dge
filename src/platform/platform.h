#ifndef PLATFORM_H
#define PLATFORM_H

#include <sl3dge/sl3dge.h>

typedef void PlatformReadBinary_t(const char *path, i64 *file_size, u32 *content);
DLL_EXPORT PlatformReadBinary_t PlatformReadBinary;

typedef struct PlatformAPI {
    PlatformReadBinary_t *ReadBinary;
} PlatformAPI;

#endif