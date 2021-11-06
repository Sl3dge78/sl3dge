#include <stdio.h>
#include <GL/GL.h>

#include "utils/sl3dge.h"

typedef enum GLTFAccessorType {
    GLTF_ACCESSOR_TYPE_SCALAR,
    GLTF_ACCESSOR_TYPE_VEC2,
    GLTF_ACCESSOR_TYPE_VEC3,
    GLTF_ACCESSOR_TYPE_VEC4,
    GLTF_ACCESSOR_TYPE_MAT4,
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
    u32 byte_stride;
    u32 target;
} GLTFBufferView;

typedef struct GLTFBuffer {
    u32 byte_length;
    char *uri;
    void *data;
} GLTFBuffer;

typedef struct GLTFImage {
    char *uri;
} GLTFImage;

typedef enum GLTFPrimitiveAttributesFlags {
    PRIMITIVE_ATTRIBUTE_POSITION = 1 << 0,
    PRIMITIVE_ATTRIBUTE_NORMAL = 1 << 1,
    PRIMITIVE_ATTRIBUTE_TEXCOORD_0 = 1 << 2,
    PRIMITIVE_ATTRIBUTE_JOINTS_0 = 1 << 3,
    PRIMITIVE_ATTRIBUTE_WEIGHTS_0 = 1 << 4,
    
    PRIMITIVE_SKINNED = PRIMITIVE_ATTRIBUTE_JOINTS_0 | PRIMITIVE_ATTRIBUTE_WEIGHTS_0,
} GLTFPrimitiveAttributesFlags;

typedef struct GLTFPrimitive {
    u32 attributes_set;
    
    u32 indices;
    
    u32 position;
    u32 normal;
    u32 texcoord_0;
    u32 joints_0;
    u32 weights_0;
    
    u32 material;
    u32 mode;
} GLTFPrimitive;

typedef struct GLTFMesh {
    char *name;
    
    u32 primitive_count;
    GLTFPrimitive *primitives;
} GLTFMesh;

typedef struct GLTFSkin {
    u32 inverse_bind_matrices;
    
    u32 joint_count;
    u32 *joints;
    
    char *name;
} GLTFSkin;

typedef struct GLTFNode {
    char *name;
    
    Transform xform;
    
    u32 child_count;
    u32 *children;
    
} GLTFNode;

typedef enum GLTFAnimationPath {
    ANIMATION_PATH_TRANSLATION,
    ANIMATION_PATH_ROTATION,
    ANIMATION_PATH_SCALE,
}  GLTFAnimationPath;

typedef struct GLTFChannel {
    u32 sampler;
    u32 target_node;
    GLTFAnimationPath path;
    
} GLTFChannel;

typedef enum GLTFInterpolationType {
    INTERPOLATION_LINEAR,
} GLTFInterpolationType;

typedef struct GLTFSampler {
    u32 input;
    GLTFInterpolationType interpolation;
    u32 output;
    
} GLTFSampler;

typedef struct GLTFAnimation {
    u32 channel_count;
    GLTFChannel *channels;
    
    char *name;
    
    u32 sampler_count;
    GLTFSampler *samplers;
    
} GLTFAnimation;

typedef struct GLTF {
    const char* path;
    
    u32 accessor_count;
    GLTFAccessor *accessors;
    
    u32 buffer_view_count;
    GLTFBufferView* buffer_views;
    
    u32 buffer_count;
    GLTFBuffer *buffers;
    
    u32 image_count;
    GLTFImage *images;
    
    u32 mesh_count;
    GLTFMesh *meshes;
    
    u32 skin_count;
    GLTFSkin *skins;
    
    u32 node_count;
    GLTFNode *nodes;
    
    u32 animation_count;
    GLTFAnimation *animations;
    
} GLTF;

void Base64Decode(u32 size, char *src, char *dst) {
    u32 buffer = 0;
    u32 buffer_bits = 0;
    
    for(u32 i = 0; i < size; ++i) {
        while(buffer_bits < 8) {
            char ch = *src++;
            
            i32 index = (unsigned)(ch - 'A') < 26   ? (ch - 'A')
                : (unsigned)(ch - 'a') < 26 ? (ch - 'a') + 26
                : (unsigned)(ch - '0') < 10 ? (ch - '0') + 52
                : ch == '+'                 ? 62
                : ch == '/'                 ? 63
                : -1;
            
            buffer = (buffer << 6) | index;
            buffer_bits += 6;
        }
        
        dst[i] = (u8)(buffer >> (buffer_bits - 8));
        buffer_bits -= 8;
    }
}

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
                depth++;
            } break;
            case ',' : {
                if(depth == 0) 
                    count ++;
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
    return (count + 1);
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
char *JsonParseF32(char *ptr, f32 *dst){
    sscanf(ptr, "%f", dst);
    while((*ptr >= '0' && *ptr <= '9') || *ptr == '-' || *ptr == '.' || *ptr == 'e') ptr++;
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
                } else if(strcmp(type, "MAT4") == 0) {
                    acc->type = GLTF_ACCESSOR_TYPE_MAT4;
                } else {
                    sWarn("JSON: Unread accessor \"%s\" : \"%s\"", key, type);
                    ptr = JsonSkipValue(ptr);
                }
            } else {
                sTrace("JSON: Unread accessor value \"%s\"", key);
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
            } else if(strcmp(key, "byteStride") == 0) {
                ptr = JsonParseU32(ptr, &bv->byte_stride);
            } else {
                sTrace("JSON: Unread value %s", key);
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

char *GLTFParseBuffers(char *ptr, GLTFBuffer **buffers, u32 *count, const char* path, PlatformAPI *platform) {
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
                
                if (strncmp(ptr, "\"data:", 6) == 0) { // We are embedded
                    sTrace("Loading buffer as embeded");
                    buf->data = sCalloc(buf->byte_length, 1);
                    ptr += 6;
                    
                    while(*(ptr++) != ',');
                    Base64Decode(buf->byte_length, ptr, buf->data);
                    while(*(ptr++) != '\"');
                } else {
                    u32 length = JsonGetStringLength(ptr);
                    buf->uri = sCalloc(length, sizeof(char));
                    ptr = JsonParseString(ptr, buf->uri);
                    
                    char directory[128] = {0};
                    const char *last_sep = strrchr(path, '/');
                    u32 size = last_sep - path;
                    strncpy_s(directory, ARRAY_SIZE(directory), path, size);
                    directory[size] = '/';
                    directory[size + 1] = '\0';
                    
                    i64 buffer_size;
                    char buffer_path[512] = {0};
                    strcat(buffer_path, directory);
                    strcat(buffer_path, buf->uri);
                    platform->ReadBinary(buffer_path, &buffer_size, NULL);
                    buf->data = sCalloc(buffer_size, 1);
                    platform->ReadBinary(buffer_path, &buffer_size, buf->data);
                    
                }
            } else {
                sTrace("JSON: Unread value %s", key);
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
                sTrace("JSON: Unread value %s", key);
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

char *GLTFParsePrimitives(char *ptr, GLTFPrimitive **primitives, u32 *count) {
    sTrace("GLTF: Reading Buffers");
    u32 array_count = JsonCountArray(ptr);
    *count = array_count;
    *primitives = sCalloc(array_count, sizeof(GLTFPrimitive));
    ptr++; // '['
    ptr = EatSpaces(ptr);
    
    for(u32 i = 0; i < array_count; i++) {
        ASSERT(*ptr == '{');
        ptr++;
        ptr = EatSpaces(ptr);
        GLTFPrimitive *prim = &(*primitives)[i];
        
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
            if(strcmp(key, "attributes") == 0) {
                
                ptr++; // '{'
                for(;;) {
                    ptr = EatSpaces(ptr);
                    char attr[32];
                    ptr = JsonParseString(ptr, attr);
                    ptr = JsonEatColon(ptr);
                    if(strcmp(attr,"NORMAL") == 0) {
                        ptr = JsonParseU32(ptr, &prim->normal);
                        prim->attributes_set |= PRIMITIVE_ATTRIBUTE_NORMAL;
                    } else if(strcmp(attr, "POSITION") == 0) {
                        ptr = JsonParseU32(ptr, &prim->position);
                        prim->attributes_set |= PRIMITIVE_ATTRIBUTE_POSITION;
                    } else if(strcmp(attr, "TEXCOORD_0") == 0) {
                        ptr = JsonParseU32(ptr, &prim->texcoord_0);
                        prim->attributes_set |= PRIMITIVE_ATTRIBUTE_TEXCOORD_0;
                    } else if(strcmp(attr, "JOINTS_0") == 0) {
                        ptr = JsonParseU32(ptr, &prim->joints_0);
                        prim->attributes_set |= PRIMITIVE_ATTRIBUTE_JOINTS_0;
                    } else if(strcmp(attr, "WEIGHTS_0") == 0) {
                        ptr = JsonParseU32(ptr, &prim->weights_0);
                        prim->attributes_set |= PRIMITIVE_ATTRIBUTE_WEIGHTS_0;
                    } else {
                        sTrace("JSON: Unread value %s", attr);
                        ptr = JsonSkipValue(ptr);
                    }
                    if(*ptr == ',')
                        ptr++;
                    ptr = EatSpaces(ptr);
                    if(*ptr == '}') {
                        ptr++;
                        break;
                    }
                }
            } else if(strcmp(key, "indices") == 0) {
                ptr = JsonParseU32(ptr, &prim->indices);
            } else if(strcmp(key, "material") == 0) {
                ptr = JsonParseU32(ptr, &prim->material);
            } else if(strcmp(key, "mode") == 0) {
                ptr = JsonParseU32(ptr, &prim->mode);
            } else {
                sTrace("JSON: Unread value %s", key);
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

char *GLTFParseMeshes(char *ptr, GLTFMesh **meshes, u32 *count) {
    sTrace("GLTF: Reading Buffers");
    u32 array_count = JsonCountArray(ptr);
    *count = array_count;
    *meshes = sCalloc(array_count, sizeof(GLTFMesh));
    ptr++; // '['
    ptr = EatSpaces(ptr);
    
    for(u32 i = 0; i < array_count; i++) {
        ASSERT(*ptr == '{');
        ptr++;
        ptr = EatSpaces(ptr);
        GLTFMesh *mesh = &(*meshes)[i];
        
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
            if(strcmp(key, "primitives") == 0) {
                ptr = GLTFParsePrimitives(ptr, &mesh->primitives, &mesh->primitive_count);
            } else if(strcmp(key, "name") == 0) {
                u32 length = JsonGetStringLength(ptr);
                mesh->name = sCalloc(length, sizeof(char));
                ptr = JsonParseString(ptr, mesh->name);
            } else {
                sTrace("JSON: Unread value %s", key);
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


char *GLTFParseSkins(char *ptr, GLTFSkin **skins, u32 *count) {
    sTrace("GLTF: Reading Skins");
    u32 array_count = JsonCountArray(ptr);
    *count = array_count;
    *skins = sCalloc(array_count, sizeof(GLTFSkin));
    ptr++; // '['
    ptr = EatSpaces(ptr);
    
    for(u32 i = 0; i < array_count; i++) {
        ASSERT(*ptr == '{');
        ptr++;
        ptr = EatSpaces(ptr);
        GLTFSkin *skin = &(*skins)[i];
        
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
            if(strcmp(key, "inverseBindMatrices") == 0) {
                ptr = JsonParseU32(ptr, &skin->inverse_bind_matrices);
            } else if(strcmp(key, "name") == 0) {
                u32 length = JsonGetStringLength(ptr);
                skin->name = sCalloc(length, sizeof(char));
                ptr = JsonParseString(ptr, skin->name);
            } else if(strcmp(key, "joints") == 0) {
                skin->joint_count = JsonCountArray(ptr);
                skin->joints = sCalloc(skin->joint_count, sizeof(u32));
                ptr++; // '['
                ptr = EatSpaces(ptr);
                for(u32 j = 0; j < skin->joint_count; j++) {
                    ptr = JsonParseU32(ptr, &skin->joints[j]);
                    if(*ptr == ',')
                        ptr++;
                    ptr = EatSpaces(ptr);
                }
                ASSERT(*ptr == ']');
                ptr++;
            } else {
                sTrace("JSON: Unread value %s", key);
                ptr = JsonSkipValue(ptr);
            }
            if(*ptr == ',')
                ptr++;
            ptr = EatSpaces(ptr);
        }
    }
    
    ASSERT(*ptr == ']');
    ptr++;
    
    sTrace("GLTF: Read %d Skins", array_count);
    return (ptr);
}

char *GLTFParseNodes(char *ptr, GLTFNode **nodes, u32 *count) {
    sTrace("GLTF: Reading Nodes");
    u32 array_count = JsonCountArray(ptr);
    *count = array_count;
    *nodes = sCalloc(array_count, sizeof(GLTFNode));
    ptr++; // '['
    ptr = EatSpaces(ptr);
    
    for(u32 i = 0; i < array_count; i++) {
        ASSERT(*ptr == '{');
        ptr++;
        ptr = EatSpaces(ptr);
        GLTFNode *node = &(*nodes)[i];
        transform_identity(&node->xform);
        
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
            
            if(strcmp(key, "name") == 0) {
                u32 length = JsonGetStringLength(ptr);
                node->name = sCalloc(length, sizeof(char));
                ptr = JsonParseString(ptr, node->name);
            } else if(strcmp(key, "children") == 0) {
                node->child_count = JsonCountArray(ptr);
                node->children = sCalloc(node->child_count, sizeof(u32));
                ptr ++; // '['
                for(u32 j = 0; j < node->child_count; j++) {
                    ptr = EatSpaces(ptr);
                    ptr = JsonParseU32(ptr, &node->children[j]);
                    if(*ptr == ',')
                        ptr++;
                }
                ptr = EatSpaces(ptr);
                ASSERT(*ptr == ']');
                ptr++;
            } else if(strcmp(key, "rotation") == 0) {
                ptr ++; // '['
                ptr = EatSpaces(ptr);
                ptr = JsonParseF32(ptr, &node->xform.rotation.x);
                ptr++; // ','
                ptr = EatSpaces(ptr);
                ptr = JsonParseF32(ptr, &node->xform.rotation.y);
                ptr++; // ','
                ptr = EatSpaces(ptr);
                ptr = JsonParseF32(ptr, &node->xform.rotation.z);
                ptr++; // ','
                ptr = EatSpaces(ptr);
                ptr = JsonParseF32(ptr, &node->xform.rotation.w);
                ptr = EatSpaces(ptr);
                ASSERT(*ptr == ']');
                ptr++;
                
            } else if(strcmp(key, "translation") == 0) {
                ptr ++; // '['
                ptr = EatSpaces(ptr);
                ptr = JsonParseF32(ptr, &node->xform.translation.x);
                ptr++; // ','
                ptr = EatSpaces(ptr);
                ptr = JsonParseF32(ptr, &node->xform.translation.y);
                ptr++; // ','
                ptr = EatSpaces(ptr);
                ptr = JsonParseF32(ptr, &node->xform.translation.z);
                ptr = EatSpaces(ptr);
                ASSERT(*ptr == ']');
                ptr++;
            } else {
                sTrace("JSON: Unread value %s", key);
                ptr = JsonSkipValue(ptr);
            }
            if(*ptr == ',')
                ptr++;
            ptr = EatSpaces(ptr);
        }
    }
    
    ASSERT(*ptr == ']');
    ptr++;
    
    sTrace("GLTF: Read %d Nodes", array_count);
    return (ptr);
}

char *GLTFParseChannels(char *ptr, GLTFChannel **channels, u32 *count) {
    sTrace("GLTF: Reading Channels");
    
    u32 array_count = JsonCountArray(ptr);
    *count = array_count;
    *channels = sCalloc(array_count, sizeof(GLTFChannel));
    ptr++; // '['
    ptr = EatSpaces(ptr);
    
    for(u32 i = 0; i < array_count; i++) {
        ASSERT(*ptr == '{');
        ptr++;
        ptr = EatSpaces(ptr);
        GLTFChannel *channel = &(*channels)[i];
        
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
            
            if(strcmp(key, "sampler") == 0) {
                ptr = EatSpaces(ptr);
                ptr = JsonParseU32(ptr, &channel->sampler);
            } else if (strcmp(key, "target") == 0) {
                ptr = EatSpaces(ptr);
                ptr++; // '{'
                ptr = EatSpaces(ptr);
                for(;;) {
                    
                    if(*ptr == '}') {
                        ptr++; // '}'
                        ptr++; // ','
                        ptr = EatSpaces(ptr);
                        break;
                    }
                    
                    ptr = EatSpaces(ptr);
                    ptr = JsonParseString(ptr, key);
                    ptr = JsonEatColon(ptr);
                    
                    if(strcmp(key, "node") == 0) {
                        ptr = EatSpaces(ptr);
                        ptr = JsonParseU32(ptr, &channel->target_node);
                        ptr = EatSpaces(ptr);
                    } else if(strcmp(key,"path") == 0) {
                        char path[32];
                        ptr = EatSpaces(ptr);
                        ptr = JsonParseString(ptr, path);
                        if(strcmp(path, "translation") == 0) {
                            channel->path = ANIMATION_PATH_TRANSLATION;
                        } else if(strcmp(path, "rotation") == 0) {
                            channel->path = ANIMATION_PATH_ROTATION;
                        } else if(strcmp(path, "scale") == 0) {
                            channel->path = ANIMATION_PATH_SCALE;
                        } else {
                            sTrace("JSON: Channel/Target > Unread value %s", key);
                            ptr = JsonSkipValue(ptr);
                        }
                        ptr = EatSpaces(ptr);
                    } else {
                        sTrace("JSON: Unread value %s", key);
                        ptr = JsonSkipValue(ptr);
                    }
                    
                    if(*ptr == ',')
                        ptr++;
                }
                
            } else {
                sTrace("JSON: Unread value %s", key);
                ptr = JsonSkipValue(ptr);
            }
            
            if(*ptr == ',')
                ptr++;
            ptr = EatSpaces(ptr);
        }
    }
    
    ASSERT(*ptr == ']');
    ptr++;
    
    sTrace("GLTF: Read %d Channels", array_count);
    return (ptr);
}

char *GLTFParseSamplers(char *ptr, GLTFSampler **samplers, u32 *count) {
    sTrace("GLTF: Reading Channels");
    
    u32 array_count = JsonCountArray(ptr);
    *count = array_count;
    *samplers = sCalloc(array_count, sizeof(GLTFSampler));
    ptr++; // '['
    ptr = EatSpaces(ptr);
    
    for(u32 i = 0; i < array_count; i++) {
        ASSERT(*ptr == '{');
        ptr++;
        ptr = EatSpaces(ptr);
        GLTFSampler *sampler = &(*samplers)[i];
        
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
            
            if(strcmp(key, "input") == 0) {
                ptr = EatSpaces(ptr);
                ptr = JsonParseU32(ptr, &sampler->input);
            } else if (strcmp(key, "output") == 0) {
                ptr = EatSpaces(ptr);
                ptr = JsonParseU32(ptr, &sampler->output);
            } else if (strcmp(key, "interpolation") == 0) {
                ptr = EatSpaces(ptr);
                char interpolation[32] ;
                ptr = JsonParseString(ptr, interpolation);
                
                if(strcmp(interpolation, "LINEAR") == 0) {
                    sampler->interpolation = INTERPOLATION_LINEAR;
                } else {
                    sError("GLTF: Animation Sampler: Unknown interpolation type %s", interpolation);
                    ASSERT(0);
                }
                
            } else {
                sTrace("JSON: Unread value %s", key);
                ptr = JsonSkipValue(ptr);
            }
            
            if(*ptr == ',')
                ptr++;
            ptr = EatSpaces(ptr);
        }
    }
    
    ASSERT(*ptr == ']');
    ptr++;
    
    sTrace("GLTF: Read %d Channels", array_count);
    return (ptr);
}

char *GLTFParseAnimations(char *ptr, GLTFAnimation **animations, u32 *count) {
    sTrace("GLTF: Reading Animations");
    u32 array_count = JsonCountArray(ptr);
    *count = array_count;
    *animations = sCalloc(array_count, sizeof(GLTFAnimation));
    ptr++; // '['
    ptr = EatSpaces(ptr);
    
    for(u32 i = 0; i < array_count; i++) {
        ASSERT(*ptr == '{');
        ptr++;
        ptr = EatSpaces(ptr);
        GLTFAnimation *animation = &(*animations)[i];
        
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
            
            if(strcmp(key, "name") == 0) {
                u32 length = JsonGetStringLength(ptr);
                animation->name = sCalloc(length, sizeof(char));
                ptr = JsonParseString(ptr, animation->name);
            } else if(strcmp(key, "channels") == 0) {
                ptr = GLTFParseChannels(ptr, &animation->channels, &animation->channel_count);
                ptr = EatSpaces(ptr);
            } else if(strcmp(key, "samplers") == 0) {
                ptr = GLTFParseSamplers(ptr, &animation->samplers, &animation->sampler_count);
                ptr = EatSpaces(ptr);
            } else {
                sTrace("JSON: Unread value %s", key);
                ptr = JsonSkipValue(ptr);
            }
            
            if(*ptr == ',')
                ptr++;
            ptr = EatSpaces(ptr);
        }
    }
    
    ASSERT(*ptr == ']');
    ptr++;
    
    sTrace("GLTF: Read %d Animations", array_count);
    return (ptr);
    
}

u32 GLTFGetBoneIDFromNode(const GLTF *gltf, const u32 node_id) {
    
    for(u32 i = 0; i < gltf->skins[0].joint_count; ++i) {
        if(gltf->skins[0].joints[i] == node_id)
            return (i);
    }
    ASSERT(0);
    return 0;
}

void GLTFCopyAccessor(const GLTF *gltf, const u32 acc_id, void *dst, const u32 offset, const u32 dst_stride) {
    GLTFAccessor *acc = &gltf->accessors[acc_id];
    GLTFBufferView *view = &gltf->buffer_views[acc->buffer_view];
    
    char *buf = (char *)gltf->buffers[view->buffer].data + view->byte_offset + acc->byte_offset;
    size_t size = 0;
    switch(acc->component_type) {
        case GL_UNSIGNED_SHORT: size = sizeof(u16); break;
        case GL_UNSIGNED_INT: size = sizeof(u32); break;
        case GL_FLOAT: size = sizeof(f32); break;
        case GL_UNSIGNED_BYTE: size = sizeof(u8); break;
        default:
        sError("Unsupported component type : %d", acc->component_type);
        ASSERT(0);
        break;
    };
    switch(acc->type) {
        case GLTF_ACCESSOR_TYPE_SCALAR: break;
        case GLTF_ACCESSOR_TYPE_VEC4: size *= 4; break;
        case GLTF_ACCESSOR_TYPE_VEC3: size *= 3; break;
        case GLTF_ACCESSOR_TYPE_VEC2: size *= 2; break;
        case GLTF_ACCESSOR_TYPE_MAT4: size *= 4 * 4; break;
        default:
        sError("Unsupported type : %d", acc->type);
        ASSERT(0);
        break;
    };
    
    const u32 stride = view->byte_stride == 0 ? size : view->byte_stride;
    
    dst = (void *)((char *)dst + offset);
    
    for(u32 i = 0; i < acc->count; i++) {
        memcpy(dst, buf, size);
        buf += stride;
        dst = (void *)((char *)dst + dst_stride);
    }
}

GLTF *LoadGLTF(const char *path, PlatformAPI *platform) {
    
    FILE *file = fopen(path, "r");
    if(!file) {
        sError("Unable to read file");
        return NULL;
    }
    
    i32 file_size;
    platform->ReadWholeFile(path, &file_size, NULL);
    char *file_start = sCalloc(file_size, sizeof(char));
    platform->ReadWholeFile(path, &file_size, file_start);
    
    sTrace("File read. %d characters", file_size);
    
    GLTF *gltf = sCalloc(1, sizeof(GLTF));
    
    gltf->path = path;
    
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
                        ptr = GLTFParseBuffers(ptr, &gltf->buffers, &gltf->buffer_count, path, platform);
                    else if (strcmp(key, "images") == 0)
                        ptr = GLTFParseImages(ptr, &gltf->images, &gltf->image_count);
                    else if (strcmp(key, "meshes") == 0)
                        ptr = GLTFParseMeshes(ptr, &gltf->meshes, &gltf->mesh_count);
                    else if (strcmp(key, "skins") == 0)
                        ptr = GLTFParseSkins(ptr, &gltf->skins, &gltf->skin_count);
                    else if (strcmp(key, "nodes") == 0)
                        ptr = GLTFParseNodes(ptr, &gltf->nodes, &gltf->node_count);
                    else if (strcmp(key, "animations") == 0)
                        ptr = GLTFParseAnimations(ptr, &gltf->animations, &gltf->animation_count);
                    else  {
                        sTrace("JSON: Unread value %s", key);
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
        if(gltf->buffers[i].uri)
            sFree(gltf->buffers[i].uri);
        
        sFree(gltf->buffers[i].data);
    }
    sFree(gltf->buffers);
    
    if(gltf->image_count > 0) {
        for(u32 i = 0; i < gltf->image_count; i++) {
            sFree(gltf->images[i].uri);
        }
        sFree(gltf->images);
    }
    
    if(gltf->mesh_count > 0) { 
        for(u32 i = 0; i < gltf->mesh_count; i++) {
            sFree(gltf->meshes[i].primitives);
            sFree(gltf->meshes[i].name);
        }
        sFree(gltf->meshes);
    }
    
    if(gltf->skin_count > 0) {
        for(u32 i = 0; i < gltf->skin_count; i++) {
            sFree(gltf->skins[i].joints);
            sFree(gltf->skins[i].name);
        }
        sFree(gltf->skins);
    } 
    
    if(gltf->node_count > 0) {
        for(u32 i = 0; i < gltf->node_count; i++) {
            sFree(gltf->nodes[i].name);
            if(gltf->nodes[i].child_count > 0)
                sFree(gltf->nodes[i].children);
        }
        sFree(gltf->nodes);
    } 
    
    if(gltf->animation_count > 0) {
        for(u32 i = 0; i < gltf->animation_count; i++) {
            sFree(gltf->animations[i].channels);
            sFree(gltf->animations[i].samplers);
            sFree(gltf->animations[i].name);
        }
        sFree(gltf->animations);
    }
    
    sFree(gltf);
}