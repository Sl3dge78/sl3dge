#include <windows.h>
#include <GL/GL.h>

#ifdef __INTELLISENSE__
#define GL_GLEXT_PROTOTYPES
#endif
#include <GL/glext.h>

#ifndef __INTELLISENSE__
#include "opengl_functions.h"
#endif

#ifdef __WIN32__
#include "renderer/opengl/opengl_win32.c"
#endif

#include <sl3dge-utils/sl3dge.h>
#include <cgltf/cgltf.h>

#include "renderer/renderer.h"
#include "renderer/opengl/opengl_renderer.h"
#include "renderer/gltf.c"

void APIENTRY GLMessageCallback(GLenum source,
                                GLenum type,
                                GLuint id,
                                GLenum severity,
                                GLsizei length,
                                const GLchar *message,
                                const void *userParam) {
    if(severity == GL_DEBUG_SEVERITY_HIGH) {
        sError("GL ERROR: %s", message);
        //ASSERT(0);
    } else if(severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_LOW) {
        sWarn("GL WARN: %s", message);
    }
}

internal void GLCheckShaderCompilation(u32 shader) {
    i32 result;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if(!result) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        sError("Unable to compile shader:\n%s");
    }
}

internal void GLCreateAndCompileShader(GLenum type, const char *code, u32 *result) {
    *result = glCreateShader(type);
    glShaderSource(*result, 1, &code, NULL);
    glCompileShader(*result);
    GLCheckShaderCompilation(*result);
}

u32 RendererCreateMesh(Renderer *renderer, const char *path) {
    ASSERT_MSG(0, "Not implemented");
    return 0;
}

u32 RendererLoadMesh(Renderer *renderer, const char *path, Mesh *mesh) {
    sLog("Loading Mesh...");
    *mesh = (Mesh){0};
    renderer->mesh_count++;

    char directory[128] = {0};
    const char *last_sep = strrchr(path, '/');
    u32 size = last_sep - path;
    strncpy_s(directory, ARRAY_SIZE(directory), path, size);
    directory[size] = '/';
    directory[size + 1] = '\0';

    cgltf_data *data;
    cgltf_options options = {0};
    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if(result != cgltf_result_success) {
        sError("Error reading mesh");
        ASSERT(0);
    }

    cgltf_load_buffers(&options, data, path);

    // Vertex & Index Buffer
    glGenBuffers(1, &mesh->vertex_buffer);
    glGenBuffers(1, &mesh->index_buffer);
    glGenVertexArrays(1, &mesh->vertex_array);

    glBindVertexArray(mesh->vertex_array);
    u32 i = 0;
    for(u32 m = 0; m < data->meshes_count; ++m) {
        if(data->meshes[m].primitives_count > 1) {
            sWarn("Only 1 primitive supported yet");
            // TODO
        }
        for(u32 p = 0; p < data->meshes[m].primitives_count; p++) {
            cgltf_primitive *prim = &data->meshes[m].primitives[p];
            mesh->index_count = data->meshes[m].primitives[p].indices->count;
            u32 index_buffer_size = 0;
            void *index_data = GLTFGetIndexBuffer(prim, &index_buffer_size);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->index_buffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_buffer_size, index_data, GL_STATIC_DRAW);
            sFree(index_data);

            mesh->vertex_count = (u32)prim->attributes[0].data->count;
            u32 vertex_buffer_size = 0;
            void *vertex_data = GLTFGetVertexBuffer(prim, &vertex_buffer_size);

            glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer);
            glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, vertex_data, GL_STATIC_DRAW);
            sFree(vertex_data);
            ++i;
        }
    }

    glObjectLabel(GL_BUFFER, mesh->vertex_buffer, -1, "GLTF VTX BUFFER");
    glObjectLabel(GL_BUFFER, mesh->index_buffer, -1, "GLTF IDX BUFFER");
    glObjectLabel(GL_VERTEX_ARRAY, mesh->vertex_array, -1, "GLTF ARRAY BUFFER");

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);

    // Textures
    for(u32 i = 0; i < data->textures_count; ++i) {
        u32 *texture = &renderer->textures[renderer->texture_count++];

        char *image_path = data->textures[i].image->uri;
        char full_image_path[256] = {0};
        strcat_s(full_image_path, 256, directory);
        strcat_s(full_image_path, 256, image_path);

        PNG_Image *image = sLoadImage(full_image_path);
        glGenTextures(1, texture);
        glBindTexture(GL_TEXTURE_2D, *texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glObjectLabel(GL_TEXTURE, *texture, -1, "NAME");
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     image->width,
                     image->height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     image->pixels);
        sDestroyImage(image);

        if(data->textures[i].type == cgltf_texture_type_base_color) {
            mesh->diffuse_texture = *texture;
        }
    }

    cgltf_free(data);

    // TEMP: hardcoded mesh id
    renderer->meshes[0] = mesh;
    sLog("Loading done");

    return 0;
}

void RendererDestroyMesh(Mesh *mesh) {
    glDeleteBuffers(1, &mesh->index_buffer);
    glDeleteBuffers(1, &mesh->vertex_buffer);
}

Renderer *RendererCreate(PlatformWindow *window) {
    Renderer *renderer = sCalloc(1, sizeof(Renderer));

    renderer->window = window;
    PlatformCreateOpenGLContext(renderer, window);
    GLLoadFunctions();
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(GLMessageCallback, 0);

    // Shader
    char *vtx_shader = PlatformReadWholeFile("resources/shaders/gl/main.vert");
    GLCreateAndCompileShader(GL_VERTEX_SHADER, vtx_shader, &renderer->vertex_shader);
    sFree(vtx_shader);

    char *frag_shader = PlatformReadWholeFile("resources/shaders/gl/main.frag");
    GLCreateAndCompileShader(GL_FRAGMENT_SHADER, frag_shader, &renderer->fragment_shader);
    sFree(frag_shader);

    renderer->shader_program = glCreateProgram();
    glAttachShader(renderer->shader_program, renderer->vertex_shader);
    glAttachShader(renderer->shader_program, renderer->fragment_shader);
    glLinkProgram(renderer->shader_program);
    glObjectLabel(GL_PROGRAM, renderer->shader_program, -1, "MAIN SHADER");

    glDeleteShader(renderer->vertex_shader);
    glDeleteShader(renderer->fragment_shader);

    renderer->meshes = sCalloc(1, sizeof(Mesh *));

    RendererLoadMesh(renderer, "resources/3d/Motorcycle/motorcycle.gltf", &renderer->moto);
    //RendererLoadMesh(renderer, "resources/models/gltf_samples/Box/glTF/Box.gltf");

    glViewport(0, 0, renderer->width, renderer->height);
    glClearColor(0.43f, 0.77f, 0.91f, 0.0f);

    glUseProgram(renderer->shader_program);
    renderer->camera_proj = mat4_perspective_gl(90, 1280.0f / 720.0f, 0.1, 1000);
    u32 proj_loc = glGetUniformLocation(renderer->shader_program, "projection");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, renderer->camera_proj.v);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    return renderer;
}

void RendererDrawFrame(Renderer *renderer) {
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glUseProgram(renderer->shader_program);
    u32 view_loc = glGetUniformLocation(renderer->shader_program, "view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, renderer->camera_view.v);

    glBindTexture(GL_TEXTURE_2D, renderer->moto.diffuse_texture);
    glBindVertexArray(renderer->moto.vertex_array);
    glDrawElements(GL_TRIANGLES, renderer->moto.index_count, GL_UNSIGNED_INT, 0);

    PlatformSwapBuffers(renderer);
}

void RendererDestroy(Renderer *renderer) {
    RendererDestroyMesh(renderer->meshes[0]);

    sFree(renderer->meshes);
    glDeleteProgram(renderer->shader_program);
    sFree(renderer);
}

void RendererUpdateWindow(Renderer *renderer, PlatformWindow *window) {
}

MeshInstance RendererInstantiateMesh(Renderer *renderer, u32 mesh_id) {
    MeshInstance result = {0};
    return result;
}

void RendererSetCamera(Renderer *renderer, const Vec3 position, const Vec3 forward, const Vec3 up) {
    renderer->camera_pos = position;
    renderer->camera_view = mat4_look_at(vec3_add(position, forward), position, up);
    mat4_inverse(&renderer->camera_view, &renderer->camera_view_inverse);
}

void RendererSetSunDirection(Renderer *renderer, const Vec3 direction) {
}
