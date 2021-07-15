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

enum KeyState {
    KEY_NOTPRESSED = 0x0,
    KEY_PRESSED = 0x1,
    KEY_UP = 0x2,
    KEY_DOWN = 0x4,
};

enum ScanCodes {
    SCANCODE_Q = 0x10,
    SCANCODE_W = 0x11,
    SCANCODE_E = 0x12,
    SCANCODE_O = 0x18,
    SCANCODE_P = 0x19,
    SCANCODE_A = 0x1E,
    SCANCODE_S = 0x1F,
    SCANCODE_D = 0x20,
    SCANCODE_M = 0x27,
    SCANCODE_LSHIFT = 0x2A,
    SCANCODE_SPACE = 0x39
};

typedef u8 Keyboard[256];

#endif