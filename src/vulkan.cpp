#include <SDL/SDL_vulkan.h>

/*
 === TODO ===
 CRITICAL
 - Draw the cube in raster

 MAJOR
  - Remove RTX from reqs

 BACKLOG
 - Normals
  - Window Resize

 IMPROVEMENTS
 - Envoyer les buffers au shaders
 - Scene data (qui vient du gltf) 
 - Staging buffers
 - Handle pipeline caches
 - Pipline dynamic states ?
*/

#define DECL_FUNC(name) global PFN_##name pfn_##name
#define LOAD_INSTANCE_FUNC(instance, name) pfn_##name = (PFN_##name)vkGetInstanceProcAddr(instance, #name); \
ASSERT(pfn_##name)
#define LOAD_DEVICE_FUNC(name) pfn_##name = (PFN_##name)vkGetDeviceProcAddr(device, #name); \
ASSERT(pfn_##name)

typedef struct Image {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView image_view;
} Image;

typedef struct Buffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDeviceAddress address;
    VkDeviceSize size;
} Buffer;

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

typedef struct VulkanRenderer {
    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkDescriptorPool descriptor_pool;
    
    VkRenderPass render_pass;
    
    VkDescriptorSet descriptor_sets[2];
    u32 descriptor_set_count;
    
    u32 push_constant_size;
    
    VkDescriptorSetLayout app_set_layout;
    VkDescriptorSetLayout game_set_layout;
    
    VkFramebuffer *framebuffers;
    
} VulkanRenderer;

typedef struct VulkanContext {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkSurfaceKHR surface;
    
    VkPhysicalDeviceMemoryProperties memory_properties;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtx_properties;
    
    VkDevice device;
    u32 graphics_queue_id;
    VkQueue graphics_queue;
    u32 transfer_queue_id;
    VkQueue transfer_queue;
    u32 present_queue_id;
    VkQueue present_queue;
    
    VkCommandPool graphics_command_pool;
    
    Swapchain swapchain;
    PushConstant push_constants;
    
    Buffer cam_buffer;
    
} VulkanContext;

global VkDebugUtilsMessengerEXT debug_messenger;

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

internal void DEBUGNameObject(const VkDevice device, const u64 object, const VkObjectType type, const char* name) {
    
    VkDebugUtilsObjectNameInfoEXT name_info = {};
    name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    name_info.pNext = NULL;
    name_info.objectType = type;
    name_info.objectHandle = object;
    name_info.pObjectName = name;
    pfn_vkSetDebugUtilsObjectNameEXT(device, &name_info);
}

internal void DEBUGNameBuffer(const VkDevice device, Buffer* buffer, const char *name) {
    DEBUGNameObject(device, (u64)buffer->buffer, VK_OBJECT_TYPE_BUFFER, name);
    DEBUGNameObject(device, (u64)buffer->memory, VK_OBJECT_TYPE_DEVICE_MEMORY, name);
}

internal void DEBUGNameImage(const VkDevice device, Image* image, const char* name) {
    DEBUGNameObject(device, (u64)image->image, VK_OBJECT_TYPE_IMAGE, name);
    DEBUGNameObject(device, (u64)image->memory, VK_OBJECT_TYPE_DEVICE_MEMORY, name);
    DEBUGNameObject(device, (u64)image->image_view, VK_OBJECT_TYPE_IMAGE_VIEW, name);
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
            default :
            SDL_LogError(0, "Unhandled error : %d", result);
            break;
        }
        ASSERT(false);
        exit(result);
    }
}

internal void LoadDeviceFuncPointers(VkDevice device) {
    
    LOAD_DEVICE_FUNC(vkCreateRayTracingPipelinesKHR);
    LOAD_DEVICE_FUNC(vkCmdTraceRaysKHR);
    LOAD_DEVICE_FUNC(vkGetRayTracingShaderGroupHandlesKHR);
    LOAD_DEVICE_FUNC(vkGetBufferDeviceAddressKHR);
    LOAD_DEVICE_FUNC(vkCreateAccelerationStructureKHR);
    LOAD_DEVICE_FUNC(vkGetAccelerationStructureBuildSizesKHR);
    LOAD_DEVICE_FUNC(vkCmdBuildAccelerationStructuresKHR);
    LOAD_DEVICE_FUNC(vkDestroyAccelerationStructureKHR);
    LOAD_DEVICE_FUNC(vkGetAccelerationStructureDeviceAddressKHR);
}

// ================================
//
// NOTE(Guigui):  MEMORY MANAGEMENT
//
// ================================

internal i32 FindMemoryType(const VkPhysicalDeviceMemoryProperties *memory_properties, const u32 type, const VkMemoryPropertyFlags flags) {
    
    for(u32 i = 0; i < memory_properties->memoryTypeCount; i++) {
        if ((type & (1 << i))  && (memory_properties->memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }
    ASSERT(0); // TODO(Guigui): Handle this more cleanly
    return -1;
}

// TODO(Guigui): Check to see if this really requires the entire context
internal void CreateBuffer(const VulkanContext *context, const VkDeviceSize size, const VkBufferUsageFlags buffer_usage, const VkMemoryPropertyFlags memory_flags, Buffer *buffer) {
    *buffer = {};
    
    buffer->size = size;
    
    // Create the buffer
    {
        VkBufferCreateInfo buffer_ci = {};
        buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_ci.pNext = NULL;
        buffer_ci.flags = 0;
        buffer_ci.size = size;
        buffer_ci.usage = buffer_usage;
        buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_ci.queueFamilyIndexCount = 0;
        buffer_ci.pQueueFamilyIndices = NULL;
        
        VkResult result = vkCreateBuffer(context->device, &buffer_ci, NULL, &buffer->buffer);
        AssertVkResult(result);
    }
    // Allocate the memory
    {
        VkMemoryRequirements requirements = {};
        vkGetBufferMemoryRequirements(context->device, buffer->buffer, &requirements);
        
        VkMemoryAllocateFlagsInfo flags_info = {};
        flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        flags_info.pNext = NULL;
        flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
        flags_info.deviceMask = 0;
        
        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = &flags_info;
        alloc_info.allocationSize = requirements.size;
        alloc_info.memoryTypeIndex = FindMemoryType(&context->memory_properties, requirements.memoryTypeBits, memory_flags);
        VkResult result = vkAllocateMemory(context->device, &alloc_info, NULL, &buffer->memory);
        AssertVkResult(result);
    }
    // Bind the buffer to the memory
    VkResult result = vkBindBufferMemory(context->device, buffer->buffer, buffer->memory, 0);
    AssertVkResult(result);
    
    if(buffer_usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR) {
        // Get its address
        VkBufferDeviceAddressInfo dai = {
            VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            NULL,
            buffer->buffer
        };
        buffer->address = pfn_vkGetBufferDeviceAddressKHR(context->device,&dai);
    }
}

internal void UploadToBuffer(const VkDevice device, Buffer* buffer, void* data, size_t size) {
    void* dst;
    VkResult result = vkMapMemory(device, buffer->memory, 0, buffer->size, 0, &dst);
    AssertVkResult(result);
    memcpy(dst, data, size);
    vkUnmapMemory(device, buffer->memory);
}

internal void MapBuffer(const VkDevice device, Buffer* buffer, void** data) {
    VkResult result = vkMapMemory(device, buffer->memory, 0, buffer->size, 0, data);
    AssertVkResult(result);
}

internal void UnmapBuffer(const VkDevice device, Buffer* buffer) {
    vkUnmapMemory(device, buffer->memory);
}

internal void DestroyBuffer(const VkDevice device, Buffer *buffer) {
    vkFreeMemory(device, buffer->memory, NULL);
    vkDestroyBuffer(device, buffer->buffer, NULL);
    buffer = {};
}

internal void DestroyImage(const VkDevice device, Image* image) {
    vkDestroyImage(device, image->image, NULL);
    vkDestroyImageView(device, image->image_view, NULL);
    vkFreeMemory(device, image->memory, NULL);
}

// COMMAND BUFFERS
internal void AllocateCommandBuffers(const VkDevice device, const VkCommandPool pool, const u32 count, VkCommandBuffer* command_buffers) {
    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.pNext = NULL;
    allocate_info.commandPool = pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    AssertVkResult(vkAllocateCommandBuffers(device, &allocate_info, command_buffers));
}

internal void BeginCommandBuffer(const VkDevice device, const VkCommandBuffer cmd, VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = NULL;
    begin_info.flags = flags;
    begin_info.pInheritanceInfo = 0;
    AssertVkResult(vkBeginCommandBuffer(cmd, &begin_info));
}

internal void AllocateAndBeginCommandBuffer(const VkDevice device, const VkCommandPool pool, VkCommandBuffer* cmd) {
    AllocateCommandBuffers(device, pool, 1, cmd);
    BeginCommandBuffer(device, *cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

internal void EndAndExecuteCommandBuffer(const VkDevice device, const VkQueue queue, const VkCommandPool pool, VkCommandBuffer cmd) {
    vkEndCommandBuffer(cmd);
    VkSubmitInfo si = {VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, 0, 1, &cmd, 0, NULL};
    vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(device, pool, 1, &cmd);
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
// NOTE(Guigui): Instance & Devices Creation
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
    const u32 extension_count = 9;
    const char* extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_MAINTENANCE3_EXTENSION_NAME,
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
        VK_KHR_SHADER_CLOCK_EXTENSION_NAME
    };
    
    // Features
    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features = {};
    descriptor_indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptor_indexing_features.pNext = NULL;
    descriptor_indexing_features.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
    descriptor_indexing_features.runtimeDescriptorArray = VK_TRUE;
    
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accel_feature = {};
    accel_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accel_feature.pNext = &descriptor_indexing_features;
    accel_feature.accelerationStructure = VK_TRUE;
    
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_feature = {};
    rt_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rt_feature.pNext = &accel_feature;
    rt_feature.rayTracingPipeline = VK_TRUE;
    
    VkPhysicalDeviceBufferDeviceAddressFeatures device_address = {};
    device_address.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    device_address.pNext = &rt_feature;
    device_address.bufferDeviceAddress = VK_TRUE;
    
    VkPhysicalDeviceFeatures2 features2 = {};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.features = {};
    features2.features.samplerAnisotropy = VK_TRUE;
    features2.features.shaderInt64 = VK_TRUE;
    features2.pNext = &device_address;
    
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

// =======================
//
// NOTE(Guigui): SWAPCHAIN
//
// =======================

// TODO(Guigui): Is this needed?
internal void FillSwapchainCreateInfo(VkPhysicalDevice physical_device, VkSurfaceKHR surface, SDL_Window* window, VkSwapchainCreateInfoKHR *create_info){
    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
    
    create_info->sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info->pNext = NULL;
    create_info->flags = 0;
    create_info->surface = surface;
    
    // Img count
    u32 image_count = 3;
    if(surface_capabilities.maxImageCount != 0 && surface_capabilities.maxImageCount < image_count) {
        image_count = surface_capabilities.maxImageCount;
    }
    if(image_count < surface_capabilities.minImageCount) {
        image_count = surface_capabilities.minImageCount;
    }
    create_info->minImageCount = image_count;
    
    // Format
    u32 format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, NULL);
    VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)calloc(format_count, sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device,surface, &format_count, formats);
    create_info->imageFormat = formats[0].format; //TODO: Maybe pick a format instead of taking the first one?
    create_info->imageColorSpace = formats[1].colorSpace;
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
    create_info->imageExtent = extent;
    
    create_info->imageArrayLayers = 1;
    create_info->imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ;
    create_info->imageSharingMode = VK_SHARING_MODE_EXCLUSIVE ;
    create_info->queueFamilyIndexCount = 0;
    create_info->pQueueFamilyIndices = NULL; // NOTE: Needed only if sharing mode is shared
    create_info->preTransform = surface_capabilities.currentTransform;
    create_info->compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; // NOTE: Default present mode
    u32 present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device,surface,&present_mode_count,NULL);
    VkPresentModeKHR *present_modes = (VkPresentModeKHR *)calloc(present_mode_count, sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device,surface,&present_mode_count,present_modes);
    for(u32 i = 0; i < present_mode_count;i++) {
        if(present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }
    free(present_modes);
    create_info->presentMode = present_mode;
    create_info->clipped = VK_FALSE;
    create_info->oldSwapchain = VK_NULL_HANDLE;
}

void DestroySwapchain(const VulkanContext* context, Swapchain *swapchain) {
    
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

internal void CreateOrUpdateSwapchain(const VulkanContext* context, SDL_Window* window, Swapchain *swapchain) {
    if(swapchain->swapchain != VK_NULL_HANDLE) {
        DestroySwapchain(context, swapchain);
    }
    
    VkSwapchainCreateInfoKHR create_info = {};
    FillSwapchainCreateInfo(context->physical_device, context->surface, window, &create_info);
    create_info.oldSwapchain = swapchain->swapchain;
    VkResult result = vkCreateSwapchainKHR(context->device, &create_info, NULL, &swapchain->swapchain);
    AssertVkResult(result);
    
    swapchain->format = create_info.imageFormat;
    swapchain->extent = create_info.imageExtent;
    
    // Images
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
    
    // Command buffer
    swapchain->command_buffers = (VkCommandBuffer *)calloc(swapchain->image_count, sizeof(VkCommandBuffer));
    VkCommandBufferAllocateInfo cmd_buf_ai = {};
    cmd_buf_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buf_ai.pNext = NULL;
    cmd_buf_ai.commandPool = context->graphics_command_pool;
    cmd_buf_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buf_ai.commandBufferCount = swapchain->image_count;
    result = vkAllocateCommandBuffers(context->device, &cmd_buf_ai, swapchain->command_buffers);
    AssertVkResult(result);
    
    //Fences
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
    
    // Image semaphores
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

void VulkanUpdateDescriptors(VulkanContext *context, GameData *game_data) {
    UploadToBuffer(context->device, &context->cam_buffer, &game_data->cam_matrix, sizeof(game_data->cam_matrix));
}

internal void CreateRasterPipeline(const VkDevice device, const VkPipelineLayout layout, Swapchain *swapchain, VkPipeline *pipeline, VkRenderPass *render_pass) {
    
    VkRenderPassCreateInfo render_pass_ci = {};
    render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_ci.pNext = NULL;
    render_pass_ci.flags = 0;
    
    VkAttachmentDescription color_attachment = {};
    color_attachment.flags = 0;
    color_attachment.format = swapchain->format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT ;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    render_pass_ci.attachmentCount = 1;
    render_pass_ci.pAttachments = &color_attachment;
    
    VkSubpassDescription subpss_desc = {};
    subpss_desc.flags = 0;
    subpss_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpss_desc.inputAttachmentCount = 0;
    subpss_desc.pInputAttachments = NULL;
    
    VkAttachmentReference color_ref = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    subpss_desc.colorAttachmentCount = 1;
    subpss_desc.pColorAttachments = &color_ref;
    subpss_desc.pResolveAttachments = NULL;
    subpss_desc.pDepthStencilAttachment = NULL;
    subpss_desc.preserveAttachmentCount = 0;
    subpss_desc.pPreserveAttachments = NULL;
    
    render_pass_ci.subpassCount = 1;
    render_pass_ci.pSubpasses = &subpss_desc;
    render_pass_ci.dependencyCount = 0;
    render_pass_ci.pDependencies = NULL;
    
    AssertVkResult(vkCreateRenderPass(device, &render_pass_ci, NULL, render_pass));
    
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
    vtx_input_binding.stride = 12; // TODO(Guigui): Needs to be dependent on the gltf
    vtx_input_binding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
    vertex_input.pVertexBindingDescriptions = &vtx_input_binding;
    
    vertex_input.vertexAttributeDescriptionCount = 2;
    VkVertexInputAttributeDescription vtx_descriptions[] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 288}, // Position
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}    // Normal
        
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
    rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
    
    pipeline_ci.pDepthStencilState = NULL;
    
    VkPipelineColorBlendStateCreateInfo color_blend_state = {};
    color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state.pNext = NULL;
    color_blend_state.flags = 0;
    color_blend_state.logicOpEnable = VK_FALSE;
    color_blend_state.logicOp = VK_LOGIC_OP_COPY;
    
    VkPipelineColorBlendAttachmentState color_blend_attachement = {};
    color_blend_attachement.blendEnable = VK_FALSE;
    /*
    VkBlendFactor            srcColorBlendFactor;
    VkBlendFactor            dstColorBlendFactor;
    VkBlendOp                colorBlendOp;
    VkBlendFactor            srcAlphaBlendFactor;
    VkBlendFactor            dstAlphaBlendFactor;
    VkBlendOp                alphaBlendOp;
    VkColorComponentFlags    colorWriteMask;
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
    
    pipeline_ci.renderPass = *render_pass;
    pipeline_ci.subpass = 0;
    pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_ci.basePipelineIndex = 0;
    
    // TODO: handle pipeline caching
    AssertVkResult(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_ci, NULL, pipeline));
    
    vkDeviceWaitIdle(device);
    vkDestroyShaderModule(device, pipeline_ci.pStages[0].module, NULL);
    vkDestroyShaderModule(device, pipeline_ci.pStages[1].module, NULL);
    
}

void VulkanReloadShaders(VulkanContext *context, VulkanRenderer *renderer) {
    vkDestroyPipeline(context->device, renderer->pipeline, NULL);
    CreateRasterPipeline(context->device, renderer->layout, &context->swapchain, &renderer->pipeline, &renderer->render_pass);
}


internal void WriteDescriptorSets(const VkDevice device, const VkDescriptorSet *descriptor_set, Buffer *cam_buffer) {
    
    u32 game_writes_count = 1;
    VkDescriptorBufferInfo bi_cam = { cam_buffer->buffer, 0, VK_WHOLE_SIZE };
    VkWriteDescriptorSet game_writes[1];
    game_writes[0] = 
    { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, descriptor_set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, NULL, &bi_cam, NULL};
    
    vkUpdateDescriptorSets(device, game_writes_count, game_writes, 0, NULL);
}



internal void CreatePipelineLayout(const VkDevice device, VulkanRenderer *renderer) {
    
    // Set Layout
    
    // TODO(Guigui): TEMP
    const u32 mesh_count = 1;
    const u32 material_count = 1;
    
    // Our set layout
    renderer->descriptor_set_count = 1;
    
    const u32 app_descriptor_count = 0;
    /*
    VkDescriptorSetLayoutBinding app_bindings[app_descriptor_count] = {};
    app_bindings[0] = { 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, NULL };
    app_bindings[1] = { 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, NULL };
    
    VkDescriptorSetLayoutCreateInfo app_set_create_info = {};
    app_set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    app_set_create_info.pNext = NULL;
    app_set_create_info.flags = 0;
    app_set_create_info.bindingCount = app_descriptor_count;
    app_set_create_info.pBindings = app_bindings;
    VkResult result = vkCreateDescriptorSetLayout(device, &app_set_create_info, NULL, &renderer->app_set_layout);
    AssertVkResult(result);
    */
    
    // Get the game set layout
    const VkDescriptorSetLayoutBinding game_bindings[] = {
        { // CAMERA MATRICES
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            NULL
        },
        { // SCENE DESCRIPTION
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            1,
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            NULL
        },
        { // VERTEX BUFFER
            2,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            mesh_count,
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            NULL
        },
        { // INDEX BUFFER
            3,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            mesh_count,
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            NULL
        },
        { // MATERIAL BUFFER
            4,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            material_count,
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            NULL
        }
    };
    const u32 game_descriptor_count = sizeof(game_bindings) / sizeof(game_bindings[0]);
    
    VkDescriptorSetLayoutCreateInfo game_set_create_info = {};
    game_set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    game_set_create_info.pNext = NULL;
    game_set_create_info.flags = 0;
    game_set_create_info.bindingCount = game_descriptor_count;
    game_set_create_info.pBindings = game_bindings;
    AssertVkResult(vkCreateDescriptorSetLayout(device, &game_set_create_info, NULL, &renderer->game_set_layout));
    
    // Descriptor Pool
    // TODO(Guigui): if the game doesn't use one of these types, we get a validation error
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0},
    };
    const u32 pool_sizes_count = sizeof(pool_sizes) / sizeof(pool_sizes[0]);
    
    
    // App
    /*
    for(u32 d = 0; d < app_descriptor_count; d++) {
        for(u32 i = 0; i < pool_sizes_count ; i ++){
            if(app_bindings[d].descriptorType == pool_sizes[i].type) {
                pool_sizes[i].descriptorCount ++;
            }
        }
    }
*/
    
    //Game
    for(u32 d = 0; d < game_descriptor_count; d++) {
        for(u32 i = 0; i < pool_sizes_count ; i ++){
            if(game_bindings[d].descriptorType == pool_sizes[i].type) {
                pool_sizes[i].descriptorCount ++;
            }
        }
    }
    
    VkDescriptorPoolCreateInfo pool_ci = {};
    pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_ci.pNext = NULL;
    pool_ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_ci.maxSets = renderer->descriptor_set_count;
    pool_ci.poolSizeCount = pool_sizes_count;
    pool_ci.pPoolSizes = pool_sizes;
    AssertVkResult(vkCreateDescriptorPool(device, &pool_ci, NULL, &renderer->descriptor_pool));
    
    // Descriptor Set
    VkDescriptorSetLayout set_layouts[1] = {renderer->game_set_layout};
    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.pNext = NULL;
    allocate_info.descriptorPool = renderer->descriptor_pool;
    allocate_info.descriptorSetCount = renderer->descriptor_set_count;
    allocate_info.pSetLayouts = set_layouts;
    AssertVkResult(vkAllocateDescriptorSets(device, &allocate_info, renderer->descriptor_sets));
    
    // Push constants
    // TODO(Guigui): Currently limited to 1 PC
    
    VkPushConstantRange push_constant_range = 
    { VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, 0, sizeof(PushConstant) };
    const u32 push_constant_count = 1;
    renderer->push_constant_size = push_constant_range.size;
    
    VkPipelineLayoutCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.setLayoutCount = renderer->descriptor_set_count;
    create_info.pSetLayouts = set_layouts;
    create_info.pushConstantRangeCount = push_constant_count;
    create_info.pPushConstantRanges = &push_constant_range;
    
    AssertVkResult(vkCreatePipelineLayout(device, &create_info, NULL, &renderer->layout));
}


void VulkanDrawFrame(VulkanContext* context, VulkanRenderer *renderer) {
    
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
    
    VkRenderPassBeginInfo renderpass_begin = {};
    renderpass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpass_begin.pNext = 0;
    renderpass_begin.renderPass = renderer->render_pass;
    renderpass_begin.framebuffer = renderer->framebuffers[image_id];
    renderpass_begin.renderArea = {{0,0}, swapchain->extent};
    renderpass_begin.clearValueCount = 1;
    VkClearValue clear_value = {};
    clear_value.color = {0.0f, 0.0f, 0.0f, 0.0f};
    renderpass_begin.pClearValues = &clear_value;
    vkCmdBeginRenderPass(cmd, &renderpass_begin, VK_SUBPASS_CONTENTS_INLINE );
    
    
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


VulkanContext* VulkanCreateContext(SDL_Window* window){
    VulkanContext* context = (VulkanContext*)malloc(sizeof(VulkanContext));
    CreateVkInstance(window, &context->instance);
    SDL_Vulkan_CreateSurface(window,context->instance,&context->surface);
    CreateVkPhysicalDevice(context->instance, &context->physical_device);
    
    // Get device properties
    vkGetPhysicalDeviceMemoryProperties(context->physical_device, &context->memory_properties);
    
    context->rtx_properties = {};
    context->rtx_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    
    VkPhysicalDeviceProperties2 properties = {};
    properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties.pNext = &context->rtx_properties;
    
    vkGetPhysicalDeviceProperties2(context->physical_device, &properties);
    
    GetQueuesId(context);
    CreateVkDevice(context->physical_device, context->graphics_queue_id, context->transfer_queue_id,context->present_queue_id, &context->device);
    
    vkGetDeviceQueue(context->device, context->graphics_queue_id, 0, &context->graphics_queue);
    vkGetDeviceQueue(context->device, context->present_queue_id, 0, &context->present_queue);
    vkGetDeviceQueue(context->device, context->transfer_queue_id, 0, &context->transfer_queue);
    
    VkCommandPoolCreateInfo pool_create_info = {};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.pNext = NULL;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_create_info.queueFamilyIndex = context->graphics_queue_id;
    VkResult result = vkCreateCommandPool(context->device, &pool_create_info, NULL, &context->graphics_command_pool);
    AssertVkResult(result);
    
    context->swapchain.swapchain = VK_NULL_HANDLE; //Not sure if this is required
    CreateOrUpdateSwapchain(context, window, &context->swapchain);
    
    
    mat4 id = mat4_identity();
    CreateBuffer(context, sizeof(mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &context->cam_buffer);
    UploadToBuffer(context->device, &context->cam_buffer, &id, sizeof(mat4));
    DEBUGNameBuffer(context->device, &context->cam_buffer, "Camera Info");
    
    return context;
}

VulkanRenderer *VulkanCreateRenderer(VulkanContext *context) {
    VulkanRenderer *renderer = (VulkanRenderer*)malloc(sizeof(VulkanRenderer));
    CreatePipelineLayout(context->device, renderer);
    CreateRasterPipeline(context->device, renderer->layout, &context->swapchain, &renderer->pipeline, &renderer->render_pass);
    
    // Framebuffers
    renderer->framebuffers = (VkFramebuffer *)calloc(context->swapchain.image_count, sizeof(VkFramebuffer));
    for(u32 i = 0; i < context->swapchain.image_count; ++i) {
        
        VkFramebufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.pNext = NULL;
        create_info.flags = 0;
        create_info.renderPass = renderer->render_pass;
        create_info.attachmentCount = 1;
        create_info.pAttachments = &context->swapchain.image_views[i];
        create_info.width = context->swapchain.extent.width;
        create_info.height = context->swapchain.extent.height;
        create_info.layers = 1;
        AssertVkResult(vkCreateFramebuffer(context->device, &create_info, NULL, &renderer->framebuffers[i]));
        
    }
    
    WriteDescriptorSets(context->device, renderer->descriptor_sets, &context->cam_buffer);
    
    return renderer;
}

void VulkanDestroyRenderer(VulkanContext *context, VulkanRenderer *renderer) {
    
    for(u32 i = 0; i < context->swapchain.image_count; ++i) {
        
        vkDestroyFramebuffer(context->device, renderer->framebuffers[i], NULL);
        
    }
    free(renderer->framebuffers);
    
    vkDestroyRenderPass(context->device, renderer->render_pass, NULL);
    
    vkFreeDescriptorSets(context->device, renderer->descriptor_pool, 1, renderer->descriptor_sets);
    vkDestroyDescriptorPool(context->device, renderer->descriptor_pool, NULL);
    
    vkDestroyDescriptorSetLayout(context->device, renderer->game_set_layout, NULL);
    vkDestroyPipelineLayout(context->device, renderer->layout, NULL);
    vkDestroyPipeline(context->device, renderer->pipeline, NULL);
    
    free(renderer);
}

void VulkanDestroyContext(VulkanContext *context){
    
    vkDeviceWaitIdle(context->device);
    
    DestroyBuffer(context->device, &context->cam_buffer);
    
    DestroySwapchain(context, &context->swapchain);
    vkDestroySwapchainKHR(context->device, context->swapchain.swapchain, NULL);
    vkDestroySurfaceKHR(context->instance, context->surface, NULL);
    
    vkDestroyCommandPool(context->device,context->graphics_command_pool,NULL);
    
    vkDestroyDevice(context->device, NULL);
    pfn_vkDestroyDebugUtilsMessengerEXT(context->instance, debug_messenger, NULL);
    vkDestroyInstance(context->instance, NULL);
    
    free(context);
}