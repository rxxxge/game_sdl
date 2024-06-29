// TODO ==================================
/*
	1. MORE REFACTORING
	2. FINISH MOVEMENT
	3. LOAD MAP
	4. ADD COLLISION
	5. WORK WITH CAMERA
*/ 
// =======================================

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <SDL.h>
#include <SDL_image.h>

#define FPS 165

typedef struct {
	SDL_Texture *texture;				// Driver-specific representation of pixel data
	SDL_Rect src_rect;					// To load texture and display animation
	SDL_Rect dest_rect;					// To scale and change position
	float speed;						// Actor speed
	int frame_widht, texture_width;		// Texture width/height is used to store width/height of the original sprite image with animation
	int frame_height, texture_height;	// Frame width/height is used to store widht/height of one single sprite in the texture image
} actor_t;

typedef struct {
	actor_t **actors;
	int actor_count;
} game_t;

// Application state
typedef enum {
	QUIT,
	RUNNING,
	PAUSED,
} app_state_t;

// Application type struct
typedef struct {
	// Configuration
	app_state_t state;
	SDL_Window *window;				// The opaque type used to identify a window
	SDL_Renderer *renderer;			// A structure representing rendering state
	actor_t actor;

	// Game state
	game_t game;

	// Timers/Controls
	float frame_time;
	int prev_time;
	int current_time;
	float delta_time;
	const uint8_t *key_state;
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
	app->frame_time = 0;
	app->prev_time = 0;
	app->current_time = 0;
	app->delta_time = 0;

	return true;
}


bool set_config(config_t *config, int argc, char *argv[]) {
	// Set default settings
	*config = (config_t){
		.title 			= "App",
		.window_width 	= 1920,
		.window_height 	= 1080,
		.flags 			= 0,
		.renderer_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC,
	};

	// Override defaults
	for (int i = 1; i < argc; ++i)
		(void)argv[i];	// Unused variables

	return true;
}

#ifdef DEBUG
bool load_image(app_t *app, const config_t config, const char *img_file) {

	// Load image from path
	app->actor.texture = IMG_LoadTexture(app->renderer, img_file);
	if (!app->actor.texture) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not load texture into graphics hardware memory: %s\n", SDL_GetError());
		return false;
	}

	// Load image into graphics hardware memory
	// app->actor.texture = SDL_CreateTextureFromSurface(app->renderer, app->actor.surface);
	// if (!app->actor.texture) {
	// 	SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not load image into graphics hardware memory: %s\n", SDL_GetError());
	// 	return false;
	// }

	// Clears main memory
	// SDL_FreeSurface(app->actor.surface);

	// 
	SDL_QueryTexture(app->actor.texture, NULL, NULL, &app->actor.texture_width, &app->actor.texture_height);

	app->actor.frame_widht = app->actor.texture_width / 4;
	app->actor.frame_height = app->actor.texture_height / 13;

	// app->actor.rect.w /= 2;
	// app->actor.rect.h /= 2;
	app->actor.rect.w = app->actor.frame_widht;
	app->actor.rect.h = app->actor.frame_height;

	// app->actor.rect.x = (config.window_width - app->actor.rect.w) / 2;
	// app->actor.rect.y = (config.window_height - app->actor.rect.h) / 2;
	(void)config; // ==========================================================

	app->actor.rect.x = 0;
	app->actor.rect.y = 0;

	app->actor.speed = 300.0f;

	return true;
}
#endif

void handle_input(app_t *app, SDL_Rect *dest_rect, const float delta_time) {
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

			case SDLK_w:
			case SDLK_UP:
				dest_rect->y -= app->actor.speed * delta_time;
				// Upper boundary
				if (dest_rect->y < 0) {
					dest_rect->y = 0;
			 		break;
				}
				break;

			case SDLK_s:
			case SDLK_DOWN:
				dest_rect->y += app->actor.speed * delta_time;
			    // Bottom boundary
				if (dest_rect->y + dest_rect->h > 1080) {
					dest_rect->y = 1080 - dest_rect->h;
					break;
				}
				break;

			case SDLK_a:
			case SDLK_LEFT:
				dest_rect->x -= app->actor.speed * delta_time;
				// Left boundary
			    if (dest_rect->x < 0) {
			    	dest_rect->x = 0;
			    	break;
			    }
				break;

			case SDLK_d:
			case SDLK_RIGHT:
                dest_rect->x += app->actor.speed * delta_time;
    			// Right boundary
                if (dest_rect->x + dest_rect->w > 1920) {
                    dest_rect->x = 1920 - dest_rect->w;  
                    break;
                }
				break;

			default:
				break;
			}

			break;

		default:
			break;
		}
	}
}

void handle_continuous_input(void) {
	return;
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

	// if (!load_image(&app, config, "player/CAT_ORANGE.png")) exit(EXIT_FAILURE);


	app.actor.texture = IMG_LoadTexture(app.renderer, "player/CAT_GRAY.png");
	if (!app.actor.texture) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not load texture into graphics hardware memory: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	SDL_QueryTexture(app.actor.texture, NULL, NULL, &app.actor.texture_width, &app.actor.texture_height);

	app.actor.frame_widht = app.actor.texture_width / 4;
	app.actor.frame_height = app.actor.texture_height / 13;
	

	// For Animation
	app.actor.src_rect.x = app.actor.src_rect.y = 0;
	app.actor.src_rect.w = app.actor.frame_widht;
	app.actor.src_rect.h = app.actor.frame_height;

	// Size + position
	// SDL_Rect dest_rect;
	app.actor.dest_rect.w = app.actor.frame_widht * 5;
	app.actor.dest_rect.h = app.actor.frame_height * 5;
	app.actor.dest_rect.x = (config.window_width - app.actor.dest_rect.w) / 2;
	app.actor.dest_rect.y = (config.window_height - app.actor.dest_rect.h) / 2;
	app.actor.speed = 500.0f;

	// Game Loop
	while (app.state != QUIT) {

		app.prev_time = app.current_time;
		app.current_time = SDL_GetTicks();
		app.delta_time = (app.current_time - app.prev_time) / 1000.0f;  // Miliseconds passed
		
		// Handle input
		handle_input(&app, &app.actor.dest_rect, app.delta_time);

		if (app.state == PAUSED) continue;

		app.key_state = SDL_GetKeyboardState(NULL);
		if (app.key_state[SDL_SCANCODE_RIGHT])
			app.actor.dest_rect.x += app.actor.speed * app.delta_time;
		else if (app.key_state[SDL_SCANCODE_LEFT])
			app.actor.dest_rect.x -= app.actor.speed * app.delta_time;

		app.frame_time += app.delta_time;

		if (app.frame_time >= 0.20f) {
			app.frame_time = 0;
			app.actor.src_rect.x += app.actor.frame_widht;
			if (app.actor.src_rect.x >= app.actor.texture_width)
				app.actor.src_rect.x = 0;
		}

		
		SDL_RenderClear(app.renderer);														// Clear the screen
		SDL_RenderCopy(app.renderer, app.actor.texture, &app.actor.src_rect, &app.actor.dest_rect);		
		SDL_RenderPresent(app.renderer);													// Trigger the double buffers for multiple rendering

		// 60 fps
		// SDL_Delay(1000 / FPS);
	}


	// Shutdown and cleanup
	cleanup(&app);
	exit(EXIT_SUCCESS);
}