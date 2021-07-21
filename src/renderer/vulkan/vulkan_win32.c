// Surface creation
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>

#include "platform/platform.h"
#include "platform/platform_win32.h"

void PlatformGetInstanceExtensions(u32 *count, const char **extensions) {
    *count = 2;

    if(extensions != NULL) {
        extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME;
        extensions[1] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
    }
}

void PlatformCreateVkSurface(VkInstance instance, PlatformWindow *window, VkSurfaceKHR *surface) {
    VkWin32SurfaceCreateInfoKHR ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    ci.pNext = NULL;
    ci.flags = 0;
    ci.hinstance = window->hinstance;
    ci.hwnd = window->hwnd;
    vkCreateWin32SurfaceKHR(instance, &ci, NULL, surface);
}