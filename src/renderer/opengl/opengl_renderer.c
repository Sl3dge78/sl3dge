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
        sError("Unable to compile shader:\n%s", log);
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

    // SHADOWMAP
    {
        renderer->shadowmap_program = glCreateProgram();
        char *vtx_code = PlatformReadWholeFile("resources/shaders/gl/shadow.vert");
        u32 vtx_shader;
        GLCreateAndCompileShader(GL_VERTEX_SHADER, vtx_code, &vtx_shader);
        sFree(vtx_code);
        glAttachShader(renderer->shadowmap_program, vtx_shader);

        char *frag_code = PlatformReadWholeFile("resources/shaders/gl/shadow.frag");
        u32 frag_shader;
        GLCreateAndCompileShader(GL_FRAGMENT_SHADER, frag_code, &frag_shader);
        sFree(frag_code);
        glAttachShader(renderer->shadowmap_program, frag_shader);

        glLinkProgram(renderer->shadowmap_program);
        glObjectLabel(GL_PROGRAM, renderer->shadowmap_program, -1, "Shadowmap Program");
        i32 result;
        glGetProgramiv(renderer->shadowmap_program, GL_LINK_STATUS, &result);
        ASSERT(result == GL_TRUE);

        glDeleteShader(vtx_shader);
        glDeleteShader(frag_shader);

        glGenTextures(1, &renderer->shadowmap_texture);
        glBindTexture(GL_TEXTURE_2D, renderer->shadowmap_texture);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_DEPTH_COMPONENT,
                     2048,
                     2048,
                     0,
                     GL_DEPTH_COMPONENT,
                     GL_FLOAT,
                     NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

        glGenFramebuffers(1, &renderer->shadowmap_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->shadowmap_framebuffer);
        glObjectLabel(GL_FRAMEBUFFER, renderer->shadowmap_framebuffer, -1, "Shadowmap framebuffer");
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, renderer->shadowmap_texture, 0);
    }
    // MAIN
    {
        char *vtx_shader = PlatformReadWholeFile("resources/shaders/gl/main.vert");
        u32 vertex_shader;
        GLCreateAndCompileShader(GL_VERTEX_SHADER, vtx_shader, &vertex_shader);
        sFree(vtx_shader);

        char *frag_shader = PlatformReadWholeFile("resources/shaders/gl/main.frag");
        u32 fragment_shader;
        GLCreateAndCompileShader(GL_FRAGMENT_SHADER, frag_shader, &fragment_shader);
        sFree(frag_shader);

        renderer->main_program = glCreateProgram();
        glAttachShader(renderer->main_program, vertex_shader);
        glAttachShader(renderer->main_program, fragment_shader);
        glLinkProgram(renderer->main_program);
        glObjectLabel(GL_PROGRAM, renderer->main_program, -1, "MAIN SHADER");
        glUseProgram(renderer->main_program);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        glUniform1i(glGetUniformLocation(renderer->main_program, "shadow_map"), 0);
        glUniform1i(glGetUniformLocation(renderer->main_program, "diffuse"), 1);

        glGenFramebuffers(1, &renderer->main_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->main_framebuffer);
        glObjectLabel(GL_FRAMEBUFFER, renderer->main_framebuffer, -1, "Color framebuffer");
        // Color attachement
        glGenTextures(1, &renderer->main_render_target);
        glBindTexture(GL_TEXTURE_2D, renderer->main_render_target);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGB,
                     renderer->width,
                     renderer->height,
                     0,
                     GL_RGB,
                     GL_UNSIGNED_BYTE,
                     NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->main_render_target, 0);

        glGenTextures(1, &renderer->main_depthmap);
        glBindTexture(GL_TEXTURE_2D, renderer->main_depthmap);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_DEPTH_COMPONENT,
                     renderer->width,
                     renderer->height,
                     0,
                     GL_DEPTH_COMPONENT,
                     GL_FLOAT,
                     NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, renderer->main_depthmap, 0);
        /*
        glGenRenderbuffers(1, &renderer->main_renderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, renderer->main_renderbuffer);
        glRenderbufferStorage(
            GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, renderer->width, renderer->height);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                  GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER,
                                  renderer->main_renderbuffer);
        */
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        renderer->meshes = sCalloc(1, sizeof(Mesh *));
        RendererLoadMesh(renderer, "resources/3d/Motorcycle/motorcycle.gltf", &renderer->moto);
    }
    // ---------------
    // POST PROCESS
    {
        glGenVertexArrays(1, &renderer->screen_quad);
        glBindVertexArray(renderer->screen_quad);
        glGenBuffers(1, &renderer->screen_quad_vbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, renderer->screen_quad_vbuffer);
        f32 quad[] = {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f,
                      -1.0f, 1.0f,  0.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f};

        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), 0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)(2 * sizeof(f32)));
        glEnableVertexAttribArray(1);

        char *pp_vshader = PlatformReadWholeFile("resources/shaders/gl/post.vert");
        u32 vertex_shader;
        GLCreateAndCompileShader(GL_VERTEX_SHADER, pp_vshader, &vertex_shader);
        sFree(pp_vshader);

        char *pp_fshader = PlatformReadWholeFile("resources/shaders/gl/post.frag");
        u32 fragment_shader;
        GLCreateAndCompileShader(GL_FRAGMENT_SHADER, pp_fshader, &fragment_shader);
        sFree(pp_fshader);

        renderer->postprocess_program = glCreateProgram();
        glAttachShader(renderer->postprocess_program, vertex_shader);
        glAttachShader(renderer->postprocess_program, fragment_shader);
        glLinkProgram(renderer->postprocess_program);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        glUseProgram(renderer->postprocess_program);
        glUniform1i(glGetUniformLocation(renderer->postprocess_program, "shadow_map"), 0);
        glUniform1i(glGetUniformLocation(renderer->postprocess_program, "depth_map"), 1);
        glUniform1i(glGetUniformLocation(renderer->postprocess_program, "screen_texture"), 2);
    }

    glUseProgram(renderer->main_program);
    renderer->camera_proj = mat4_perspective_gl(90.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);
    mat4_inverse(&renderer->camera_proj, &renderer->camera_proj_inverse);

    glUniformMatrix4fv(glGetUniformLocation(renderer->main_program, "projection"),
                       1,
                       GL_FALSE,
                       renderer->camera_proj.v);
    glUseProgram(renderer->postprocess_program);
    glUniformMatrix4fv(glGetUniformLocation(renderer->postprocess_program, "proj_inverse"),
                       1,
                       GL_FALSE,
                       renderer->camera_proj_inverse.v);
    glUniformMatrix4fv(glGetUniformLocation(renderer->postprocess_program, "proj"),
                       1,
                       GL_FALSE,
                       renderer->camera_proj.v);

    glDisable(GL_CULL_FACE);

    return renderer;
}

void RendererDrawScene(Renderer *renderer) {
    // Draw bike
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->moto.diffuse_texture);
    glBindVertexArray(renderer->moto.vertex_array);
    glDrawElements(GL_TRIANGLES, renderer->moto.index_count, GL_UNSIGNED_INT, 0);
}

void RendererDrawFrame(Renderer *renderer) {
    //------------------
    // Shadow map
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 2048, 2048);
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->shadowmap_framebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(renderer->shadowmap_program);
    RendererDrawScene(renderer);

    //------------------
    // Color pass
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, renderer->width, renderer->height);

    glBindFramebuffer(GL_FRAMEBUFFER, renderer->main_framebuffer);
    glClearColor(0.43f, 0.77f, 0.91f, 0.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glUseProgram(renderer->main_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->shadowmap_texture);

    u32 view_loc = glGetUniformLocation(renderer->main_program, "view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, renderer->camera_view.v);
    RendererDrawScene(renderer);

    //---------------
    // Post process
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(renderer->postprocess_program);
    glUniformMatrix4fv(glGetUniformLocation(renderer->postprocess_program, "cam_view"),
                       1,
                       GL_FALSE,
                       renderer->camera_view.v);
    glUniformMatrix4fv(glGetUniformLocation(renderer->postprocess_program, "view_inverse"),
                       1,
                       GL_FALSE,
                       renderer->camera_view_inverse.v);
    glUniform3f(glGetUniformLocation(renderer->postprocess_program, "view_pos"),
                renderer->camera_pos.x,
                renderer->camera_pos.y,
                renderer->camera_pos.z);
    glUniform3f(glGetUniformLocation(renderer->postprocess_program, "light_dir"),
                renderer->light_dir.x,
                renderer->light_dir.y,
                renderer->light_dir.z);

    glBindVertexArray(renderer->screen_quad);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->shadowmap_texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->main_depthmap);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, renderer->main_render_target);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    PlatformSwapBuffers(renderer);
}

void RendererDestroy(Renderer *renderer) {
    glDeleteFramebuffers(1, &renderer->main_framebuffer);
    RendererDestroyMesh(renderer->meshes[0]);

    sFree(renderer->meshes);
    glDeleteProgram(renderer->main_program);
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
    const Mat4 ortho = mat4_ortho_zoom(1.0f / 1.0f, 100.0f, -600.0f, 600.0f);
    Mat4 look = mat4_look_at(
        (Vec3){0.0f, 0.0f, 0.0f}, vec3_fmul(direction, -1.0f), (Vec3){0.0f, 1.0f, 0.0f});
    renderer->light_matrix = mat4_mul(&ortho, &look);
    renderer->light_dir = direction;

    glUseProgram(renderer->shadowmap_program);
    u32 mat_loc = glGetUniformLocation(renderer->shadowmap_program, "light_matrix");
    glUniformMatrix4fv(mat_loc, 1, GL_FALSE, renderer->light_matrix.v);

    glUseProgram(renderer->main_program);
    u32 light_loc = glGetUniformLocation(renderer->main_program, "light_dir");
    glUniform3f(light_loc, renderer->light_dir.x, renderer->light_dir.y, renderer->light_dir.z);

    mat_loc = glGetUniformLocation(renderer->main_program, "light_matrix");
    glUniformMatrix4fv(mat_loc, 1, GL_FALSE, renderer->light_matrix.v);

    glUseProgram(renderer->postprocess_program);
    mat_loc = glGetUniformLocation(renderer->postprocess_program, "light_matrix");
    glUniformMatrix4fv(mat_loc, 1, GL_FALSE, renderer->light_matrix.v);
}
