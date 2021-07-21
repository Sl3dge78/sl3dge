#include <sl3dge-utils/sl3dge.h>

#include "platform/platform_win32.h"
#include "renderer/opengl/opengl_renderer.h"

void PlatformCreateOpenGLContext(Renderer *renderer, PlatformWindow *window) {
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

    HGLRC opengl_rc = wglCreateContext(window->dc);
    if(!wglMakeCurrent(window->dc, opengl_rc)) {
        DWORD error = GetLastError();
        sError("Unable to init opengl %d", error);
    }

    renderer->width = window->w;
    renderer->height = window->h;
}

void PlatformSwapBuffers(Renderer *renderer) {
    SwapBuffers(renderer->window->dc);
}