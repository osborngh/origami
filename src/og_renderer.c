/* Author: Da Vinci
 * This Code is in the Public Domain
 */

#include "origami/og_renderer.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL __debug_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
		VkDebugUtilsMessageTypeFlagsEXT msgType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void *pUserData) {
	OG_LOG_INFOVAR("VALIDATION ERROR", pCallbackData->pMessage);
	return false;
}

void _init_window(OGContext *og_ctx, OGConfig *og_cfg) {
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

void _init_vulkan(OGContext *og_ctx, OGConfig *og_cfg) {
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

	uint32_t glfw_extc = 0;
	const char** glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_extc);

	VkInstanceCreateInfo instance_info = {};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app_info;
	instance_info.enabledExtensionCount = glfw_extc;
	instance_info.ppEnabledExtensionNames = glfw_ext;

	if (og_cfg->vd_layers) {
		instance_info.enabledLayerCount = OG_ARR_SIZE(layers);
		instance_info.ppEnabledLayerNames = layers;
	} else {
		instance_info.enabledLayerCount = 0;
	}
	OG_CHECK_VK(vkCreateInstance(&instance_info, NULL, &og_ctx->instance), "Vulkan Instance Creation Failed");

	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(og_ctx->instance, "vkCreateDebugUtilsMessengerEXT");

	if (func) {
		VkDebugUtilsMessengerCreateInfoEXT debug_info = {};
		debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debug_info.pfnUserCallback = __debug_callback;

		func(og_ctx->instance, &debug_info, NULL, &og_ctx->debug_messenger);
	}
}

void _create_surface(OGContext *og_ctx) {
	OG_CHECK_VK(glfwCreateWindowSurface(og_ctx->instance, og_ctx->window, NULL, &og_ctx->surface), "Window Surface Creation Failed");
}

void _handle_default_events(OGContext *og_ctx) {
	if (glfwGetKey(og_ctx->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		og_ctx->running = false;
	}
}

void _choose_physical_device(OGContext *og_ctx) {
	og_ctx->graphics_idx = -1;

	uint32_t pd_count = 0;
	VkPhysicalDevice pd_devices[5];
	vkEnumeratePhysicalDevices(og_ctx->instance, &pd_count, NULL);

	vkEnumeratePhysicalDevices(og_ctx->instance, &pd_count, pd_devices);

	for (uint32_t i = 0; i < pd_count; i++) {
		VkPhysicalDevice p_device = pd_devices[i];

		uint32_t queue_family_count = 0;
		VkQueueFamilyProperties queue_properties[10];

		vkGetPhysicalDeviceQueueFamilyProperties(p_device, &queue_family_count, NULL);
		vkGetPhysicalDeviceQueueFamilyProperties(p_device, &queue_family_count, queue_properties);

		for (uint32_t j = 0; j < queue_family_count; j++) {
			if (queue_properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				VkBool32 surface_support = VK_FALSE;
				OG_CHECK_VK(vkGetPhysicalDeviceSurfaceSupportKHR(p_device, j, og_ctx->surface, &surface_support), "Physical Device Surface Support Not Available");

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

void _create_logical_device(OGContext *og_ctx) {
	const char* extensions[] = {
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

	OG_CHECK_VK(vkCreateDevice(og_ctx->physical_device, &device_info, NULL, &og_ctx->logical_device), "Logical Device Creation Failed");
}

void _create_swapchain(OGContext *og_ctx) {

	VkSurfaceCapabilitiesKHR surf_caps = {};
	OG_CHECK_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(og_ctx->physical_device, og_ctx->surface, &surf_caps), "Surface Capabilities Not Available");

	VkSwapchainCreateInfoKHR sc_info = {};
	sc_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	sc_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	sc_info.surface = og_ctx->surface;
	sc_info.preTransform = surf_caps.currentTransform;
	sc_info.imageExtent = surf_caps.currentExtent;
	sc_info.minImageCount = (surf_caps.minImageCount + 1) > surf_caps.maxImageCount ?
		sc_info.minImageCount - 1 : sc_info.minImageCount;

	OG_CHECK_VK(vkCreateSwapchainKHR(og_ctx->logical_device, &sc_info, NULL, &og_ctx->swapchain), "SwapChain Creation Failed");
}


void og_init(OGContext *og_ctx, OGConfig *og_cfg) {
	_init_window(og_ctx, og_cfg);
	_init_vulkan(og_ctx, og_cfg);

	_create_surface(og_ctx);

	_choose_physical_device(og_ctx);
	_create_logical_device(og_ctx);

	og_ctx->running = true;
}

void og_poll_events(OGContext *og_ctx) {
	glfwPollEvents();

	_handle_default_events(og_ctx);
}

void og_quit(OGContext *og_ctx) {
	vkDestroyDevice(og_ctx->logical_device, NULL);
	vkDestroySurfaceKHR(og_ctx->instance, og_ctx->surface, NULL);
	glfwDestroyWindow(og_ctx->window);
	glfwTerminate();
	vkDestroyInstance(og_ctx->instance, NULL);
}
