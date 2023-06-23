/* Author: Da Vinci
 * This Code is in the Public Domain
 */

#ifndef __OG_RENDERER_H__
#define __OG_RENDERER_H__

#include "common.h"

typedef struct {
	bool vd_layers;
	const char* app_name;
	uint32_t win_width;
	uint32_t win_height;
} OGConfig;

typedef struct {
	GLFWwindow* window;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;

	VkSurfaceKHR surface;
	VkSurfaceFormatKHR surf_format;

	VkPhysicalDevice physical_device;
	VkDevice logical_device;
	VkSwapchainKHR swapchain;

	VkQueue graphics_queue;
	VkImage sc_images[5];
	VkCommandPool command_pool;
	VkCommandBuffer curr_cmd_buffer;
	

	VkSemaphore acquire_img_semaphore;
	VkSemaphore submit_semaphore;

	uint32_t img_idx;
	uint32_t graphics_idx;
	bool running;
} OGContext;

// static VKAPI_ATTR VkBool32 VKAPI_CALL __debug_callback();

OG_INT void _init_window(OGContext *og_ctx, OGConfig *og_cfg);
OG_INT void _init_vulkan(OGContext *og_ctx, OGConfig *og_cfg);
OG_INT void _create_surface(OGContext *og_ctx);
OG_INT bool _main_loop(OGContext *og_ctx);
OG_INT void _handle_default_events(OGContext *og_ctx);
OG_INT void _choose_physical_device(OGContext *og_ctx);
OG_INT void _create_logical_device(OGContext *og_ctx);
OG_INT void _create_swapchain(OGContext *og_ctx);
OG_INT void _create_command_pool(OGContext *og_ctx);
OG_INT void _create_sync_objects(OGContext *og_ctx);


// Origami's API

OG_API typedef VkClearColorValue OGColor;

OG_API void og_init(OGContext *og_ctx, OGConfig *og_cfg);
OG_API void og_poll_events(OGContext *og_ctx);
OG_API void og_clear_screen(OGContext *og_ctx, OGColor color);
OG_API void og_render(OGContext *og_ctx, void(*render)());
OG_API void og_quit(OGContext *og_ctx);

#endif // __OG_RENDERER_H__
