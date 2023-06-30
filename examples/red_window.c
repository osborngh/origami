#include "origami/og_renderer.h"

#define WIDTH 800
#define HEIGHT 600

// #define OG_DEBUG_STRINGS // Human Readable Text For Debugging

void render();

int main() {
	OGContext ctx = {};
	OGContext* p_ctx = &ctx;
	OGConfig cfg = { 
		.vd_layers = true,
		.app_name = "Red Window",
		.win_width = WIDTH,
		.win_height = HEIGHT,
	};

	og_init(p_ctx, &cfg);

	while (p_ctx->running) {
		og_poll_events(p_ctx);
		og_render(p_ctx, render);
	}
	// vkDeviceWaitIdle(p_ctx->logical_device);
	og_quit(p_ctx);
}

void render(OGContext *og_ctx) {
	OGColor color = {{1, 0, 0, 1}};
	og_clear_screen(og_ctx, color);
}
