#ifndef PLATFORM_H
#define PLATFORM_H

struct PlatformWindow;
typedef struct PlatformWindow PlatformWindow;

typedef void PlatformReadWholeFile_t(const char *path, i32 *file_size, char *dest);
DLL_EXPORT PlatformReadWholeFile_t PlatformReadWholeFile;

typedef void PlatformGetWindowSize_t(const PlatformWindow *window, u32 *w, u32 *h);
DLL_EXPORT PlatformGetWindowSize_t PlatformGetWindowSize;

typedef void PlatformReadBinary_t(const char *path, i64 *file_size, u32 *content);
DLL_EXPORT PlatformReadBinary_t PlatformReadBinary;

typedef void PlatformGetInstanceExtensions_t(u32 *count, const char **extensions);
DLL_EXPORT PlatformGetInstanceExtensions_t PlatformGetInstanceExtensions;

typedef void PlatformSetCaptureMouse_t(bool val);
DLL_EXPORT PlatformSetCaptureMouse_t PlatformSetCaptureMouse;

typedef struct PlatformAPI {
    PlatformReadBinary_t *ReadBinary;
    PlatformReadWholeFile_t *ReadWholeFile;
    PlatformGetWindowSize_t *GetWindowSize;
    PlatformSetCaptureMouse_t *SetCaptureMouse;
} PlatformAPI;

#define MOUSE_LEFT 1
#define MOUSE_MIDDLE 2
#define MOUSE_RIGHT 4

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
    SCANCODE_TILDE = 0x29,
    SCANCODE_LSHIFT = 0x2A,
    SCANCODE_SPACE = 0x39,
    SCANCODE_ARRET_DEFIL = 0x46,
};

typedef u8 Keyboard[256];

typedef struct Input {
    Keyboard keyboard;
    Keyboard old_keyboard;
    u8 mouse;
    i32 mouse_x;
    i32 mouse_delta_x;
    i32 mouse_y;
    i32 mouse_delta_y;
    char text_input;
    bool read_text_input;
} Input;

#endif
