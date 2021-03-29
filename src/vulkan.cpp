#include <SDL/SDL_vulkan.h>

// TODO LIST
//
// Hot reloading of the game
// > Faire un struct avec le mesh et la camera
// > Rebuild le tlas a chaque reload de l'app
// Load Scene
// Staging buffers
// Two descriptor sets one for render image & tlas (managed by context) one for the rest (managed by app)
// 

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
    
    VkDescriptorSet descriptor_sets[2];
    u32 descriptor_set_count;
    
    VkDescriptorSetLayout app_set_layout;
    VkDescriptorSetLayout game_set_layout;
    
    u32 push_constant_size;
    
    Image render_image;
    
    Buffer rgen_sbt;
    Buffer rmiss_sbt;
    Buffer rchit_sbt;
    
    VkStridedDeviceAddressRegionKHR strides[4];
    
} VulkanRenderer;

typedef struct CameraInfo {
    alignas(16) mat4 view;
    alignas(16) mat4 proj;
} CameraInfo;

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
    
    Buffer vertex_buffer;
    Buffer index_buffer;
    
    Buffer BLAS_buffer;
    VkAccelerationStructureKHR BLAS;
    Buffer TLAS_buffer;
    VkAccelerationStructureKHR TLAS;
    Buffer instance_data;
    
    CameraInfo cam_info;
    Buffer cam_buffer;
    
} VulkanContext;

u32 aligned_size(const u32 value, const u32 alignment) {
    
    return (value + alignment - 1) & ~(alignment - 1);
    
}

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

void DestroySwapchainContext(const VulkanContext* context, Swapchain *swapchain) {
    
    vkFreeCommandBuffers(context->device,context->graphics_command_pool,swapchain->image_count, swapchain->command_buffers);
    free(swapchain->command_buffers);
    
    free(swapchain->images);
    
    for(u32 i = 0; i < swapchain->image_count; i++) {
        vkDestroyFence(context->device, swapchain->fences[i], NULL);
        vkDestroySemaphore(context->device, swapchain->image_acquired_semaphore[i], NULL);
        vkDestroySemaphore(context->device, swapchain->render_complete_semaphore[i], NULL);
    }
    free(swapchain->fences);
    free(swapchain->image_acquired_semaphore);
    free(swapchain->render_complete_semaphore);
}

internal void CreateOrUpdateSwapchain(const VulkanContext* context, SDL_Window* window, Swapchain *swapchain) {
    if(swapchain->swapchain != VK_NULL_HANDLE) {
        DestroySwapchainContext(context, swapchain);
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
    
    for(u32 i = 0; i < swapchain->image_count; i++) {
        DEBUGNameObject(context->device, (u64)swapchain->images[i], VK_OBJECT_TYPE_IMAGE, "Swapchain image");
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

//
//
// NOTE(Guigui): PIPELINE
//
//

internal void CreateRtxRenderImage(const VkDevice device, const Swapchain *swapchain, const VkPhysicalDeviceMemoryProperties *memory_properties, Image *image) {
    VkImageCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.imageType = VK_IMAGE_TYPE_2D;
    create_info.format = swapchain->format;
    create_info.extent = {swapchain->extent.width, swapchain->extent.height, 1};
    create_info.mipLevels = 1;
    create_info.arrayLayers = 1;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT ;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices = NULL;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    
    VkResult result = vkCreateImage(device, &create_info, NULL, &image->image);
    AssertVkResult(result);
    
    VkMemoryRequirements requirements = {};
    vkGetImageMemoryRequirements(device, image->image, &requirements);
    
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.allocationSize = requirements.size;
    alloc_info.memoryTypeIndex = FindMemoryType(memory_properties, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    result = vkAllocateMemory(device, &alloc_info, NULL, &image->memory);
    AssertVkResult(result);
    
    result = vkBindImageMemory(device, image->image, image->memory, 0);
    AssertVkResult(result);
    
    VkImageViewCreateInfo imv_ci = {};
    imv_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imv_ci.pNext = NULL;
    imv_ci.flags = 0;
    imv_ci.image = image->image;
    imv_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imv_ci.format = swapchain->format;
    imv_ci.components = {};
    imv_ci.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    result = vkCreateImageView(device, &imv_ci, NULL, &image->image_view);
    AssertVkResult(result);
    DEBUGNameImage(device, image, "RTX Render Image");
}

internal void WriteDescriptorSets(const VkDevice device, const VkDescriptorSet* descriptor_set, const VulkanRenderer* pipeline, const VulkanContext *context) {
    
    VkDescriptorImageInfo img_info =  { 
        VK_NULL_HANDLE, pipeline->render_image.image_view, VK_IMAGE_LAYOUT_GENERAL
    };
    
    VkWriteDescriptorSetAccelerationStructureKHR as_write = { 
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR, NULL, 1, &context->TLAS 
    };
    
    VkWriteDescriptorSet app_writes[2] = {
        { 
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            *descriptor_set,
            0, 0, 1,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &img_info, NULL, NULL
        },
        { 
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, 
            &as_write,
            *descriptor_set,
            1, 0, 1,
            VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            NULL, NULL, NULL
        }
    };
    
    vkUpdateDescriptorSets(device, 2, app_writes, 0, NULL);
    
    u32 game_writes_count = 1;
    VkDescriptorBufferInfo bi_cam = { context->cam_buffer.buffer, 0, VK_WHOLE_SIZE };
    VkWriteDescriptorSet game_writes[1];
    game_writes[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, descriptor_set[1], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, NULL, &bi_cam, NULL};
    
    vkUpdateDescriptorSets(device, game_writes_count, game_writes, 0, NULL);
}

internal void CreatePipelineLayout(const VkDevice device, const Image* result_image, VulkanRenderer *pipeline, GameCode *game) {
    
    // Set Layout
    
    // TODO(Guigui): TEMP
    const u32 mesh_count = 1;
    const u32 material_count = 1;
    
    // Our set layout
    pipeline->descriptor_set_count = 2;
    const u32 app_descriptor_count = 2;
    VkDescriptorSetLayoutBinding app_bindings[app_descriptor_count] = {};
    app_bindings[0] = { 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, NULL };
    app_bindings[1] = { 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, NULL };
    
    VkDescriptorSetLayoutCreateInfo app_set_create_info = {};
    app_set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    app_set_create_info.pNext = NULL;
    app_set_create_info.flags = 0;
    app_set_create_info.bindingCount = app_descriptor_count;
    app_set_create_info.pBindings = app_bindings;
    VkResult result = vkCreateDescriptorSetLayout(device, &app_set_create_info, NULL, &pipeline->app_set_layout);
    AssertVkResult(result);
    
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
    const u32 game_descriptor_count = sizeof(game_bindings) / sizeof(game_bindings[0]);;
    
    VkDescriptorSetLayoutCreateInfo game_set_create_info = {};
    game_set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    game_set_create_info.pNext = NULL;
    game_set_create_info.flags = 0;
    game_set_create_info.bindingCount = game_descriptor_count;
    game_set_create_info.pBindings = game_bindings;
    result = vkCreateDescriptorSetLayout(device, &game_set_create_info, NULL, &pipeline->game_set_layout);
    AssertVkResult(result);
    
    // Descriptor Pool
    // TODO(Guigui): if the game doesn't use one of these types, we get a validation error
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0},
        {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 0}
    };
    const u32 pool_sizes_count = sizeof(pool_sizes) / sizeof(pool_sizes[0]);
    
    // App
    for(u32 d = 0; d < app_descriptor_count; d++) {
        for(u32 i = 0; i < pool_sizes_count ; i ++){
            if(app_bindings[d].descriptorType == pool_sizes[i].type) {
                pool_sizes[i].descriptorCount ++;
            }
        }
    }
    
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
    pool_ci.maxSets = pipeline->descriptor_set_count;
    pool_ci.poolSizeCount = pool_sizes_count;
    pool_ci.pPoolSizes = pool_sizes;
    AssertVkResult(vkCreateDescriptorPool(device, &pool_ci, NULL, &pipeline->descriptor_pool));
    
    // Descriptor Set
    VkDescriptorSetLayout set_layouts[2] = {pipeline->app_set_layout, pipeline->game_set_layout};
    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.pNext = NULL;
    allocate_info.descriptorPool = pipeline->descriptor_pool;
    allocate_info.descriptorSetCount = pipeline->descriptor_set_count;
    allocate_info.pSetLayouts = set_layouts;
    AssertVkResult(vkAllocateDescriptorSets(device, &allocate_info, pipeline->descriptor_sets));
    
    // Push constants
    // TODO(Guigui): Currently limited to 1 PC
    
    VkPushConstantRange push_constant_range = 
    { VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, 0, sizeof(PushConstant) };
    const u32 push_constant_count = 1;
    pipeline->push_constant_size = push_constant_range.size;
    
    VkPipelineLayoutCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.setLayoutCount = pipeline->descriptor_set_count;
    create_info.pSetLayouts = set_layouts;
    create_info.pushConstantRangeCount = push_constant_count;
    create_info.pPushConstantRanges = &push_constant_range;
    
    AssertVkResult(vkCreatePipelineLayout(device, &create_info, NULL, &pipeline->layout));
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

internal void CreatePipeline(const VkDevice device, const VkPipelineLayout *layout, const VkPhysicalDeviceRayTracingPipelinePropertiesKHR* rtx_properties, VkPipeline *pipeline) {
    
    VkRayTracingPipelineCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    create_info.pNext = NULL;
    create_info.flags = 0;
    
    // SHADERS
    const u32 shader_count = 4;
    create_info.stageCount = shader_count;
    VkPipelineShaderStageCreateInfo temp = {};
    temp.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    temp.pNext = NULL;
    temp.flags = 0;
    temp.pName = "main";
    temp.pSpecializationInfo = NULL;
    
    VkPipelineShaderStageCreateInfo rgen = temp;
    rgen.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    CreateVkShaderModule("resources/shaders/raytrace.rgen.spv", device, &rgen.module);
    
    VkPipelineShaderStageCreateInfo rmiss = temp;
    rmiss.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    CreateVkShaderModule("resources/shaders/raytrace.rmiss.spv", device, &rmiss.module);
    
    VkPipelineShaderStageCreateInfo rmiss_shadow = temp;
    rmiss_shadow.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    CreateVkShaderModule("resources/shaders/shadow.rmiss.spv", device, &rmiss_shadow.module);
    
    VkPipelineShaderStageCreateInfo rchit = temp;
    rchit.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    CreateVkShaderModule("resources/shaders/raytrace.rchit.spv", device, &rchit.module);
    
    VkPipelineShaderStageCreateInfo stages[shader_count] = {
        rgen, rmiss, rmiss_shadow, rchit
    };
    create_info.pStages = stages;
    
    // GROUPS
    create_info.groupCount = shader_count;
    VkRayTracingShaderGroupCreateInfoKHR group_temp = {}; // Used as template
    group_temp.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    group_temp.pNext = NULL;
    group_temp.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group_temp.generalShader = VK_SHADER_UNUSED_KHR;
    group_temp.closestHitShader = VK_SHADER_UNUSED_KHR;
    group_temp.anyHitShader = VK_SHADER_UNUSED_KHR;
    group_temp.intersectionShader = VK_SHADER_UNUSED_KHR;
    group_temp.pShaderGroupCaptureReplayHandle = NULL;
    
    VkRayTracingShaderGroupCreateInfoKHR rgen_group = group_temp;
    rgen_group.generalShader = 0;
    VkRayTracingShaderGroupCreateInfoKHR rmiss_group = group_temp;
    rmiss_group.generalShader = 1;
    VkRayTracingShaderGroupCreateInfoKHR rmiss_shadow_group = group_temp;
    rmiss_shadow_group.generalShader = 2;
    VkRayTracingShaderGroupCreateInfoKHR rchit_group = group_temp;
    rchit_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    rchit_group.closestHitShader = 3;
    VkRayTracingShaderGroupCreateInfoKHR groups[shader_count] = {
        rgen_group, rmiss_group, rmiss_shadow_group, rchit_group
    };
    create_info.pGroups = groups;
    
    create_info.maxPipelineRayRecursionDepth = rtx_properties->maxRayRecursionDepth;
    create_info.pLibraryInfo = NULL;
    create_info.pLibraryInterface = NULL;
    create_info.pDynamicState = NULL;
    create_info.layout = *layout;
    create_info.basePipelineHandle = NULL;
    create_info.basePipelineIndex = 0;
    
    VkResult result = pfn_vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &create_info, NULL, pipeline);
    AssertVkResult(result);
    
    vkDestroyShaderModule(device, rgen.module, NULL);
    vkDestroyShaderModule(device, rmiss.module, NULL);
    vkDestroyShaderModule(device, rmiss_shadow.module, NULL);
    vkDestroyShaderModule(device, rchit.module, NULL);
}

void VulkanReloadShaders(VulkanContext *context, VulkanRenderer *renderer) {
    vkDestroyPipeline(context->device, renderer->pipeline, NULL);
    
    CreatePipeline(context->device, &renderer->layout, &context->rtx_properties, &renderer->pipeline);
}

// =========================
//
// NOTE(Guigui): ACCELERATION STRUCTURES
//
// =========================

internal void CreateBLAS(VulkanContext *context, Buffer *vertex_buffer, const u32 vtx_count, Buffer *index_buffer, const u32 primitive_count) {
    
    VkAccelerationStructureGeometryKHR geometry = {};
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.pNext = NULL;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.geometry.triangles.pNext = NULL;
    geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    geometry.geometry.triangles.vertexData.deviceAddress = vertex_buffer->address;
    geometry.geometry.triangles.vertexStride = sizeof(vec3);
    geometry.geometry.triangles.maxVertex = vtx_count;
    geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    geometry.geometry.triangles.indexData.deviceAddress = index_buffer->address;
    geometry.geometry.triangles.transformData.deviceAddress = VK_NULL_HANDLE;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    
    VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
    build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build_info.pNext = NULL;
    build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build_info.srcAccelerationStructure = VK_NULL_HANDLE;
    build_info.geometryCount = 1;
    build_info.pGeometries = &geometry;
    build_info.ppGeometries = NULL;
    build_info.scratchData.deviceAddress = NULL;
    
    // Query the build size
    VkAccelerationStructureBuildSizesInfoKHR size_info = {};
    size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    pfn_vkGetAccelerationStructureBuildSizesKHR(context->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_info, &primitive_count, &size_info);
    
    //Alloc the scratch buffer
    Buffer scratch;
    CreateBuffer(context, size_info.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &scratch);
    build_info.scratchData.deviceAddress = scratch.address;
    
    //Alloc the blas buffer
    CreateBuffer(context, size_info.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &context->BLAS_buffer);
    DEBUGNameBuffer(context->device, &context->BLAS_buffer, "BLAS");
    
    VkAccelerationStructureCreateInfoKHR blas_ci = {};
    blas_ci.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    blas_ci.pNext = NULL;
    blas_ci.createFlags = 0;
    blas_ci.buffer = context->BLAS_buffer.buffer;
    blas_ci.offset = 0;
    blas_ci.size = size_info.accelerationStructureSize; // Will be queried below
    blas_ci.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    blas_ci.deviceAddress = 0;
    VkResult result = pfn_vkCreateAccelerationStructureKHR(context->device, &blas_ci, NULL, &context->BLAS);
    AssertVkResult(result);
    DEBUGNameObject(context->device, (u64)context->BLAS, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, "BLAS");
    
    
    // Now build it !!
    build_info.dstAccelerationStructure = context->BLAS;
    VkAccelerationStructureBuildRangeInfoKHR range_info = { primitive_count, 0, 0, 0 };
    VkAccelerationStructureBuildRangeInfoKHR* infos[] = { &range_info }; 
    
    VkCommandBuffer cmd_buffer;
    AllocateCommandBuffers(context->device, context->graphics_command_pool, 1, &cmd_buffer);
    BeginCommandBuffer(context->device, cmd_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    pfn_vkCmdBuildAccelerationStructuresKHR(cmd_buffer, 1, &build_info, infos);
    VkMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        NULL, 
        VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR
    };
    
    vkCmdPipelineBarrier(cmd_buffer,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                         0,
                         1, &barrier,
                         0, NULL, 0, NULL);
    vkEndCommandBuffer(cmd_buffer);
    VkSubmitInfo si = {VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, 0, 1, &cmd_buffer, 0, NULL};
    vkQueueSubmit(context->graphics_queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(context->graphics_queue);
    
    vkFreeCommandBuffers(context->device, context->graphics_command_pool, 1, &cmd_buffer);
    DestroyBuffer(context->device, &scratch);
}

internal void CreateTLAS(VulkanContext *context) {
    
    const u32 primitive_count = 1;
    
    mat4 mat = {};
    mat = mat4_identity();
    mat4_transpose(&mat);
    VkTransformMatrixKHR xform = {};
    memcpy(&xform, &mat, sizeof(xform));
    
    VkAccelerationStructureDeviceAddressInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    info.pNext = NULL;
    info.accelerationStructure = context->BLAS;
    
    VkAccelerationStructureInstanceKHR instance = {};
    instance.transform = xform;
    instance.instanceCustomIndex = 0;
    instance.mask = 0xFF;
    instance.instanceShaderBindingTableRecordOffset = 0;
    instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR ;
    instance.accelerationStructureReference = (u64)pfn_vkGetAccelerationStructureDeviceAddressKHR(context->device, &info);
    
    CreateBuffer(context, sizeof(VkAccelerationStructureInstanceKHR),  VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &context->instance_data);
    DEBUGNameBuffer(context->device, &context->instance_data, "Instance Data");
    UploadToBuffer(context->device, &context->instance_data, &instance, sizeof(VkAccelerationStructureInstanceKHR));
    
    VkAccelerationStructureGeometryKHR geometry = {};
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.pNext = NULL;
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.pNext = NULL;
    geometry.geometry.instances.arrayOfPointers = VK_FALSE;
    geometry.geometry.instances.data.deviceAddress = context->instance_data.address;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    
    VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
    build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build_info.pNext = NULL;
    build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR ;
    build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build_info.srcAccelerationStructure = VK_NULL_HANDLE;
    build_info.dstAccelerationStructure = VK_NULL_HANDLE; // Set below
    build_info.geometryCount = 1;
    build_info.pGeometries = &geometry;
    build_info.ppGeometries = NULL;
    build_info.scratchData.deviceAddress = NULL; // Set below
    
    //Get size info
    VkAccelerationStructureBuildSizesInfoKHR size_info = {};
    size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    pfn_vkGetAccelerationStructureBuildSizesKHR(context->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_info, &primitive_count, &size_info);
    CreateBuffer(context, size_info.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &context->TLAS_buffer);
    DEBUGNameBuffer(context->device, &context->TLAS_buffer, "TLAS");
    Buffer scratch;
    CreateBuffer(context, size_info.buildScratchSize,  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &scratch);
    
    VkAccelerationStructureCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    create_info.pNext = NULL;
    create_info.createFlags = 0;
    create_info.buffer = context->TLAS_buffer.buffer;
    create_info.offset = 0;
    create_info.size = size_info.accelerationStructureSize; 
    create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    create_info.deviceAddress = 0;
    AssertVkResult(pfn_vkCreateAccelerationStructureKHR(context->device, &create_info, NULL, &context->TLAS));
    DEBUGNameObject(context->device, (u64)context->TLAS, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, "TLAS");
    
    // Now build it
    build_info.dstAccelerationStructure = context->TLAS;
    build_info.scratchData.deviceAddress = scratch.address;
    
    VkAccelerationStructureBuildRangeInfoKHR range_info = { primitive_count, 0, 0, 0 };
    VkAccelerationStructureBuildRangeInfoKHR* infos[] = { &range_info }; 
    
    vkDeviceWaitIdle(context->device);
    
    VkCommandBuffer cmd_buffer;
    AllocateCommandBuffers(context->device, context->graphics_command_pool, 1, &cmd_buffer);
    BeginCommandBuffer(context->device, cmd_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    
    VkMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        NULL, 
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR
    };
    
    vkCmdPipelineBarrier(cmd_buffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                         0,
                         1, &barrier,
                         0, NULL, 0, NULL);
    
    pfn_vkCmdBuildAccelerationStructuresKHR(cmd_buffer, 1, &build_info, infos);
    vkEndCommandBuffer(cmd_buffer);
    
    VkSubmitInfo si = {VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, 0, 1, &cmd_buffer, 0, NULL};
    vkQueueSubmit(context->graphics_queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(context->graphics_queue);
    
    vkFreeCommandBuffers(context->device, context->graphics_command_pool, 1, &cmd_buffer);
    DestroyBuffer(context->device, &scratch);
}

// =========================
//
// NOTE(Guigui): RENDERING
//
// =========================

void VulkanDrawFrame(VulkanContext* context, VulkanRenderer *pipeline) {
    u32 image_id;
    Swapchain* swapchain = &context->swapchain;
    
    VkResult result;
    result = vkAcquireNextImageKHR(context->device, swapchain->swapchain, UINT64_MAX, swapchain->image_acquired_semaphore[swapchain->semaphore_id],VK_NULL_HANDLE, &image_id);
    AssertVkResult(result);// TODO(Guigui): Recreate swapchain if Suboptimal or outofdate;
    
    // If the frame hasn't finished rendering wait for it to finish
    result = vkWaitForFences(context->device, 1, &swapchain->fences[image_id], VK_TRUE, UINT64_MAX);
    AssertVkResult(result);
    vkResetFences(context->device, 1, &swapchain->fences[image_id]);
    
    VkCommandBuffer cmd = swapchain->command_buffers[image_id];
    
    const VkCommandBufferBeginInfo begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, 0, NULL };
    result = vkBeginCommandBuffer(cmd, &begin_info);
    AssertVkResult(result);
    
    VkImageMemoryBarrier img_barrier = {};
    img_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    img_barrier.pNext = NULL;
    img_barrier.srcAccessMask = 0;
    img_barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    img_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    img_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    img_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_barrier.image = pipeline->render_image.image;
    img_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0, 0,NULL, 0, NULL, 1, &img_barrier);
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline->pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline->layout, 0, pipeline->descriptor_set_count, pipeline->descriptor_sets, 0, 0);
    vkCmdPushConstants(cmd, pipeline->layout, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
                       0, pipeline->push_constant_size, &context->push_constants);
    
    pfn_vkCmdTraceRaysKHR(cmd, &pipeline->strides[0], &pipeline->strides[1], &pipeline->strides[2], &pipeline->strides[3], swapchain->extent.width, swapchain->extent.height, 1);
    
    img_barrier.image = pipeline->render_image.image;
    img_barrier.srcAccessMask = 0;
    img_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    img_barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    img_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0, 0,NULL, 0, NULL, 1, &img_barrier);
    
    img_barrier.image = swapchain->images[image_id];
    img_barrier.srcAccessMask = 0;
    img_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    img_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    img_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0, 0,NULL, 0, NULL, 1, &img_barrier);
    
    VkImageCopy image_copy = {};
    image_copy.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    image_copy.srcOffset = {0, 0, 0};
    image_copy.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    image_copy.dstOffset = {0, 0, 0};
    image_copy.extent = { swapchain->extent.width, swapchain->extent.height, 1};
    vkCmdCopyImage(cmd, pipeline->render_image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapchain->images[image_id], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);
    
    img_barrier.image = pipeline->render_image.image;
    img_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    img_barrier.dstAccessMask = 0;
    img_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    img_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0, 0,NULL, 0, NULL, 1, &img_barrier);
    
    img_barrier.image = swapchain->images[image_id];
    img_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    img_barrier.dstAccessMask = 0;
    img_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; 
    img_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &img_barrier);
    
    result = vkEndCommandBuffer(cmd);
    AssertVkResult(result);
    
    // Submit the queue
    // TODO(Guigui): Record buffers beforehand, no need to do it each frame now
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


// =========================
//
// NOTE(Guigui): ENTRY
//
// =========================

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
    
    context->cam_info.proj = mat4_perspective(90.f, 16.0f/9.0f, 0.1f, 1000.0f);
    context->cam_info.view = mat4_identity();
    mat4_translate(&context->cam_info.view, {0.0f,0.0f,-1.0f});
    
    CreateBuffer(context, sizeof(CameraInfo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &context->cam_buffer);
    UploadToBuffer(context->device, &context->cam_buffer, &context->cam_info, sizeof(CameraInfo));
    DEBUGNameBuffer(context->device, &context->cam_buffer, "Camera Info");
    
    return context;
}

VulkanRenderer *VulkanCreateRenderer(VulkanContext *context, GameCode *game) {
    VulkanRenderer *pipeline = (VulkanRenderer*)malloc(sizeof(VulkanRenderer));
    
    CreateRtxRenderImage(context->device, &context->swapchain, &context->memory_properties,  &pipeline->render_image);
    
    CreatePipelineLayout(context->device, &pipeline->render_image, pipeline, game);
    CreatePipeline(context->device, &pipeline->layout, &context->rtx_properties, &pipeline->pipeline);
    
    // SHADER BINDING TABLE
    {
        const u32 handle_size = context->rtx_properties.shaderGroupHandleSize;
        const u32 handle_size_aligned = aligned_size(context->rtx_properties.shaderGroupHandleSize, context->rtx_properties.shaderGroupHandleAlignment);
        const u32 group_count = 4;
        const u32 sbt_size = group_count * handle_size_aligned;
        
        CreateBuffer(context, handle_size_aligned, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &pipeline->rgen_sbt);
        DEBUGNameBuffer(context->device,  &pipeline->rgen_sbt, "RGEN SBT");
        
        CreateBuffer(context, handle_size_aligned, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &pipeline->rmiss_sbt);
        DEBUGNameBuffer(context->device,  &pipeline->rmiss_sbt, "RMISS SBT");
        
        CreateBuffer(context, handle_size_aligned, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &pipeline->rchit_sbt);
        DEBUGNameBuffer(context->device,  &pipeline->rchit_sbt, "RCHIT SBT");
        
        
        u8* data = (u8*)malloc(sbt_size);
        
        VkResult result = pfn_vkGetRayTracingShaderGroupHandlesKHR(context->device, pipeline->pipeline, 0, group_count, sbt_size, data);
        AssertVkResult(result);
        
        UploadToBuffer(context->device, &pipeline->rgen_sbt, data, handle_size_aligned);
        UploadToBuffer(context->device, &pipeline->rmiss_sbt, data + handle_size_aligned, handle_size_aligned * 2);
        UploadToBuffer(context->device, &pipeline->rchit_sbt, data + (3 * handle_size_aligned), handle_size_aligned);
        
        pipeline->strides[0] ={ pipeline->rgen_sbt.address, handle_size_aligned, handle_size_aligned };
        pipeline->strides[1] ={ pipeline->rmiss_sbt.address, handle_size_aligned, handle_size_aligned };
        pipeline->strides[2] ={ pipeline->rchit_sbt.address, handle_size_aligned, handle_size_aligned };
        pipeline->strides[3] ={ 0u, 0u, 0u };
        free(data);
    }
    
    u32 vtx_count = 0;
    u32 idx_count = 0;
    game->GetScene(&vtx_count, NULL, &idx_count, NULL);
    vec3 *vertices = (vec3 *)calloc(vtx_count, sizeof(vec3));
    u32 *indices = (u32 *)calloc(idx_count, sizeof(u32));
    game->GetScene(&vtx_count, vertices, &idx_count, indices);
    
    const u32 primitive_count = idx_count / 3;
    
    CreateBuffer(context, sizeof(vec3) * vtx_count, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &context->vertex_buffer);
    DEBUGNameBuffer(context->device, &context->vertex_buffer, "Vertices");
    UploadToBuffer(context->device, &context->vertex_buffer, (void*)vertices, sizeof(vec3) * vtx_count);
    
    CreateBuffer(context, sizeof(u32) * idx_count, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &context->index_buffer);
    DEBUGNameBuffer(context->device, &context->index_buffer, "Indices");
    UploadToBuffer(context->device, &context->index_buffer, (void*)indices, sizeof(u32) * idx_count);
    
    free(vertices);
    free(indices);
    
    CreateBLAS(context, &context->vertex_buffer, vtx_count, &context->index_buffer, primitive_count);
    CreateTLAS(context);
    
    WriteDescriptorSets(context->device, pipeline->descriptor_sets, pipeline, context);
    
    return pipeline;
    
}

void VulkanDestroyRenderer(VulkanContext *context, VulkanRenderer *pipeline) {
    
    DestroyBuffer(context->device, &context->TLAS_buffer);
    pfn_vkDestroyAccelerationStructureKHR(context->device, context->TLAS, NULL);
    DestroyBuffer(context->device, &context->instance_data);
    
    DestroyBuffer(context->device, &context->vertex_buffer);
    DestroyBuffer(context->device, &context->index_buffer);
    
    DestroyBuffer(context->device, &context->BLAS_buffer);
    pfn_vkDestroyAccelerationStructureKHR(context->device, context->BLAS, NULL);
    
    DestroyBuffer(context->device, &pipeline->rgen_sbt);
    DestroyBuffer(context->device, &pipeline->rmiss_sbt);
    DestroyBuffer(context->device, &pipeline->rchit_sbt);
    DestroyImage(context->device, &pipeline->render_image);
    
    vkFreeDescriptorSets(context->device, pipeline->descriptor_pool, 2, pipeline->descriptor_sets);
    vkDestroyDescriptorPool(context->device, pipeline->descriptor_pool, NULL);
    
    vkDestroyDescriptorSetLayout(context->device, pipeline->app_set_layout, NULL);
    vkDestroyDescriptorSetLayout(context->device, pipeline->game_set_layout, NULL);
    vkDestroyPipelineLayout(context->device, pipeline->layout, NULL);
    vkDestroyPipeline(context->device, pipeline->pipeline, NULL);
    
    free(pipeline);
    
}

void VulkanDestroyContext(VulkanContext *context){
    
    vkDeviceWaitIdle(context->device);
    
    DestroyBuffer(context->device, &context->cam_buffer);
    
    DestroySwapchainContext(context, &context->swapchain);
    vkDestroySwapchainKHR(context->device, context->swapchain.swapchain, NULL);
    vkDestroySurfaceKHR(context->instance, context->surface, NULL);
    
    vkDestroyCommandPool(context->device,context->graphics_command_pool,NULL);
    
    vkDestroyDevice(context->device, NULL);
    pfn_vkDestroyDebugUtilsMessengerEXT(context->instance, debug_messenger, NULL);
    vkDestroyInstance(context->instance, NULL);
    
    free(context);
}