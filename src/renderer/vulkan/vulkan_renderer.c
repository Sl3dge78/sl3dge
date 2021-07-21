#include <vulkan/vulkan.h>

#include "game.h"
#include "renderer/gltf.c"

#ifdef __WIN32__
#include "renderer/vulkan/vulkan_win32.c"
#endif

#include "renderer/vulkan/vulkan_renderer.h"
#include "renderer/vulkan/vulkan_helper.c"
#include "renderer/vulkan/vulkan_pipeline.c"
#include "renderer/vulkan/vulkan_mesh.c"

global VkDebugUtilsMessengerEXT debug_messenger;

// ========================
//
// CONTEXT
//
// ========================

internal void CreateVkInstance(VkInstance *instance, PlatformAPI *platform) {
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Handmade";
    app_info.applicationVersion = 1;
    app_info.pEngineName = "Simple Lightweight 3D Game Engine";
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.pApplicationInfo = &app_info;

    const char *validation[] = {"VK_LAYER_KHRONOS_validation"};
    create_info.enabledLayerCount = 1;
    create_info.ppEnabledLayerNames = validation;

    // Our extensions
    u32 sl3_count = 1;
    const char *sl3_extensions[1] = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};

    // Get platform extensions
    u32 platform_count = 0;
    PlatformGetInstanceExtensions(&platform_count, NULL);
    const char **platform_extensions = (const char **)sCalloc(platform_count, sizeof(char *));
    PlatformGetInstanceExtensions(&platform_count, platform_extensions);

    u32 total_count = platform_count + sl3_count;

    const char **all_extensions = (const char **)sCalloc(total_count, sizeof(char *));
    memcpy(all_extensions, platform_extensions, platform_count * sizeof(char *));
    memcpy(all_extensions + platform_count, sl3_extensions, sl3_count * sizeof(char *));

    sFree(platform_extensions);

#if defined(_DEBUG)
    sLog("Requested extensions :");
    for(u32 i = 0; i < total_count; i++) {
        sLog("   %s", all_extensions[i]);
    }
#endif

    create_info.enabledExtensionCount = total_count;
    create_info.ppEnabledExtensionNames = all_extensions;

    AssertVkResult(vkCreateInstance(&create_info, NULL, instance));
    sFree(all_extensions);

    { // Create debug messenger
        // TODO : Do that only if we're in debug mode

        // Get the functions
        VK_LOAD_INSTANCE_FUNC(*instance, vkCreateDebugUtilsMessengerEXT);
        VK_LOAD_INSTANCE_FUNC(*instance, vkDestroyDebugUtilsMessengerEXT);
        VK_LOAD_INSTANCE_FUNC(*instance, vkSetDebugUtilsObjectNameEXT);
        VK_LOAD_INSTANCE_FUNC(*instance, vkCmdBeginDebugUtilsLabelEXT);
        VK_LOAD_INSTANCE_FUNC(*instance, vkCmdEndDebugUtilsLabelEXT);

        // Create the messenger
        VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
        debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.pNext = NULL;
        debug_create_info.flags = 0;
        debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback = &DebugCallback;
        debug_create_info.pUserData = NULL;

        pfn_vkCreateDebugUtilsMessengerEXT(*instance, &debug_create_info, NULL, &debug_messenger);
    }
}

internal void CreateVkPhysicalDevice(VkInstance instance, VkPhysicalDevice *physical_device) {
    u32 device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    VkPhysicalDevice *physical_devices =
        (VkPhysicalDevice *)sCalloc(device_count, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &device_count, physical_devices);

    for(u32 i = 0; i < device_count; i++) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
    }
    // TODO : Pick the device according to our specs
    *physical_device = physical_devices[0];
    sFree(physical_devices);
}

internal void LoadDeviceFuncPointers(VkDevice device) {
    VK_LOAD_DEVICE_FUNC(vkCreateRayTracingPipelinesKHR);
    VK_LOAD_DEVICE_FUNC(vkCmdTraceRaysKHR);
    VK_LOAD_DEVICE_FUNC(vkGetRayTracingShaderGroupHandlesKHR);
    VK_LOAD_DEVICE_FUNC(vkGetBufferDeviceAddressKHR);
    VK_LOAD_DEVICE_FUNC(vkCreateAccelerationStructureKHR);
    VK_LOAD_DEVICE_FUNC(vkGetAccelerationStructureBuildSizesKHR);
    VK_LOAD_DEVICE_FUNC(vkCmdBuildAccelerationStructuresKHR);
    VK_LOAD_DEVICE_FUNC(vkDestroyAccelerationStructureKHR);
    VK_LOAD_DEVICE_FUNC(vkGetAccelerationStructureDeviceAddressKHR);
}

internal void GetQueuesId(Renderer *context) {
    // Get queue info
    u32 queue_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(context->physical_device, &queue_count, NULL);
    VkQueueFamilyProperties *queue_properties =
        sCalloc(queue_count, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(
        context->physical_device, &queue_count, queue_properties);

    u32 set_flags = 0;

    for(u32 i = 0; i < queue_count && set_flags != 7; i++) {
        if(!(set_flags & 1) && queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            context->graphics_queue_id = i;
            set_flags |= 1;
            // graphics_queue.count = queue_properties[i].queueCount; // TODO : ??
        }

        if(!(set_flags & 2) && queue_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT &&
           !(queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            context->transfer_queue_id = i;
            set_flags |= 2;
            // transfer_queue.count = queue_properties[i].queueCount; // TODO : ??
        }

        if(!(set_flags & 4)) {
            VkBool32 is_supported = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(
                context->physical_device, i, context->surface, &is_supported);
            if(is_supported) {
                context->present_queue_id = i;
                set_flags |= 4;
            }
        }
    }
    sFree(queue_properties);
}

internal void CreateVkDevice(VkPhysicalDevice physical_device,
                             const u32 graphics_queue,
                             const u32 transfer_queue,
                             const u32 present_queue,
                             VkDevice *device) {
    // Queues
    VkDeviceQueueCreateInfo queues_ci[3] = {0};

    float queue_priority = 1.0f;

    queues_ci[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queues_ci[0].pNext = NULL;
    queues_ci[0].flags = 0;
    queues_ci[0].queueFamilyIndex = graphics_queue;
    queues_ci[0].queueCount = 1;
    queues_ci[0].pQueuePriorities = &queue_priority;

    queues_ci[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queues_ci[1].pNext = NULL;
    queues_ci[1].flags = 0;
    queues_ci[1].queueFamilyIndex = transfer_queue;
    queues_ci[1].queueCount = 1;
    queues_ci[1].pQueuePriorities = &queue_priority;

    queues_ci[2].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queues_ci[2].pNext = NULL;
    queues_ci[2].flags = 0;
    queues_ci[2].queueFamilyIndex = present_queue;
    queues_ci[2].queueCount = 1;
    queues_ci[2].pQueuePriorities = &queue_priority;

    // Extensions
    const char *extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                                VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                                VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
                                VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
                                VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
                                VK_KHR_RAY_QUERY_EXTENSION_NAME,
                                VK_KHR_SHADER_CLOCK_EXTENSION_NAME};
    const u32 extension_count = sizeof(extensions) / sizeof(extensions[0]);

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accel_feature = {0};
    accel_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accel_feature.pNext = NULL;
    accel_feature.accelerationStructure = VK_TRUE;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_feature = {0};
    rt_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rt_feature.pNext = &accel_feature;
    rt_feature.rayTracingPipeline = VK_TRUE;

    VkPhysicalDeviceBufferDeviceAddressFeatures device_address = {0};
    device_address.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    device_address.pNext = &rt_feature;
    device_address.bufferDeviceAddress = VK_TRUE;

    VkPhysicalDeviceRobustness2FeaturesEXT robustness = {0};
    robustness.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
    robustness.pNext = &device_address;
    robustness.robustBufferAccess2 = VK_FALSE;
    robustness.robustImageAccess2 = VK_FALSE;
    robustness.nullDescriptor = VK_TRUE;

    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing = {0};
    descriptor_indexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptor_indexing.pNext = &robustness;
    descriptor_indexing.runtimeDescriptorArray = VK_TRUE;
    descriptor_indexing.descriptorBindingVariableDescriptorCount = VK_TRUE;
    descriptor_indexing.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

    VkPhysicalDeviceFeatures2 features2 = {0};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &descriptor_indexing;
    //features2.features = 0;
    features2.features.samplerAnisotropy = VK_TRUE;
    features2.features.shaderInt64 = VK_TRUE;

    VkDeviceCreateInfo device_create_info = {0};
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
}

internal void CreateSwapchain(const Renderer *context, Swapchain *swapchain) {
    VkSwapchainCreateInfoKHR create_info = {0};

    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.surface = context->surface;

    // Img count
    VkSurfaceCapabilitiesKHR surface_capabilities = {0};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        context->physical_device, context->surface, &surface_capabilities);
    u32 image_count = 3;
    if(surface_capabilities.maxImageCount != 0 &&
       surface_capabilities.maxImageCount < image_count) {
        image_count = surface_capabilities.maxImageCount;
    }
    if(image_count < surface_capabilities.minImageCount) {
        image_count = surface_capabilities.minImageCount;
    }
    create_info.minImageCount = image_count;

    // Format
    u32 format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        context->physical_device, context->surface, &format_count, NULL);
    VkSurfaceFormatKHR *formats =
        (VkSurfaceFormatKHR *)sCalloc(format_count, sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        context->physical_device, context->surface, &format_count, formats);
    VkSurfaceFormatKHR picked_format = formats[0];
    for(u32 i = 0; i < format_count; ++i) {
        VkImageFormatProperties properties;
        VkResult result = vkGetPhysicalDeviceImageFormatProperties(context->physical_device,
                                                                   formats[i].format,
                                                                   VK_IMAGE_TYPE_2D,
                                                                   VK_IMAGE_TILING_OPTIMAL,
                                                                   VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                                       VK_IMAGE_USAGE_STORAGE_BIT,
                                                                   0,
                                                                   &properties);
        if(result != VK_ERROR_FORMAT_NOT_SUPPORTED) {
            picked_format = formats[i];
            break;
        }

        // if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace ==
        // VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        //	picked_format = formats[i];
        //	sLog("Swapchain format: VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR");
        //	break;
        //}
    }
    create_info.imageFormat = picked_format.format;
    create_info.imageColorSpace = picked_format.colorSpace;
    swapchain->format = picked_format.format;
    sFree(formats);

    // Extent
    VkExtent2D extent = {0};
    if(surface_capabilities.currentExtent.width != 0xFFFFFFFF) {
        extent = surface_capabilities.currentExtent;
    } else {
        ASSERT(0);
        i32 w = 1280; // TODO
        i32 h = 720;
        //SDL_Vulkan_GetDrawableSize(window, &w, &h);

        extent.width = w;
        extent.height = h;
    }
    create_info.imageExtent = extent;
    swapchain->extent = extent;

    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices = NULL; // NOTE: Needed only if sharing
                                            // mode is shared
    create_info.preTransform = surface_capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    // Present mode
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; // NOTE: Default present mode
    u32 present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        context->physical_device, context->surface, &present_mode_count, NULL);
    VkPresentModeKHR *present_modes =
        (VkPresentModeKHR *)sCalloc(present_mode_count, sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        context->physical_device, context->surface, &present_mode_count, present_modes);
    for(u32 i = 0; i < present_mode_count; i++) {
        if(present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }
    sFree(present_modes);

    create_info.presentMode = present_mode;

    create_info.clipped = VK_FALSE;
    create_info.oldSwapchain = swapchain->swapchain;

    VkResult result =
        vkCreateSwapchainKHR(context->device, &create_info, NULL, &swapchain->swapchain);
    AssertVkResult(result);

    // Image views
    vkGetSwapchainImagesKHR(context->device, swapchain->swapchain, &swapchain->image_count, NULL);
    swapchain->images = (VkImage *)sCalloc(swapchain->image_count, sizeof(VkImage));
    vkGetSwapchainImagesKHR(
        context->device, swapchain->swapchain, &swapchain->image_count, swapchain->images);

    swapchain->image_views = (VkImageView *)sCalloc(swapchain->image_count, sizeof(VkImageView));

    for(u32 i = 0; i < swapchain->image_count; i++) {
        DEBUGNameObject(
            context->device, (u64)swapchain->images[i], VK_OBJECT_TYPE_IMAGE, "Swapchain image");

        VkImageViewCreateInfo image_view_ci = {0};
        image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_ci.pNext = NULL;
        image_view_ci.flags = 0;
        image_view_ci.image = swapchain->images[i];
        image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_ci.format = swapchain->format;
        image_view_ci.components = (VkComponentMapping){VK_COMPONENT_SWIZZLE_IDENTITY,
                                                        VK_COMPONENT_SWIZZLE_IDENTITY,
                                                        VK_COMPONENT_SWIZZLE_IDENTITY,
                                                        VK_COMPONENT_SWIZZLE_IDENTITY};
        image_view_ci.subresourceRange =
            (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        AssertVkResult(
            vkCreateImageView(context->device, &image_view_ci, NULL, &swapchain->image_views[i]));
    }

    // Command buffers
    swapchain->command_buffers =
        (VkCommandBuffer *)sCalloc(swapchain->image_count, sizeof(VkCommandBuffer));
    VkCommandBufferAllocateInfo cmd_buf_ai = {0};
    cmd_buf_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buf_ai.pNext = NULL;
    cmd_buf_ai.commandPool = context->graphics_command_pool;
    cmd_buf_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buf_ai.commandBufferCount = swapchain->image_count;
    result = vkAllocateCommandBuffers(context->device, &cmd_buf_ai, swapchain->command_buffers);
    AssertVkResult(result);

    // Fences
    swapchain->fences = (VkFence *)sCalloc(swapchain->image_count, sizeof(VkFence));
    VkFenceCreateInfo ci = {
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, VK_FENCE_CREATE_SIGNALED_BIT};

    for(u32 i = 0; i < swapchain->image_count; i++) {
        result = vkCreateFence(context->device, &ci, NULL, &swapchain->fences[i]);
        AssertVkResult(result);
    }

    // Semaphores
    swapchain->image_acquired_semaphore =
        (VkSemaphore *)sCalloc(swapchain->image_count, sizeof(VkSemaphore));
    swapchain->render_complete_semaphore =
        (VkSemaphore *)sCalloc(swapchain->image_count, sizeof(VkSemaphore));
    VkSemaphoreCreateInfo semaphore_ci = {0};
    semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for(u32 i = 0; i < swapchain->image_count; i++) {
        vkCreateSemaphore(
            context->device, &semaphore_ci, NULL, &swapchain->image_acquired_semaphore[i]);
        vkCreateSemaphore(
            context->device, &semaphore_ci, NULL, &swapchain->render_complete_semaphore[i]);
    }
    swapchain->semaphore_id = 0;

    sLog("Swapchain created with %d images", swapchain->image_count);
}

internal void DestroySwapchain(const Renderer *context, Swapchain *swapchain) {
    vkFreeCommandBuffers(context->device,
                         context->graphics_command_pool,
                         swapchain->image_count,
                         swapchain->command_buffers);
    sFree(swapchain->command_buffers);

    sFree(swapchain->images);

    for(u32 i = 0; i < swapchain->image_count; i++) {
        vkDestroyFence(context->device, swapchain->fences[i], NULL);
        vkDestroySemaphore(context->device, swapchain->image_acquired_semaphore[i], NULL);
        vkDestroySemaphore(context->device, swapchain->render_complete_semaphore[i], NULL);
        vkDestroyImageView(context->device, swapchain->image_views[i], NULL);
    }
    sFree(swapchain->image_views);
    sFree(swapchain->fences);
    sFree(swapchain->image_acquired_semaphore);
    sFree(swapchain->render_complete_semaphore);
}

internal void BeginRenderGroup(VkCommandBuffer cmd,
                               const RenderGroup *render_group,
                               VkFramebuffer target,
                               const VkExtent2D extent) {
    VkRenderPassBeginInfo renderpass_begin = {0};
    renderpass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpass_begin.pNext = 0;
    renderpass_begin.renderPass = render_group->render_pass;
    renderpass_begin.framebuffer = target;
    renderpass_begin.renderArea = (VkRect2D){{0, 0}, extent};

    renderpass_begin.clearValueCount = render_group->clear_values_count;
    renderpass_begin.pClearValues = render_group->clear_values;
    vkCmdBeginRenderPass(cmd, &renderpass_begin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, render_group->pipeline);
    vkCmdBindDescriptorSets(cmd,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            render_group->layout,
                            0,
                            render_group->descriptor_set_count,
                            render_group->descriptor_sets,
                            0,
                            NULL);
}

internal void DestroyRenderGroup(Renderer *context, RenderGroup *render_group) {
    vkFreeDescriptorSets(context->device,
                         context->descriptor_pool,
                         render_group->descriptor_set_count,
                         render_group->descriptor_sets);
    sFree(render_group->descriptor_sets);
    for(u32 i = 0; i < render_group->descriptor_set_count; ++i) {
        vkDestroyDescriptorSetLayout(context->device, render_group->set_layouts[i], 0);
    }
    sFree(render_group->set_layouts);

    vkDestroyPipelineLayout(context->device, render_group->layout, 0);

    vkDestroyPipeline(context->device, render_group->pipeline, NULL);

    vkDestroyRenderPass(context->device, render_group->render_pass, 0);

    sFree(render_group->clear_values);
}

internal void CreateShadowMapRenderGroup(Renderer *renderer, RenderGroup *render_group) {
    VkRenderPassCreateInfo render_pass_ci = {0};
    render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_ci.pNext = NULL;
    render_pass_ci.flags = 0;

    VkAttachmentDescription depth_attachment = {0};
    depth_attachment.flags = 0;
    depth_attachment.format = renderer->depth_format;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentDescription attachments[] = {depth_attachment};

    render_pass_ci.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
    render_pass_ci.pAttachments = attachments;

    VkAttachmentReference depth_ref = {0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDependency dependencies[2] = {0};
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

    VkSubpassDescription subpass_desc = {0};
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

    AssertVkResult(
        vkCreateRenderPass(renderer->device, &render_pass_ci, NULL, &render_group->render_pass));

    // Set Layout
    render_group->descriptor_set_count = 1;
    render_group->set_layouts = (VkDescriptorSetLayout *)sCalloc(render_group->descriptor_set_count,
                                                                 sizeof(VkDescriptorSetLayout));
    render_group->descriptor_sets =
        (VkDescriptorSet *)sCalloc(render_group->descriptor_set_count, sizeof(VkDescriptorSet));
    const VkDescriptorSetLayoutBinding bindings[] = {{// CAMERA MATRICES
                                                      0,
                                                      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                      1,
                                                      VK_SHADER_STAGE_VERTEX_BIT,
                                                      NULL}};
    const u32 descriptor_count = sizeof(bindings) / sizeof(bindings[0]);

    VkDescriptorSetLayoutCreateInfo game_set_create_info = {0};
    game_set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    game_set_create_info.pNext = NULL;
    game_set_create_info.flags = 0;
    game_set_create_info.bindingCount = descriptor_count;
    game_set_create_info.pBindings = bindings;
    AssertVkResult(vkCreateDescriptorSetLayout(
        renderer->device, &game_set_create_info, NULL, render_group->set_layouts));

    // Descriptor Set
    VkDescriptorSetAllocateInfo allocate_info = {0};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.pNext = NULL;
    allocate_info.descriptorPool = renderer->descriptor_pool;
    allocate_info.descriptorSetCount = render_group->descriptor_set_count;
    allocate_info.pSetLayouts = render_group->set_layouts;
    AssertVkResult(
        vkAllocateDescriptorSets(renderer->device, &allocate_info, render_group->descriptor_sets));
    DEBUGNameObject(renderer->device,
                    (u64)render_group->descriptor_sets[0],
                    VK_OBJECT_TYPE_DESCRIPTOR_SET,
                    "SHADOW MAP");

    // Push constants
    VkPushConstantRange push_constant_range = {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant)};
    const u32 push_constant_count = 1;
    //render_group->push_constant_size = push_constant_range.size;

    // Layout
    VkPipelineLayoutCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.setLayoutCount = render_group->descriptor_set_count;
    create_info.pSetLayouts = render_group->set_layouts;
    create_info.pushConstantRangeCount = push_constant_count;
    create_info.pPushConstantRanges = &push_constant_range;

    AssertVkResult(
        vkCreatePipelineLayout(renderer->device, &create_info, NULL, &render_group->layout));

    // Writes
    const u32 static_writes_count = 1;
    VkWriteDescriptorSet static_writes[static_writes_count];

    VkDescriptorBufferInfo bi_cam = {renderer->camera_info_buffer.buffer, 0, VK_WHOLE_SIZE};
    static_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    static_writes[0].pNext = NULL;
    static_writes[0].dstSet = render_group->descriptor_sets[0];
    static_writes[0].dstBinding = 0;
    static_writes[0].dstArrayElement = 0;
    static_writes[0].descriptorCount = 1;
    static_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    static_writes[0].pImageInfo = NULL;
    static_writes[0].pBufferInfo = &bi_cam;
    static_writes[0].pTexelBufferView = NULL;

    vkUpdateDescriptorSets(renderer->device, static_writes_count, static_writes, 0, NULL);

    VkGraphicsPipelineCreateInfo pipeline_ci = {0};
    pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_ci.pNext = NULL;
    pipeline_ci.flags = 0;

    VkPipelineShaderStageCreateInfo stages_ci;
    stages_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages_ci.pNext = NULL;
    stages_ci.flags = 0;
    stages_ci.stage = VK_SHADER_STAGE_VERTEX_BIT;
    CreateVkShaderModule("resources/shaders/shadowmap.vert.spv",
                         renderer->device,
                         renderer->platform,
                         &stages_ci.module);
    stages_ci.pName = "main";
    stages_ci.pSpecializationInfo = NULL;

    pipeline_ci.stageCount = 1;
    pipeline_ci.pStages = &stages_ci;

    VkVertexInputBindingDescription vtx_input_binding = {
        0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription vtx_descriptions[] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
    };
    VkPipelineVertexInputStateCreateInfo vertex_input =
        PipelineGetDefaultVertexInputState(&vtx_input_binding, 3, vtx_descriptions);
    pipeline_ci.pVertexInputState = &vertex_input;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
        PipelineGetDefaultInputAssemblyState();
    pipeline_ci.pInputAssemblyState = &input_assembly_state;

    pipeline_ci.pTessellationState = NULL;

    VkViewport viewport = {0.f,
                           0.f,
                           (f32)renderer->shadowmap_extent.width,
                           (f32)renderer->shadowmap_extent.height,
                           0.f,
                           1.f};
    VkRect2D scissor = {{0, 0}, renderer->shadowmap_extent};
    VkPipelineViewportStateCreateInfo viewport_state =
        PipelineGetDefaultViewportState(1, &viewport, 1, &scissor);
    pipeline_ci.pViewportState = &viewport_state;

    VkPipelineRasterizationStateCreateInfo rasterization_state =
        PipelineGetDefaultRasterizationState();
    pipeline_ci.pRasterizationState = &rasterization_state;

    VkPipelineMultisampleStateCreateInfo multisample_state =
        PipelineGetDefaultMultisampleState(VK_SAMPLE_COUNT_1_BIT);
    pipeline_ci.pMultisampleState = &multisample_state;

    VkPipelineDepthStencilStateCreateInfo stencil_state = PipelineGetDefaultDepthStencilState();
    pipeline_ci.pDepthStencilState = &stencil_state;

    pipeline_ci.pColorBlendState = NULL;

    pipeline_ci.pDynamicState = NULL;
    pipeline_ci.layout = render_group->layout;

    pipeline_ci.renderPass = render_group->render_pass;
    pipeline_ci.subpass = 0;
    pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_ci.basePipelineIndex = 0;

    AssertVkResult(vkCreateGraphicsPipelines(
        renderer->device, VK_NULL_HANDLE, 1, &pipeline_ci, NULL, &render_group->pipeline));

    vkDeviceWaitIdle(renderer->device);
    vkDestroyShaderModule(renderer->device, pipeline_ci.pStages[0].module, NULL);

    render_group->clear_values_count = 1;
    render_group->clear_values =
        (VkClearValue *)sCalloc(render_group->clear_values_count, sizeof(VkClearValue));

    render_group->clear_values[0].depthStencil = (VkClearDepthStencilValue){1.0f, 0};
}

internal void CreateMainRenderGroup(Renderer *renderer, RenderGroup *render_group) {
    // TODO : Séparer ca en 2. un avec la texture. et un avec le reste. Bind la texture pour chaque primitive si necessaire
    render_group->descriptor_set_count = 1;
    render_group->set_layouts = (VkDescriptorSetLayout *)sCalloc(render_group->descriptor_set_count,
                                                                 sizeof(VkDescriptorSetLayout));
    render_group->descriptor_sets =
        (VkDescriptorSet *)sCalloc(render_group->descriptor_set_count, sizeof(VkDescriptorSet));

    const VkDescriptorSetLayoutBinding bindings[] = {
        {// CAMERA MATRICES
         0,
         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         1,
         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
         NULL},
        {// MATERIALS
         1,
         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         1,
         VK_SHADER_STAGE_FRAGMENT_BIT,
         NULL},
        {// TEXTURES
         2,
         VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         renderer->textures_count > 0 ? renderer->textures_count : 1, // HACK
         VK_SHADER_STAGE_FRAGMENT_BIT,
         NULL},
        {// SHADOWMAP READ
         3,
         VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         1,
         VK_SHADER_STAGE_FRAGMENT_BIT,
         NULL}};

    const u32 descriptor_count = sizeof(bindings) / sizeof(bindings[0]);

    VkDescriptorSetLayoutCreateInfo game_set_create_info = {0};
    game_set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    game_set_create_info.pNext = NULL;
    game_set_create_info.flags = 0;
    game_set_create_info.bindingCount = descriptor_count;
    game_set_create_info.pBindings = bindings;
    AssertVkResult(vkCreateDescriptorSetLayout(
        renderer->device, &game_set_create_info, NULL, &render_group->set_layouts[0]));

    // Descriptor Set
    VkDescriptorSetAllocateInfo allocate_info = {0};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.pNext = NULL;
    allocate_info.descriptorPool = renderer->descriptor_pool;
    allocate_info.descriptorSetCount = render_group->descriptor_set_count;
    allocate_info.pSetLayouts = render_group->set_layouts;
    AssertVkResult(
        vkAllocateDescriptorSets(renderer->device, &allocate_info, render_group->descriptor_sets));
    DEBUGNameObject(renderer->device,
                    (u64)render_group->descriptor_sets[0],
                    VK_OBJECT_TYPE_DESCRIPTOR_SET,
                    "MAIN SET");
    // Writes
    const u32 static_writes_count = 3;
    VkWriteDescriptorSet static_writes[static_writes_count];

    VkDescriptorBufferInfo bi_cam = {renderer->camera_info_buffer.buffer, 0, VK_WHOLE_SIZE};
    static_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    static_writes[0].pNext = NULL;
    static_writes[0].dstSet = render_group->descriptor_sets[0];
    static_writes[0].dstBinding = 0;
    static_writes[0].dstArrayElement = 0;
    static_writes[0].descriptorCount = 1;
    static_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    static_writes[0].pImageInfo = NULL;
    static_writes[0].pBufferInfo = &bi_cam;
    static_writes[0].pTexelBufferView = NULL;

    VkDescriptorBufferInfo materials = {renderer->mat_buffer.buffer, 0, VK_WHOLE_SIZE};
    static_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    static_writes[1].pNext = NULL;
    static_writes[1].dstSet = render_group->descriptor_sets[0];
    static_writes[1].dstBinding = 1;
    static_writes[1].dstArrayElement = 0;
    static_writes[1].descriptorCount = 1;
    static_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    static_writes[1].pImageInfo = NULL;
    static_writes[1].pBufferInfo = &materials;
    static_writes[1].pTexelBufferView = NULL;

    VkDescriptorImageInfo image_info = {renderer->shadowmap_sampler,
                                        renderer->shadowmap.image_view,
                                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
    static_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    static_writes[2].pNext = NULL;
    static_writes[2].dstSet = render_group->descriptor_sets[0];
    static_writes[2].dstBinding = 3;
    static_writes[2].dstArrayElement = 0;
    static_writes[2].descriptorCount = 1;
    static_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    static_writes[2].pImageInfo = &image_info;
    static_writes[2].pBufferInfo = NULL;
    static_writes[2].pTexelBufferView = NULL;

    vkUpdateDescriptorSets(renderer->device, static_writes_count, static_writes, 0, NULL);

    { // Render pass
        /*
        VkRenderPassCreateInfo render_pass_ci = {};
        render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_ci.pNext = NULL;
        render_pass_ci.flags = 0;

        VkAttachmentDescription color_attachment = {};
        color_attachment.flags = 0;
        color_attachment.format = renderer->swapchain.format;
        color_attachment.samples = renderer->msaa_level;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depth_attachment = {};
        depth_attachment.flags = 0;
        depth_attachment.format = VK_FORMAT_D32_SFLOAT;
        depth_attachment.samples = renderer->msaa_level;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        VkAttachmentDescription attachments[] = {color_attachment, depth_attachment};

        render_pass_ci.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
        render_pass_ci.pAttachments = attachments;

        VkSubpassDescription subpass_desc = {};
        subpass_desc.flags = 0;
        subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_desc.inputAttachmentCount = 0;
        subpass_desc.pInputAttachments = NULL;

        VkAttachmentReference color_ref = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentReference depth_ref = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        subpass_desc.colorAttachmentCount = 1;
        subpass_desc.pColorAttachments = &color_ref;
        subpass_desc.pResolveAttachments = NULL;
        subpass_desc.pDepthStencilAttachment = &depth_ref;
        subpass_desc.preserveAttachmentCount = 0;
        subpass_desc.pPreserveAttachments = NULL;

        render_pass_ci.subpassCount = 1;
        render_pass_ci.pSubpasses = &subpass_desc;
        render_pass_ci.dependencyCount = 0;
        render_pass_ci.pDependencies = NULL;
*/
        VkAttachmentDescription2 color_attachment = {0};
        color_attachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
        color_attachment.pNext = NULL;
        color_attachment.flags = 0;
        color_attachment.format = renderer->swapchain.format;
        color_attachment.samples = renderer->msaa_level;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription2 depth_attachment = {0};
        depth_attachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
        depth_attachment.pNext = NULL;
        depth_attachment.flags = 0;
        depth_attachment.format = renderer->depth_format;
        depth_attachment.samples = renderer->msaa_level;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription2 depth_resolve_attachment = {0};
        depth_resolve_attachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
        depth_resolve_attachment.pNext = NULL;
        depth_resolve_attachment.flags = 0;
        depth_resolve_attachment.format = renderer->depth_format;
        depth_resolve_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depth_resolve_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_resolve_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth_resolve_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_resolve_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_resolve_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_resolve_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentDescription2 attachments[] = {
            color_attachment, depth_attachment, depth_resolve_attachment};

        VkAttachmentReference2 color_attachement_ref = {0};
        color_attachement_ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
        color_attachement_ref.attachment = 0;
        color_attachement_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachement_ref.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        VkAttachmentReference2 depth_attachement_ref = {0};
        depth_attachement_ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
        depth_attachement_ref.attachment = 1;
        depth_attachement_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_attachement_ref.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        VkAttachmentReference2 depth_resolve_attachement_ref = {0};
        depth_resolve_attachement_ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
        depth_resolve_attachement_ref.attachment = 2;
        depth_resolve_attachement_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_resolve_attachement_ref.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        VkSubpassDescriptionDepthStencilResolve depth_resolve = {0};
        depth_resolve.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
        depth_resolve.pNext = NULL;
        depth_resolve.depthResolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
        depth_resolve.stencilResolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
        depth_resolve.pDepthStencilResolveAttachment = &depth_resolve_attachement_ref;

        VkSubpassDescription2 subpasses = {0};
        subpasses.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
        subpasses.pNext = &depth_resolve;
        subpasses.flags = 0;
        subpasses.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses.viewMask = 0;
        subpasses.inputAttachmentCount = 0;
        subpasses.pInputAttachments = NULL;
        subpasses.colorAttachmentCount = 1;
        subpasses.pColorAttachments = &color_attachement_ref;
        subpasses.pResolveAttachments = NULL;
        subpasses.pDepthStencilAttachment = &depth_attachement_ref;
        subpasses.preserveAttachmentCount = 0;
        subpasses.pPreserveAttachments = NULL;

        VkRenderPassCreateInfo2 renderpass_ci = {0};
        renderpass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
        renderpass_ci.pNext = NULL;
        renderpass_ci.flags = 0;
        renderpass_ci.attachmentCount = ARRAY_SIZE(attachments);
        renderpass_ci.pAttachments = attachments;
        renderpass_ci.subpassCount = 1;
        renderpass_ci.pSubpasses = &subpasses;
        renderpass_ci.dependencyCount = 0;
        renderpass_ci.pDependencies = NULL;
        renderpass_ci.correlatedViewMaskCount = 0;
        renderpass_ci.pCorrelatedViewMasks = NULL;

        AssertVkResult(
            vkCreateRenderPass2(renderer->device, &renderpass_ci, 0, &render_group->render_pass));
    }
    { // Build layout
        // Push constants
        VkPushConstantRange push_constant_range = {
            VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant)};
        const u32 push_constant_count = 1;

        // Layout
        VkPipelineLayoutCreateInfo create_info = {0};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        create_info.pNext = NULL;
        create_info.flags = 0;
        create_info.setLayoutCount = render_group->descriptor_set_count;
        create_info.pSetLayouts = render_group->set_layouts;
        create_info.pushConstantRangeCount = push_constant_count;
        create_info.pPushConstantRanges = &push_constant_range;

        AssertVkResult(
            vkCreatePipelineLayout(renderer->device, &create_info, NULL, &render_group->layout));
    }
    PipelineCreateDefault(renderer->device,
                          renderer->platform,
                          "resources/shaders/general.vert.spv",
                          "resources/shaders/general.frag.spv",
                          &renderer->swapchain.extent,
                          renderer->msaa_level,
                          render_group->layout,
                          render_group->render_pass,
                          &render_group->pipeline);

    render_group->clear_values_count = 2;
    render_group->clear_values =
        (VkClearValue *)sCalloc(render_group->clear_values_count, sizeof(VkClearValue));

    render_group->clear_values[0].color = (VkClearColorValue){{0.43f, 0.77f, 0.91f, 0.0f}};
    render_group->clear_values[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};
}

internal void CreateVolumetricRenderGroup(Renderer *renderer, RenderGroup *render_group) {
    { // Descriptors
        render_group->descriptor_set_count = 1;
        render_group->set_layouts = (VkDescriptorSetLayout *)sCalloc(
            render_group->descriptor_set_count, sizeof(VkDescriptorSetLayout));
        render_group->descriptor_sets =
            (VkDescriptorSet *)sCalloc(render_group->descriptor_set_count, sizeof(VkDescriptorSet));

        const VkDescriptorSetLayoutBinding bindings[] = {{// CAMERA MATRICES
                                                          0,
                                                          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                          1,
                                                          VK_SHADER_STAGE_FRAGMENT_BIT,
                                                          NULL},
                                                         {// DEPTH TEXTURE
                                                          1,
                                                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                          1,
                                                          VK_SHADER_STAGE_FRAGMENT_BIT,
                                                          NULL},
                                                         {// SHADOWMAP READ
                                                          2,
                                                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                          1,
                                                          VK_SHADER_STAGE_FRAGMENT_BIT,
                                                          NULL}};
        const u32 descriptor_count = sizeof(bindings) / sizeof(bindings[0]);

        VkDescriptorSetLayoutCreateInfo game_set_create_info = {0};
        game_set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        game_set_create_info.pNext = NULL;
        game_set_create_info.flags = 0;
        game_set_create_info.bindingCount = descriptor_count;
        game_set_create_info.pBindings = bindings;
        AssertVkResult(vkCreateDescriptorSetLayout(
            renderer->device, &game_set_create_info, NULL, &render_group->set_layouts[0]));
        DEBUGNameObject(renderer->device,
                        (u64)render_group->set_layouts[0],
                        VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                        "Volumetric descriptor set layout");

        // Descriptor Set
        VkDescriptorSetAllocateInfo allocate_info = {0};
        allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocate_info.pNext = NULL;
        allocate_info.descriptorPool = renderer->descriptor_pool;
        allocate_info.descriptorSetCount = render_group->descriptor_set_count;
        allocate_info.pSetLayouts = render_group->set_layouts;
        AssertVkResult(vkAllocateDescriptorSets(
            renderer->device, &allocate_info, render_group->descriptor_sets));
        for(u32 i = 0; i < render_group->descriptor_set_count; ++i) {
            DEBUGNameObject(renderer->device,
                            (u64)render_group->descriptor_sets[i],
                            VK_OBJECT_TYPE_DESCRIPTOR_SET,
                            "Volumetric descriptor set");
        }
    }
    { // Writes
        const u32 static_writes_count = 3;
        VkWriteDescriptorSet static_writes[static_writes_count];

        VkDescriptorBufferInfo bi_cam = {renderer->camera_info_buffer.buffer, 0, VK_WHOLE_SIZE};
        static_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        static_writes[0].pNext = NULL;
        static_writes[0].dstSet = render_group->descriptor_sets[0];
        static_writes[0].dstBinding = 0;
        static_writes[0].dstArrayElement = 0;
        static_writes[0].descriptorCount = 1;
        static_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        static_writes[0].pImageInfo = NULL;
        static_writes[0].pBufferInfo = &bi_cam;
        static_writes[0].pTexelBufferView = NULL;

        VkDescriptorImageInfo depth_image_info = {renderer->depth_sampler,
                                                  renderer->resolved_depth_image.image_view,
                                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        static_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        static_writes[1].pNext = NULL;
        static_writes[1].dstSet = render_group->descriptor_sets[0];
        static_writes[1].dstBinding = 1;
        static_writes[1].dstArrayElement = 0;
        static_writes[1].descriptorCount = 1;
        static_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        static_writes[1].pImageInfo = &depth_image_info;
        static_writes[1].pBufferInfo = NULL;
        static_writes[1].pTexelBufferView = NULL;

        VkDescriptorImageInfo image_info = {renderer->shadowmap_sampler,
                                            renderer->shadowmap.image_view,
                                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
        static_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        static_writes[2].pNext = NULL;
        static_writes[2].dstSet = render_group->descriptor_sets[0];
        static_writes[2].dstBinding = 2;
        static_writes[2].dstArrayElement = 0;
        static_writes[2].descriptorCount = 1;
        static_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        static_writes[2].pImageInfo = &image_info;
        static_writes[2].pBufferInfo = NULL;
        static_writes[2].pTexelBufferView = NULL;

        vkUpdateDescriptorSets(renderer->device, static_writes_count, static_writes, 0, NULL);
    }
    { // Render pass

        VkAttachmentDescription previous_attachment = {0};
        previous_attachment.flags = 0;
        previous_attachment.format = renderer->swapchain.format;
        previous_attachment.samples = renderer->msaa_level;
        previous_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        previous_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        previous_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        previous_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        previous_attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        previous_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription resolve_attachment = {0};
        resolve_attachment.flags = 0;
        resolve_attachment.format = renderer->swapchain.format;
        resolve_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        resolve_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resolve_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        resolve_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resolve_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        resolve_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        resolve_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription attachments[] = {previous_attachment, resolve_attachment};

        VkAttachmentReference color_ref[] = {
            {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        };

        VkAttachmentReference resolve_ref = {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpass_desc = {0};
        subpass_desc.flags = 0;
        subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_desc.inputAttachmentCount = 0;
        subpass_desc.pInputAttachments = NULL;
        subpass_desc.colorAttachmentCount = ARRAY_SIZE(color_ref);
        subpass_desc.pColorAttachments = color_ref;
        subpass_desc.pResolveAttachments = &resolve_ref;
        subpass_desc.pDepthStencilAttachment = NULL;
        subpass_desc.preserveAttachmentCount = 0;
        subpass_desc.pPreserveAttachments = NULL;

        VkRenderPassCreateInfo render_pass_ci = {0};
        render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_ci.pNext = NULL;
        render_pass_ci.flags = 0;
        render_pass_ci.attachmentCount = ARRAY_SIZE(attachments);
        render_pass_ci.pAttachments = attachments;
        render_pass_ci.subpassCount = 1;
        render_pass_ci.pSubpasses = &subpass_desc;
        render_pass_ci.dependencyCount = 0;
        render_pass_ci.pDependencies = NULL;

        AssertVkResult(vkCreateRenderPass(
            renderer->device, &render_pass_ci, NULL, &render_group->render_pass));
    }
    { // Pipeline layout
        VkPipelineLayoutCreateInfo create_info = {0};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        create_info.pNext = NULL;
        create_info.flags = 0;
        create_info.setLayoutCount = render_group->descriptor_set_count;
        create_info.pSetLayouts = render_group->set_layouts;
        create_info.pushConstantRangeCount = 0;
        create_info.pPushConstantRanges = NULL;

        AssertVkResult(
            vkCreatePipelineLayout(renderer->device, &create_info, NULL, &render_group->layout));
    }
    { // Pipeline

        VkGraphicsPipelineCreateInfo pipeline_ci;
        pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_ci.pNext = NULL;
        pipeline_ci.flags = 0;

        VkPipelineShaderStageCreateInfo stages_ci[2];
        stages_ci[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages_ci[0].pNext = NULL;
        stages_ci[0].flags = 0;
        stages_ci[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        CreateVkShaderModule("resources/shaders/volumetric.vert.spv",
                             renderer->device,
                             renderer->platform,
                             &stages_ci[0].module);
        stages_ci[0].pName = "main";
        stages_ci[0].pSpecializationInfo = NULL;

        stages_ci[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages_ci[1].pNext = NULL;
        stages_ci[1].flags = 0;
        stages_ci[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        CreateVkShaderModule("resources/shaders/volumetric.frag.spv",
                             renderer->device,
                             renderer->platform,
                             &stages_ci[1].module);
        stages_ci[1].pName = "main";
        stages_ci[1].pSpecializationInfo = NULL;
        pipeline_ci.stageCount = 2;

        pipeline_ci.pStages = stages_ci;

        VkPipelineVertexInputStateCreateInfo vertex_input =
            PipelineGetDefaultVertexInputState(NULL, 0, NULL);
        vertex_input.vertexBindingDescriptionCount = 0;
        vertex_input.vertexAttributeDescriptionCount = 0;
        pipeline_ci.pVertexInputState = &vertex_input;

        VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
            PipelineGetDefaultInputAssemblyState();
        pipeline_ci.pInputAssemblyState = &input_assembly_state;

        VkViewport viewport;
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = renderer->swapchain.extent.width;
        viewport.height = renderer->swapchain.extent.height;
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        VkRect2D scissor;
        scissor.offset = (VkOffset2D){0, 0};
        scissor.extent = renderer->swapchain.extent;
        VkPipelineViewportStateCreateInfo viewport_state =
            PipelineGetDefaultViewportState(1, &viewport, 1, &scissor);
        pipeline_ci.pViewportState = &viewport_state;

        VkPipelineRasterizationStateCreateInfo rasterization_state =
            PipelineGetDefaultRasterizationState();
        pipeline_ci.pRasterizationState = &rasterization_state;

        VkPipelineMultisampleStateCreateInfo multisample_state =
            PipelineGetDefaultMultisampleState(renderer->msaa_level);
        pipeline_ci.pMultisampleState = &multisample_state;

        VkPipelineDepthStencilStateCreateInfo depth_state = PipelineGetDefaultDepthStencilState();
        depth_state.depthWriteEnable = VK_FALSE;
        depth_state.depthTestEnable = VK_FALSE;
        pipeline_ci.pDepthStencilState = &depth_state;

        VkPipelineColorBlendAttachmentState color_blend_attachement = {0};
        color_blend_attachement.blendEnable = VK_TRUE;
        color_blend_attachement.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_attachement.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachement.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachement.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_attachement.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachement.alphaBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachement.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo color_blend_state =
            PipelineGetDefaultColorBlendState(1, &color_blend_attachement);
        pipeline_ci.pColorBlendState = &color_blend_state;

        pipeline_ci.pDynamicState = NULL; // TODO: look at this
        pipeline_ci.layout = render_group->layout;

        pipeline_ci.renderPass = render_group->render_pass;
        pipeline_ci.subpass = 0;
        pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_ci.basePipelineIndex = 0;

        // TODO: handle pipeline caching
        AssertVkResult(vkCreateGraphicsPipelines(
            renderer->device, VK_NULL_HANDLE, 1, &pipeline_ci, NULL, &render_group->pipeline));

        vkDeviceWaitIdle(renderer->device);
        vkDestroyShaderModule(renderer->device, pipeline_ci.pStages[0].module, NULL);
        vkDestroyShaderModule(renderer->device, pipeline_ci.pStages[1].module, NULL);
    }
    render_group->clear_values_count = 1;
    render_group->clear_values =
        (VkClearValue *)sCalloc(render_group->clear_values_count, sizeof(VkClearValue));

    render_group->clear_values[0].color = (VkClearColorValue){{0.f, 0.0f, 0.0f, 0.0f}};
}

// Creates the swapchain and other resources that are screen resolution dependent
void CreateScreenResources(Renderer *renderer, PlatformWindow *window) {
    // Swapchain
    renderer->swapchain.swapchain = VK_NULL_HANDLE;
    CreateSwapchain(renderer, &renderer->swapchain);

    { // Depth image
        CreateMultiSampledImage(renderer->device,
                                &renderer->memory_properties,
                                renderer->depth_format,
                                renderer->swapchain.extent,
                                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                renderer->msaa_level,
                                &renderer->depth_image);
        DEBUGNameImage(renderer->device, &renderer->depth_image, "DEPTH IMAGE");

        CreateImage(renderer->device,
                    &renderer->memory_properties,
                    renderer->depth_format,
                    renderer->swapchain.extent,
                    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    &renderer->resolved_depth_image);
        DEBUGNameImage(renderer->device, &renderer->resolved_depth_image, "RESOLVED DEPTH IMAGE");

        // MSAA Image
        CreateMultiSampledImage(renderer->device,
                                &renderer->memory_properties,
                                renderer->swapchain.format,
                                renderer->swapchain.extent,
                                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                renderer->msaa_level,
                                &renderer->msaa_image);
        DEBUGNameImage(renderer->device, &renderer->msaa_image, "MSAA RENDER TGT");
    }

    { // Main
        CreateMainRenderGroup(renderer, &renderer->main_render_group);
        CreateMultiSampledImage(renderer->device,
                                &renderer->memory_properties,
                                renderer->swapchain.format,
                                renderer->swapchain.extent,
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                renderer->msaa_level,
                                &renderer->color_pass_image);
        VkFramebufferCreateInfo ci = {0};
        ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        ci.pNext = NULL;
        ci.flags = 0;
        ci.renderPass = renderer->main_render_group.render_pass;

        VkImageView attachments[] = {renderer->color_pass_image.image_view,
                                     renderer->depth_image.image_view,
                                     renderer->resolved_depth_image.image_view};

        ci.attachmentCount = ARRAY_SIZE(attachments);
        ci.pAttachments = attachments;
        ci.width = renderer->swapchain.extent.width;
        ci.height = renderer->swapchain.extent.height;
        ci.layers = 1;
        AssertVkResult(
            vkCreateFramebuffer(renderer->device, &ci, NULL, &renderer->color_pass_framebuffer));
    }
    { // Volumetric
        CreateVolumetricRenderGroup(renderer, &renderer->volumetric_render_group);
        renderer->framebuffers =
            (VkFramebuffer *)sCalloc(renderer->swapchain.image_count, sizeof(VkFramebuffer));

        for(u32 i = 0; i < renderer->swapchain.image_count; ++i) {
            VkFramebufferCreateInfo create_info = {0};
            create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            create_info.pNext = NULL;
            create_info.flags = 0;
            create_info.renderPass = renderer->volumetric_render_group.render_pass;

            VkImageView attachments[] = {renderer->color_pass_image.image_view,
                                         renderer->swapchain.image_views[i]};

            create_info.attachmentCount = ARRAY_SIZE(attachments);
            create_info.pAttachments = attachments;
            create_info.width = renderer->swapchain.extent.width;
            create_info.height = renderer->swapchain.extent.height;
            create_info.layers = 1;
            AssertVkResult(vkCreateFramebuffer(
                renderer->device, &create_info, NULL, &renderer->framebuffers[i]));
        }
    }

    renderer->camera_info.proj = mat4_perspective(
        70.0f, renderer->swapchain.extent.width / renderer->swapchain.extent.height, 0.1f, 1000.0f);
    sLog("%d, %d", renderer->swapchain.extent.width, renderer->swapchain.extent.height);
    mat4_inverse(&renderer->camera_info.proj, &renderer->camera_info.proj_inverse);
}

void DestroyScreenResources(Renderer *renderer) {
    // Volumetric render group
    DestroyRenderGroup(renderer, &renderer->volumetric_render_group);
    for(u32 i = 0; i < renderer->swapchain.image_count; ++i) {
        vkDestroyFramebuffer(renderer->device, renderer->framebuffers[i], NULL);
    }
    sFree(renderer->framebuffers);

    // Color Pass
    vkDestroyFramebuffer(renderer->device, renderer->color_pass_framebuffer, NULL);
    DestroyImage(renderer->device, &renderer->color_pass_image);
    DestroyRenderGroup(renderer, &renderer->main_render_group);

    DestroyImage(renderer->device, &renderer->depth_image);
    DestroyImage(renderer->device, &renderer->resolved_depth_image);

    DestroyImage(renderer->device, &renderer->msaa_image);

    DestroySwapchain(renderer, &renderer->swapchain);
    vkDestroySwapchainKHR(renderer->device, renderer->swapchain.swapchain, NULL);
    vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
}

Renderer *RendererCreate(PlatformWindow *window, PlatformAPI *platform_api) {
    Renderer *renderer = (Renderer *)sMalloc(sizeof(Renderer));
    renderer->platform = platform_api;

    CreateVkInstance(&renderer->instance, platform_api);
    PlatformCreateVkSurface(renderer->instance, window, &renderer->surface);
    CreateVkPhysicalDevice(renderer->instance, &renderer->physical_device);

    // Get device properties
    vkGetPhysicalDeviceMemoryProperties(renderer->physical_device, &renderer->memory_properties);
    vkGetPhysicalDeviceProperties(renderer->physical_device, &renderer->physical_device_properties);

    // TODO : pick that don't hard code it
    renderer->depth_format = VK_FORMAT_D32_SFLOAT;

    // MSAA
    VkSampleCountFlags msaa_levels =
        renderer->physical_device_properties.limits.framebufferColorSampleCounts &
        renderer->physical_device_properties.limits.framebufferDepthSampleCounts;
    if(msaa_levels & VK_SAMPLE_COUNT_8_BIT) {
        renderer->msaa_level = VK_SAMPLE_COUNT_8_BIT;
        sLog("VK_SAMPLE_COUNT_8_BIT");
    } else if(msaa_levels & VK_SAMPLE_COUNT_4_BIT) {
        renderer->msaa_level = VK_SAMPLE_COUNT_4_BIT;
        sLog("VK_SAMPLE_COUNT_4_BIT");
    } else if(msaa_levels & VK_SAMPLE_COUNT_2_BIT) {
        renderer->msaa_level = VK_SAMPLE_COUNT_2_BIT;
        sLog("VK_SAMPLE_COUNT_2_BIT");
    } else {
        renderer->msaa_level = VK_SAMPLE_COUNT_1_BIT;
        sLog("VK_SAMPLE_COUNT_1_BIT");
    }

    GetQueuesId(renderer);
    CreateVkDevice(renderer->physical_device,
                   renderer->graphics_queue_id,
                   renderer->transfer_queue_id,
                   renderer->present_queue_id,
                   &renderer->device);

    LoadDeviceFuncPointers(renderer->device);

    vkGetDeviceQueue(renderer->device, renderer->graphics_queue_id, 0, &renderer->graphics_queue);
    vkGetDeviceQueue(renderer->device, renderer->present_queue_id, 0, &renderer->present_queue);
    vkGetDeviceQueue(renderer->device, renderer->transfer_queue_id, 0, &renderer->transfer_queue);

    // Graphics Command Pool
    VkCommandPoolCreateInfo pool_create_info = {0};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.pNext = NULL;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_create_info.queueFamilyIndex = renderer->graphics_queue_id;
    VkResult result = vkCreateCommandPool(
        renderer->device, &pool_create_info, NULL, &renderer->graphics_command_pool);
    AssertVkResult(result);

    // Descriptor Pool
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
        //{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 100},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100},
    };
    const u32 pool_sizes_count = sizeof(pool_sizes) / sizeof(pool_sizes[0]);

    VkDescriptorPoolCreateInfo pool_ci = {0};
    pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_ci.pNext = NULL;
    pool_ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_ci.maxSets = 10;
    pool_ci.poolSizeCount = pool_sizes_count;
    pool_ci.pPoolSizes = pool_sizes;
    AssertVkResult(
        vkCreateDescriptorPool(renderer->device, &pool_ci, NULL, &renderer->descriptor_pool));

    VkSamplerCreateInfo sampler_ci = {0};
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
    sampler_ci.maxAnisotropy = renderer->physical_device_properties.limits.maxSamplerAnisotropy;
    sampler_ci.compareEnable = VK_FALSE;
    sampler_ci.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_ci.minLod = 0.0f;
    sampler_ci.maxLod = 0.0f;
    sampler_ci.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_ci.unnormalizedCoordinates = VK_FALSE;
    AssertVkResult(vkCreateSampler(renderer->device, &sampler_ci, NULL, &renderer->depth_sampler));

    {
        // Camera info
        CreateBuffer(renderer->device,
                     &renderer->memory_properties,
                     sizeof(CameraMatrices),
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                     &renderer->camera_info_buffer);
        DEBUGNameBuffer(renderer->device, &renderer->camera_info_buffer, "Camera Info");

        // Scene info
        // Materials
        renderer->materials_count = 0;
        CreateBuffer(renderer->device,
                     &renderer->memory_properties,
                     128 * sizeof(Material),
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &renderer->mat_buffer);
        DEBUGNameBuffer(renderer->device, &renderer->mat_buffer, "SCENE MATS");

        // Texture Sampler
        VkSamplerCreateInfo sampler_ci = {0};
        sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_ci.pNext = NULL;
        sampler_ci.flags = 0;
        sampler_ci.magFilter = VK_FILTER_NEAREST;
        sampler_ci.minFilter = VK_FILTER_NEAREST;
        sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_ci.mipLodBias = 0.0f;
        sampler_ci.anisotropyEnable = VK_TRUE;
        sampler_ci.maxAnisotropy = renderer->physical_device_properties.limits.maxSamplerAnisotropy;
        sampler_ci.compareEnable = VK_FALSE;
        sampler_ci.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_ci.minLod = 0.0f;
        sampler_ci.maxLod = 0.0f;
        sampler_ci.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_ci.unnormalizedCoordinates = VK_FALSE;
        AssertVkResult(
            vkCreateSampler(renderer->device, &sampler_ci, NULL, &renderer->texture_sampler));

        // Textures
        renderer->textures_capacity = 1;
        renderer->textures = (Image *)sCalloc(renderer->textures_capacity, sizeof(Image));
        renderer->textures_count = 0;

        // TEMP: switch to dyn arrays
        renderer->meshes = (Mesh **)sCalloc(1, sizeof(Mesh *));
        renderer->mesh_count = 0;
    }
    { // ShadowMap group
        renderer->shadowmap_extent = (VkExtent2D){4096, 4096};
        CreateShadowMapRenderGroup(renderer, &renderer->shadowmap_render_group);

        CreateImage(renderer->device,
                    &renderer->memory_properties,
                    renderer->depth_format,
                    renderer->shadowmap_extent,
                    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    &renderer->shadowmap);
        DEBUGNameImage(renderer->device, &renderer->shadowmap, "SHADOW MAP");

        VkFramebufferCreateInfo framebuffer_create_info = {0};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = NULL;
        framebuffer_create_info.flags = 0;
        framebuffer_create_info.renderPass = renderer->shadowmap_render_group.render_pass;
        framebuffer_create_info.attachmentCount = 1;
        framebuffer_create_info.pAttachments = &renderer->shadowmap.image_view;
        framebuffer_create_info.width = renderer->shadowmap_extent.width;
        framebuffer_create_info.height = renderer->shadowmap_extent.height;
        framebuffer_create_info.layers = 1;
        AssertVkResult(vkCreateFramebuffer(
            renderer->device, &framebuffer_create_info, NULL, &renderer->shadowmap_framebuffer));

        VkSamplerCreateInfo shdw_sampler_ci = {0};
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
        AssertVkResult(vkCreateSampler(
            renderer->device, &shdw_sampler_ci, NULL, &renderer->shadowmap_sampler));
    }

    CreateScreenResources(renderer, window);

    return renderer;
}

void RendererDestroy(Renderer *renderer) {
    vkDeviceWaitIdle(renderer->device);

    DestroyScreenResources(renderer);

    for(u32 i = 0; i < renderer->textures_count; ++i) {
        DestroyImage(renderer->device, &renderer->textures[i]);
    }
    sFree(renderer->textures);

    DestroyBuffer(renderer->device, &renderer->mat_buffer);

    for(u32 i = 0; i < renderer->mesh_count; ++i) {
        RendererDestroyMesh(renderer, i);
    }
    sFree(renderer->meshes);

    // Shadowmap render group
    DestroyImage(renderer->device, &renderer->shadowmap);
    vkDestroySampler(renderer->device, renderer->shadowmap_sampler, NULL);
    vkDestroyFramebuffer(renderer->device, renderer->shadowmap_framebuffer, NULL);
    DestroyRenderGroup(renderer, &renderer->shadowmap_render_group);

    vkDestroySampler(renderer->device, renderer->texture_sampler, NULL);
    vkDestroySampler(renderer->device, renderer->depth_sampler, NULL);

    DestroyBuffer(renderer->device, &renderer->camera_info_buffer);

    vkDestroyDescriptorPool(renderer->device, renderer->descriptor_pool, NULL);
    vkDestroyCommandPool(renderer->device, renderer->graphics_command_pool, NULL);

    vkDestroyDevice(renderer->device, NULL);
    pfn_vkDestroyDebugUtilsMessengerEXT(renderer->instance, debug_messenger, NULL);
    vkDestroyInstance(renderer->instance, NULL);

    sFree(renderer);

    DBG_END();
}

void VulkanReloadShaders(Renderer *renderer) {
    vkDestroyPipeline(renderer->device, renderer->main_render_group.pipeline, NULL);

    PipelineCreateDefault(renderer->device,
                          renderer->platform,
                          "resources/shaders/general.vert.spv",
                          "resources/shaders/general.frag.spv",
                          &renderer->swapchain.extent,
                          renderer->msaa_level,
                          renderer->main_render_group.layout,
                          renderer->main_render_group.render_pass,
                          &renderer->main_render_group.pipeline);

    DestroyRenderGroup(renderer, &renderer->shadowmap_render_group);
    CreateShadowMapRenderGroup(renderer, &renderer->shadowmap_render_group);

    DestroyRenderGroup(renderer, &renderer->volumetric_render_group);
    CreateVolumetricRenderGroup(renderer, &renderer->volumetric_render_group);
}

// ================
//
// DRAWING
//
// ================

void RendererUpdateWindow(Renderer *renderer, PlatformWindow *window) {
    DestroyScreenResources(renderer);
    PlatformCreateVkSurface(renderer->instance, window, &renderer->surface);
    VkBool32 is_supported = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(
        renderer->physical_device, renderer->present_queue_id, renderer->surface, &is_supported);
    ASSERT(is_supported);
    CreateScreenResources(renderer, window);
    // Cheat
    VulkanUpdateTextureDescriptorSet(renderer->device,
                                     renderer->main_render_group.descriptor_sets[0],
                                     renderer->texture_sampler,
                                     renderer->textures_count,
                                     renderer->textures);
}

void RendererDrawFrame(Renderer *renderer) {
    UploadToBuffer(renderer->device,
                   &renderer->camera_info_buffer,
                   &renderer->camera_info,
                   sizeof(renderer->camera_info));

    u32 image_id;
    Swapchain *swapchain = &renderer->swapchain;

    VkResult result;
    result = vkAcquireNextImageKHR(renderer->device,
                                   swapchain->swapchain,
                                   UINT64_MAX,
                                   swapchain->image_acquired_semaphore[swapchain->semaphore_id],
                                   VK_NULL_HANDLE,
                                   &image_id);
    if(result != VK_ERROR_OUT_OF_DATE_KHR)
        AssertVkResult(result);

    // If the frame hasn't finished rendering wait for it to finish
    AssertVkResult(
        vkWaitForFences(renderer->device, 1, &swapchain->fences[image_id], VK_TRUE, UINT64_MAX));

    vkResetFences(renderer->device, 1, &swapchain->fences[image_id]);

    VkCommandBuffer cmd = swapchain->command_buffers[image_id];

    const VkCommandBufferBeginInfo begin_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, 0, NULL};
    AssertVkResult(vkBeginCommandBuffer(cmd, &begin_info));

    { // Shadow map
        VkDebugUtilsLabelEXT marker = {
            VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, NULL, "SHADOW MAP", {0.0, 0.0, 0.0, 0.0}};
        pfn_vkCmdBeginDebugUtilsLabelEXT(cmd, &marker);
        BeginRenderGroup(cmd,
                         &renderer->shadowmap_render_group,
                         renderer->shadowmap_framebuffer,
                         renderer->shadowmap_extent);
        Frame frame = {cmd, renderer->shadowmap_render_group.layout};
        for(u32 i = 0; i < renderer->mesh_count; ++i) {
            Mesh *mesh = renderer->meshes[i];
            RendererDrawMesh(&frame, mesh, mesh->instance_count, mesh->instance_transforms);
        }
        vkCmdEndRenderPass(cmd);
        pfn_vkCmdEndDebugUtilsLabelEXT(cmd);
    }

    { // Color
        VkDebugUtilsLabelEXT marker = {
            VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, NULL, "COLOR", {0.0, 0.0, 0.0, 0.0}};
        pfn_vkCmdBeginDebugUtilsLabelEXT(cmd, &marker);
        BeginRenderGroup(
            cmd, &renderer->main_render_group, renderer->color_pass_framebuffer, swapchain->extent);
        Frame frame = {cmd, renderer->main_render_group.layout};
        for(u32 i = 0; i < renderer->mesh_count; ++i) {
            Mesh *mesh = renderer->meshes[i];
            RendererDrawMesh(&frame, mesh, mesh->instance_count, mesh->instance_transforms);
        }
        vkCmdEndRenderPass(cmd);
        pfn_vkCmdEndDebugUtilsLabelEXT(cmd);
    }

    { // Fog
        // TODO: maybe this doesnt need to be in a separate render group
        VkDebugUtilsLabelEXT marker = {
            VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, NULL, "FOG", {0.0, 0.0, 0.0, 0.0}};
        pfn_vkCmdBeginDebugUtilsLabelEXT(cmd, &marker);
        BeginRenderGroup(cmd,
                         &renderer->volumetric_render_group,
                         renderer->framebuffers[image_id],
                         swapchain->extent);
        vkCmdDraw(cmd, 6, 1, 0, 0);
        vkCmdEndRenderPass(cmd);
        pfn_vkCmdEndDebugUtilsLabelEXT(cmd);
    }

    AssertVkResult(vkEndCommandBuffer(cmd));

    const VkPipelineStageFlags stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &swapchain->image_acquired_semaphore[swapchain->semaphore_id];
    submit_info.pWaitDstStageMask = &stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &swapchain->render_complete_semaphore[swapchain->semaphore_id];
    result = vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, swapchain->fences[image_id]);
    AssertVkResult(result);

    // Present
    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = NULL;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &swapchain->render_complete_semaphore[swapchain->semaphore_id];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain->swapchain;
    present_info.pImageIndices = &image_id;
    present_info.pResults = NULL;
    result = vkQueuePresentKHR(renderer->present_queue, &present_info);
    if(result != VK_ERROR_OUT_OF_DATE_KHR) {
        AssertVkResult(result);
    }

    swapchain->semaphore_id = (swapchain->semaphore_id + 1) % swapchain->image_count;

    vkDeviceWaitIdle(renderer->device);
}