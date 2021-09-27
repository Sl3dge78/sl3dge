#pragma once
// SLEAK
// Helper functions for debugging, mainly detecting memory leaks

// --------
// Usage

// Include this file where you need, define the macro DEBUG, call DEBUG_Begin() at the start, and DEBUG_End() at the end of your program.
// It will override malloc, calloc, realloc and free and give you a rundown of what wasn't freed. There's also an assert on freeing an unknown pointer
// If your program has different contexts, (dynamic libraries for example), you can use DEBUG_GetLeakList() and DEBUG_SetLeakList(list) to link the contexts together

#if 0

#define DEBUG

int main() {
    Leak_Begin();
    
    // Do allocations
    
    Leak_End();
}

#endif

#include "sTypes.h"
#include "sLogging.h"

#if defined(DEBUG)

#include <stdio.h>
#include <stdlib.h>

#define ASSERT(expression)                                                                         \
if(!(expression)) {                                                                            \
__builtin_trap();                                                                          \
}

#define ASSERT_MSG(expression, msg)                                                                \
if(!(expression)) {                                                                            \
sError(msg);                                                                               \
__builtin_trap();                                                                          \
}

#define HANG(expression)                                                                           \
if(!(expression))                                                                              \
while(1)                                                                                   \
;

#define HANG_MSG(expression, msg)                                                                  \
if(!(expression))                                                                              \
sError(msg);                                                                               \
while(1)                                                                                       \
;

global bool DBG_keep_console_open;

#define KEEP_CONSOLE_OPEN(value) DBG_keep_console_open |= value

typedef struct MemoryInfo {
    void *ptr;
    size_t size;
    const char *file;
    u32 line;
} MemoryInfo;

typedef struct MemoryLeak {
    MemoryInfo info;
    struct MemoryLeak *next;
} MemoryLeak;

typedef struct MemoryLeakList {
    MemoryLeak *array_start;
    MemoryLeak *array_end;
} MemoryLeakList;

global MemoryLeakList *list;

void Leak_Begin() {
    list = calloc(1, sizeof(MemoryLeakList));
}

void *Leak_GetList() {
    return list;
}

void Leak_SetList(void *in_list) {
    list = (MemoryLeakList *)in_list;
}

internal void add_memory_info(void *ptr, size_t size, const char *filename, u32 line) {
    MemoryLeak *leak = (MemoryLeak *)malloc(sizeof(MemoryLeak));
    
    leak->info.ptr = ptr;
    leak->info.size = size;
    leak->info.file = filename;
    leak->info.line = line;
    leak->next = NULL;
    //sLog("Allocation 0x%p, %s:%d", ptr, filename, line);
    
    if(list->array_start == NULL) {
        list->array_start = leak;
        list->array_end = leak;
    } else {
        list->array_end->next = leak;
        list->array_end = leak;
    }
}

internal void delete_memory_info(void *ptr) {
    ASSERT_MSG(list->array_start, "Attempting to free a nullptr!");
    
    // Si c'est le premier dans la liste
    if(list->array_start->info.ptr == ptr) {
        MemoryLeak *leak = list->array_start;
        list->array_start = list->array_start->next;
        
        free(leak);
        return;
    }
    
    // Sinon
    for(MemoryLeak *leak = list->array_start; leak != NULL; leak = leak->next) {
        MemoryLeak *next_leak = leak->next;
        if(!next_leak) {
            // The ptr couldn't be found in the list of leaks. Multiple options
            // 1. We could have freed a nullptr. Hopefully the program will crash in that case
            // 2. DLL/Threads shinanigans, ie : when we reloaded a dll, this header was included, so a new list was created, so when we free the ptr, it doesn't exist in the list
            sWarn("Attempting to free a nullptr!");
            return;
        }
        
        if(next_leak->info.ptr == ptr) { // Si il faut supprimer le suivant
            if(list->array_end == next_leak) {
                leak->next = NULL;
                list->array_end = leak;
            } else {
                leak->next = next_leak->next;
            }
            free(next_leak);
            return;
        }
    }
}

internal void clear_array() {
    MemoryLeak *leak = list->array_start;
    MemoryLeak *to_delete = list->array_start;
    
    while(leak != NULL) {
        leak = leak->next;
        free(to_delete);
        to_delete = leak;
    }
    
    free(list);
}

void *_malloc(size_t size, const char *filename, u32 line) {
    void *ptr = malloc(size);
    if(ptr != NULL) {
        add_memory_info(ptr, size, filename, line);
    }
    return ptr;
}

void *_calloc(size_t num, size_t size, const char *filename, u32 line) {
    void *ptr = calloc(num, size);
    if(ptr != NULL) {
        add_memory_info(ptr, num * size, filename, line);
    }
    return ptr;
}

void *_realloc(void *ptr, size_t new_size, const char *filename, u32 line) {
    void *new_ptr = realloc(ptr, new_size);
    if(new_ptr != NULL) {
        if(ptr != NULL)
            delete_memory_info(ptr);
        
        add_memory_info(new_ptr, new_size, filename, line);
    }
    return new_ptr;
}

void _free(void *ptr) {
    ASSERT_MSG(ptr, "Attempting to free ptr 0x0");
    //sLog("Freed %p", ptr);
    delete_memory_info(ptr);
    free(ptr);
}

void _free_verbose(void *ptr, const char *string) {
    sLog("Freed %p %s", ptr, string);
    delete_memory_info(ptr);
    free(ptr);
}

internal bool DumpMemoryLeaks() {
    int count = 0;
    for(MemoryLeak *leak = list->array_start; leak != NULL; leak = leak->next) {
        sError("Memory leak found - Address: %p | Size: %06d | Last alloc: %s:%d",
               leak->info.ptr,
               (u64)leak->info.size,
               leak->info.file,
               leak->info.line);
        count++;
    }
    clear_array();
    return count;
}

void Leak_End() {
    DBG_keep_console_open |= DumpMemoryLeaks();
    
    if(DBG_keep_console_open) {
        system("pause");
    }
}

#define sMalloc(size) _malloc(size, __FILE__, __LINE__)
#define sCalloc(num, size) _calloc(num, size, __FILE__, __LINE__)
#define sRealloc(ptr, size) _realloc(ptr, size, __FILE__, __LINE__)
#define sFree(ptr) _free(ptr)
//#define sFree(ptr) _free_verbose(ptr, #ptr)

#else // #if DEBUG

#define ASSERT(expression)
#define ASSERT_MSG(expression, msg)
#define HANG(expression)
#define HANG_MSG(expression, msg)
#define KEEP_CONSOLE_OPEN(value)
#define Leak_Begin()
#define Leak_End()
#define Leak_GetList() 0
#define Leak_SetList(arg)

#define sMalloc(size) malloc(size)
#define sCalloc(num, size) calloc(num, size)
#define sRealloc(ptr, size) realloc(ptr, size)
#define sFree(ptr) free(ptr)

#endif // #if DEBUG
