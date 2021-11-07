#pragma once

typedef struct Array {
    void *ptr;
    u32 count;
    u32 capacity;
    u32 size;
} Array;

Array ArrayCreate(const u32 initial_capacity, const u32 element_size) {
    Array array;
    array.capacity = initial_capacity;
    array.count = 0;
    array.size = element_size;
    array.ptr = sCalloc(initial_capacity, element_size);
    return array;
}

void ArrayDestroy(Array array) {
    sFree(array.ptr);
}

inline void *ArrayGetElementAt(const Array array, const u32 id) {
    return (u8 *)array.ptr + (array.size * id);
}

u32 ArrayGetNewElement(Array *array) {
    if(array->count == array->capacity) {
        u32 new_capacity;
        if (array->capacity <= 0) 
            new_capacity = 1;
        else {
            new_capacity = array->capacity * 2;
        }
        void *ptr = sRealloc(array->ptr, array->size * new_capacity);
        ASSERT(ptr);
        if(ptr) {
            array->capacity = new_capacity;
            array->ptr = ptr;
        }
    }
    return array->count++;
}

u32 ArrayGetNewElements(Array *array, const u32 nb) {
    u32 new_count = array->count + nb;
    
    if(array->capacity < new_count) {
        u32 new_capacity = MAX(new_count, array->capacity * 2);
        void *ptr = sRealloc(array->ptr, array->size * new_capacity);
        ASSERT(ptr);
        if(ptr) {
            array->ptr = ptr;
        }
        array->capacity = new_capacity;
    }
    
    u32 result = array->count;
    array->count = new_count;
    return result;
    
}