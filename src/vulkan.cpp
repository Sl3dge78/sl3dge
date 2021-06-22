#include <SDL/SDL_image.h>
#include <SDL/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#include "game.h"

/*
 === TODO ===
 CRITICAL
 - Optimize volumetric fog
> Move it to a new render pass.
> Will need the z-buffer and the shadowmap
> What else? think about it. Currently in the shader it needs the world pos. How do i get it from the zbuffer? How do I get them along the ray? Should
be fine
- Cleanup the pipelines creations. How do we group that?

 MAJOR
- Descriptor set 0 > could be the same for all pipelines
- Check the frame sync, we might not be at 60.

 BACKLOG

- Find a way to draw the sun even though no fragments are on the screen
- Draw multiple gltf scenes
- Window Resize > Pipline dynamic states ?
- Mipmaps
- Volumetric clouds?

 IMPROVEMENTS
- Can't create a scene without textures
- Find the way to display the fps in the window tile, i used to have it
- IBL?
- Utiliser des Staging buffers
- Handle pipeline caches
- Pick the device according to our specs
- Pipeline dynamic state
- Apparently we should allocate big stacks only
*/

#include "gltf.cpp"
#include "vulkan_layer.h"
#include "vulkan_scene.cpp"

internal void AssertVkResult(VkResult result) {
	if (result < 0) {
		switch (result) {
			case VK_ERROR_EXTENSION_NOT_PRESENT:
				SDL_LogError(0, "VK_ERROR_EXTENSION_NOT_PRESENT");
				break;
			case VK_ERROR_LAYER_NOT_PRESENT:
				SDL_LogError(0, "VK_ERROR_LAYER_NOT_PRESENT");
				break;
			case VK_ERROR_DEVICE_LOST: {
				SDL_LogError(0, "VK_ERROR_DEVICE_LOST");
			} break;
			case VK_ERROR_OUT_OF_POOL_MEMORY: {
				SDL_LogError(0, "VK_ERROR_OUT_OF_POOL_MEMORY");
			} break;
			case VK_ERROR_INITIALIZATION_FAILED: {
				SDL_LogError(0, "VK_ERROR_INITIALIZATION_FAILED");
			} break;
			case VK_ERROR_OUT_OF_DATE_KHR: {
				SDL_LogError(0, "VK_ERROR_OUT_OF_DATE_KHR");
			} break;
			default:
				SDL_LogError(0, "Unhandled Vulkan error : %d", result);
				break;
		}
		ASSERT(false);
		exit(result);
	}
}

global VkDebugUtilsMessengerEXT debug_messenger;

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
		// SDL_LogInfo(0, pCallbackData->pMessage);
	} else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		// SDL_LogWarn(0, pCallbackData->pMessage);
	} else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		SDL_LogError(0, pCallbackData->pMessage);
		KEEP_CONSOLE_OPEN(true);
	}
	return VK_FALSE;
}

internal void DEBUGPrintInstanceExtensions() {
	u32 property_count = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &property_count, NULL);

	VkExtensionProperties *extensions = (VkExtensionProperties *)calloc(property_count, sizeof(VkExtensionProperties));
	vkEnumerateInstanceExtensionProperties(NULL, &property_count, extensions);

	SDL_Log("Available extensions :");
	for (u32 i = 0; i < property_count; i++) {
		SDL_Log("%s", extensions[i].extensionName);
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

// This allocates memory, free it!
internal u32 *AllocateAndLoadBinary(const char *path, i64 *file_size) {
	FILE *file = fopen(path, "rb");
	if (!file) {
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

internal void CreateVkShaderModule(const char *path, VkDevice device, VkShaderModule *shader_module) {
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

	const char *validation[] = { "VK_LAYER_KHRONOS_validation" };
	create_info.enabledLayerCount = 1;
	create_info.ppEnabledLayerNames = validation;

	// Our extensions
	u32 sl3_count = 1;
	const char *sl3_extensions[1] = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };

	// Get SDL extensions
	u32 sdl_count = 0;
	SDL_Vulkan_GetInstanceExtensions(window, &sdl_count, NULL);
	const char **sdl_extensions = (const char **)calloc(sdl_count, sizeof(char *));
	SDL_Vulkan_GetInstanceExtensions(window, &sdl_count, sdl_extensions);

	u32 total_count = sdl_count + sl3_count;

	const char **all_extensions = (const char **)calloc(total_count, sizeof(char *));
	memcpy(all_extensions, sdl_extensions, sdl_count * sizeof(char *));
	memcpy(all_extensions + sdl_count, sl3_extensions, sl3_count * sizeof(char *));

	free(sdl_extensions);
	/*
	SDL_Log("Requested extensions");
	for (int i = 0; i < total_count; i++) {
		SDL_Log(all_extensions[i]);
	}
	*/

	create_info.enabledExtensionCount = sdl_count + sl3_count;
	create_info.ppEnabledExtensionNames = all_extensions;

	VkResult result = vkCreateInstance(&create_info, NULL, instance);

	AssertVkResult(result);

	free(all_extensions);

	{ // Create debug messenger
		// TODO : Do that only if we're in debug mode

		// Get the functions
		LOAD_INSTANCE_FUNC(*instance, vkCreateDebugUtilsMessengerEXT);
		LOAD_INSTANCE_FUNC(*instance, vkDestroyDebugUtilsMessengerEXT);
		LOAD_INSTANCE_FUNC(*instance, vkSetDebugUtilsObjectNameEXT);

		// Create the messenger
		VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
		debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debug_create_info.pNext = NULL;
		debug_create_info.flags = 0;
		debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
										VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
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

	for (int i = 0; i < device_count; i++) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
	}
	// TODO : Pick the device according to our specs
	*physical_device = physical_devices[0];
	free(physical_devices);
}

internal void GetQueuesId(VulkanContext *context) {
	// Get queue info
	u32 queue_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(context->physical_device, &queue_count, NULL);
	VkQueueFamilyProperties *queue_properties = (VkQueueFamilyProperties *)calloc(queue_count, sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(context->physical_device, &queue_count, queue_properties);

	u32 set_flags = 0;

	for (int i = 0; i < queue_count && set_flags != 7; i++) {
		if (!(set_flags & 1) && queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			context->graphics_queue_id = i;
			set_flags |= 1;
			// graphics_queue.count = queue_properties[i].queueCount; // TODO ??
		}

		if (!(set_flags & 2) && queue_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT && !(queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
			context->transfer_queue_id = i;
			set_flags |= 2;
			// transfer_queue.count = queue_properties[i].queueCount; // TODO ??
		}

		if (!(set_flags & 4)) {
			VkBool32 is_supported = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(context->physical_device, i, context->surface, &is_supported);
			if (is_supported) {
				context->present_queue_id = i;
				set_flags |= 4;
			}
		}
	}
	free(queue_properties);
}

internal void CreateVkDevice(
		VkPhysicalDevice physical_device, const u32 graphics_queue, const u32 transfer_queue, const u32 present_queue, VkDevice *device) {
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
	const char *extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, VK_KHR_RAY_QUERY_EXTENSION_NAME, VK_KHR_SHADER_CLOCK_EXTENSION_NAME };
	const u32 extension_count = sizeof(extensions) / sizeof(extensions[0]);

	VkPhysicalDeviceAccelerationStructureFeaturesKHR accel_feature = {};
	accel_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	accel_feature.pNext = NULL;
	accel_feature.accelerationStructure = VK_TRUE;

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_feature = {};
	rt_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	rt_feature.pNext = &accel_feature;
	rt_feature.rayTracingPipeline = VK_TRUE;

	VkPhysicalDeviceBufferDeviceAddressFeatures device_address = {};
	device_address.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	device_address.pNext = &rt_feature;
	device_address.bufferDeviceAddress = VK_TRUE;

	VkPhysicalDeviceRobustness2FeaturesEXT robustness = {};
	robustness.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
	robustness.pNext = &device_address;
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
}

internal void CreateSwapchain(const VulkanContext *context, SDL_Window *window, Swapchain *swapchain) {
	VkSwapchainCreateInfoKHR create_info = {};

	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.surface = context->surface;

	// Img count
	VkSurfaceCapabilitiesKHR surface_capabilities = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physical_device, context->surface, &surface_capabilities);
	u32 image_count = 3;
	if (surface_capabilities.maxImageCount != 0 && surface_capabilities.maxImageCount < image_count) {
		image_count = surface_capabilities.maxImageCount;
	}
	if (image_count < surface_capabilities.minImageCount) {
		image_count = surface_capabilities.minImageCount;
	}
	create_info.minImageCount = image_count;

	// Format
	u32 format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(context->physical_device, context->surface, &format_count, NULL);
	VkSurfaceFormatKHR *formats = (VkSurfaceFormatKHR *)calloc(format_count, sizeof(VkSurfaceFormatKHR));
	vkGetPhysicalDeviceSurfaceFormatsKHR(context->physical_device, context->surface, &format_count, formats);
	VkSurfaceFormatKHR picked_format = formats[0];
	for (u32 i = 0; i < format_count; ++i) {
		VkImageFormatProperties properties;
		VkResult result = vkGetPhysicalDeviceImageFormatProperties(context->physical_device, formats[i].format, VK_IMAGE_TYPE_2D,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT, 0, &properties);
		if (result != VK_ERROR_FORMAT_NOT_SUPPORTED) {
			picked_format = formats[i];
			break;
		}

		// if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
		//	picked_format = formats[i];
		//	SDL_Log("Swapchain format: VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR");
		//	break;
		//}
	}
	create_info.imageFormat = picked_format.format;
	create_info.imageColorSpace = picked_format.colorSpace;
	swapchain->format = picked_format.format;
	free(formats);

	// Extent
	VkExtent2D extent = {};
	if (surface_capabilities.currentExtent.width != 0xFFFFFFFF) {
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
	create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
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
	for (u32 i = 0; i < present_mode_count; i++) {
		if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
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

	swapchain->image_views = (VkImageView *)calloc(swapchain->image_count, sizeof(VkImageView));

	for (u32 i = 0; i < swapchain->image_count; i++) {
		DEBUGNameObject(context->device, (u64)swapchain->images[i], VK_OBJECT_TYPE_IMAGE, "Swapchain image");

		VkImageViewCreateInfo image_view_ci = {};
		image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_ci.pNext = NULL;
		image_view_ci.flags = 0;
		image_view_ci.image = swapchain->images[i];
		image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_ci.format = swapchain->format;
		image_view_ci.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY };
		image_view_ci.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

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
	VkFenceCreateInfo ci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, VK_FENCE_CREATE_SIGNALED_BIT };

	for (u32 i = 0; i < swapchain->image_count; i++) {
		result = vkCreateFence(context->device, &ci, NULL, &swapchain->fences[i]);
		AssertVkResult(result);
	}

	// Semaphores
	swapchain->image_acquired_semaphore = (VkSemaphore *)calloc(swapchain->image_count, sizeof(VkSemaphore));
	swapchain->render_complete_semaphore = (VkSemaphore *)calloc(swapchain->image_count, sizeof(VkSemaphore));
	VkSemaphoreCreateInfo semaphore_ci = {};
	semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	for (u32 i = 0; i < swapchain->image_count; i++) {
		vkCreateSemaphore(context->device, &semaphore_ci, NULL, &swapchain->image_acquired_semaphore[i]);
		vkCreateSemaphore(context->device, &semaphore_ci, NULL, &swapchain->render_complete_semaphore[i]);
	}
	swapchain->semaphore_id = 0;

	SDL_Log("Swapchain created with %d images", swapchain->image_count);
}

internal void DestroySwapchain(const VulkanContext *context, Swapchain *swapchain) {
	vkFreeCommandBuffers(context->device, context->graphics_command_pool, swapchain->image_count, swapchain->command_buffers);
	free(swapchain->command_buffers);

	free(swapchain->images);

	for (u32 i = 0; i < swapchain->image_count; i++) {
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
	color_attachment.samples = sample_count;
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

	VkAttachmentDescription attachments[] = { color_attachment, depth_attachment, msaa_attachment };

	render_pass_ci.attachmentCount = 3;
	render_pass_ci.pAttachments = attachments;

	VkSubpassDescription subpass_desc = {};
	subpass_desc.flags = 0;
	subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_desc.inputAttachmentCount = 0;
	subpass_desc.pInputAttachments = NULL;

	VkAttachmentReference color_ref = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depth_ref = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
	VkAttachmentReference resolve_ref = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
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

	VkAttachmentDescription attachments[] = { depth_attachment };

	render_pass_ci.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
	render_pass_ci.pAttachments = attachments;

	VkAttachmentReference depth_ref = { 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
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

	const VkDescriptorSetLayoutBinding bindings[] = { { // CAMERA MATRICES
			0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, NULL } };
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
	VkPushConstantRange push_constant_range = { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant) };
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

internal void CreateShadowMapPipeline(const VkDevice device, const Swapchain *swapchain, const VkPipelineLayout layout,
		const VkRenderPass render_pass, const VkExtent2D extent, VkPipeline *pipeline) {
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
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) },
		{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) },
		{ 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) },
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
	scissor.offset = { 0, 0 };
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

extern "C" __declspec(dllexport) VulkanContext *VulkanCreateContext(SDL_Window *window) {
	VulkanContext *context = (VulkanContext *)malloc(sizeof(VulkanContext));
	CreateVkInstance(window, &context->instance);
	SDL_Vulkan_CreateSurface(window, context->instance, &context->surface);
	CreateVkPhysicalDevice(context->instance, &context->physical_device);

	// Get device properties
	vkGetPhysicalDeviceMemoryProperties(context->physical_device, &context->memory_properties);
	vkGetPhysicalDeviceProperties(context->physical_device, &context->physical_device_properties);

	// MSAA
	VkSampleCountFlags msaa_levels = context->physical_device_properties.limits.framebufferColorSampleCounts &
									 context->physical_device_properties.limits.framebufferDepthSampleCounts;
	if (msaa_levels & VK_SAMPLE_COUNT_8_BIT) {
		context->msaa_level = VK_SAMPLE_COUNT_8_BIT;
		SDL_Log("VK_SAMPLE_COUNT_8_BIT");
	} else if (msaa_levels & VK_SAMPLE_COUNT_4_BIT) {
		context->msaa_level = VK_SAMPLE_COUNT_4_BIT;
		SDL_Log("VK_SAMPLE_COUNT_4_BIT");
	} else if (msaa_levels & VK_SAMPLE_COUNT_2_BIT) {
		context->msaa_level = VK_SAMPLE_COUNT_2_BIT;
		SDL_Log("VK_SAMPLE_COUNT_2_BIT");
	} else {
		context->msaa_level = VK_SAMPLE_COUNT_1_BIT;
		SDL_Log("VK_SAMPLE_COUNT_1_BIT");
	}

	GetQueuesId(context);
	CreateVkDevice(context->physical_device, context->graphics_queue_id, context->transfer_queue_id, context->present_queue_id, &context->device);

	LoadDeviceFuncPointers(context->device);

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
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
		{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 100 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 },
	};
	const u32 pool_sizes_count = sizeof(pool_sizes) / sizeof(pool_sizes[0]);

	VkDescriptorPoolCreateInfo pool_ci = {};
	pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_ci.pNext = NULL;
	pool_ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_ci.maxSets = 10;
	pool_ci.poolSizeCount = pool_sizes_count;
	pool_ci.pPoolSizes = pool_sizes;
	AssertVkResult(vkCreateDescriptorPool(context->device, &pool_ci, NULL, &context->descriptor_pool));

	context->swapchain.swapchain = VK_NULL_HANDLE;
	CreateSwapchain(context, window, &context->swapchain);

	CreateBuffer(context->device, &context->memory_properties, sizeof(CameraMatrices), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &context->cam_buffer);
	DEBUGNameBuffer(context->device, &context->cam_buffer, "Camera Info");

	// Sampler
	VkSamplerCreateInfo sampler_ci = {};
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
	sampler_ci.maxAnisotropy = context->physical_device_properties.limits.maxSamplerAnisotropy;
	sampler_ci.compareEnable = VK_FALSE;
	sampler_ci.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_ci.minLod = 0.0f;
	sampler_ci.maxLod = 0.0f;
	sampler_ci.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_ci.unnormalizedCoordinates = VK_FALSE;
	AssertVkResult(vkCreateSampler(context->device, &sampler_ci, NULL, &context->texture_sampler));

	// Depth image
	CreateMultiSampledImage(context->device, &context->memory_properties, VK_FORMAT_D32_SFLOAT,
			{ context->swapchain.extent.width, context->swapchain.extent.height, 1 }, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, context->msaa_level, &context->depth_image);
	// MSAA Image
	CreateMultiSampledImage(context->device, &context->memory_properties, context->swapchain.format,
			{ context->swapchain.extent.width, context->swapchain.extent.height, 1 },
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, context->msaa_level,
			&context->msaa_image);
	// RTX Render Image
	CreateRtxRenderImage(context->device, &context->swapchain, &context->memory_properties, &context->rtx_image);

	CreateRenderPass(context->device, &context->swapchain, context->msaa_level, &context->render_pass);

	// Framebuffers
	context->framebuffers = (VkFramebuffer *)calloc(context->swapchain.image_count, sizeof(VkFramebuffer));
	for (u32 i = 0; i < context->swapchain.image_count; ++i) {
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
	context->shadowmap_extent = { 4096, 4096 };
	CreateShadowMapRenderPass(context->device, &context->swapchain, &context->shadowmap_render_pass);
	CreateShadowMapLayout(context, &context->shadowmap_layout);
	CreateShadowMapPipeline(context->device, &context->swapchain, context->shadowmap_layout.layout, context->shadowmap_render_pass,
			context->shadowmap_extent, &context->shadowmap_pipeline);

	CreateImage(context->device, &context->memory_properties, VK_FORMAT_D32_SFLOAT, context->shadowmap_extent,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &context->shadowmap);
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

internal void DestroyLayout(VkDevice device, VkDescriptorPool pool, VulkanLayout *layout) {
	vkFreeDescriptorSets(device, pool, layout->descriptor_set_count, &layout->descriptor_set);
	vkDestroyDescriptorSetLayout(device, layout->set_layout, NULL);
	vkDestroyPipelineLayout(device, layout->layout, NULL);
}

extern "C" __declspec(dllexport) void VulkanDestroyContext(VulkanContext *context) {
	vkDeviceWaitIdle(context->device);

	// Shadowmap
	DestroyImage(context->device, &context->shadowmap);
	vkDestroySampler(context->device, context->shadowmap_sampler, NULL);
	vkDestroyRenderPass(context->device, context->shadowmap_render_pass, NULL);
	DestroyLayout(context->device, context->descriptor_pool, &context->shadowmap_layout);
	vkDestroyPipeline(context->device, context->shadowmap_pipeline, NULL);
	vkDestroyFramebuffer(context->device, context->shadowmap_framebuffer, NULL);

	for (u32 i = 0; i < context->swapchain.image_count; ++i) {
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
	vkDestroyCommandPool(context->device, context->graphics_command_pool, NULL);

	vkDestroyDevice(context->device, NULL);
	pfn_vkDestroyDebugUtilsMessengerEXT(context->instance, debug_messenger, NULL);
	vkDestroyInstance(context->instance, NULL);

	free(context);

	DBG_END();
}

extern "C" __declspec(dllexport) void VulkanReloadShaders(VulkanContext *context, Scene *scene) {
	vkDestroyPipeline(context->device, scene->pipeline, NULL);
	// CreateRtxPipeline(context->device, &scene->layout, &scene->pipeline);
	CreateScenePipeline(context->device, scene->layout, &context->swapchain, context->render_pass, context->msaa_level, &scene->pipeline);

	vkDestroyPipeline(context->device, context->shadowmap_pipeline, NULL);
	CreateShadowMapPipeline(context->device, &context->swapchain, context->shadowmap_layout.layout, context->shadowmap_render_pass,
			context->shadowmap_extent, &context->shadowmap_pipeline);
}

// ================
//
// DRAWING
//
// ================

extern "C" __declspec(dllexport) void VulkanDrawFrame(VulkanContext *context, Scene *scene, GameData *game_data) {
	VulkanUpdateDescriptors(context, game_data);

	u32 image_id;
	Swapchain *swapchain = &context->swapchain;

	VkResult result;
	result = vkAcquireNextImageKHR(context->device, swapchain->swapchain, UINT64_MAX, swapchain->image_acquired_semaphore[swapchain->semaphore_id],
			VK_NULL_HANDLE, &image_id);
	AssertVkResult(result); // TODO(Guigui): Recreate swapchain if Suboptimal or outofdate;

	// If the frame hasn't finished rendering wait for it to finish
	AssertVkResult(vkWaitForFences(context->device, 1, &swapchain->fences[image_id], VK_TRUE, UINT64_MAX));

	vkResetFences(context->device, 1, &swapchain->fences[image_id]);

	VkCommandBuffer cmd = swapchain->command_buffers[image_id];

	const VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, 0, NULL };
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
	renderpass_begin.renderArea = { { 0, 0 }, context->shadowmap_extent };
	VkClearValue clear_value = {};
	clear_value.depthStencil = { 1.0f, 0 };
	renderpass_begin.clearValueCount = 1;
	renderpass_begin.pClearValues = &clear_value;
	vkCmdBeginRenderPass(cmd, &renderpass_begin, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, context->shadowmap_pipeline);
	vkCmdBindDescriptorSets(
			cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, context->shadowmap_layout.layout, 0, 1, &context->shadowmap_layout.descriptor_set, 0, NULL);
	for (u32 i = 0; i < scene->total_primitives_count; i++) {
		Primitive *prim = &scene->primitives[i];

		PushConstant push = { scene->transforms[prim->node_id], prim->material_id };
		vkCmdPushConstants(cmd, scene->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant), &push);
		vkCmdDrawIndexed(cmd, prim->index_count, 1, prim->index_offset, prim->vertex_offset, 0);
	}
	vkCmdEndRenderPass(cmd);

	// Color
	renderpass_begin.renderPass = context->render_pass;
	renderpass_begin.framebuffer = context->framebuffers[image_id];
	renderpass_begin.renderArea = { { 0, 0 }, swapchain->extent };

	VkClearValue clear_values[2] = {};
	clear_values[0].color = { 0.53f, 0.80f, 0.92f, 0.0f };
	clear_values[1].depthStencil = { 1.0f, 0 };
	renderpass_begin.clearValueCount = 2;
	renderpass_begin.pClearValues = clear_values;

	vkCmdBeginRenderPass(cmd, &renderpass_begin, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene->pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene->layout, 0, scene->descriptor_set_count, scene->descriptor_sets, 0, NULL);

	for (u32 i = 0; i < scene->total_primitives_count; i++) {
		Primitive *prim = &scene->primitives[i];

		PushConstant push = { scene->transforms[prim->node_id], prim->material_id };
		vkCmdPushConstants(cmd, scene->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant), &push);
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