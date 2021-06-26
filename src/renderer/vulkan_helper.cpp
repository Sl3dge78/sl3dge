/* date = April 24th 2021 6:29 pm */

#ifndef VULKAN_HELPER_CPP
#define VULKAN_HELPER_CPP

#include <vulkan/vulkan.h>
#include <SDL/SDL.h>
#include <sl3dge/sl3dge.h>

#include "vulkan_types.h"

#define DECL_FUNC(name) static PFN_##name pfn_##name
#define LOAD_INSTANCE_FUNC(instance, name)                                                         \
    pfn_##name = (PFN_##name)vkGetInstanceProcAddr(instance, #name);                               \
    ASSERT(pfn_##name);
#define LOAD_DEVICE_FUNC(name)                                                                     \
    pfn_##name = (PFN_##name)vkGetDeviceProcAddr(device, #name);                                   \
    ASSERT(pfn_##name);

DECL_FUNC(vkSetDebugUtilsObjectNameEXT);
DECL_FUNC(vkGetBufferDeviceAddressKHR);

internal void AssertVkResult(VkResult result) {
    if(result < 0) {
        switch(result) {
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            SDL_LogError(0,
                         "VK_ERROR_EXTENSION_"
                         "NOT_PRESENT");
            break;
        case VK_ERROR_LAYER_NOT_PRESENT:
            SDL_LogError(0,
                         "VK_ERROR_LAYER_"
                         "NOT_PRESENT");
            break;
        case VK_ERROR_DEVICE_LOST: {
            SDL_LogError(0, "VK_ERROR_DEVICE_LOST");
        } break;
        case VK_ERROR_OUT_OF_POOL_MEMORY: {
            SDL_LogError(0,
                         "VK_ERROR_OUT_OF_"
                         "POOL_MEMORY");
        } break;
        case VK_ERROR_INITIALIZATION_FAILED: {
            SDL_LogError(0,
                         "VK_ERROR_INITIALIZATION_"
                         "FAILED");
        } break;
        case VK_ERROR_OUT_OF_DATE_KHR: {
            SDL_LogError(0,
                         "VK_ERROR_OUT_OF_"
                         "DATE_KHR");
        } break;
        default:
            SDL_LogError(0,
                         "Unhandled Vulkan error "
                         ": %d",
                         result);
            break;
        }
        ASSERT(false);
        exit(result);
    }
}

// This allocates memory, free it!
internal u32 *AllocateAndLoadBinary(const char *path, i64 *file_size) {
    FILE *file;
    fopen_s(&file, path, "rb");
    if(!file) {
        SDL_LogError(0, "Unable to open file");
        SDL_LogError(0, path);
    }
    // Get the size
    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    rewind(file);

    // Copy into result
    u32 *result = (u32 *)malloc(*file_size);
    fread(result, 1, *file_size, file);

    fclose(file);
    return result;
}

internal void
CreateVkShaderModule(const char *path, VkDevice device, VkShaderModule *shader_module) {
    i64 size = 0;
    u32 *code = AllocateAndLoadBinary(path, &size);

    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.codeSize = size;
    create_info.pCode = code;

    VkResult result = vkCreateShaderModule(device, &create_info, NULL, shader_module);
    free(code);
    AssertVkResult(result);
}

internal void DestroyLayout(VkDevice device, VkDescriptorPool pool, VulkanLayout *layout) {
    vkFreeDescriptorSets(device, pool, layout->descriptor_set_count, &layout->descriptor_set);
    vkDestroyDescriptorSetLayout(device, layout->set_layout, NULL);
    vkDestroyPipelineLayout(device, layout->layout, NULL);
}

VKAPI_ATTR VkBool32 VKAPI_CALL
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageTypes,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
    if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        SDL_LogInfo(0, pCallbackData->pMessage);
    } else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        SDL_LogWarn(0, pCallbackData->pMessage);
    } else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        SDL_LogError(0, pCallbackData->pMessage);
        KEEP_CONSOLE_OPEN(true);
    }
    return VK_FALSE;
}

internal void DEBUGPrintInstanceExtensions() {
    u32 property_count = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &property_count, NULL);

    VkExtensionProperties *extensions =
        (VkExtensionProperties *)calloc(property_count, sizeof(VkExtensionProperties));
    vkEnumerateInstanceExtensionProperties(NULL, &property_count, extensions);

    SDL_Log("Available extensions :");
    for(u32 i = 0; i < property_count; i++) {
        SDL_Log("%s", extensions[i].extensionName);
    }
}

internal void DEBUGNameObject(const VkDevice device,
                              const u64 object,
                              const VkObjectType type,
                              const char *name) {
    VkDebugUtilsObjectNameInfoEXT name_info = {};
    name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    name_info.pNext = NULL;
    name_info.objectType = type;
    name_info.objectHandle = object;
    name_info.pObjectName = name;
    pfn_vkSetDebugUtilsObjectNameEXT(device, &name_info);
}

internal i32 FindMemoryType(const VkPhysicalDeviceMemoryProperties *memory_properties,
                            const u32 type,
                            const VkMemoryPropertyFlags flags) {
    for(u32 i = 0; i < memory_properties->memoryTypeCount; i++) {
        if(type & (1 << i)) {
            if((memory_properties->memoryTypes[i].propertyFlags & flags) == flags) {
                return i;
            }
        }
    }
    ASSERT(0); // TODO(Guigui): Handle this more cleanly
    return -1;
}

internal void DEBUGNameBuffer(const VkDevice device, Buffer *buffer, const char *name) {
    DEBUGNameObject(device, (u64)buffer->buffer, VK_OBJECT_TYPE_BUFFER, name);
    DEBUGNameObject(device, (u64)buffer->memory, VK_OBJECT_TYPE_DEVICE_MEMORY, name);
}

internal void CreateBuffer(const VkDevice device,
                           VkPhysicalDeviceMemoryProperties *memory_properties,
                           const VkDeviceSize size,
                           const VkBufferUsageFlags buffer_usage,
                           const VkMemoryPropertyFlags memory_flags,
                           Buffer *buffer) {
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

        VkResult result = vkCreateBuffer(device, &buffer_ci, NULL, &buffer->buffer);
        AssertVkResult(result);
    }
    // Allocate the memory
    {
        VkMemoryRequirements requirements = {};
        vkGetBufferMemoryRequirements(device, buffer->buffer, &requirements);

        VkMemoryAllocateFlagsInfo flags_info = {};
        flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        flags_info.pNext = NULL;
        flags_info.flags = 0;
        flags_info.deviceMask = 0;
        if(buffer_usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR)
            flags_info.flags |= VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = &flags_info;
        alloc_info.allocationSize = requirements.size;
        alloc_info.memoryTypeIndex =
            FindMemoryType(memory_properties, requirements.memoryTypeBits, memory_flags);
        VkResult result = vkAllocateMemory(device, &alloc_info, NULL, &buffer->memory);
        AssertVkResult(result);
    }
    // Bind the buffer to the memory
    VkResult result = vkBindBufferMemory(device, buffer->buffer, buffer->memory, 0);
    AssertVkResult(result);

    if(buffer_usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR) {
        // Get its address
        VkBufferDeviceAddressInfo dai = {
            VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, NULL, buffer->buffer};
        buffer->address = pfn_vkGetBufferDeviceAddressKHR(device, &dai);
    }
}

internal inline void
UploadToBuffer(const VkDevice device, Buffer *buffer, void *data, size_t size) {
    void *dst;
    VkResult result = vkMapMemory(device, buffer->memory, 0, buffer->size, 0, &dst);
    AssertVkResult(result);
    memcpy(dst, data, size);
    vkUnmapMemory(device, buffer->memory);
}

internal void MapBuffer(const VkDevice device, Buffer *buffer, void **data) {
    AssertVkResult(vkMapMemory(device, buffer->memory, 0, buffer->size, 0, data));
}

internal void UnmapBuffer(const VkDevice device, Buffer *buffer) {
    vkUnmapMemory(device, buffer->memory);
}

internal void DestroyBuffer(const VkDevice device, Buffer *buffer) {
    vkFreeMemory(device, buffer->memory, NULL);
    vkDestroyBuffer(device, buffer->buffer, NULL);
    buffer = {};
}

internal void DEBUGNameImage(const VkDevice device, Image *image, const char *name) {
    DEBUGNameObject(device, (u64)image->image, VK_OBJECT_TYPE_IMAGE, name);
    DEBUGNameObject(device, (u64)image->memory, VK_OBJECT_TYPE_DEVICE_MEMORY, name);
    DEBUGNameObject(device, (u64)image->image_view, VK_OBJECT_TYPE_IMAGE_VIEW, name);
}

internal void CreateImage(const VkDevice device,
                          const VkPhysicalDeviceMemoryProperties *memory_properties,
                          const VkFormat format,
                          const VkExtent2D extent,
                          const VkImageUsageFlags usage,
                          const VkMemoryPropertyFlags memory_flags,
                          Image *image) {
    VkImageCreateInfo image_ci = {};
    image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.pNext = NULL;
    image_ci.flags = 0;
    image_ci.imageType = VK_IMAGE_TYPE_2D;
    image_ci.format = format;
    image_ci.extent = {extent.width, extent.height, 1};
    image_ci.mipLevels = 1;
    image_ci.arrayLayers = 1;
    image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
    image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_ci.usage = usage;
    image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_ci.queueFamilyIndexCount = 0;
    image_ci.pQueueFamilyIndices = 0;
    image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    AssertVkResult(vkCreateImage(device, &image_ci, NULL, &image->image));

    VkMemoryRequirements requirements = {};
    vkGetImageMemoryRequirements(device, image->image, &requirements);

    VkMemoryAllocateFlagsInfo flags_info = {};
    flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    flags_info.pNext = NULL;
    flags_info.flags = 0;
    flags_info.deviceMask = 0;

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = &flags_info;
    alloc_info.allocationSize = requirements.size;
    alloc_info.memoryTypeIndex =
        FindMemoryType(memory_properties, requirements.memoryTypeBits, memory_flags);
    AssertVkResult(vkAllocateMemory(device, &alloc_info, NULL, &image->memory));

    AssertVkResult(vkBindImageMemory(device, image->image, image->memory, 0));

    VkImageViewCreateInfo image_view_ci = {};
    image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_ci.pNext = NULL;
    image_view_ci.flags = 0;
    image_view_ci.image = image->image;
    image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_ci.format = format;
    image_view_ci.components = {VK_COMPONENT_SWIZZLE_IDENTITY,
                                VK_COMPONENT_SWIZZLE_IDENTITY,
                                VK_COMPONENT_SWIZZLE_IDENTITY,
                                VK_COMPONENT_SWIZZLE_IDENTITY};
    if(usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
        image_view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    else
        image_view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_ci.subresourceRange.baseMipLevel = 0;
    image_view_ci.subresourceRange.levelCount = 1;
    image_view_ci.subresourceRange.baseArrayLayer = 0;
    image_view_ci.subresourceRange.layerCount = 1;
    AssertVkResult(vkCreateImageView(device, &image_view_ci, NULL, &image->image_view));
}

internal void CreateMultiSampledImage(const VkDevice device,
                                      const VkPhysicalDeviceMemoryProperties *memory_properties,
                                      const VkFormat format,
                                      const VkExtent3D extent,
                                      const VkImageUsageFlags usage,
                                      const VkMemoryPropertyFlags memory_flags,
                                      VkSampleCountFlagBits sample_count,
                                      Image *image) {
    VkImageCreateInfo image_ci = {};
    image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.pNext = NULL;
    image_ci.flags = 0;
    image_ci.imageType = VK_IMAGE_TYPE_2D;
    image_ci.format = format;
    image_ci.extent = extent;
    image_ci.mipLevels = 1;
    image_ci.arrayLayers = 1;
    image_ci.samples = sample_count;
    image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_ci.usage = usage;
    image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_ci.queueFamilyIndexCount = 0;
    image_ci.pQueueFamilyIndices = 0;
    image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    AssertVkResult(vkCreateImage(device, &image_ci, NULL, &image->image));

    VkMemoryRequirements requirements = {};
    vkGetImageMemoryRequirements(device, image->image, &requirements);

    VkMemoryAllocateFlagsInfo flags_info = {};
    flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    flags_info.pNext = NULL;
    flags_info.flags = 0;
    flags_info.deviceMask = 0;

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = &flags_info;
    alloc_info.allocationSize = requirements.size;
    alloc_info.memoryTypeIndex =
        FindMemoryType(memory_properties, requirements.memoryTypeBits, memory_flags);
    AssertVkResult(vkAllocateMemory(device, &alloc_info, NULL, &image->memory));

    AssertVkResult(vkBindImageMemory(device, image->image, image->memory, 0));

    VkImageViewCreateInfo image_view_ci = {};
    image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_ci.pNext = NULL;
    image_view_ci.flags = 0;
    image_view_ci.image = image->image;
    image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_ci.format = format;
    image_view_ci.components = {VK_COMPONENT_SWIZZLE_IDENTITY,
                                VK_COMPONENT_SWIZZLE_IDENTITY,
                                VK_COMPONENT_SWIZZLE_IDENTITY,
                                VK_COMPONENT_SWIZZLE_IDENTITY};
    if(usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
        image_view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    else
        image_view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_ci.subresourceRange.baseMipLevel = 0;
    image_view_ci.subresourceRange.levelCount = 1;
    image_view_ci.subresourceRange.baseArrayLayer = 0;
    image_view_ci.subresourceRange.layerCount = 1;
    AssertVkResult(vkCreateImageView(device, &image_view_ci, NULL, &image->image_view));
}

internal void CopyBufferToImage(VkCommandBuffer cmd,
                                VkExtent2D extent,
                                u32 pitch,
                                Buffer *image_buffer,
                                Image *image) {
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {extent.width, extent.height, 1};

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = NULL;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image->image;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,
                         0,
                         NULL,
                         0,
                         NULL,
                         1,
                         &barrier);

    vkCmdCopyBufferToImage(
        cmd, image_buffer->buffer, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier barrier2 = {};
    barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier2.pNext = NULL;
    barrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier2.image = image->image;
    barrier2.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0,
                         NULL,
                         0,
                         NULL,
                         1,
                         &barrier2);
}

internal void DestroyImage(const VkDevice device, Image *image) {
    vkDestroyImage(device, image->image, NULL);
    vkDestroyImageView(device, image->image_view, NULL);
    vkFreeMemory(device, image->memory, NULL);
}

// COMMAND BUFFERS
internal void AllocateCommandBuffers(const VkDevice device,
                                     const VkCommandPool pool,
                                     const u32 count,
                                     VkCommandBuffer *command_buffers) {
    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.pNext = NULL;
    allocate_info.commandPool = pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = count;
    AssertVkResult(vkAllocateCommandBuffers(device, &allocate_info, command_buffers));
}

internal void BeginCommandBuffer(const VkDevice device,
                                 const VkCommandBuffer cmd,
                                 VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = NULL;
    begin_info.flags = flags;
    begin_info.pInheritanceInfo = 0;
    AssertVkResult(vkBeginCommandBuffer(cmd, &begin_info));
}

internal void AllocateAndBeginCommandBuffer(const VkDevice device,
                                            const VkCommandPool pool,
                                            VkCommandBuffer *cmd) {
    AllocateCommandBuffers(device, pool, 1, cmd);
    BeginCommandBuffer(device, *cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

internal void EndAndExecuteCommandBuffer(const VkDevice device,
                                         const VkQueue queue,
                                         const VkCommandPool pool,
                                         VkCommandBuffer cmd) {
    vkEndCommandBuffer(cmd);
    VkSubmitInfo si = {VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, 0, 1, &cmd, 0, NULL};
    vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(device, pool, 1, &cmd);
}

#endif // VULKAN_HELPER_CPP
