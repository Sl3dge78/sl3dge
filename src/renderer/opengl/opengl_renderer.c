#define WIN32_LEAN_AND_MEAN
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

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

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
        ASSERT(0);
    } else if(severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_LOW) {
        sWarn("GL WARN: %s", message);
    }
}

// ---------------
// Programs & shaders

internal bool CheckShaderCompilation(u32 shader) {
    i32 result;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if(!result) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        sError("Unable to compile shader:\n%s", log);
    }
    return result;
}

internal u32 CreateAndCompileShader(GLenum type, const char *code) {
    u32 result = glCreateShader(type);
    glShaderSource(result, 1, &code, NULL);
    glCompileShader(result);
    ASSERT(CheckShaderCompilation(result));
    return result;
}

internal u32 CreateProgram(const char *name) {
    u32 result = glCreateProgram();
    char buffer[128] = {0};

    snprintf(buffer, 128, "resources/shaders/gl/%s.vert", name);
    char *vtx_code = PlatformReadWholeFile(buffer);
    ASSERT(vtx_code != NULL); // Vertex shaders are mandatory
    u32 vtx_shader = CreateAndCompileShader(GL_VERTEX_SHADER, vtx_code);
    sFree(vtx_code);
    glAttachShader(result, vtx_shader);

    snprintf(buffer, 128, "resources/shaders/gl/%s.frag", name);
    char *frag_code = PlatformReadWholeFile(buffer);

    u32 frag_shader = 0;
    if(frag_code == NULL) {
        sLog("No fragment shader for %s", name);
    } else {
        u32 frag_shader = CreateAndCompileShader(GL_FRAGMENT_SHADER, frag_code);
        sFree(frag_code);
        glAttachShader(result, frag_shader);
    }
    glLinkProgram(result);
    glObjectLabel(GL_PROGRAM, result, -1, name);
    glDeleteShader(vtx_shader);
    if(frag_code != NULL) {
        glDeleteShader(frag_shader);
    }
    return result;
}

// --------------
// Mesh

internal Mesh *GetNewMesh(Renderer *renderer) {
    if(renderer->mesh_count == renderer->mesh_capacity) {
        u32 new_capacity = renderer->mesh_capacity * 2;
        void *ptr = sRealloc(renderer->meshes, sizeof(Mesh) * new_capacity);
        ASSERT(ptr);
        if(ptr) {
            renderer->meshes = (Mesh *)ptr;
        }
    }

    return &renderer->meshes[renderer->mesh_count];
}

MeshHandle RendererLoadMesh(Renderer *renderer, const char *path) {
    sLog("Loading Mesh...");

    Mesh *mesh = GetNewMesh(renderer);

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
        char *image_path = data->textures[i].image->uri;
        char full_image_path[256] = {0};
        strcat_s(full_image_path, 256, directory);
        strcat_s(full_image_path, 256, image_path);

        PNG_Image *image = sLoadImage(full_image_path);
        u32 texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glObjectLabel(GL_TEXTURE, texture, -1, "NAME");
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
            mesh->diffuse_texture = texture;
        }
    }

    cgltf_free(data);

    sLog("Loading done");

    return renderer->mesh_count++;
}

MeshHandle RendererLoadMeshFromVertices(Renderer *renderer, const Vertex *vertices, const u32 vertex_count, const u32 *indices, const u32 index_count) {
    Mesh *mesh = GetNewMesh(renderer);

    mesh->index_count = index_count;
    mesh->vertex_count = vertex_count;

    // Vertex & Index Buffer
    glGenBuffers(1, &mesh->vertex_buffer);
    glGenBuffers(1, &mesh->index_buffer);
    glGenVertexArrays(1, &mesh->vertex_array);
    glBindVertexArray(mesh->vertex_array);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(u32), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(Vertex), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);

    glObjectLabel(GL_BUFFER, mesh->vertex_buffer, -1, "GEN VTX BUFFER");
    glObjectLabel(GL_BUFFER, mesh->index_buffer, -1, "GEN IDX BUFFER");
    glObjectLabel(GL_VERTEX_ARRAY, mesh->vertex_array, -1, "GEN ARRAY BUFFER");

    mesh->diffuse_texture = renderer->white_texture;

    return renderer->mesh_count++;
}

void RendererDestroyMesh(Mesh *mesh) {
    glDeleteBuffers(1, &mesh->index_buffer);
    glDeleteBuffers(1, &mesh->vertex_buffer);
}

// ---------------
// Shadowmap

internal void CreateShadowmapRenderPass(ShadowmapRenderPass *pass) {
    pass->program = CreateProgram("shadow");

    glGenTextures(1, &pass->texture);
    glBindTexture(GL_TEXTURE_2D, pass->texture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 2048, 2048, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glGenFramebuffers(1, &pass->framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, pass->framebuffer);
    glObjectLabel(GL_FRAMEBUFFER, pass->framebuffer, -1, "Shadowmap framebuffer");
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass->texture, 0);
}

internal void DestroyShadowmapRenderPass(ShadowmapRenderPass *pass) {
    glDeleteProgram(pass->program);
    glDeleteFramebuffers(1, &pass->framebuffer);
    glDeleteTextures(1, &pass->texture);
}

internal void BeginShadowmapRenderPass(ShadowmapRenderPass *pass) {
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 2048, 2048);
    glBindFramebuffer(GL_FRAMEBUFFER, pass->framebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(pass->program);
}

// -------------
// Color

internal void CreateColorRenderPass(const u32 width, const u32 height, ColorRenderPass *pass) {
    pass->program = CreateProgram("color");
    glUseProgram(pass->program);
    glUniform1i(glGetUniformLocation(pass->program, "shadow_map"), 0);
    glUniform1i(glGetUniformLocation(pass->program, "diffuse"), 1);

    glGenFramebuffers(1, &pass->framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, pass->framebuffer);
    glObjectLabel(GL_FRAMEBUFFER, pass->framebuffer, -1, "Color framebuffer");

    // Color attachment
    glGenTextures(1, &pass->render_target);
    glBindTexture(GL_TEXTURE_2D, pass->render_target);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->render_target, 0);

    // Depth attachment
    glGenTextures(1, &pass->depthmap);
    glBindTexture(GL_TEXTURE_2D, pass->depthmap);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass->depthmap, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

internal void DestroyColorRenderPass(ColorRenderPass *pass) {
    glDeleteFramebuffers(1, &pass->framebuffer);
    glDeleteTextures(1, &pass->render_target);
    glDeleteProgram(pass->program);
    glDeleteTextures(1, &pass->depthmap);
}

internal void BeginColorRenderPass(Renderer *renderer, ColorRenderPass *pass) {
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, renderer->width, renderer->height);

    glBindFramebuffer(GL_FRAMEBUFFER, pass->framebuffer);
    glClearColor(0.43f, 0.77f, 0.91f, 0.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glUseProgram(pass->program);

    // Shadowmap
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->shadowmap_pass.texture);
}

// --------------
// Volumetric

internal void CreateVolumetricRenderPass(VolumetricRenderPass *pass) {
    pass->program = CreateProgram("volumetric");

    glUseProgram(pass->program);
    glUniform1i(glGetUniformLocation(pass->program, "shadow_map"), 0);
    glUniform1i(glGetUniformLocation(pass->program, "depth_map"), 1);
    glUniform1i(glGetUniformLocation(pass->program, "screen_texture"), 2);
}

internal void DestroyVolumetricRenderPass(VolumetricRenderPass *pass) {
    glDeleteProgram(pass->program);
}

internal void BeginVolumetricRenderPass(Renderer *renderer, VolumetricRenderPass *pass) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(pass->program);
    glUniformMatrix4fv(
        glGetUniformLocation(pass->program, "cam_view"), 1, GL_FALSE, renderer->camera_view.v);
    glUniformMatrix4fv(glGetUniformLocation(pass->program, "view_inverse"),
                       1,
                       GL_FALSE,
                       renderer->camera_view_inverse.v);
    glUniform3f(glGetUniformLocation(pass->program, "view_pos"),
                renderer->camera_pos.x,
                renderer->camera_pos.y,
                renderer->camera_pos.z);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->shadowmap_pass.texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->color_pass.depthmap);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, renderer->color_pass.render_target);
}

// ---------------
// Renderer

void UpdateCameraProj(Renderer *renderer) {
    renderer->camera_proj =
        mat4_perspective_gl(90.0f, (f32)renderer->width / (f32)renderer->height, 0.1f, 100000.0f);
    mat4_inverse(&renderer->camera_proj, &renderer->camera_proj_inverse);

    // Init uniforms
    glUseProgram(renderer->color_pass.program);
    u32 loc = glGetUniformLocation(renderer->color_pass.program, "projection");
    glUniformMatrix4fv(loc, 1, GL_FALSE, renderer->camera_proj.v);

    glUseProgram(renderer->vol_pass.program);
    loc = glGetUniformLocation(renderer->vol_pass.program, "proj_inverse");
    glUniformMatrix4fv(loc, 1, GL_FALSE, renderer->camera_proj_inverse.v);

    glUseProgram(renderer->ui_program);
    const Mat4 ortho = mat4_ortho_gl(0, renderer->height, 0, renderer->width, -1, 1);
    glUniformMatrix4fv(glGetUniformLocation(renderer->ui_program, "proj"), 1, GL_FALSE, ortho.v);
}

Renderer *RendererCreate(PlatformWindow *window) {
    Renderer *renderer = sCalloc(1, sizeof(Renderer));

    renderer->window = window;
    PlatformCreateorUpdateOpenGLContext(renderer, window);
    GLLoadFunctions();

    // Global init
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(GLMessageCallback, 0);
    glDisable(GL_CULL_FACE);

    // Meshes
    renderer->mesh_count = 0;
    renderer->mesh_capacity = 8;
    renderer->meshes = sCalloc(renderer->mesh_capacity, sizeof(Mesh));

    // Render passes
    CreateShadowmapRenderPass(&renderer->shadowmap_pass);
    CreateColorRenderPass(renderer->width, renderer->height, &renderer->color_pass);
    CreateVolumetricRenderPass(&renderer->vol_pass);

    // Screen quad
    glGenVertexArrays(1, &renderer->screen_quad);
    glBindVertexArray(renderer->screen_quad);
    glGenBuffers(1, &renderer->screen_quad_vbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->screen_quad_vbuffer);
    const f32 quad[] = {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)(2 * sizeof(f32)));
    glEnableVertexAttribArray(1);

    { // UI
        // Init push buffer
        renderer->ui_push_buffer.size = 0;
        renderer->ui_push_buffer.max_size = sizeof(PushBufferEntryText) * 256;
        renderer->ui_push_buffer.buf = sCalloc(renderer->ui_push_buffer.max_size, 1);

        // Load font
        unsigned char *ttf_buffer = sCalloc(1 << 20, sizeof(unsigned char));
        FILE *f = fopen("resources/font/LiberationMono-Regular.ttf", "rb");
        ASSERT(f);
        fread(ttf_buffer, 1, 1 << 20, f);
        fclose(f);

        unsigned char *temp_bmp = sCalloc(512 * 512, sizeof(unsigned char));
        renderer->char_data = sCalloc(96, sizeof(stbtt_bakedchar));

        stbtt_BakeFontBitmap(ttf_buffer, 0, 20.0f, temp_bmp, 512, 512, 32, 96, renderer->char_data);

        sFree(ttf_buffer);
        glGenTextures(1, &renderer->glyphs_texture);
        glBindTexture(GL_TEXTURE_2D, renderer->glyphs_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 512, 512, 0, GL_RED, GL_UNSIGNED_BYTE, temp_bmp);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        sFree(temp_bmp);

        // Init glyph array
        glGenVertexArrays(1, &renderer->ui_vertex_array);
        glBindVertexArray(renderer->ui_vertex_array);
        glGenBuffers(1, &renderer->ui_vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, renderer->ui_vertex_buffer);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), 0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)(2 * sizeof(f32)));
        glEnableVertexAttribArray(1);

        // Load ui shader
        renderer->ui_program = CreateProgram("ui");

        // Load white texture
        glGenTextures(1, &renderer->white_texture);
        glBindTexture(GL_TEXTURE_2D, renderer->white_texture);
        const u8 white[] = {
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 4, 4, 0, GL_RED, GL_UNSIGNED_BYTE, &white);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    { // Scene
        renderer->scene_pushbuffer.size = 0;
        renderer->scene_pushbuffer.max_size = sizeof(PushBufferEntryMesh) * 70;
        renderer->scene_pushbuffer.buf = sCalloc(renderer->scene_pushbuffer.max_size, 1);
    }
    UpdateCameraProj(renderer);

    return renderer;
}

void RendererDestroy(Renderer *renderer) {
    // UI
    sFree(renderer->ui_push_buffer.buf);
    sFree(renderer->scene_pushbuffer.buf);

    sFree(renderer->char_data);
    glDeleteTextures(1, &renderer->white_texture);
    glDeleteTextures(1, &renderer->glyphs_texture);

    // Render passes
    DestroyShadowmapRenderPass(&renderer->shadowmap_pass);
    DestroyColorRenderPass(&renderer->color_pass);
    DestroyVolumetricRenderPass(&renderer->vol_pass);

    // Screen quad
    glDeleteVertexArrays(1, &renderer->screen_quad_vbuffer);
    glDeleteBuffers(1, &renderer->screen_quad);

    // Meshes
    for(u32 i = 0; i < renderer->mesh_count; i++) {
        RendererDestroyMesh(&renderer->meshes[i]);
    }
    sFree(renderer->meshes);

    sFree(renderer);
}

// -------------
// Drawing

internal void DrawScene(Renderer *renderer, u32 program) {
    // Draw bike
    glActiveTexture(GL_TEXTURE1);

    PushBuffer *pushb = &renderer->scene_pushbuffer;
    if(pushb->size == 0)
        return;

    for(u32 address = 0; address < pushb->size;) {
        PushBufferEntryType *type = (PushBufferEntryType *)(pushb->buf + address);

        ASSERT(*type == PushBufferEntryType_Mesh); // Only meshes for now
        PushBufferEntryMesh *entry = (PushBufferEntryMesh *)(pushb->buf + address);

        Mesh *mesh = &renderer->meshes[entry->mesh_handle];

        glBindTexture(GL_TEXTURE_2D, mesh->diffuse_texture);
        glBindVertexArray(mesh->vertex_array);
        glUniformMatrix4fv(glGetUniformLocation(program, "transform"), 1, GL_FALSE, entry->transform->v);
        glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);

        address += sizeof(PushBufferEntryMesh);
    }
}

internal void DrawScreenQuad(Renderer *renderer) {
    glBindVertexArray(renderer->screen_quad);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

internal void DrawUI(Renderer *renderer, PushBuffer *push_buffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(0);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(renderer->ui_program);
    glActiveTexture(GL_TEXTURE0);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->ui_vertex_buffer);
    glBindVertexArray(renderer->ui_vertex_array);
    const u32 color_uniform = glGetUniformLocation(renderer->ui_program, "color");

    for(u32 address = 0; address < push_buffer->size;) {
        PushBufferEntryType *type = (PushBufferEntryType *)(push_buffer->buf + address);

        switch(*type) {
        case PushBufferEntryType_Quad: {
            PushBufferEntryQuad *entry = (PushBufferEntryQuad *)(push_buffer->buf + address);

            const f32 vtx[] = {
                entry->l,
                entry->t,
                0.0f,
                1.0f, // UL
                entry->r,
                entry->t,
                1.0f,
                1.0f, // UR
                entry->l,
                entry->b,
                0.0f,
                0.0f, // LL
                entry->r,
                entry->b,
                1.0f,
                0.0f, // LR
            };
            glUniform4f(
                color_uniform, entry->colour.x, entry->colour.y, entry->colour.z, entry->colour.w);
            glBindTexture(GL_TEXTURE_2D, renderer->white_texture);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vtx), &vtx, GL_DYNAMIC_DRAW);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            address += sizeof(PushBufferEntryQuad);
            continue;
        }
        case PushBufferEntryType_Text: {
            glBindTexture(GL_TEXTURE_2D, renderer->glyphs_texture);

            PushBufferEntryText *entry = (PushBufferEntryText *)(push_buffer->buf + address);
            const char *txt = entry->text;
            f32 x = (f32)entry->x;
            f32 y = (f32)entry->y;

            while(*txt) {
                if(*txt >= 32 && *txt <= 127) {
                    stbtt_aligned_quad q;
                    stbtt_GetBakedQuad(renderer->char_data, 512, 512, *txt - 32, &x, &y, &q, 1);

                    f32 vtx[] = {
                        q.x0,
                        q.y0,
                        q.s0,
                        q.t0, // 0, 0
                        q.x1,
                        q.y0,
                        q.s1,
                        q.t0, // 1, 0
                        q.x0,
                        q.y1,
                        q.s0,
                        q.t1, // 0, 1
                        q.x1,
                        q.y1,
                        q.s1,
                        q.t1, // 1, 1
                    };

                    glUniform4f(color_uniform,
                                entry->colour.x,
                                entry->colour.y,
                                entry->colour.z,
                                entry->colour.w);
                    glBufferData(GL_ARRAY_BUFFER, sizeof(vtx), vtx, GL_DYNAMIC_DRAW);
                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                }
                txt++;
            }

            address += sizeof(PushBufferEntryText);
            continue;
        } break;
        default: {
            ASSERT(0);
        }
        }
    }
}

void RendererDrawFrame(Renderer *renderer) {
    // ------------------
    // Shadow map
    BeginShadowmapRenderPass(&renderer->shadowmap_pass);
    DrawScene(renderer, renderer->shadowmap_pass.program);

    // ------------------
    // Color pass
    BeginColorRenderPass(renderer, &renderer->color_pass);
    DrawScene(renderer, renderer->color_pass.program);

    // ---------------
    // Post process
    BeginVolumetricRenderPass(renderer, &renderer->vol_pass);
    DrawScreenQuad(renderer);

    DrawUI(renderer, &renderer->ui_push_buffer);

    PlatformSwapBuffers(renderer);

    // Clear pushbuffers
    renderer->scene_pushbuffer.size = 0; // Maybe we don't need to reset it each frame? do some tests
    renderer->ui_push_buffer.size = 0;
}

void RendererUpdateWindow(Renderer *renderer, PlatformWindow *window) {
    sLog("Update window!");
    PlatformGetWindowSize(window, &renderer->width, &renderer->height);

    DestroyColorRenderPass(&renderer->color_pass);
    CreateColorRenderPass(renderer->width, renderer->height, &renderer->color_pass);
    UpdateCameraProj(renderer);

    glUseProgram(renderer->color_pass.program);
    u32 loc = glGetUniformLocation(renderer->color_pass.program, "view");
    glUniformMatrix4fv(loc, 1, GL_FALSE, renderer->camera_view.v);
    loc = glGetUniformLocation(renderer->color_pass.program, "light_dir");
    glUniform3f(loc, renderer->light_dir.x, renderer->light_dir.y, renderer->light_dir.z);
    loc = glGetUniformLocation(renderer->color_pass.program, "light_matrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, renderer->light_matrix.v);
}

void RendererSetCamera(Renderer *renderer, const Mat4 *view) {
    renderer->camera_pos = mat4_get_translation(view);
    renderer->camera_view = *view;
    mat4_inverse(&renderer->camera_view, &renderer->camera_view_inverse);

    // Update uniforms

    // Color pass
    glUseProgram(renderer->color_pass.program);
    u32 loc = glGetUniformLocation(renderer->color_pass.program, "view");
    glUniformMatrix4fv(loc, 1, GL_FALSE, renderer->camera_view.v);

    // Volumetric pass
    glUseProgram(renderer->vol_pass.program);
    loc = glGetUniformLocation(renderer->vol_pass.program, "view_inverse");
    glUniformMatrix4fv(loc, 1, GL_FALSE, renderer->camera_view_inverse.v);
    loc = glGetUniformLocation(renderer->vol_pass.program, "view_pos");
    glUniform3f(loc, renderer->camera_pos.x, renderer->camera_pos.y, renderer->camera_pos.z);
}

void RendererSetSunDirection(Renderer *renderer, const Vec3 direction) {
    const Mat4 ortho = mat4_ortho_zoom(1.0f / 1.0f, 100.0f, -600.0f, 600.0f);
    Mat4 look = mat4_look_at(
        (Vec3){0.0f, 0.0f, 0.0f}, vec3_fmul(direction, -1.0f), (Vec3){0.0f, 1.0f, 0.0f});
    renderer->light_matrix = mat4_mul(&ortho, &look);
    renderer->light_dir = direction;

    // Update uniforms
    // Shadowmap
    glUseProgram(renderer->shadowmap_pass.program);
    u32 loc = glGetUniformLocation(renderer->shadowmap_pass.program, "light_matrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, renderer->light_matrix.v);

    // Color pass
    glUseProgram(renderer->color_pass.program);
    loc = glGetUniformLocation(renderer->color_pass.program, "light_dir");
    glUniform3f(loc, renderer->light_dir.x, renderer->light_dir.y, renderer->light_dir.z);
    loc = glGetUniformLocation(renderer->color_pass.program, "light_matrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, renderer->light_matrix.v);

    // Volumetric
    glUseProgram(renderer->vol_pass.program);
    loc = glGetUniformLocation(renderer->vol_pass.program, "light_dir");
    glUniform3f(loc, renderer->light_dir.x, renderer->light_dir.y, renderer->light_dir.z);
    loc = glGetUniformLocation(renderer->vol_pass.program, "light_matrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, renderer->light_matrix.v);
}

PushBuffer *RendererGetUIPushBuffer(Renderer *renderer) {
    return &renderer->ui_push_buffer;
}

PushBuffer *RendererGetScenePushBuffer(Renderer *renderer) {
    return &renderer->scene_pushbuffer;
}