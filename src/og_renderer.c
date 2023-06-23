/* Author: Da Vinci
 * This Code is in the Public Domain
 */

#include "origami/og_renderer.h"
#include "origami/common.h"
#include <vulkan/vulkan_core.h>

OG_API void og_init(OGContext *og_ctx, OGConfig *og_cfg) {
	_init_window(og_ctx, og_cfg);
	_init_vulkan(og_ctx, og_cfg);
	_create_surface(og_ctx);

	_choose_physical_device(og_ctx);
	_create_logical_device(og_ctx);

	_create_swapchain(og_ctx);
	_create_command_pool(og_ctx);

	_create_sync_objects(og_ctx);

	og_ctx->running = true;
}

OG_API void og_clear_screen(OGContext *og_ctx, OGColor color) {
	VkImageSubresourceRange range = {};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.layerCount = 1;
	range.levelCount = 1;

	vkCmdClearColorImage(og_ctx->curr_cmd_buffer, og_ctx->sc_images[og_ctx->img_idx],
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, &color, 1, &range);
}

OG_API void og_render(OGContext *og_ctx, void (*render)(OGContext*)) {
	OG_CHECK_VK(vkAcquireNextImageKHR(og_ctx->logical_device, og_ctx->swapchain, 0, og_ctx->acquire_img_semaphore, 0, &og_ctx->img_idx), "Image Acquisition Failed");

	VkCommandBufferAllocateInfo cmd_alloc_info = {};
	cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_alloc_info.commandBufferCount = 1;
	cmd_alloc_info.commandPool = og_ctx->command_pool;
	cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	OG_CHECK_VK(vkAllocateCommandBuffers(og_ctx->logical_device,
				&cmd_alloc_info, &og_ctx->curr_cmd_buffer), "Command Buffer Allocation Failed");

	VkCommandBufferBeginInfo cb_begin_info = {};
	cb_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cb_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	
	OG_CHECK_VK(vkBeginCommandBuffer(og_ctx->curr_cmd_buffer, &cb_begin_info), "Command Buffer Begin Failed");

	// Rendering
	{
		render(og_ctx);
	}

	OG_CHECK_VK(vkEndCommandBuffer(og_ctx->curr_cmd_buffer), "Command Buffer End Failed");

	VkPipelineStageFlags wdst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &og_ctx->curr_cmd_buffer;
	submit_info.pSignalSemaphores = &og_ctx->submit_semaphore;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &og_ctx->acquire_img_semaphore;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitDstStageMask = &wdst_stage_mask;

	OG_CHECK_VK(vkQueueSubmit(og_ctx->graphics_queue, 1, &submit_info, 0), "Queue Submit Failed");


	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pSwapchains = &og_ctx->swapchain;
	present_info.swapchainCount = 1;
	present_info.pImageIndices = &og_ctx->img_idx;
	present_info.pWaitSemaphores = &og_ctx->submit_semaphore;
	present_info.waitSemaphoreCount = 1;

	OG_CHECK_VK(vkQueuePresentKHR(og_ctx->graphics_queue, &present_info), "Queue Present Failed");

	vkDeviceWaitIdle(og_ctx->logical_device);
	vkFreeCommandBuffers(og_ctx->logical_device, og_ctx->command_pool, 1, &og_ctx->curr_cmd_buffer);
}


OG_API void og_poll_events(OGContext *og_ctx) {
	glfwPollEvents();

	_handle_default_events(og_ctx);
}

OG_API void og_quit(OGContext *og_ctx) {
	vkDestroySemaphore(og_ctx->logical_device, og_ctx->acquire_img_semaphore, NULL);
	vkDestroySemaphore(og_ctx->logical_device, og_ctx->submit_semaphore, NULL);

	vkDestroyCommandPool(og_ctx->logical_device, og_ctx->command_pool, NULL);
	vkDestroySwapchainKHR(og_ctx->logical_device, og_ctx->swapchain, NULL);
	vkDestroyDevice(og_ctx->logical_device, NULL);
	vkDestroySurfaceKHR(og_ctx->instance, og_ctx->surface, NULL);

	glfwDestroyWindow(og_ctx->window);
	vkDestroyInstance(og_ctx->instance, NULL);
	glfwTerminate();
}

OG_INT static VKAPI_ATTR VkBool32 VKAPI_CALL __debug_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
		VkDebugUtilsMessageTypeFlagsEXT msgType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void *pUserData) {
	OG_LOG_INFOVAR("VALIDATION ERROR", pCallbackData->pMessage);
	return false;
}

OG_INT void _init_window(OGContext *og_ctx, OGConfig *og_cfg) {
	if (glfwInit() != GLFW_TRUE) {
		OG_LOG_ERR("GLFW Initialization Failed");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	
	og_ctx->window = glfwCreateWindow(og_cfg->win_width,
			og_cfg->win_height, og_cfg->app_name, NULL, NULL);

	if (!og_ctx->window) {
		OG_LOG_ERR("GLFW Window Creation Failed");
	}
}

OG_INT void _init_vulkan(OGContext *og_ctx, OGConfig *og_cfg) {
	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = og_cfg->app_name;
	app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	app_info.pEngineName = "ORIGAMI";
	app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0); 
	app_info.apiVersion = VK_API_VERSION_1_2;

	const char* layers[1] = {
		"VK_LAYER_KHRONOS_validation",
	};

	uint32_t glfw_ext_c = 0;
	const char** glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_c);

	VkInstanceCreateInfo instance_info = {};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app_info;
	instance_info.enabledExtensionCount = glfw_ext_c;
	instance_info.ppEnabledExtensionNames = glfw_ext;

	if (og_cfg->vd_layers) {
		instance_info.enabledLayerCount = OG_ARR_SIZE(layers);
		instance_info.ppEnabledLayerNames = layers;
	} else {
		instance_info.enabledLayerCount = 0;
	}
	OG_CHECK_VK(vkCreateInstance(&instance_info, NULL, &og_ctx->instance), "Vulkan Instance Creation Failed");

	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(og_ctx->instance, "vkCreateDebugUtilsMessengerEXT");

	if (func) {
		VkDebugUtilsMessengerCreateInfoEXT debug_info = {};
		debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debug_info.pfnUserCallback = __debug_callback;

		func(og_ctx->instance, &debug_info, NULL, &og_ctx->debug_messenger);
	}
}

OG_INT void _create_surface(OGContext *og_ctx) {
	OG_CHECK_VK(glfwCreateWindowSurface(og_ctx->instance, og_ctx->window, NULL, &og_ctx->surface), "Window Surface Creation Failed");
}

OG_INT void _handle_default_events(OGContext *og_ctx) {
	if (glfwGetKey(og_ctx->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		og_ctx->running = false;
	}
}
OG_INT void _choose_physical_device(OGContext *og_ctx) {
	og_ctx->graphics_idx = -1;

	uint32_t pd_count = 0;
	vkEnumeratePhysicalDevices(og_ctx->instance, &pd_count, NULL);
	VkPhysicalDevice pd_devices[pd_count];

	OG_CHECK_VK(vkEnumeratePhysicalDevices(og_ctx->instance, &pd_count, pd_devices), "Enumerate Physical Device Failed");

	for (uint32_t i = 0; i < pd_count; i++) {
		VkPhysicalDevice p_device = pd_devices[i];

		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(p_device, &queue_family_count, NULL);

		VkQueueFamilyProperties queue_properties[queue_family_count];
		vkGetPhysicalDeviceQueueFamilyProperties(p_device, &queue_family_count, queue_properties);

		for (uint32_t j = 0; j < queue_family_count; j++) {
			if (queue_properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				VkBool32 surface_support = VK_FALSE;
				OG_CHECK_VK(vkGetPhysicalDeviceSurfaceSupportKHR(p_device, j,
							og_ctx->surface, &surface_support), "Physical Device Surface Support Not Available");

				if (surface_support) {
					og_ctx->graphics_idx = j;
					og_ctx->physical_device  = p_device;
					break;
				}
			}
		}
	}
	if (og_ctx->graphics_idx < 0) {
		OG_LOG_ERR("Failed To Find Graphics Queue Family Support");
	}
}

OG_INT void _create_logical_device(OGContext *og_ctx) {
	const char* extensions[1] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	float priority = 1.0f;
	VkDeviceQueueCreateInfo queue_info = {};
	queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_info.pQueuePriorities = &priority;
	queue_info.queueCount = 1;
	queue_info.queueFamilyIndex = og_ctx->graphics_idx;

	VkDeviceCreateInfo device_info = {};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.queueCreateInfoCount = 1;
	device_info.pQueueCreateInfos = &queue_info;
	device_info.enabledExtensionCount = OG_ARR_SIZE(extensions);
	device_info.ppEnabledExtensionNames = extensions;

	OG_CHECK_VK(vkCreateDevice(og_ctx->physical_device, &device_info, NULL,
				&og_ctx->logical_device), "Logical Device Creation Failed");

	vkGetDeviceQueue(og_ctx->logical_device, og_ctx->graphics_idx, 0, &og_ctx->graphics_queue);
}

OG_INT void _create_swapchain(OGContext *og_ctx) {
	VkSurfaceCapabilitiesKHR surf_caps = {};
	OG_CHECK_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(og_ctx->physical_device,
				og_ctx->surface, &surf_caps), "Surface Capabilities Not Available");

	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(og_ctx->physical_device, og_ctx->surface, &format_count, 0);

	VkSurfaceFormatKHR surf_formats[format_count];
	OG_CHECK_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(og_ctx->physical_device,
				og_ctx->surface, &format_count, surf_formats), "Surface Formats Not Available");

	for (uint32_t i = 0; i < format_count; i++) {
		VkSurfaceFormatKHR surf_format = surf_formats[i];

		if (surf_format.format == VK_FORMAT_B8G8R8A8_SRGB) {
			og_ctx->surf_format = surf_format;
			break; }
	}

	VkPresentModeKHR present_mode = 0;

	uint32_t present_mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(og_ctx->physical_device, og_ctx->surface, &present_mode_count, 0);
	VkPresentModeKHR present_modes[present_mode_count];

	OG_CHECK_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(og_ctx->physical_device,
				og_ctx->surface, &present_mode_count, present_modes), "Surface Present Mode Get Failed");

	for (uint32_t i = 0; i < present_mode_count; i++) {
		if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			present_mode = present_modes[i];
			break;
		// Guaranteed To Be Present By Standard
		} else if (present_modes[i] == VK_PRESENT_MODE_FIFO_KHR) {
			present_mode = present_modes[i];
		}
	}

	VkSwapchainCreateInfoKHR sc_info = {};
	sc_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	sc_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	sc_info.surface = og_ctx->surface;
	sc_info.preTransform = surf_caps.currentTransform;
	sc_info.imageExtent = surf_caps.currentExtent;
	sc_info.minImageCount = surf_caps.minImageCount + 1;
	sc_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	sc_info.imageArrayLayers = 1;
	sc_info.imageFormat = og_ctx->surf_format.format;
	sc_info.imageColorSpace = og_ctx->surf_format.colorSpace;
	sc_info.presentMode = present_mode;
	sc_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	sc_info.queueFamilyIndexCount = 0;
	sc_info.pQueueFamilyIndices = &og_ctx->graphics_idx;
	sc_info.clipped = VK_TRUE;
	sc_info.oldSwapchain = VK_NULL_HANDLE;

	OG_CHECK_VK(vkCreateSwapchainKHR(og_ctx->logical_device, &sc_info,
				NULL, &og_ctx->swapchain), "SwapChain Creation Failed");

	uint32_t sc_img_count = 0;
	vkGetSwapchainImagesKHR(og_ctx->logical_device, og_ctx->swapchain, &sc_img_count, 0);

	vkGetSwapchainImagesKHR(og_ctx->logical_device, og_ctx->swapchain, &sc_img_count, og_ctx->sc_images);
}

OG_INT void _create_command_pool(OGContext *og_ctx) {
	VkCommandPoolCreateInfo pool_create_info = {};
	pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_create_info.queueFamilyIndex = og_ctx->graphics_idx;

	OG_CHECK_VK(vkCreateCommandPool(og_ctx->logical_device, &pool_create_info, NULL,
				&og_ctx->command_pool), "Command Pool Creation Failed");
}

OG_INT void _create_sync_objects(OGContext *og_ctx) {
	VkSemaphoreCreateInfo sem_create_info = {};
	sem_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	OG_CHECK_VK(vkCreateSemaphore(og_ctx->logical_device, &sem_create_info,
				NULL, &og_ctx->acquire_img_semaphore), "Acquire Image Semaphore Creation Failed");
	OG_CHECK_VK(vkCreateSemaphore(og_ctx->logical_device, &sem_create_info,
				NULL, &og_ctx->submit_semaphore), "Submit Semaphore Creation Failed");
}
