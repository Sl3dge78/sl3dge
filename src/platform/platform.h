#ifndef PLATFORM_H
#define PLATFORM_H

#include <sl3dge-utils/sl3dge.h>
#include <vulkan/vulkan.h>

struct PlatformWindow;
typedef struct PlatformWindow PlatformWindow;

typedef void PlatformReadBinary_t(const char *path, i64 *file_size, u32 *content);
DLL_EXPORT PlatformReadBinary_t PlatformReadBinary;

typedef void
PlatformCreateVkSurface_t(VkInstance instance, PlatformWindow *window, VkSurfaceKHR *surface);
DLL_EXPORT PlatformCreateVkSurface_t PlatformCreateVkSurface;

typedef void PlatformGetInstanceExtensions_t(u32 *count, const char **extensions);
DLL_EXPORT PlatformGetInstanceExtensions_t PlatformGetInstanceExtensions;

typedef struct PlatformAPI {
    PlatformReadBinary_t *ReadBinary;
    PlatformCreateVkSurface_t *CreateVkSurface;
    PlatformGetInstanceExtensions_t *GetInstanceExtensions;
} PlatformAPI;

#define MOUSE_LEFT 0
#define MOUSE_MIDDLE 1
#define MOUSE_RIGHT 2

#endif