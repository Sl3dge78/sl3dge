#ifndef MESH_H
#define MESH_H

#include "renderer/vulkan_types.h"

struct Mesh {
    Buffer buffer;        // idx & vtx buffer
    u32 all_index_offset; // Indices start at this offset in the buffer

    u32 total_vertex_count;
    u32 total_index_count;
    u32 total_primitives_count;
    Primitive *primitives;

    u32 primitive_nodes_count;
    Mat4 *primitive_transforms;

    u32 instance_count;
    Mat4 *instance_transforms;
};

Mesh *LoadMesh(const char *file, Renderer *renderer);
void DestroyMesh(Renderer *context, Mesh *mesh);
void DrawMesh(Frame *frame, Mesh *mesh);

#endif