#include <sl3dge-utils/sl3dge.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/GL.h>
#include <GL/glext.h>
#include <GL/wglext.h>

#include "platform/platform_win32.h"
#include "renderer/opengl/opengl_renderer.h"
#include "renderer/opengl/opengl_functions.h"

#define LOAD_GL_FUNC(type, name) name = (type)wglGetProcAddress(#name)

void GLLoadFunctions() {
    LOAD_GL_FUNC(PFNGLGENBUFFERSPROC, glGenBuffers);
    LOAD_GL_FUNC(PFNGLBINDBUFFERPROC, glBindBuffer);
    LOAD_GL_FUNC(PFNGLBUFFERDATAPROC, glBufferData);
    LOAD_GL_FUNC(PFNGLCREATESHADERPROC, glCreateShader);
    LOAD_GL_FUNC(PFNGLSHADERSOURCEPROC, glShaderSource);
    LOAD_GL_FUNC(PFNGLCOMPILESHADERPROC, glCompileShader);
    LOAD_GL_FUNC(PFNGLGETSHADERIVPROC, glGetShaderiv);
    LOAD_GL_FUNC(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
    LOAD_GL_FUNC(PFNGLATTACHSHADERPROC, glAttachShader);
    LOAD_GL_FUNC(PFNGLLINKPROGRAMPROC, glLinkProgram);
    LOAD_GL_FUNC(PFNGLCREATEPROGRAMPROC, glCreateProgram);
    LOAD_GL_FUNC(PFNGLDELETESHADERPROC, glDeleteShader);
    LOAD_GL_FUNC(PFNGLUSEPROGRAMPROC, glUseProgram);
    LOAD_GL_FUNC(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
    LOAD_GL_FUNC(PFNGLDELETEPROGRAMPROC, glDeleteProgram);
    LOAD_GL_FUNC(PFNGLDELETEBUFFERSPROC, glDeleteBuffers);
    LOAD_GL_FUNC(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
    LOAD_GL_FUNC(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
    LOAD_GL_FUNC(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
    LOAD_GL_FUNC(PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallback);
    LOAD_GL_FUNC(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
    LOAD_GL_FUNC(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);
    LOAD_GL_FUNC(PFNGLOBJECTLABELPROC, glObjectLabel);
    LOAD_GL_FUNC(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers);
    LOAD_GL_FUNC(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer);
    LOAD_GL_FUNC(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D);
    LOAD_GL_FUNC(PFNGLGENRENDERBUFFERSPROC, glGenRenderbuffers);
    LOAD_GL_FUNC(PFNGLBINDRENDERBUFFERPROC, glBindRenderbuffer);
    LOAD_GL_FUNC(PFNGLRENDERBUFFERSTORAGEPROC, glRenderbufferStorage);
    LOAD_GL_FUNC(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glFramebufferRenderbuffer);
    LOAD_GL_FUNC(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers);
    LOAD_GL_FUNC(PFNGLUNIFORM3FPROC, glUniform3f);
    LOAD_GL_FUNC(PFNGLACTIVETEXTUREPROC, glActiveTexture);
    LOAD_GL_FUNC(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
    LOAD_GL_FUNC(PFNGLUNIFORM1IPROC, glUniform1i);
    LOAD_GL_FUNC(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays);
    LOAD_GL_FUNC(PFNGLUNIFORM4FPROC, glUniform4f);
}

void PlatformCreateorUpdateOpenGLContext(Renderer *renderer, PlatformWindow *window) {
    if(window->opengl_rc != 0) {
        wglDeleteContext(window->opengl_rc);
        window->opengl_rc = 0;
    }

    PIXELFORMATDESCRIPTOR desired_pixel_fmt = {0};
    desired_pixel_fmt.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    desired_pixel_fmt.nVersion = 1;
    desired_pixel_fmt.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
    desired_pixel_fmt.cColorBits = 32;
    desired_pixel_fmt.cAlphaBits = 8;
    desired_pixel_fmt.iLayerType = PFD_MAIN_PLANE;

    i32 suggested_pixel_fmt_id = ChoosePixelFormat(window->dc, &desired_pixel_fmt);
    PIXELFORMATDESCRIPTOR suggested_pixel_fmt;
    DescribePixelFormat(
        window->dc, suggested_pixel_fmt_id, sizeof(PIXELFORMATDESCRIPTOR), &suggested_pixel_fmt);
    SetPixelFormat(window->dc, suggested_pixel_fmt_id, &suggested_pixel_fmt);

    HGLRC temp_rc = wglCreateContext(window->dc);
    if(!wglMakeCurrent(window->dc, temp_rc)) {
        DWORD error = GetLastError();
        sError("Unable to init opengl %d", error);
    }

    i32 version_major = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &version_major);
    i32 version_minor = 0;
    glGetIntegerv(GL_MINOR_VERSION, &version_minor);
    sLog("OpenGL V%d.%d", version_major, version_minor);

    i32 attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB,
                     version_major,
                     WGL_CONTEXT_MINOR_VERSION_ARB,
                     version_minor,
                     WGL_CONTEXT_FLAGS_ARB,
                     WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
                     WGL_CONTEXT_PROFILE_MASK_ARB,
                     WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                     0};
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;
    wglCreateContextAttribsARB =
        (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    window->opengl_rc = wglCreateContextAttribsARB(window->dc, 0, attribs);
    if(!wglMakeCurrent(window->dc, window->opengl_rc)) {
        DWORD error = GetLastError();
        sError("Unable to init opengl %d", error);
    }
    wglDeleteContext(temp_rc);
}

void PlatformSwapBuffers(Renderer *renderer) {
    SwapBuffers(renderer->window->dc);
}