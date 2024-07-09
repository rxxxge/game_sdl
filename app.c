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
	actor_t **actors;					// Dynamic array of pointers to actors
	int actor_count;					// Number of actors in the game
	SDL_Renderer *renderer;				// Renderer for drawing actors
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

// Handles movement without delays
void handle_continuous_input(app_t *app, actor_t *actor, config_t config) {

	app->key_state = SDL_GetKeyboardState(NULL);

	if (app->key_state[SDL_SCANCODE_RIGHT]) {
		actor->dest_rect.x += actor->speed * app->delta_time;
		// Right boundary
		if (actor->dest_rect.x + actor->dest_rect.w > (int)config.window_width)
            actor->dest_rect.x = config.window_width - actor->dest_rect.w;  
	}
	else if (app->key_state[SDL_SCANCODE_LEFT]) {
		actor->dest_rect.x -= actor->speed * app->delta_time;
		// Left boundary
		if (actor->dest_rect.x < 0)
            actor->dest_rect.x = 0; 
	}
	else if (app->key_state[SDL_SCANCODE_UP]) {
		actor->dest_rect.y -= actor->speed * app->delta_time;
		// Upper boundary
		if (actor->dest_rect.y < 0)
            actor->dest_rect.y = 0;
	}
	else if (app->key_state[SDL_SCANCODE_DOWN]) {
		actor->dest_rect.y += actor->speed * app->delta_time;
		// Bottom boundary
		if (actor->dest_rect.y + actor->dest_rect.h > (int)config.window_height)
            actor->dest_rect.y = config.window_height - actor->dest_rect.h;
	}
 
	app->frame_time += app->delta_time;

	if (app->frame_time >= 0.20f) {
		app->frame_time = 0;
		actor->src_rect.x += actor->frame_widht;
		if (actor->src_rect.x >= actor->texture_width)
			actor->src_rect.x = 0;
	}
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

bool load_actor(game_t *game, actor_t *actor, config_t config, const char *actor_src) {
	// load and initialize actor
	actor->texture = IMG_LoadTexture(game->renderer, actor_src);
	if (!actor->texture) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not load actor texture into graphics hardware memory: %s\n", SDL_GetError());
		return false;
	}

	SDL_Log("Loading actor texture into graphics memory\n");
	SDL_QueryTexture(actor->texture, NULL, NULL, &actor->texture_width, &actor->texture_height);

	actor->frame_widht = actor->texture_width / 4;
	actor->frame_height = actor->texture_height / 13;
	

	// For Animation
	actor->src_rect.x = actor->src_rect.y = 0;
	actor->src_rect.w = actor->frame_widht;
	actor->src_rect.h = actor->frame_height;

	// Size + position
	actor->dest_rect.w = actor->frame_widht * 5;
	actor->dest_rect.h = actor->frame_height * 5;
	actor->dest_rect.x = (config.window_width - actor->dest_rect.w) / 2;
	actor->dest_rect.y = (config.window_height - actor->dest_rect.h) / 2;
	actor->speed = 500.0f;

	return true;
}

bool add_actor(game_t *game, config_t config, const char *actor_src) {

	actor_t actor = {0};
	if (!load_actor(game, &actor, config, actor_src)) return false;

	game->actors = realloc(game->actors, sizeof(actor_t *) * (game->actor_count + 1));
	game->actors[game->actor_count++] = &actor;

	return true;
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

	// Initialize game
	game_t game = {0};
	app.game = game;
	game.actor_count = 0;


	// actor_t actor = {0};
	// if (!load_actor(&app, &actor, config, "player/CAT_GRAY.png")) exit(EXIT_FAILURE);

	if (!add_actor(&game, config, "player/CAT_GRAY.png")) exit(EXIT_FAILURE);

	// Game Loop
	while (app.state != QUIT) {

		app.prev_time = app.current_time;
		app.current_time = SDL_GetTicks();
		app.delta_time = (app.current_time - app.prev_time) / 1000.0f;  // Miliseconds passed
		
		// Handle input
		handle_input(&app);

		if (app.state == PAUSED) continue;

		handle_continuous_input(&app, game.actors[0], config);

		
		SDL_RenderClear(app.renderer);														// Clear the screen
		SDL_RenderCopy(app.renderer, game.actors[0]->texture, &game.actors[0]->src_rect, &game.actors[0]->dest_rect);		
		SDL_RenderPresent(app.renderer);													// Trigger the double buffers for multiple rendering

		// 60 fps
		// SDL_Delay(1000 / FPS);
	}


	// Shutdown and cleanup
	cleanup(&app);
	exit(EXIT_SUCCESS);
}