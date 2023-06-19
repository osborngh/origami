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
	VkSurfaceKHR surface;
	VkDebugUtilsMessengerEXT debug_messenger;

	int graphics_idx;
	VkPhysicalDevice physical_device;
	VkDevice logical_device;
	VkSwapchainKHR swapchain;

	bool running;
} OGContext;

// static VKAPI_ATTR VkBool32 VKAPI_CALL __debug_callback();

void _init_window(OGContext *og_ctx, OGConfig *og_cfg);
void _init_vulkan(OGContext *og_ctx, OGConfig *og_cfg);
void _create_surface(OGContext *og_ctx);
bool _main_loop(OGContext *og_ctx);
void _handle_default_events(OGContext *og_ctx);
void _choose_physical_device(OGContext *og_ctx);
void _create_logical_device(OGContext *og_ctx);


// Origami's API
OG_API void og_init(OGContext *og_ctx, OGConfig *og_cfg);
OG_API void og_poll_events(OGContext *og_ctx);
OG_API void og_quit(OGContext *og_ctx);

#endif // __OG_RENDERER_H__
