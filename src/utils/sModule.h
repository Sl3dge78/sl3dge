#pragma once

#ifdef __WIN32__
#include <windows.h>
#include "sTypes.h"

// SAMPLE
/*

// Load module
Module game_module = {};
    Win32LoadModule(&game_module, "game");
    // > Load pfn

// Check for changes - In main loop :
if(Win32ShouldReloadModule(&game_module)) {
            Win32CloseModule(&game_module);
            Win32LoadModule(&game_module, "game");
            // > Load pfn
         }

// Close module
Win32CloseModule(&game_module);

*/

typedef struct Module {
    char meta_path[64];
    FILETIME last_write_time;
    HMODULE dll;
    char tmp_dll[64];
} Module;

internal inline FILETIME Win32GetLastWriteTime(const char *file_name) {
    FILETIME last_write_time = {0};

    WIN32_FIND_DATA data;
    HANDLE handle = FindFirstFile(file_name, &data);
    if(handle != INVALID_HANDLE_VALUE) {
        last_write_time = data.ftLastWriteTime;
        FindClose(handle);
    }

    return last_write_time;
}

internal bool Win32ShouldReloadModule(Module *module) {
    FILETIME current_time = Win32GetLastWriteTime(module->meta_path);
    if(CompareFileTime(&module->last_write_time,
                       &current_time)) { // Has the file changed ?
        module->last_write_time = current_time;
        return true;
    }
    return false;
}

internal bool Win32LoadModule(Module *module, const char *name) {
    sTrace("Loading module %s", name);

    const char *path = "bin\\";

    strcpy_s(module->meta_path, ARRAY_SIZE(module->meta_path), "");
    strcat_s(module->meta_path, ARRAY_SIZE(module->meta_path), path);
    strcat_s(module->meta_path, ARRAY_SIZE(module->meta_path), name);
    strcat_s(module->meta_path, ARRAY_SIZE(module->meta_path), ".meta");

    char orig_dll[64] = {0};
    strcat_s(orig_dll, ARRAY_SIZE(orig_dll), path);
    strcat_s(orig_dll, ARRAY_SIZE(orig_dll), name);
    strcat_s(orig_dll, ARRAY_SIZE(orig_dll), ".dll");

    strcpy_s(module->tmp_dll, ARRAY_SIZE(module->tmp_dll), "");
    strcat_s(module->tmp_dll, ARRAY_SIZE(module->tmp_dll), path);
    strcat_s(module->tmp_dll, ARRAY_SIZE(module->tmp_dll), name);
    strcat_s(module->tmp_dll, ARRAY_SIZE(module->tmp_dll), "_temp.dll");

    // Copy the .dll to the new _temp.dll to allow writing by the compiler, only
    // if it exists. CopyFile deletes the destination even on failure, so do the
    // copy only if the file exists. This will reload the previous _tmp
    WIN32_FIND_DATA f;
    HANDLE hdl = FindFirstFile(orig_dll, &f);
    if(hdl != INVALID_HANDLE_VALUE) {
        FindClose(hdl);
        DeleteFile(module->tmp_dll);
        if(!CopyFile(orig_dll, module->tmp_dll, FALSE)) {
            DWORD dw = GetLastError();
            sError("Error copying module %s, %d", name, dw);
        }
    } else {
        sWarn("%s.dll doesn't exist, reloading the previous module.", name);
    }

    module->dll = LoadLibrary(module->tmp_dll);

    if(!module->dll) {
        sError("Unable to load module %s", name);
        return false;
    }

    module->last_write_time = Win32GetLastWriteTime(module->meta_path);

    sTrace("Module loaded : %s", name);
    return true;
}

internal void Win32CloseModule(Module *module) {
    FreeLibrary(module->dll);
}
#endif
