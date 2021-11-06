//#pragma once
#include "utils/sl3dge.h"

typedef struct Renderer Renderer;

typedef u32 GetRendererSize_t();
GetRendererSize_t *pfn_GetRendererSize;

typedef void RendererInit_t(Renderer *renderer, PlatformAPI *platform_api, PlatformWindow *window);
RendererInit_t *pfn_RendererInit;

typedef void RendererDestroyBackend_t(Renderer *renderer);
RendererDestroyBackend_t *pfn_RendererDestroyBackend;

typedef void RendererInitBackend_t(Renderer *renderer, PlatformAPI *platform_api);
RendererInitBackend_t *pfn_RendererInitBackend;

typedef void RendererDrawFrame_t(Renderer *renderer);
RendererDrawFrame_t *pfn_RendererDrawFrame;

typedef void RendererDestroy_t(Renderer *renderer);
RendererDestroy_t *pfn_RendererDestroy;

typedef void RendererUpdateWindow_t(Renderer *renderer, PlatformAPI *platform_api, const u32 width, const u32 height);
RendererUpdateWindow_t *pfn_RendererUpdateWindow;