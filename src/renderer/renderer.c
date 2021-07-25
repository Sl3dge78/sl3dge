#include <sl3dge-utils/sl3dge.h>
#include <cgltf/cgltf.h>

#include "renderer/renderer.h"

#ifdef RENDERER_VULKAN
#include "renderer/vulkan/vulkan_renderer.c"
#elif RENDERER_OPENGL
#include "renderer/opengl/opengl_renderer.c"
#endif
