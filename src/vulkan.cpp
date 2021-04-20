#include <SDL/SDL_vulkan.h>

/*
 === TODO ===
 CRITICAL
 - Descriptor set 0 > could be the same for all pipelines

 MAJOR
  - Shadow Mapping

 BACKLOG
 - Mipmaps

 IMPROVEMENTS
- IBL
- Utiliser des Staging buffers
- Window Resize > Pipline dynamic states ?
 - Handle pipeline caches
 
- MSAA
*/

#define DECL_FUNC(name) global PFN_##name pfn_##name
#define LOAD_INSTANCE_FUNC(instance, name) pfn_##name = (PFN_##name)vkGetInstanceProcAddr(instance, #name); \
ASSERT(pfn_##name)
#define LOAD_DEVICE_FUNC(name) pfn_##name = (PFN_##name)vkGetDeviceProcAddr(device, #name); \
ASSERT(pfn_##name)

DECL_FUNC(vkCreateDebugUtilsMessengerEXT);
DECL_FUNC(vkDestroyDebugUtilsMessengerEXT);        
DECL_FUNC(vkSetDebugUtilsObjectNameEXT);          

DECL_FUNC(vkGetBufferDeviceAddressKHR);
DECL_FUNC(vkCreateRayTracingPipelinesKHR);         
DECL_FUNC(vkCmdTraceRaysKHR);                
DECL_FUNC(vkGetRayTracingShaderGroupHandlesKHR);
DECL_FUNC(vkCreateAccelerationStructureKHR);
DECL_FUNC(vkGetAccelerationStructureBuildSizesKHR);
DECL_FUNC(vkCmdBuildAccelerationStructuresKHR);
DECL_FUNC(vkDestroyAccelerationStructureKHR);
DECL_FUNC(vkGetAccelerationStructureDeviceAddressKHR);

internal void AssertVkResult(VkResult result) {
    if(result < 0) {
        switch (result) {
            case VK_ERROR_EXTENSION_NOT_PRESENT:
            SDL_LogError(0,"VK_ERROR_EXTENSION_NOT_PRESENT");
            break;
            case VK_ERROR_LAYER_NOT_PRESENT:
            SDL_LogError(0,"VK_ERROR_LAYER_NOT_PRESENT");
            break;
            case VK_ERROR_DEVICE_LOST:{
                SDL_LogError(0,"VK_ERROR_DEVICE_LOST");
            }break;
            case VK_ERROR_OUT_OF_POOL_MEMORY:{
                SDL_LogError(0,"VK_ERROR_OUT_OF_POOL_MEMORY");
            }break;
            case VK_ERROR_INITIALIZATION_FAILED :{
                SDL_LogError(0,"VK_ERROR_INITIALIZATION_FAILED");
            }break;
            case VK_ERROR_OUT_OF_DATE_KHR :{
                SDL_LogError(0,"VK_ERROR_OUT_OF_DATE_KHR");
            }break;
            default :
            SDL_LogError(0, "Unhandled Vulkan error : %d", result);
            break;
        }
        ASSERT(false);
        exit(result);
    }
}

#include "VulkanTypes.cpp"

typedef struct Swapchain {
    VkSwapchainKHR swapchain;
    u32 image_count;
    VkFormat format;
    VkExtent2D extent;
    
    VkImage* images;
    VkImageView* image_views;
    
    VkCommandBuffer* command_buffers;
    VkFence* fences;
    
    VkSemaphore* image_acquired_semaphore;
    VkSemaphore* render_complete_semaphore;
    u32 semaphore_id;
    
} Swapchain;

typedef struct VulkanLayout {
    VkPipelineLayout layout;
    
    VkDescriptorSetLayout set_layout;
    
    u32 descriptor_set_count;
    VkDescriptorSet descriptor_set;
    
    u32 push_constant_size;
    
} VulkanLayout;

typedef struct Scene {
    VkPipeline pipeline;
    VulkanLayout layout;
    
    Buffer vtx_buffer;
    Buffer idx_buffer;
    
    u32 total_vertex_count;
    u32 total_index_count;
    
    u32 nodes_count;
    mat4 *transforms;
    
    u32 total_primitives_count;
    Primitive *primitives;
    
    u32 materials_count;
    Buffer mat_buffer;
    
    u32 textures_count;
    Image *textures;
    
} Scene;

typedef struct VulkanContext {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkSurfaceKHR surface;
    
    VkPhysicalDeviceMemoryProperties memory_properties;
    VkPhysicalDeviceProperties physical_device_properties;
    
    VkSampleCountFlagBits msaa_level;
    
    VkDevice device;
    u32 graphics_queue_id;
    VkQueue graphics_queue;
    u32 transfer_queue_id;
    VkQueue transfer_queue;
    u32 present_queue_id;
    VkQueue present_queue;
    
    VkCommandPool graphics_command_pool;
    VkDescriptorPool descriptor_pool;
    
    Swapchain swapchain;
    
    VkRenderPass render_pass;
    VkSampler texture_sampler;
    VkFramebuffer *framebuffers;
    
    Image depth_image;
    Image msaa_image;
    
    Buffer cam_buffer;
    
    Image shadowmap;
    VkExtent2D shadowmap_extent;
    VkSampler shadowmap_sampler;
    VkRenderPass shadowmap_render_pass;
    VulkanLayout shadowmap_layout;
    VkPipeline shadowmap_pipeline;
    VkFramebuffer shadowmap_framebuffer;
    
} VulkanContext;

typedef struct PushConstant {
    alignas(16) mat4 transform;
    alignas(4) u32 material;
    alignas(4) u32 color_texture;
} PushConstant;

global VkDebugUtilsMessengerEXT debug_messenger;

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT){
        SDL_LogInfo(0,pCallbackData->pMessage);
    }else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        SDL_LogWarn(0,pCallbackData->pMessage);
    } else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        SDL_LogError(0,pCallbackData->pMessage);
    }
    KEEP_CONSOLE_OPEN(true);
    return VK_FALSE;
}

internal void DEBUGPrintInstanceExtensions(){
    u32 property_count = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &property_count,NULL);
    
    VkExtensionProperties* extensions = (VkExtensionProperties*)calloc(property_count, sizeof(VkExtensionProperties));
    vkEnumerateInstanceExtensionProperties(NULL, &property_count,extensions);
    
    SDL_Log("Available extensions :");
    for(u32 i = 0; i < property_count; i++) {
        SDL_Log("%s", extensions[i].extensionName);
    }
}


internal void LoadDeviceFuncPointers(VkDevice device) {
    
    
    
}

internal void CreateVkShaderModule(const char *path, VkDevice device, VkShaderModule *shader_module) {
    i64 size = 0;
    u32 *code = Win32AllocateAndLoadBinary(path, &size);
    
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.codeSize = size;
    create_info.pCode = code;
    
    VkResult result = vkCreateShaderModule(device,&create_info,NULL,shader_module);
    free(code);
    AssertVkResult(result);
}


// ========================
//
// CONTEXT
//
// ========================

internal void CreateVkInstance(SDL_Window *window, VkInstance *instance) {
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Handmade";
    app_info.applicationVersion = 1;
    app_info.pEngineName = "Simple Lightweight 3D Game Engine";
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION_1_2;
    
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.pApplicationInfo = &app_info;
    
    const char* validation[] = {"VK_LAYER_KHRONOS_validation"};
    create_info.enabledLayerCount = 1;
    create_info.ppEnabledLayerNames = validation;
    
    // Our extensions
    u32 sl3_count = 1;
    const char* sl3_extensions [1] = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    
    // Get SDL extensions
    u32 sdl_count = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &sdl_count, NULL);
    const char** sdl_extensions = (const char**)calloc(sdl_count, sizeof(char *));
    SDL_Vulkan_GetInstanceExtensions(window, &sdl_count, sdl_extensions);
    
    u32 total_count = sdl_count + sl3_count;
    
    const char** all_extensions = (const char**)calloc(total_count, sizeof(char*));
    memcpy(all_extensions, sdl_extensions, sdl_count * sizeof(char*));
    memcpy(all_extensions + sdl_count, sl3_extensions, sl3_count * sizeof(char*));
    
    free(sdl_extensions);
    
    SDL_Log("Requested extensions");
    for(int i = 0; i < total_count; i++) {
        SDL_Log(all_extensions[i]);
    }
    
    create_info.enabledExtensionCount = sdl_count + sl3_count;
    create_info.ppEnabledExtensionNames = all_extensions;
    
    VkResult result = vkCreateInstance(&create_info, NULL, instance);
    
    AssertVkResult(result);
    
    free(all_extensions);
    
    {   // Create debug messenger
        // TODO : Do that only if we're in debug mode
        
        // Get the functions
        LOAD_INSTANCE_FUNC(*instance, vkCreateDebugUtilsMessengerEXT);
        LOAD_INSTANCE_FUNC(*instance, vkDestroyDebugUtilsMessengerEXT);
        LOAD_INSTANCE_FUNC(*instance, vkSetDebugUtilsObjectNameEXT);
        
        // Create the messenger
        VkDebugUtilsMessengerCreateInfoEXT debug_create_info  = {};
        debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.pNext = NULL;
        debug_create_info.flags = 0;
        debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT ;
        debug_create_info.pfnUserCallback = &DebugCallback;
        debug_create_info.pUserData = NULL;
        
        pfn_vkCreateDebugUtilsMessengerEXT(*instance, &debug_create_info, NULL, &debug_messenger);
    }
}

internal void CreateVkPhysicalDevice(VkInstance instance, VkPhysicalDevice *physical_device) {
    
    u32 device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    VkPhysicalDevice *physical_devices = (VkPhysicalDevice *)calloc(device_count, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &device_count, physical_devices);
    
    for(int i = 0; i < device_count; i ++) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
    }
    // TODO : Pick the device according to our specs
    *physical_device = physical_devices[0];
    free (physical_devices);
}

internal void GetQueuesId(VulkanContext *context) {
    
    // Get queue info
    u32 queue_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(context->physical_device, &queue_count, NULL);
    VkQueueFamilyProperties *queue_properties = (VkQueueFamilyProperties *)calloc(queue_count, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(context->physical_device, &queue_count, queue_properties);
    
    u32 set_flags = 0;
    
    for(int i = 0; i < queue_count && set_flags != 7 ; i ++) {
        if(!(set_flags & 1) && queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            context->graphics_queue_id = i;
            set_flags |= 1;
            //graphics_queue.count = queue_properties[i].queueCount; // TODO ??
        }
        
        if(!(set_flags & 2) && queue_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT && !(queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            context->transfer_queue_id = i;
            set_flags |= 2;
            //transfer_queue.count = queue_properties[i].queueCount; // TODO ??
        }
        
        if(!(set_flags & 4)) {
            VkBool32 is_supported = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(context->physical_device,i,context->surface,&is_supported);
            if(is_supported) {
                context->present_queue_id = i;
                set_flags |= 4;
            }
        }
    }
    free(queue_properties);
}

internal void CreateVkDevice(VkPhysicalDevice physical_device, const u32 graphics_queue, const u32 transfer_queue, const u32 present_queue, VkDevice *device) {
    
    // Queues
    VkDeviceQueueCreateInfo queues_ci[3] = {};
    
    float queue_priority = 1.0f;
    
    queues_ci[0] = {};
    queues_ci[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queues_ci[0].pNext = NULL;
    queues_ci[0].flags = 0;
    queues_ci[0].queueFamilyIndex = graphics_queue;
    queues_ci[0].queueCount = 1;
    queues_ci[0].pQueuePriorities = &queue_priority;
    
    queues_ci[1] = {};
    queues_ci[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queues_ci[1].pNext = NULL;
    queues_ci[1].flags = 0;
    queues_ci[1].queueFamilyIndex = transfer_queue;
    queues_ci[1].queueCount = 1;
    queues_ci[1].pQueuePriorities = &queue_priority;
    
    queues_ci[2] = {};
    queues_ci[2].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queues_ci[2].pNext = NULL;
    queues_ci[2].flags = 0;
    queues_ci[2].queueFamilyIndex = present_queue;
    queues_ci[2].queueCount = 1;
    queues_ci[2].pQueuePriorities = &queue_priority;
    
    // Extensions
    const u32 extension_count = 1;
    const char* extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    VkPhysicalDeviceRobustness2FeaturesEXT robustness = {};
    robustness.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
    robustness.pNext = NULL;
    robustness.robustBufferAccess2 = VK_FALSE;
    robustness.robustImageAccess2 = VK_FALSE;
    robustness.nullDescriptor = VK_TRUE;
    
    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing = {};
    descriptor_indexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptor_indexing.pNext = &robustness;
    descriptor_indexing.runtimeDescriptorArray = VK_TRUE;
    descriptor_indexing.descriptorBindingVariableDescriptorCount = VK_TRUE;
    descriptor_indexing.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    
    VkPhysicalDeviceFeatures2 features2 = {};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &descriptor_indexing;
    features2.features = {};
    features2.features.samplerAnisotropy = VK_TRUE;
    features2.features.shaderInt64 = VK_TRUE;
    
    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = &features2;
    device_create_info.flags = 0;
    device_create_info.queueCreateInfoCount = 2;
    device_create_info.pQueueCreateInfos = queues_ci;
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = 0;
    device_create_info.enabledExtensionCount = extension_count;
    device_create_info.ppEnabledExtensionNames = extensions;
    device_create_info.pEnabledFeatures = NULL;
    
    VkResult result = vkCreateDevice(physical_device, &device_create_info, NULL, device);
    AssertVkResult(result);
    
    LoadDeviceFuncPointers(*device);
}

internal void CreateSwapchain(const VulkanContext* context, SDL_Window* window, Swapchain *swapchain) {
    
    VkSwapchainCreateInfoKHR create_info = {};
    
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.surface = context->surface;
    
    // Img count
    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physical_device, context->surface, &surface_capabilities);
    u32 image_count = 3;
    if(surface_capabilities.maxImageCount != 0 && surface_capabilities.maxImageCount < image_count) {
        image_count = surface_capabilities.maxImageCount;
    }
    if(image_count < surface_capabilities.minImageCount) {
        image_count = surface_capabilities.minImageCount;
    }
    create_info.minImageCount = image_count;
    
    // Format
    u32 format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(context->physical_device, context->surface, &format_count, NULL);
    VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)calloc(format_count, sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(context->physical_device, context->surface, &format_count, formats);
    VkSurfaceFormatKHR picked_format = formats[0];
    for(u32 i = 0; i < format_count; ++i) {
        if(formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            picked_format = formats[i];
            SDL_Log("Swapchain format: VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR");
            break;
        }
    }
    create_info.imageFormat = picked_format.format;
    create_info.imageColorSpace = picked_format.colorSpace;
    swapchain->format = picked_format.format;
    free(formats);
    
    // Extent
    VkExtent2D extent = {};
    if(surface_capabilities.currentExtent.width != 0xFFFFFFFF) {
        extent = surface_capabilities.currentExtent;
    } else {
        int w, h;
        SDL_Vulkan_GetDrawableSize(window, &w, &h);
        
        extent.width = w;
        extent.height = h;
    }
    create_info.imageExtent = extent;
    swapchain->extent = extent;
    
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE ;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices = NULL; // NOTE: Needed only if sharing mode is shared
    create_info.preTransform = surface_capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    
    // Present mode
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; // NOTE: Default present mode
    u32 present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(context->physical_device, context->surface, &present_mode_count, NULL);
    VkPresentModeKHR *present_modes = (VkPresentModeKHR *)calloc(present_mode_count, sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(context->physical_device, context->surface, &present_mode_count, present_modes);
    for(u32 i = 0; i < present_mode_count;i++) {
        if(present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }
    free(present_modes);
    create_info.presentMode = present_mode;
    
    create_info.clipped = VK_FALSE;
    create_info.oldSwapchain = swapchain->swapchain;
    
    VkResult result = vkCreateSwapchainKHR(context->device, &create_info, NULL, &swapchain->swapchain);
    AssertVkResult(result);
    
    // Image views
    vkGetSwapchainImagesKHR(context->device, swapchain->swapchain, &swapchain->image_count, NULL);
    swapchain->images = (VkImage *)calloc(swapchain->image_count, sizeof(VkImage));
    vkGetSwapchainImagesKHR(context->device, swapchain->swapchain, &swapchain->image_count, swapchain->images);
    
    swapchain->image_views = (VkImageView*)calloc(swapchain->image_count, sizeof(VkImageView));
    
    for(u32 i = 0; i < swapchain->image_count; i++) {
        DEBUGNameObject(context->device, (u64)swapchain->images[i], VK_OBJECT_TYPE_IMAGE, "Swapchain image");
        
        VkImageViewCreateInfo image_view_ci = {};
        image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_ci.pNext = NULL;
        image_view_ci.flags = 0;
        image_view_ci.image = swapchain->images[i];
        image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D ;
        image_view_ci.format = swapchain->format;
        image_view_ci.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        image_view_ci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        
        AssertVkResult(vkCreateImageView(context->device, &image_view_ci, NULL, &swapchain->image_views[i]));
    }
    
    // Command buffers
    swapchain->command_buffers = (VkCommandBuffer *)calloc(swapchain->image_count, sizeof(VkCommandBuffer));
    VkCommandBufferAllocateInfo cmd_buf_ai = {};
    cmd_buf_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buf_ai.pNext = NULL;
    cmd_buf_ai.commandPool = context->graphics_command_pool;
    cmd_buf_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buf_ai.commandBufferCount = swapchain->image_count;
    result = vkAllocateCommandBuffers(context->device, &cmd_buf_ai, swapchain->command_buffers);
    AssertVkResult(result);
    
    // Fences
    swapchain->fences = (VkFence *)calloc(swapchain->image_count, sizeof(VkFence));
    VkFenceCreateInfo ci = {
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        NULL,
        VK_FENCE_CREATE_SIGNALED_BIT
    };
    
    for(u32 i = 0; i < swapchain->image_count; i++) {
        result = vkCreateFence(context->device, &ci, NULL, &swapchain->fences[i]);
        AssertVkResult(result);
    }
    
    // Semaphores
    swapchain->image_acquired_semaphore = (VkSemaphore *)calloc(swapchain->image_count, sizeof(VkSemaphore));
    swapchain->render_complete_semaphore = (VkSemaphore *)calloc(swapchain->image_count, sizeof(VkSemaphore));
    VkSemaphoreCreateInfo semaphore_ci = {};
    semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for(u32 i = 0; i < swapchain->image_count; i++) {
        vkCreateSemaphore(context->device, &semaphore_ci, NULL, &swapchain->image_acquired_semaphore[i]);
        vkCreateSemaphore(context->device, &semaphore_ci, NULL, &swapchain->render_complete_semaphore[i]);
    }
    swapchain->semaphore_id = 0;
    
    SDL_Log("Swapchain created with %d images", swapchain->image_count);
}

internal void DestroySwapchain(const VulkanContext* context, Swapchain *swapchain) {
    
    vkFreeCommandBuffers(context->device,context->graphics_command_pool,swapchain->image_count, swapchain->command_buffers);
    free(swapchain->command_buffers);
    
    free(swapchain->images);
    
    for(u32 i = 0; i < swapchain->image_count; i++) {
        vkDestroyFence(context->device, swapchain->fences[i], NULL);
        vkDestroySemaphore(context->device, swapchain->image_acquired_semaphore[i], NULL);
        vkDestroySemaphore(context->device, swapchain->render_complete_semaphore[i], NULL);
        vkDestroyImageView(context->device, swapchain->image_views[i], NULL);
    }
    free(swapchain->image_views);
    free(swapchain->fences);
    free(swapchain->image_acquired_semaphore);
    free(swapchain->render_complete_semaphore);
}

internal void CreateRenderPass(const VkDevice device, const Swapchain *swapchain, VkSampleCountFlagBits sample_count, VkRenderPass *render_pass) {
    VkRenderPassCreateInfo render_pass_ci = {};
    render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_ci.pNext = NULL;
    render_pass_ci.flags = 0;
    
    VkAttachmentDescription color_attachment = {};
    color_attachment.flags = 0;
    color_attachment.format = swapchain->format;
    color_attachment.samples =  sample_count;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentDescription depth_attachment = {};
    depth_attachment.flags = 0;
    depth_attachment.format = VK_FORMAT_D32_SFLOAT;
    depth_attachment.samples = sample_count;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentDescription msaa_attachment = {};
    msaa_attachment.flags = 0;
    msaa_attachment.format = swapchain->format;
    msaa_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    msaa_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    msaa_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    msaa_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    msaa_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    msaa_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    msaa_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentDescription attachments[] = {
        color_attachment,
        depth_attachment,
        msaa_attachment
    };
    
    render_pass_ci.attachmentCount = 3;
    render_pass_ci.pAttachments = attachments;
    
    VkSubpassDescription subpass_desc = {};
    subpass_desc.flags = 0;
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.inputAttachmentCount = 0;
    subpass_desc.pInputAttachments = NULL;
    
    VkAttachmentReference color_ref = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth_ref = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkAttachmentReference resolve_ref = {2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &color_ref;
    subpass_desc.pResolveAttachments = &resolve_ref;
    subpass_desc.pDepthStencilAttachment = &depth_ref;
    subpass_desc.preserveAttachmentCount = 0;
    subpass_desc.pPreserveAttachments = NULL;
    
    render_pass_ci.subpassCount = 1;
    render_pass_ci.pSubpasses = &subpass_desc;
    render_pass_ci.dependencyCount = 0;
    render_pass_ci.pDependencies = NULL;
    
    AssertVkResult(vkCreateRenderPass(device, &render_pass_ci, NULL, render_pass));
}

internal void CreateShadowMapRenderPass(const VkDevice device, const Swapchain *swapchain, VkRenderPass *render_pass) {
    VkRenderPassCreateInfo render_pass_ci = {};
    render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_ci.pNext = NULL;
    render_pass_ci.flags = 0;
    
    VkAttachmentDescription depth_attachment = {};
    depth_attachment.flags = 0;
    depth_attachment.format = VK_FORMAT_D32_SFLOAT;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    
    VkAttachmentDescription attachments[] = {
        depth_attachment
    };
    
    render_pass_ci.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
    render_pass_ci.pAttachments = attachments;
    
    VkAttachmentReference depth_ref = {0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDependency dependencies[2] = {};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    
    VkSubpassDescription subpass_desc = {};
    subpass_desc.flags = 0;
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.inputAttachmentCount = 0;
    subpass_desc.pInputAttachments = NULL;
    subpass_desc.colorAttachmentCount = 0;
    subpass_desc.pColorAttachments = NULL;
    subpass_desc.pResolveAttachments = NULL;
    subpass_desc.pDepthStencilAttachment = &depth_ref;
    subpass_desc.preserveAttachmentCount = 0;
    subpass_desc.pPreserveAttachments = NULL;
    render_pass_ci.subpassCount = 1;
    render_pass_ci.pSubpasses = &subpass_desc;
    render_pass_ci.dependencyCount = 2;
    render_pass_ci.pDependencies = dependencies;
    
    AssertVkResult(vkCreateRenderPass(device, &render_pass_ci, NULL, render_pass));
}

internal void CreateShadowMapLayout(VulkanContext *context, VulkanLayout *layout) {
    
    // Set Layout
    layout->descriptor_set_count = 1;
    
    const VkDescriptorSetLayoutBinding bindings[] = {
        { // CAMERA MATRICES
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT,
            NULL
        }
    };
    const u32 descriptor_count = sizeof(bindings) / sizeof(bindings[0]);
    
    VkDescriptorSetLayoutCreateInfo game_set_create_info = {};
    game_set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    game_set_create_info.pNext = NULL;
    game_set_create_info.flags = 0;
    game_set_create_info.bindingCount = descriptor_count;
    game_set_create_info.pBindings = bindings;
    AssertVkResult(vkCreateDescriptorSetLayout(context->device, &game_set_create_info, NULL, &layout->set_layout));
    
    // Descriptor Set
    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.pNext = NULL;
    allocate_info.descriptorPool = context->descriptor_pool;
    allocate_info.descriptorSetCount = layout->descriptor_set_count;
    allocate_info.pSetLayouts = &layout->set_layout;
    AssertVkResult(vkAllocateDescriptorSets(context->device, &allocate_info, &layout->descriptor_set));
    
    // Push constants
    VkPushConstantRange push_constant_range = 
    { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant) };
    const u32 push_constant_count = 1;
    layout->push_constant_size = push_constant_range.size;
    
    // Layout
    VkPipelineLayoutCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.setLayoutCount = layout->descriptor_set_count;
    create_info.pSetLayouts = &layout->set_layout;
    create_info.pushConstantRangeCount = push_constant_count;
    create_info.pPushConstantRanges = &push_constant_range;
    
    AssertVkResult(vkCreatePipelineLayout(context->device, &create_info, NULL, &layout->layout));
    
    // Writes
    const u32 static_writes_count = 1;
    VkWriteDescriptorSet static_writes[static_writes_count];
    
    VkDescriptorBufferInfo bi_cam = { context->cam_buffer.buffer, 0, VK_WHOLE_SIZE };
    static_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    static_writes[0].pNext = NULL;
    static_writes[0].dstSet = layout->descriptor_set;
    static_writes[0].dstBinding = 0;
    static_writes[0].dstArrayElement = 0;
    static_writes[0].descriptorCount = 1;
    static_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    static_writes[0].pImageInfo = NULL;
    static_writes[0].pBufferInfo = &bi_cam;
    static_writes[0].pTexelBufferView = NULL;
    
    vkUpdateDescriptorSets(context->device, static_writes_count, static_writes, 0, NULL);
}

internal void CreateShadowMapPipeline(const VkDevice device, const Swapchain *swapchain, const VkPipelineLayout layout, const VkRenderPass render_pass, const VkExtent2D extent, VkPipeline *pipeline) {
    VkGraphicsPipelineCreateInfo pipeline_ci = {};
    pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_ci.pNext = NULL;
    pipeline_ci.flags = 0;
    
    VkPipelineShaderStageCreateInfo stages_ci[1];
    stages_ci[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages_ci[0].pNext = NULL;
    stages_ci[0].flags = 0;
    stages_ci[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    CreateVkShaderModule("resources/shaders/shadowmap.vert.spv", device, &stages_ci[0].module);
    stages_ci[0].pName = "main";
    stages_ci[0].pSpecializationInfo = NULL;
    
    pipeline_ci.stageCount = 1;
    pipeline_ci.pStages = stages_ci;
    
    VkPipelineVertexInputStateCreateInfo vertex_input = {};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.pNext = NULL;
    vertex_input.flags = 0;
    
    vertex_input.vertexBindingDescriptionCount = 1;
    VkVertexInputBindingDescription vtx_input_binding = {};
    vtx_input_binding.binding = 0;
    vtx_input_binding.stride = sizeof(Vertex);
    vtx_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertex_input.pVertexBindingDescriptions = &vtx_input_binding;
    
    vertex_input.vertexAttributeDescriptionCount = 3;
    VkVertexInputAttributeDescription vtx_descriptions[] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, uv)},
    };
    vertex_input.pVertexAttributeDescriptions = vtx_descriptions;
    
    pipeline_ci.pVertexInputState = &vertex_input;
    
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
    input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state.pNext = NULL;
    input_assembly_state.flags = 0;
    input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state.primitiveRestartEnable = VK_FALSE;
    pipeline_ci.pInputAssemblyState = &input_assembly_state;
    
    pipeline_ci.pTessellationState = NULL;
    
    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.pNext = NULL;
    viewport_state.flags = 0;
    viewport_state.viewportCount = 1;
    VkViewport viewport = {};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = extent.width;
    viewport.height = extent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    viewport_state.pScissors = &scissor;
    pipeline_ci.pViewportState = &viewport_state;
    
    VkPipelineRasterizationStateCreateInfo rasterization_state = {};
    rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state.pNext = NULL;
    rasterization_state.flags = 0;
    rasterization_state.depthClampEnable = VK_FALSE;
    rasterization_state.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state.cullMode = VK_CULL_MODE_NONE;
    rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_state.depthBiasEnable = VK_FALSE;
    rasterization_state.depthBiasConstantFactor = 0.f;
    rasterization_state.depthBiasClamp = 0.f;
    rasterization_state.depthBiasSlopeFactor = 0.f;
    rasterization_state.lineWidth = 1.f;
    pipeline_ci.pRasterizationState = &rasterization_state;
    
    VkPipelineMultisampleStateCreateInfo multisample_state = {};
    multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.pNext = NULL;
    multisample_state.flags = 0;
    multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state.sampleShadingEnable = VK_FALSE;
    multisample_state.minSampleShading = 0.f;
    multisample_state.pSampleMask = 0;
    multisample_state.alphaToCoverageEnable = VK_FALSE;
    multisample_state.alphaToOneEnable = VK_FALSE;
    pipeline_ci.pMultisampleState = &multisample_state;
    
    VkPipelineDepthStencilStateCreateInfo stencil_state = {};
    stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    stencil_state.pNext = NULL;
    stencil_state.flags = 0;
    stencil_state.depthTestEnable = VK_TRUE;
    stencil_state.depthWriteEnable = VK_TRUE;
    stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    stencil_state.depthBoundsTestEnable = VK_FALSE;
    stencil_state.stencilTestEnable = VK_FALSE;
    stencil_state.front = {};
    stencil_state.back = {};
    stencil_state.minDepthBounds = 0.0f;
    stencil_state.maxDepthBounds = 1.0f;
    pipeline_ci.pDepthStencilState = &stencil_state;
    
    pipeline_ci.pColorBlendState = NULL;
    
    pipeline_ci.pDynamicState = NULL; // TODO(Guigui): look at this
    pipeline_ci.layout = layout;
    
    pipeline_ci.renderPass = render_pass;
    pipeline_ci.subpass = 0;
    pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_ci.basePipelineIndex = 0;
    
    // TODO: handle pipeline caching
    AssertVkResult(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_ci, NULL, pipeline));
    
    vkDeviceWaitIdle(device);
    vkDestroyShaderModule(device, pipeline_ci.pStages[0].module, NULL);
}

VulkanContext* VulkanCreateContext(SDL_Window* window){
    VulkanContext* context = (VulkanContext*)malloc(sizeof(VulkanContext));
    CreateVkInstance(window, &context->instance);
    SDL_Vulkan_CreateSurface(window,context->instance,&context->surface);
    CreateVkPhysicalDevice(context->instance, &context->physical_device);
    
    // Get device properties
    vkGetPhysicalDeviceMemoryProperties(context->physical_device, &context->memory_properties);
    vkGetPhysicalDeviceProperties(context->physical_device, &context->physical_device_properties);
    
    // MSAA
    VkSampleCountFlags msaa_levels = context->physical_device_properties.limits.framebufferColorSampleCounts & context->physical_device_properties.limits.framebufferDepthSampleCounts;
    if(msaa_levels & VK_SAMPLE_COUNT_8_BIT) {
        context->msaa_level = VK_SAMPLE_COUNT_8_BIT;
        SDL_Log("VK_SAMPLE_COUNT_8_BIT");
    }else if(msaa_levels & VK_SAMPLE_COUNT_4_BIT) {
        context->msaa_level = VK_SAMPLE_COUNT_4_BIT;
        SDL_Log("VK_SAMPLE_COUNT_4_BIT");
    }else if(msaa_levels & VK_SAMPLE_COUNT_2_BIT) {
        context->msaa_level = VK_SAMPLE_COUNT_2_BIT;
        SDL_Log("VK_SAMPLE_COUNT_2_BIT");
    } else {
        context->msaa_level = VK_SAMPLE_COUNT_1_BIT;
        SDL_Log("VK_SAMPLE_COUNT_1_BIT");
    }
    
    GetQueuesId(context);
    CreateVkDevice(context->physical_device, context->graphics_queue_id, context->transfer_queue_id,context->present_queue_id, &context->device);
    
    vkGetDeviceQueue(context->device, context->graphics_queue_id, 0, &context->graphics_queue);
    vkGetDeviceQueue(context->device, context->present_queue_id, 0, &context->present_queue);
    vkGetDeviceQueue(context->device, context->transfer_queue_id, 0, &context->transfer_queue);
    
    // Graphics Command Pool
    VkCommandPoolCreateInfo pool_create_info = {};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.pNext = NULL;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_create_info.queueFamilyIndex = context->graphics_queue_id;
    VkResult result = vkCreateCommandPool(context->device, &pool_create_info, NULL, &context->graphics_command_pool);
    AssertVkResult(result);
    
    // Descriptor Pool
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
    };
    const u32 pool_sizes_count = sizeof(pool_sizes) / sizeof(pool_sizes[0]);
    
    VkDescriptorPoolCreateInfo pool_ci = {};
    pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_ci.pNext = NULL;
    pool_ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_ci.maxSets = 2;
    pool_ci.poolSizeCount = pool_sizes_count;
    pool_ci.pPoolSizes = pool_sizes;
    AssertVkResult(vkCreateDescriptorPool(context->device, &pool_ci, NULL, &context->descriptor_pool));
    
    context->swapchain.swapchain = VK_NULL_HANDLE;
    CreateSwapchain(context, window, &context->swapchain);
    
    
    CreateBuffer(context->device, &context->memory_properties, sizeof(CameraMatrices), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &context->cam_buffer);
    DEBUGNameBuffer(context->device, &context->cam_buffer, "Camera Info");
    
    //Sampler
    VkSamplerCreateInfo sampler_ci = {};
    sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.pNext = NULL;
    sampler_ci.flags = 0;
    sampler_ci.magFilter = VK_FILTER_LINEAR;
    sampler_ci.minFilter = VK_FILTER_LINEAR;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_ci.mipLodBias = 0.0f;
    sampler_ci.anisotropyEnable = VK_TRUE;
    sampler_ci.maxAnisotropy = context->physical_device_properties.limits.maxSamplerAnisotropy;
    sampler_ci.compareEnable = VK_FALSE;
    sampler_ci.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_ci.minLod = 0.0f;
    sampler_ci.maxLod = 0.0f;
    sampler_ci.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_ci.unnormalizedCoordinates = VK_FALSE;
    AssertVkResult(vkCreateSampler(context->device, &sampler_ci, NULL, &context->texture_sampler));
    
    // Depth image
    CreateMultiSampledImage(context->device, &context->memory_properties, VK_FORMAT_D32_SFLOAT, { context->swapchain.extent.width, context->swapchain.extent.height, 1}, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, context->msaa_level, &context->depth_image);
    // MSAA Image
    CreateMultiSampledImage(context->device, &context->memory_properties, context->swapchain.format, { context->swapchain.extent.width, context->swapchain.extent.height, 1}, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, context->msaa_level, &context->msaa_image);
    CreateRenderPass(context->device, &context->swapchain, context->msaa_level, &context->render_pass);
    
    // Framebuffers
    context->framebuffers = (VkFramebuffer *)calloc(context->swapchain.image_count, sizeof(VkFramebuffer));
    for(u32 i = 0; i < context->swapchain.image_count; ++i) {
        
        VkFramebufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.pNext = NULL;
        create_info.flags = 0;
        create_info.renderPass = context->render_pass;
        create_info.attachmentCount = 3;
        
        VkImageView attachments[] = {
            context->msaa_image.image_view,
            context->depth_image.image_view,
            context->swapchain.image_views[i],
        };
        
        create_info.pAttachments = attachments;
        create_info.width = context->swapchain.extent.width;
        create_info.height = context->swapchain.extent.height;
        create_info.layers = 1;
        AssertVkResult(vkCreateFramebuffer(context->device, &create_info, NULL, &context->framebuffers[i]));
    }
    
    // ShadowMap pipe
    context->shadowmap_extent = {2048,2048};
    CreateShadowMapRenderPass(context->device, &context->swapchain, &context->shadowmap_render_pass);
    CreateShadowMapLayout(context, &context->shadowmap_layout);
    CreateShadowMapPipeline(context->device, &context->swapchain, context->shadowmap_layout.layout, context->shadowmap_render_pass, context->shadowmap_extent, &context->shadowmap_pipeline);
    
    CreateImage(context->device, &context->memory_properties, VK_FORMAT_D32_SFLOAT, context->shadowmap_extent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &context->shadowmap);
    DEBUGNameImage(context->device, &context->shadowmap, "SHADOW MAP");
    
    VkFramebufferCreateInfo framebuffer_create_info = {};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.pNext = NULL;
    framebuffer_create_info.flags = 0;
    framebuffer_create_info.renderPass = context->shadowmap_render_pass;
    framebuffer_create_info.attachmentCount = 1;
    framebuffer_create_info.pAttachments = &context->shadowmap.image_view;
    framebuffer_create_info.width = context->shadowmap_extent.width;
    framebuffer_create_info.height = context->shadowmap_extent.height;
    framebuffer_create_info.layers = 1;
    AssertVkResult(vkCreateFramebuffer(context->device, &framebuffer_create_info, NULL, &context->shadowmap_framebuffer));
    
    VkSamplerCreateInfo shdw_sampler_ci = {};
    shdw_sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    shdw_sampler_ci.pNext = NULL;
    shdw_sampler_ci.flags = 0;
    shdw_sampler_ci.magFilter = VK_FILTER_NEAREST;
    shdw_sampler_ci.minFilter = VK_FILTER_NEAREST;
    shdw_sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    shdw_sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    shdw_sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    shdw_sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    shdw_sampler_ci.mipLodBias = 0.0f;
    shdw_sampler_ci.anisotropyEnable = VK_FALSE;
    shdw_sampler_ci.maxAnisotropy = 1.0f;
    shdw_sampler_ci.compareEnable = VK_FALSE;
    shdw_sampler_ci.compareOp = VK_COMPARE_OP_ALWAYS;
    shdw_sampler_ci.minLod = 0.0f;
    shdw_sampler_ci.maxLod = 0.0f;
    shdw_sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    shdw_sampler_ci.unnormalizedCoordinates = VK_FALSE;
    AssertVkResult(vkCreateSampler(context->device, &sampler_ci, NULL, &context->shadowmap_sampler));
    
    return context;
}

void DestroyLayout(VkDevice device, VkDescriptorPool pool, VulkanLayout *layout) {
    
    vkFreeDescriptorSets(device, pool, layout->descriptor_set_count, &layout->descriptor_set);
    vkDestroyDescriptorSetLayout(device, layout->set_layout, NULL);
    vkDestroyPipelineLayout(device, layout->layout, NULL);
}

void VulkanDestroyContext(VulkanContext *context){
    
    vkDeviceWaitIdle(context->device);
    
    // Shadowmap
    DestroyImage(context->device, &context->shadowmap);
    vkDestroySampler(context->device, context->shadowmap_sampler, NULL);
    vkDestroyRenderPass(context->device, context->shadowmap_render_pass, NULL);
    DestroyLayout(context->device, context->descriptor_pool, &context->shadowmap_layout);
    vkDestroyPipeline(context->device, context->shadowmap_pipeline, NULL);
    vkDestroyFramebuffer(context->device, context->shadowmap_framebuffer, NULL);
    
    for(u32 i = 0; i < context->swapchain.image_count; ++i) {
        
        vkDestroyFramebuffer(context->device, context->framebuffers[i], NULL);
    }
    free(context->framebuffers);
    
    DestroyImage(context->device, &context->depth_image);
    DestroyImage(context->device, &context->msaa_image);
    
    vkDestroyRenderPass(context->device, context->render_pass, NULL);
    
    vkDestroySampler(context->device, context->texture_sampler, NULL);
    
    DestroyBuffer(context->device, &context->cam_buffer);
    
    DestroySwapchain(context, &context->swapchain);
    vkDestroySwapchainKHR(context->device, context->swapchain.swapchain, NULL);
    vkDestroySurfaceKHR(context->instance, context->surface, NULL);
    
    vkDestroyDescriptorPool(context->device, context->descriptor_pool, NULL);
    vkDestroyCommandPool(context->device,context->graphics_command_pool,NULL);
    
    vkDestroyDevice(context->device, NULL);
    pfn_vkDestroyDebugUtilsMessengerEXT(context->instance, debug_messenger, NULL);
    vkDestroyInstance(context->instance, NULL);
    
    free(context);
}

// ===============
//
// SCENE
//
// ===============

internal void CreateSceneLayout(VulkanContext *context, Scene *scene, VulkanLayout *layout) {
    
    // Set Layout
    layout->descriptor_set_count = 1;
    
    const VkDescriptorSetLayoutBinding bindings[] = {
        { // CAMERA MATRICES
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT,
            NULL
        },
        {  // MATERIALS
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            NULL
        },
        {  // TEXTURES
            2,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            scene->textures_count,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            NULL
        },
        {  // SHADOWMAP
            3,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            NULL
        }
    };
    const u32 descriptor_count = sizeof(bindings) / sizeof(bindings[0]);
    
    VkDescriptorSetLayoutCreateInfo game_set_create_info = {};
    game_set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    game_set_create_info.pNext = NULL;
    game_set_create_info.flags = 0;
    game_set_create_info.bindingCount = descriptor_count;
    game_set_create_info.pBindings = bindings;
    AssertVkResult(vkCreateDescriptorSetLayout(context->device, &game_set_create_info, NULL, &layout->set_layout));
    
    // Descriptor Set
    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.pNext = NULL;
    allocate_info.descriptorPool = context->descriptor_pool;
    allocate_info.descriptorSetCount = layout->descriptor_set_count;
    allocate_info.pSetLayouts = &layout->set_layout;
    AssertVkResult(vkAllocateDescriptorSets(context->device, &allocate_info, &layout->descriptor_set));
    
    // Push constants
    VkPushConstantRange push_constant_range = 
    { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant) };
    const u32 push_constant_count = 1;
    layout->push_constant_size = push_constant_range.size;
    
    // Layout
    VkPipelineLayoutCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.setLayoutCount = layout->descriptor_set_count;
    create_info.pSetLayouts = &layout->set_layout;
    create_info.pushConstantRangeCount = push_constant_count;
    create_info.pPushConstantRanges = &push_constant_range;
    
    AssertVkResult(vkCreatePipelineLayout(context->device, &create_info, NULL, &layout->layout));
    
    // Writes
    const u32 static_writes_count = 3;
    VkWriteDescriptorSet static_writes[static_writes_count];
    
    VkDescriptorBufferInfo bi_cam = { context->cam_buffer.buffer, 0, VK_WHOLE_SIZE };
    static_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    static_writes[0].pNext = NULL;
    static_writes[0].dstSet = layout->descriptor_set;
    static_writes[0].dstBinding = 0;
    static_writes[0].dstArrayElement = 0;
    static_writes[0].descriptorCount = 1;
    static_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    static_writes[0].pImageInfo = NULL;
    static_writes[0].pBufferInfo = &bi_cam;
    static_writes[0].pTexelBufferView = NULL;
    
    VkDescriptorBufferInfo materials = { scene->mat_buffer.buffer, 0, VK_WHOLE_SIZE };
    static_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    static_writes[1].pNext = NULL;
    static_writes[1].dstSet = layout->descriptor_set;
    static_writes[1].dstBinding = 1;
    static_writes[1].dstArrayElement = 0;
    static_writes[1].descriptorCount = 1;
    static_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    static_writes[1].pImageInfo = NULL;
    static_writes[1].pBufferInfo = &materials;
    static_writes[1].pTexelBufferView = NULL;
    
    
    VkDescriptorImageInfo shadowmap_info = { context->shadowmap_sampler, context->shadowmap.image_view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
    static_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    static_writes[2].pNext = NULL;
    static_writes[2].dstSet = layout->descriptor_set;
    static_writes[2].dstBinding = 3;
    static_writes[2].dstArrayElement = 0;
    static_writes[2].descriptorCount = 1;
    static_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    static_writes[2].pImageInfo = &shadowmap_info;
    static_writes[2].pBufferInfo = NULL;
    static_writes[2].pTexelBufferView = NULL;
    
    vkUpdateDescriptorSets(context->device, static_writes_count, static_writes, 0, NULL);
    
    if(scene->textures_count != 0) {
        const u32 nb_tex = scene->textures_count; 
        u32 nb_info = nb_tex > 0 ? nb_tex : 1;
        
        VkDescriptorImageInfo *images_info = (VkDescriptorImageInfo *)calloc(nb_info, sizeof(VkDescriptorImageInfo));
        
        for(u32 i = 0; i < nb_info; ++i) {
            images_info[i].sampler = context->texture_sampler;
            images_info[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            if(nb_tex == 0) {
                images_info[i].imageView = VK_NULL_HANDLE;
                break;
            }
            images_info[i].imageView = scene->textures[i].image_view;
        }
        
        VkWriteDescriptorSet textures_buffer = {};
        textures_buffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textures_buffer.pNext = NULL;
        textures_buffer.dstSet = layout->descriptor_set;
        textures_buffer.dstBinding = 2;
        textures_buffer.dstArrayElement = 0;
        textures_buffer.descriptorCount = nb_info;
        textures_buffer.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textures_buffer.pImageInfo = images_info;
        textures_buffer.pBufferInfo = NULL;
        textures_buffer.pTexelBufferView = NULL;
        
        vkUpdateDescriptorSets(context->device, 1, &textures_buffer, 0, NULL);
        free(images_info);
    }
}

internal void CreateScenePipeline(const VkDevice device, const VkPipelineLayout layout, Swapchain *swapchain, VkRenderPass render_pass, VkSampleCountFlagBits sample_count, VkPipeline *pipeline) {
    
    VkGraphicsPipelineCreateInfo pipeline_ci = {};
    pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_ci.pNext = NULL;
    pipeline_ci.flags = 0;
    
    VkPipelineShaderStageCreateInfo stages_ci[2];
    stages_ci[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages_ci[0].pNext = NULL;
    stages_ci[0].flags = 0;
    stages_ci[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    CreateVkShaderModule("resources/shaders/general.vert.spv", device, &stages_ci[0].module);
    stages_ci[0].pName = "main";
    stages_ci[0].pSpecializationInfo = NULL;
    
    stages_ci[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages_ci[1].pNext = NULL;
    stages_ci[1].flags = 0;
    stages_ci[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    CreateVkShaderModule("resources/shaders/general.frag.spv", device, &stages_ci[1].module);
    stages_ci[1].pName = "main";
    stages_ci[1].pSpecializationInfo = NULL;
    
    pipeline_ci.stageCount = 2;
    pipeline_ci.pStages = stages_ci;
    
    VkPipelineVertexInputStateCreateInfo vertex_input = {};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.pNext = NULL;
    vertex_input.flags = 0;
    
    vertex_input.vertexBindingDescriptionCount = 1;
    VkVertexInputBindingDescription vtx_input_binding = {};
    vtx_input_binding.binding = 0;
    vtx_input_binding.stride = sizeof(Vertex);
    vtx_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertex_input.pVertexBindingDescriptions = &vtx_input_binding;
    
    vertex_input.vertexAttributeDescriptionCount = 3;
    VkVertexInputAttributeDescription vtx_descriptions[] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, uv)},
    };
    vertex_input.pVertexAttributeDescriptions = vtx_descriptions;
    
    pipeline_ci.pVertexInputState = &vertex_input;
    
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
    input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state.pNext = NULL;
    input_assembly_state.flags = 0;
    input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state.primitiveRestartEnable = VK_FALSE;
    pipeline_ci.pInputAssemblyState = &input_assembly_state;
    
    pipeline_ci.pTessellationState = NULL;
    
    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.pNext = NULL;
    viewport_state.flags = 0;
    viewport_state.viewportCount = 1;
    VkViewport viewport = {};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = swapchain->extent.width;
    viewport.height = swapchain->extent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapchain->extent;
    viewport_state.pScissors = &scissor;
    pipeline_ci.pViewportState = &viewport_state;
    
    VkPipelineRasterizationStateCreateInfo rasterization_state = {};
    rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state.pNext = NULL;
    rasterization_state.flags = 0;
    rasterization_state.depthClampEnable = VK_FALSE;
    rasterization_state.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state.cullMode = VK_CULL_MODE_NONE;
    rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_state.depthBiasEnable = VK_FALSE;
    rasterization_state.depthBiasConstantFactor = 0.f;
    rasterization_state.depthBiasClamp = 0.f;
    rasterization_state.depthBiasSlopeFactor = 0.f;
    rasterization_state.lineWidth = 1.f;
    pipeline_ci.pRasterizationState = &rasterization_state;
    
    VkPipelineMultisampleStateCreateInfo multisample_state = {};
    multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.pNext = NULL;
    multisample_state.flags = 0;
    multisample_state.rasterizationSamples = sample_count;
    multisample_state.sampleShadingEnable = VK_FALSE;
    multisample_state.minSampleShading = 0.f;
    multisample_state.pSampleMask = 0;
    multisample_state.alphaToCoverageEnable = VK_FALSE;
    multisample_state.alphaToOneEnable = VK_FALSE;
    pipeline_ci.pMultisampleState = &multisample_state;
    
    VkPipelineDepthStencilStateCreateInfo stencil_state = {};
    stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    stencil_state.pNext = NULL;
    stencil_state.flags = 0;
    stencil_state.depthTestEnable = VK_TRUE;
    stencil_state.depthWriteEnable = VK_TRUE;
    stencil_state.depthCompareOp = VK_COMPARE_OP_LESS;
    stencil_state.depthBoundsTestEnable = VK_FALSE;
    stencil_state.stencilTestEnable = VK_FALSE;
    stencil_state.front = {};
    stencil_state.back = {};
    stencil_state.minDepthBounds = 0.0f;
    stencil_state.maxDepthBounds = 1.0f;
    pipeline_ci.pDepthStencilState = &stencil_state;
    
    VkPipelineColorBlendStateCreateInfo color_blend_state = {};
    color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state.pNext = NULL;
    color_blend_state.flags = 0;
    color_blend_state.logicOpEnable = VK_FALSE;
    color_blend_state.logicOp = VK_LOGIC_OP_COPY;
    
    VkPipelineColorBlendAttachmentState color_blend_attachement = {};
    color_blend_attachement.blendEnable = VK_FALSE;
    color_blend_attachement.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    /*
        VkBlendFactor            srcColorBlendFactor;
        VkBlendFactor            dstColorBlendFactor;
        VkBlendOp                colorBlendOp;
        VkBlendFactor            srcAlphaBlendFactor;
        VkBlendFactor            dstAlphaBlendFactor;
        VkBlendOp                alphaBlendOp;
        
    */
    color_blend_state.attachmentCount = 1;
    color_blend_state.pAttachments = &color_blend_attachement;
    color_blend_state.blendConstants[0] = 0.0f;
    color_blend_state.blendConstants[1] = 0.0f;
    color_blend_state.blendConstants[2] = 0.0f;
    color_blend_state.blendConstants[3] = 0.0f;
    pipeline_ci.pColorBlendState = &color_blend_state;
    
    pipeline_ci.pDynamicState = NULL; // TODO(Guigui): look at this
    pipeline_ci.layout = layout;
    
    pipeline_ci.renderPass = render_pass;
    pipeline_ci.subpass = 0;
    pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_ci.basePipelineIndex = 0;
    
    // TODO: handle pipeline caching
    AssertVkResult(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_ci, NULL, pipeline));
    
    vkDeviceWaitIdle(device);
    vkDestroyShaderModule(device, pipeline_ci.pStages[0].module, NULL);
    vkDestroyShaderModule(device, pipeline_ci.pStages[1].module, NULL);
}

void VulkanUpdateDescriptors(VulkanContext *context, GameData *game_data) {
    UploadToBuffer(context->device, &context->cam_buffer, &game_data->matrices, sizeof(game_data->matrices));
}

Scene *VulkanLoadScene(char *file, VulkanContext *context) {
    double start = SDL_GetPerformanceCounter();
    Scene *scene = (Scene *) malloc(sizeof(Scene));
    *scene = {};
    
    char *directory;
    char *last_sep = strrchr(file, '/');
    u32 size = last_sep - file;
    directory = (char *)calloc(size + 2, sizeof(char));
    strncpy(directory, file, size);
    directory[size] = '/';
    directory[size+1] = '\0';
    
    cgltf_data *data;
    cgltf_options options = {0};
    cgltf_result result = cgltf_parse_file(&options, file, &data);
    if(result != cgltf_result_success) {
        SDL_LogError(0, "Error reading scene");
        ASSERT(0);
    }
    
    cgltf_load_buffers(&options, data, file);
    
    // Transforms
    scene->nodes_count = data->nodes_count;
    scene->transforms =  (mat4 *)calloc(scene->nodes_count, sizeof(mat4));
    GLTFLoadTransforms(data, scene->transforms);
    
    // Primitives
    scene->total_primitives_count = 0;
    for(u32 m = 0; m < data->meshes_count; ++m) {
        scene->total_primitives_count += data->meshes[m].primitives_count;
    };
    scene->primitives = (Primitive *)calloc(scene->total_primitives_count, sizeof(Primitive));
    
    u32 i = 0;
    for(u32 m = 0; m < data->meshes_count; ++m) {
        
        for(u32 p = 0; p < data->meshes[m].primitives_count; p++) {
            cgltf_primitive *prim = &data->meshes[m].primitives[p];
            
            Primitive *primitive = &scene->primitives[i];
            
            primitive->vertex_offset = scene->total_vertex_count;
            scene->total_vertex_count += (u32) prim->attributes[0].data->count;
            
            primitive->index_offset = scene->total_index_count;
            primitive->index_count += prim->indices->count;
            scene->total_index_count += (u32) prim->indices->count;
            
            primitive->material_id = GLTFGetMaterialID(prim->material);
            ++i;
        }
    }
    
    // Vertex & Index Buffer
    CreateBuffer(context->device, &context->memory_properties, scene->total_vertex_count * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &scene->vtx_buffer);
    CreateBuffer(context->device, &context->memory_properties, scene->total_index_count * sizeof(u32),  VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &scene->idx_buffer);
    DEBUGNameBuffer(context->device, &scene->vtx_buffer, "GLTF VTX");
    DEBUGNameBuffer(context->device, &scene->idx_buffer, "GLTF IDX");
    void *mapped_vtx_buffer;
    void *mapped_idx_buffer;
    MapBuffer(context->device, &scene->vtx_buffer, &mapped_vtx_buffer);
    MapBuffer(context->device, &scene->idx_buffer, &mapped_idx_buffer);
    
    i = 0;
    for(u32 m = 0; m < data->meshes_count; ++m) {
        for(u32 p = 0; p < data->meshes[m].primitives_count; ++p) {
            GLTFLoadVertexAndIndexBuffer(&data->meshes[m].primitives[p], &scene->primitives[i], mapped_vtx_buffer, mapped_idx_buffer);
            ++i;
        }
    }
    UnmapBuffer(context->device, &scene->vtx_buffer);
    UnmapBuffer(context->device, &scene->idx_buffer);
    
    // Materials
    scene->materials_count = data->materials_count;
    CreateBuffer(context->device, &context->memory_properties, scene->materials_count * sizeof(Material),  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &scene->mat_buffer);
    DEBUGNameBuffer(context->device, &scene->mat_buffer, "GLTF MATS");
    void *mapped_mat_buffer;
    MapBuffer(context->device, &scene->mat_buffer, &mapped_mat_buffer);
    GLTFLoadMaterialBuffer(data, (Material*)mapped_mat_buffer);
    UnmapBuffer(context->device, &scene->mat_buffer);
    
    
    // Textures
    SDL_Log("Loading textures...");
    scene->textures_count = data->textures_count;
    if(data->textures_count > 0) {
        
        scene->textures = (Image *)calloc(data->textures_count, sizeof(Image));
        
        SDL_Surface **surfaces = (SDL_Surface **)calloc(data->textures_count, sizeof(SDL_Surface *));
        Buffer *image_buffers = (Buffer *)calloc(data->textures_count, sizeof(Buffer));
        VkCommandBuffer *cmds = (VkCommandBuffer *)calloc(data->textures_count, sizeof(VkCommandBuffer));
        
        AllocateCommandBuffers(context->device, context->graphics_command_pool, data->textures_count, cmds);
        
        for(u32 i = 0; i < data->textures_count; ++i) {
            char* image_path = data->textures[i].image->uri;
            u32 file_path_length = strlen(directory) + strlen(image_path) + 1;
            char* full_image_path = (char *)calloc(file_path_length, sizeof(char *));
            strcat(full_image_path, directory);
            strcat(full_image_path, image_path);
            
            SDL_Surface* temp_surf = IMG_Load(full_image_path);
            if(!temp_surf) {
                SDL_LogError(0, IMG_GetError());
            }
            
            if(temp_surf->format->format != SDL_PIXELFORMAT_ABGR8888) {
                surfaces[i]= SDL_ConvertSurfaceFormat(temp_surf, SDL_PIXELFORMAT_ABGR8888, 0);
                if(!surfaces[i]) {
                    SDL_LogError(0, SDL_GetError());
                }
                SDL_FreeSurface(temp_surf);
            } else {
                surfaces[i] = temp_surf;
            }
            free(full_image_path);
        }
        
        for(u32 i = 0; i < data->textures_count; ++i) {
            VkDeviceSize image_size = surfaces[i]->h * surfaces[i]->pitch;
            VkExtent2D extent = {(u32)surfaces[i]->h, (u32)surfaces[i]->w};
            
            VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
            if(data->textures[i].type == cgltf_texture_type_base_color)
                format = VK_FORMAT_R8G8B8A8_SRGB;
            
            CreateImage(context->device, &context->memory_properties, format, extent, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &scene->textures[i]);
            DEBUGNameImage(context->device, &scene->textures[i], data->textures[i].image->uri);
            CreateBuffer(context->device, &context->memory_properties, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &image_buffers[i]); 
            SDL_LockSurface(surfaces[i]);
            UploadToBuffer(context->device, &image_buffers[i], surfaces[i]->pixels, image_size);
            SDL_UnlockSurface(surfaces[i]);
            BeginCommandBuffer(context->device, cmds[i], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            CopyBufferToImage(cmds[i], extent, surfaces[i]->pitch, &image_buffers[i], &scene->textures[i]);
            vkEndCommandBuffer(cmds[i]);
            VkSubmitInfo si = {VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, 0, 1, &cmds[i], 0, NULL};
            vkQueueSubmit(context->graphics_queue, 1, &si, VK_NULL_HANDLE);
        }
        
        vkQueueWaitIdle(context->graphics_queue);
        
        vkFreeCommandBuffers(context->device, context->graphics_command_pool, data->textures_count, cmds);
        free(cmds);
        for(u32 i = 0; i < data->textures_count; ++i) {
            SDL_FreeSurface(surfaces[i]);
            DestroyBuffer(context->device, &image_buffers[i]);
        }
        free(surfaces);
        free(image_buffers);
    }
    
    CreateSceneLayout(context, scene, &scene->layout);
    CreateScenePipeline(context->device, scene->layout.layout, &context->swapchain, context->render_pass, context->msaa_level, &scene->pipeline);
    
    cgltf_free(data);
    free(directory);
    
    double end = SDL_GetPerformanceCounter();
    SDL_Log("Scene loaded in : %.2fms", (double)((end - start)*1000)/SDL_GetPerformanceFrequency());
    
    return scene;
}

void VulkanReloadShaders(VulkanContext *context, Scene *scene) {
    vkDestroyPipeline(context->device, scene->pipeline, NULL);
    CreateScenePipeline(context->device, scene->layout.layout, &context->swapchain, context->render_pass, context->msaa_level, &scene->pipeline);
    
    vkDestroyPipeline(context->device, context->shadowmap_pipeline, NULL);
    CreateShadowMapPipeline(context->device, &context->swapchain, context->shadowmap_layout.layout, context->shadowmap_render_pass, context->shadowmap_extent, &context->shadowmap_pipeline);
    
}

void VulkanFreeScene(VulkanContext *context, Scene *scene) {
    
    DestroyLayout(context->device, context->descriptor_pool, &scene->layout);
    
    vkDestroyPipeline(context->device, scene->pipeline, NULL);
    
    for(u32 i = 0; i < scene->textures_count; ++i) {
        DestroyImage(context->device, &scene->textures[i]);
    }
    free(scene->textures);
    
    free(scene->primitives);
    
    DestroyBuffer(context->device, &scene->vtx_buffer);
    DestroyBuffer(context->device, &scene->idx_buffer);
    DestroyBuffer(context->device, &scene->mat_buffer);
    free(scene->transforms);
    
    free(scene);
}

// ================
//
// DRAWING
//
// ================

void BeginRenderPass(VkCommandBuffer cmd, VkFramebuffer framebuffer, VkRenderPass render_pass, VkExtent2D extent) {
    VkRenderPassBeginInfo renderpass_begin = {};
    renderpass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpass_begin.pNext = 0;
    renderpass_begin.renderPass = render_pass;
    renderpass_begin.framebuffer = framebuffer;
    renderpass_begin.renderArea = {{0,0}, extent};
    
    VkClearValue clear_values[2] = {};
    clear_values[0].color = {0.2f, 0.2f, 0.2f, 0.0f};
    clear_values[1].depthStencil = {1.0f, 0};
    renderpass_begin.clearValueCount = 2;
    renderpass_begin.pClearValues = clear_values;
    
    vkCmdBeginRenderPass(cmd, &renderpass_begin, VK_SUBPASS_CONTENTS_INLINE );
}

void VulkanDrawFrame(VulkanContext* context, Scene *scene) {
    
    u32 image_id;
    Swapchain* swapchain = &context->swapchain;
    
    VkResult result;
    result = vkAcquireNextImageKHR(context->device, swapchain->swapchain, UINT64_MAX, swapchain->image_acquired_semaphore[swapchain->semaphore_id],VK_NULL_HANDLE, &image_id);
    AssertVkResult(result);// TODO(Guigui): Recreate swapchain if Suboptimal or outofdate;
    
    // If the frame hasn't finished rendering wait for it to finish
    AssertVkResult(vkWaitForFences(context->device, 1, &swapchain->fences[image_id], VK_TRUE, UINT64_MAX));
    
    vkResetFences(context->device, 1, &swapchain->fences[image_id]);
    
    VkCommandBuffer cmd = swapchain->command_buffers[image_id];
    
    const VkCommandBufferBeginInfo begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, 0, NULL };
    AssertVkResult(vkBeginCommandBuffer(cmd, &begin_info));
    
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &scene->vtx_buffer.buffer, &offset);
    vkCmdBindIndexBuffer(cmd, scene->idx_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    
    // Shadow map
    VkRenderPassBeginInfo renderpass_begin = {};
    renderpass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpass_begin.pNext = 0;
    renderpass_begin.renderPass = context->shadowmap_render_pass;
    renderpass_begin.framebuffer = context->shadowmap_framebuffer;
    renderpass_begin.renderArea = {{0,0}, context->shadowmap_extent};
    VkClearValue clear_value = {};
    clear_value.depthStencil = {1.0f, 0};
    renderpass_begin.clearValueCount = 1;
    renderpass_begin.pClearValues = &clear_value;
    vkCmdBeginRenderPass(cmd, &renderpass_begin, VK_SUBPASS_CONTENTS_INLINE );
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, context->shadowmap_pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, context->shadowmap_layout.layout, 0, 1, &context->shadowmap_layout.descriptor_set, 0, NULL);
    for(u32 i = 0; i < scene->total_primitives_count; i++) {
        
        Primitive *prim = &scene->primitives[i];
        
        PushConstant push = { scene->transforms[prim->node_id], prim->material_id };
        vkCmdPushConstants(cmd, scene->layout.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant), &push);
        vkCmdDrawIndexed(cmd, prim->index_count, 1, prim->index_offset, prim->vertex_offset, 0);
    }
    vkCmdEndRenderPass(cmd);
    
    // Color
    BeginRenderPass(cmd, context->framebuffers[image_id], context->render_pass, swapchain->extent);
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene->pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene->layout.layout, 0, 1, &scene->layout.descriptor_set, 0, NULL);
    
    for(u32 i = 0; i < scene->total_primitives_count; i++) {
        
        Primitive *prim = &scene->primitives[i];
        
        PushConstant push = { scene->transforms[prim->node_id], prim->material_id };
        vkCmdPushConstants(cmd, scene->layout.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant), &push);
        vkCmdDrawIndexed(cmd, prim->index_count, 1, prim->index_offset, prim->vertex_offset, 0);
    }
    
    vkCmdEndRenderPass(cmd);
    AssertVkResult(vkEndCommandBuffer(cmd));
    
    const VkPipelineStageFlags stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &swapchain->image_acquired_semaphore[swapchain->semaphore_id];
    submit_info.pWaitDstStageMask = &stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &swapchain->render_complete_semaphore[swapchain->semaphore_id];
    result = vkQueueSubmit(context->graphics_queue, 1, &submit_info, swapchain->fences[image_id]);
    AssertVkResult(result);
    
    // Present
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = NULL;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &swapchain->render_complete_semaphore[swapchain->semaphore_id];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain->swapchain;
    present_info.pImageIndices = &image_id;
    present_info.pResults = NULL;
    result = vkQueuePresentKHR(context->present_queue, &present_info);
    AssertVkResult(result);
    // TODO(Guigui): Recreate Swapchain if necessary
    
    swapchain->semaphore_id = (swapchain->semaphore_id + 1) % swapchain->image_count;
    
    vkDeviceWaitIdle(context->device);
}