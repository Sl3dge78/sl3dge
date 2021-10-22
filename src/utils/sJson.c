#include <stdio.h>
#include "utils/sl3dge.h"

typedef enum GLTFAccessorType {
    GLTF_ACCESSOR_TYPE_SCALAR,
    GLTF_ACCESSOR_TYPE_VEC2,
    GLTF_ACCESSOR_TYPE_VEC3,
    GLTF_ACCESSOR_TYPE_VEC4,
} GLTFAccessorType;

typedef struct GLTFAccessor {
    u32 buffer_view;
    u32 byte_offset;
    u32 component_type;
    u32 count;
    GLTFAccessorType type;
} GLTFAccessor;

typedef struct GLTFBufferView {
    u32 buffer;
    u32 byte_length;
    u32 byte_offset;
    u32 target;
} GLTFBufferView;

typedef struct GLTFBuffer {
    u32 byte_length;
    char *uri;
} GLTFBuffer;

typedef struct GLTFImage {
    char *uri;
} GLTFImage;

typedef struct GLTF {
    u32 accessor_count;
    GLTFAccessor *accessors;
    
    u32 buffer_view_count;
    GLTFBufferView* buffer_views;
    
    u32 buffer_count;
    GLTFBuffer *buffers;
    
    u32 image_count;
    GLTFImage *images;
} GLTF;

bool IsWhitespace(const char c) {
    return (c == '\n' || c == '\t' || c == ' ');
}

char *EatSpaces(char *ptr) {
    while(IsWhitespace(*ptr)) {
        ptr++;
    }
    return (ptr);
}

u32 JsonCountArray(char *ptr) {
    ASSERT_MSG(*ptr == '[', "Current char isn't a [");
    
    // We need to count the amount of pairs of {} before the ]
    ptr++;
    bool parse = true;
    u32 depth = 0;
    u32 count = 0;
    while(parse) {
        switch(*ptr++)  {
            case '{' : {
                if(depth == 0) 
                    count++;
                
                depth++;
            } break;
            case '}' : {
                depth --;
            } break;
            case ']' : {
                if(depth == 0) 
                    parse = false;
            } break;
            
        }
    }
    return (count);
}

char *JsonEatColon(char *ptr) {
    ptr = EatSpaces(ptr);
    ASSERT(*ptr == ':');
    ptr++;
    ptr = EatSpaces(ptr);
    return (ptr);
}

u32 JsonGetStringLength(char *ptr) {
    
    ASSERT(*ptr == '\"');
    ptr++;
    char *start = ptr;
    while(*ptr != '\"') {
        ptr++;
    }
    
    return ((ptr++) - start + 1);
}

char *JsonParseString(char *ptr, char *dst) {
    ASSERT(*ptr == '\"');
    ptr++;
    while(*ptr != '\"') {
        *dst++ = *ptr++;
    }
    ptr++;
    *dst = '\0';
    return (ptr++);
}

char *JsonParseU32(char *ptr, u32 *dst){
    sscanf(ptr, "%d", dst);
    while(*ptr >= '0' && *ptr <= '9') ptr++;
    return ptr;
}

char *JsonSkipValue(char *ptr) {
    i32 depth = 0;
    bool parse = true;
    while (ptr && parse) {
        switch(*ptr) {
            case ',':
            if(depth == 0) {
                return(ptr);
            } break;
            
            case '[':
            case '{':
            depth++;
            break;
            
            case ']':
            case '}':
            depth--;
            if(depth < 0)
                return (ptr);
            break;
        }
        ptr++;
    }
    return(ptr);
}

char *GLTFParseAccessors(char *ptr, GLTFAccessor **accessors, u32 *count) {
    sTrace("GLTF: Reading Accessors");
    u32 array_count = JsonCountArray(ptr);
    *count = array_count;
    *accessors = sCalloc(array_count, sizeof(GLTFAccessor));
    ptr++; // '['
    ptr = EatSpaces(ptr);
    
    for(u32 i = 0; i < array_count; i++) {
        ASSERT(*ptr == '{');
        ptr++;
        ptr = EatSpaces(ptr);
        GLTFAccessor *acc = &(*accessors)[i];
        
        for(;;) {
            if(*ptr == '}') {
                ptr++; // '}'
                ptr++; // ','
                ptr = EatSpaces(ptr);
                break;
            }
            
            char key[32];
            ptr = JsonParseString(ptr, key);
            ptr = JsonEatColon(ptr);
            if(strcmp(key, "bufferView") == 0) {
                ptr = JsonParseU32(ptr, &acc->buffer_view);
            } else if(strcmp(key, "byteOffset") == 0) {
                ptr = JsonParseU32(ptr, &acc->byte_offset);
            } else if(strcmp(key, "componentType") == 0) {
                ptr = JsonParseU32(ptr, &acc->component_type);
            } else if(strcmp(key, "count") == 0) {
                ptr = JsonParseU32(ptr, &acc->count);
            } else if(strcmp(key, "type") == 0) {
                char type[32];
                ptr = JsonParseString(ptr, type);
                if(strcmp(type, "SCALAR") == 0) {
                    acc->type = GLTF_ACCESSOR_TYPE_SCALAR;
                } else if(strcmp(type, "VEC3") == 0) {
                    acc->type = GLTF_ACCESSOR_TYPE_VEC3;
                } else if(strcmp(type, "VEC4") == 0) {
                    acc->type = GLTF_ACCESSOR_TYPE_VEC4;
                } else if(strcmp(type, "VEC2") == 0) {
                    acc->type = GLTF_ACCESSOR_TYPE_VEC2;
                }
            } else {
                sLog("JSON: Unread value %s", key);
                ptr = JsonSkipValue(ptr);
            }
            if(*ptr == ',')
                ptr++;
            ptr = EatSpaces(ptr);
        }
    }
    ptr = EatSpaces(ptr);
    ASSERT(*ptr == ']');
    ptr++;
    
    sTrace("GLTF: Read %d Accessors", array_count);
    return (ptr);
}

char *GLTFParseBufferViews(char *ptr, GLTFBufferView **buffer_views, u32 *count) {
    sTrace("GLTF: Reading Buffer Views");
    u32 array_count = JsonCountArray(ptr);
    *count = array_count;
    *buffer_views = sCalloc(array_count, sizeof(GLTFBufferView));
    ptr++; // '['
    ptr = EatSpaces(ptr);
    
    for(u32 i = 0; i < array_count; i++) {
        ASSERT(*ptr == '{');
        ptr++;
        ptr = EatSpaces(ptr);
        GLTFBufferView *bv = &(*buffer_views)[i];
        
        for(;;) {
            if(*ptr == '}') {
                ptr++; // '}'
                ptr++; // ','
                ptr = EatSpaces(ptr);
                break;
            }
            
            char key[32];
            ptr = JsonParseString(ptr, key);
            ptr = JsonEatColon(ptr);
            if(strcmp(key, "buffer") == 0) {
                ptr = JsonParseU32(ptr, &bv->buffer);
            } else if(strcmp(key, "byteLength") == 0) {
                ptr = JsonParseU32(ptr, &bv->byte_length);
            } else if(strcmp(key, "byteOffset") == 0) {
                ptr = JsonParseU32(ptr, &bv->byte_offset);
            } else if(strcmp(key, "target") == 0) {
                ptr = JsonParseU32(ptr, &bv->target);
            } else {
                sLog("JSON: Unread value %s", key);
                ptr = JsonSkipValue(ptr);
            }
            if(*ptr == ',')
                ptr++;
            ptr = EatSpaces(ptr);
        }
    }
    
    ASSERT(*ptr == ']');
    ptr++;
    
    sTrace("GLTF: Read %d Buffer Views", array_count);
    return (ptr);
}

char *GLTFParseBuffers(char *ptr, GLTFBuffer **buffers, u32 *count) {
    sTrace("GLTF: Reading Buffers");
    u32 array_count = JsonCountArray(ptr);
    *count = array_count;
    *buffers = sCalloc(array_count, sizeof(GLTFBuffer));
    ptr++; // '['
    ptr = EatSpaces(ptr);
    
    for(u32 i = 0; i < array_count; i++) {
        ASSERT(*ptr == '{');
        ptr++;
        ptr = EatSpaces(ptr);
        GLTFBuffer *buf = &(*buffers)[i];
        
        for(;;) {
            if(*ptr == '}') {
                ptr++; // '}'
                ptr++; // ','
                ptr = EatSpaces(ptr);
                break;
            }
            
            char key[32];
            ptr = JsonParseString(ptr, key);
            ptr = JsonEatColon(ptr);
            if(strcmp(key, "byteLength") == 0) {
                ptr = JsonParseU32(ptr, &buf->byte_length);
            } else if(strcmp(key, "uri") == 0) {
                u32 length = JsonGetStringLength(ptr);
                buf->uri = sCalloc(length, sizeof(char));
                ptr = JsonParseString(ptr, buf->uri);
            } else {
                sLog("JSON: Unread value %s", key);
                ptr = JsonSkipValue(ptr);
            }
            if(*ptr == ',')
                ptr++;
            ptr = EatSpaces(ptr);
        }
    }
    
    ASSERT(*ptr == ']');
    ptr++;
    
    sTrace("GLTF: Read %d Buffers", array_count);
    return (ptr);
}

char *GLTFParseImages(char *ptr, GLTFImage **images, u32 *count) {
    sTrace("GLTF: Reading Images");
    u32 array_count = JsonCountArray(ptr);
    *count = array_count;
    *images = sCalloc(array_count, sizeof(GLTFImage));
    ptr++; // '['
    ptr = EatSpaces(ptr);
    
    for(u32 i = 0; i < array_count; i++) {
        ASSERT(*ptr == '{');
        ptr++;
        ptr = EatSpaces(ptr);
        GLTFImage *img = &(*images)[i];
        
        for(;;) {
            if(*ptr == '}') {
                ptr++; // '}'
                ptr++; // ','
                ptr = EatSpaces(ptr);
                break;
            }
            
            char key[32];
            ptr = JsonParseString(ptr, key);
            ptr = JsonEatColon(ptr);
            if(strcmp(key, "uri") == 0) {
                u32 length = JsonGetStringLength(ptr);
                img->uri = sCalloc(length, sizeof(char));
                ptr = JsonParseString(ptr, img->uri);
            } else {
                sLog("JSON: Unread value %s", key);
                ptr = JsonSkipValue(ptr);
            }
            if(*ptr == ',')
                ptr++;
            ptr = EatSpaces(ptr);
        }
    }
    
    ASSERT(*ptr == ']');
    ptr++;
    
    sTrace("GLTF: Read %d Images", array_count);
    return (ptr);
}

GLTF *LoadGLTF(const char *path) {
    
    FILE *file = fopen(path, "r");
    if(!file) {
        sError("Unable to read file");
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    u32 file_size = ftell(file);
    char *file_start = sCalloc(file_size, sizeof(char));
    fseek(file, 0, SEEK_SET);
    fread(file_start, 1, file_size, file);
    fclose(file);
    
    sLog("File read. %d characters", file_size);
    
    GLTF *gltf = sCalloc(1, sizeof(GLTF));
    
    bool parsing = true;
    u32 depth = 0;
    
    char *ptr = EatSpaces(file_start);
    while(parsing) {
        switch(*ptr) {
            case '\0' : {
                parsing = false;
            } break;
            case '{' : {
                ptr++;
                depth++;
                ptr = EatSpaces(ptr);
            } break;
            case '}' : {
                ptr++;
                depth--;
                ptr = EatSpaces(ptr);
                if(depth == 0)
                    parsing = false;
            } break;
            case '\"' : {
                if(depth == 1) {
                    char key[32];
                    ptr = JsonParseString(ptr, key);
                    ptr = JsonEatColon(ptr);
                    if (strcmp(key, "accessors") == 0)
                        ptr = GLTFParseAccessors(ptr, &gltf->accessors, &gltf->accessor_count);
                    else if (strcmp(key, "bufferViews") == 0)
                        ptr = GLTFParseBufferViews(ptr, &gltf->buffer_views, &gltf->buffer_view_count);
                    else if (strcmp(key, "buffers") == 0)
                        ptr = GLTFParseBuffers(ptr, &gltf->buffers, &gltf->buffer_count);
                    else if (strcmp(key, "images") == 0)
                        ptr = GLTFParseImages(ptr, &gltf->images, &gltf->image_count);
                    else  {
                        sLog("JSON: Unread value %s", key);
                        ptr = JsonSkipValue(ptr);
                    }
                    if(*ptr == ',')
                        ptr ++; // ','
                    ptr = EatSpaces(ptr);
                }
            } break;
        }
    }
    
    sFree(file_start);
    
    return gltf;
}

void DestroyGLTF(GLTF *gltf) {
    
    sFree(gltf->accessors);
    sFree(gltf->buffer_views);
    
    for(u32 i = 0; i < gltf->buffer_count; i++) {
        sFree(gltf->buffers[i].uri);
    }
    sFree(gltf->buffers);
    
    for(u32 i = 0; i < gltf->image_count; i++) {
        sFree(gltf->images[i].uri);
    }
    sFree(gltf->images);
    
}

