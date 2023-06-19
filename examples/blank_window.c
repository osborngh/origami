#include "origami/og_renderer.h"

#define WIDTH 800
#define HEIGHT 600

int main() {
	OGContext ctx = {};
	OGContext* p_ctx = &ctx;
	OGConfig cfg = { 
		.vd_layers = true,
		.app_name = "My App",
		.win_width = WIDTH,
		.win_height = HEIGHT,
	};

	og_init(p_ctx, &cfg);

	while (p_ctx->running) {
		og_poll_events(&ctx);

		// Your Drawing Goes In Here

		// og_update(&ctx);
	}

	og_quit(p_ctx);
}
