#pragma once

typedef struct sArray {
    void *ptr;    // Pointer to the array
    u32 count;    // Number of elements in the array
    u32 capacity; // Total available space before a realloc is needed
    u32 size;     // Size of each element
} sArray;

// Creates a new dynamically sized array
sArray sArrayCreate(const u32 initial_capacity, const u32 element_size) {
    sArray array;
    array.capacity = initial_capacity;
    array.count = 0;
    array.size = element_size;
    array.ptr = sCalloc(initial_capacity, element_size);
    return array;
}

// Destroys the dynamically sized array
void sArrayDestroy(sArray array) {
    sFree(array.ptr);
}

// Returns a pointer to element id in the array
inline void *sArrayGet(const sArray array, const u32 id) {
    return (u8 *)array.ptr + (array.size * id);
}

// Adds a new element to the array. Returns the id of the added element
u32 sArrayAdd(sArray *array) {
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


// Adds multiple new elements to the array. Returns the id of the first one.
u32 sArrayAddMultiple(sArray *array, const u32 nb) {
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