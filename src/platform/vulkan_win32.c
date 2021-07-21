// Surface creation
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>

#include "platform/platform.h"
#include "platform/platform_win32.h"

void PlatformCreateVkSurface(VkInstance instance, PlatformWindow *window, VkSurfaceKHR *surface) {
    VkWin32SurfaceCreateInfoKHR ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    ci.pNext = NULL;
    ci.flags = 0;
    ci.hinstance = window->hinstance;
    ci.hwnd = window->hwnd;
    vkCreateWin32SurfaceKHR(instance, &ci, NULL, surface);
}