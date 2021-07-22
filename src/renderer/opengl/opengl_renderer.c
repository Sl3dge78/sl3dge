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

#include "renderer/renderer.h"
#include "renderer/opengl/opengl_renderer.h"

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

Renderer *RendererCreate(PlatformWindow *window) {
    Renderer *renderer = sMalloc(sizeof(Renderer));
    renderer->window = window;
    PlatformCreateOpenGLContext(renderer, window);

    i32 version_major = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &version_major);
    i32 version_minor = 0;
    glGetIntegerv(GL_MINOR_VERSION, &version_minor);
    sLog("OpenGL V%d.%d", version_major, version_minor);

    GLLoadFunctions();

    // Create buffer
    glGenBuffers(1, &renderer->vbo);
    glGenVertexArrays(1, &renderer->vao);

    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    const f32 vertices[] = {-0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f};
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), 0);
    glEnableVertexAttribArray(0);

    // Shader
    const char *vtx_shader =
        "#version 330 core\nlayout(location = 0) in vec3 aPos;\nvoid main() { gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0); }\0";
    GLCreateAndCompileShader(GL_VERTEX_SHADER, vtx_shader, &renderer->vertex_shader);

    const char *frag_shader =
        "#version 330 core\nout vec4 FragColor;\nvoid main() { FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f); }\0";
    GLCreateAndCompileShader(GL_FRAGMENT_SHADER, frag_shader, &renderer->fragment_shader);

    renderer->shader_program = glCreateProgram();
    glAttachShader(renderer->shader_program, renderer->vertex_shader);
    glAttachShader(renderer->shader_program, renderer->fragment_shader);
    glLinkProgram(renderer->shader_program);

    glDeleteShader(renderer->vertex_shader);
    glDeleteShader(renderer->fragment_shader);

    return renderer;
}

void RendererDrawFrame(Renderer *renderer) {
    glViewport(0, 0, renderer->width, renderer->height);

    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(renderer->shader_program);
    glBindVertexArray(renderer->vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    PlatformSwapBuffers(renderer);
}

void RendererDestroy(Renderer *renderer) {
    glDeleteProgram(renderer->shader_program);
    glDeleteBuffers(1, &renderer->vbo);
    sFree(renderer);
}

void RendererUpdateWindow(Renderer *renderer, PlatformWindow *window) {
}

u32 RendererLoadMesh(Renderer *renderer, const char *path) {
    return 0;
}

void RendererDestroyMesh(Renderer *renderer, u32 mesh) {
}

MeshInstance RendererInstantiateMesh(Renderer *renderer, u32 mesh_id) {
    MeshInstance result = {0};
    return result;
}

void RendererSetCamera(Renderer *renderer, const Vec3 position, const Vec3 forward, const Vec3 up) {
}

void RendererSetSunDirection(Renderer *renderer, const Vec3 direction) {
}
