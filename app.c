#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <SDL.h>
#include <SDL_image.h>


// Application state
typedef enum {
	QUIT,
	RUNNING,
	PAUSED,
} app_state_t;

// Application type struct
typedef struct {
	app_state_t state;
	SDL_Window *window;			// The opaque type used to identify a window
	SDL_Renderer *renderer;		// A structure representing rendering state
	SDL_Surface *surface;		// A collection of pixels used in software blitting
	SDL_Texture *texture;		// Driver-specific representation of pixel data
} app_t;

// Configuration of Application
typedef struct {
	const char *title;
	uint32_t window_width;
	uint32_t window_height;
	uint32_t flags, renderer_flags;
} config_t;


bool init_app(app_t *app, const config_t config) {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not initialize SDL: %s\n", SDL_GetError());
		return false;
	}
	SDL_Log("SDL initialized.\n");

	app->window = SDL_CreateWindow(config.title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
									config.window_width, config.window_height, config.flags);
	if (!app->window) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
		return false;
	}

	app->renderer = SDL_CreateRenderer(app->window, -1, config.renderer_flags);
	if (!app->renderer) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create renderer: %s\n", SDL_GetError());
		return false;
	}

	// If everything is OK set state to RUNNING
	app->state = RUNNING;

	return true;
}


bool set_config(config_t *config, int argc, char *argv[]) {
	// Set default settings
	*config = (config_t){
		.title 			= "App",
		.window_width 	= 1920,
		.window_height 	= 1080,
		.flags 			= 0,
		.renderer_flags = SDL_RENDERER_ACCELERATED,
	};

	// Override defaults
	for (int i = 1; i < argc; ++i)
		(void)argv[i];	// Unused variables

	return true;
}


bool load_image(app_t *app, const char *img_file) {
	// Load image from path
	app->surface = IMG_Load(img_file);
	if (!app->surface) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not load image file: %s\n", SDL_GetError());
		return false;
	}

	// Load image into graphics hardware memory
	app->texture = SDL_CreateTextureFromSurface(app->renderer, app->surface);
	if (!app->texture) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not load image into graphics hardware memory: %s\n", SDL_GetError());
		return false;
	}

	// Clears main memory
	SDL_FreeSurface(app->surface);

	return true;
}

void handle_input(app_t *app) {
	SDL_Event event;

	while(SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			app->state = QUIT;
			return;
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_ESCAPE:
				// Esc button
				app->state = QUIT;
				return;

			case SDLK_SPACE:
				// Space bar
				if (app->state == RUNNING) {
					app->state = PAUSED;
					puts("#######  PAUSED   #######");
				} else {
					app->state = RUNNING;
					puts("#######  RESUMED  #######");
				}
				return;

			default:
				break;
			}
			break;

		default:
			break;
		}
	}
}


void cleanup(app_t *app) {
	SDL_Log("Destroying renderer\n");
	SDL_DestroyRenderer(app->renderer);

	SDL_Log("Destroying window\n");
	SDL_DestroyWindow(app->window);

	SDL_Log("Quitting SDL\n");
	SDL_Quit();
}

int main(int argc, char *argv[]) {

	// Set app configuration
	config_t config = {0};
	if (!set_config(&config, argc, argv)) exit(EXIT_FAILURE);

	// Initialize SDL app
	app_t app = {0};
	if (!init_app(&app, config)) exit(EXIT_FAILURE);

	if (!load_image(&app, "character.png")) exit(EXIT_FAILURE);

	// Control image position ======================
	SDL_Rect dest;
	SDL_QueryTexture(app.texture, NULL, NULL, &dest.w, &dest.h);

	dest.w /= 6;
	dest.h /= 6;

	dest.x = (1920 - dest.w) / 2;
	dest.y = (1080 - dest.h) / 2;
	// =============================================

	// Game Loop
	while (app.state != QUIT) {
		// Handle input
		handle_input(&app);

		// Clear the screen
		SDL_RenderClear(app.renderer);
		SDL_RenderCopy(app.renderer, app.texture, NULL, &dest);

		// Triggers the double buffers for multiple rendering
		SDL_RenderPresent(app.renderer);

		// 60 fps
		SDL_Delay(1000 / 60);
	}


	// Shutdown and cleanup
	cleanup(&app);
	exit(EXIT_SUCCESS);
}