#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <SDL.h>
#include <SDL_image.h>

#define FPS 165
#define WINDOW_WIDTH 2560
#define WINDOW_HEIGHT 1440

typedef enum {
	MOVING_DOWN,
	MOVING_RIGHT,
	MOVING_LEFT,
	MOVING_UP,
	IDLE,
} actor_state_t;

typedef struct {
	actor_state_t state;
	int animation_key;
	SDL_Texture *texture;				// Driver-specific representation of pixel data
	SDL_Rect src_rect;					// To load texture and display animation
	SDL_Rect dest_rect;					// To scale and change position
	float speed;						// Actor speed
	int frame_widht, texture_width;		// Texture width/height is used to store width/height of the original sprite image with animation
	int frame_height, texture_height;	// Frame width/height is used to store widht/height of one single sprite in the texture image
} actor_t;

typedef enum {
	CAT_GRAY,
	CAT_ORANGE,
	FOX,
	BIRD_BLUE,
	BIRD_WHITE,
	RACOON,
} animal_t;

typedef struct {
	animal_t animal;
	actor_t **actors;					// Dynamic array of pointers to actors
	int actor_count;					// Number of actors in the game
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
		.window_width 	= WINDOW_WIDTH,
		.window_height 	= WINDOW_HEIGHT,
		.flags 			= SDL_WINDOW_RESIZABLE,
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
		// Animation moving right
		actor->src_rect.y = actor->frame_height * (actor->animation_key + 9);
		actor->state = MOVING_RIGHT;
		// Right boundary
		if (actor->dest_rect.x + actor->dest_rect.w > (int)config.window_width)
            actor->dest_rect.x = config.window_width - actor->dest_rect.w;  
	}
	else if (app->key_state[SDL_SCANCODE_LEFT]) {
		actor->dest_rect.x -= actor->speed * app->delta_time;
		// Animation moving left
		actor->src_rect.y = actor->frame_height * (actor->animation_key + 7);
		actor->state = MOVING_LEFT;
		// Left boundary
		if (actor->dest_rect.x < 0)
            actor->dest_rect.x = 0; 
	}
	else if (app->key_state[SDL_SCANCODE_UP]) {
		actor->dest_rect.y -= actor->speed * app->delta_time;
		// Animation moving up
		actor->src_rect.y = actor->frame_height * (actor->animation_key + 11);
		actor->state = MOVING_UP;
		// Upper boundary
		if (actor->dest_rect.y < 0)
            actor->dest_rect.y = 0;
	}
	else if (app->key_state[SDL_SCANCODE_DOWN]) {
		actor->dest_rect.y += actor->speed * app->delta_time;
		// Animation moving down
		actor->src_rect.y = actor->frame_height * (actor->animation_key + 5);
		actor->state = MOVING_DOWN;
		// Bottom boundary
		if (actor->dest_rect.y + actor->dest_rect.h > (int)config.window_height)
            actor->dest_rect.y = config.window_height - actor->dest_rect.h;
	}
	else {
		// Animation idle
		if (actor->state != IDLE) 
			actor->src_rect.y = actor->frame_height * actor->state;
		actor->state = IDLE;
	}
 
	app->frame_time += app->delta_time;

	if (app->frame_time >= 0.20f) {
		app->frame_time = 0;
		actor->src_rect.x += actor->frame_widht;
		if (actor->state != IDLE && actor->src_rect.x >= actor->texture_width) {
			actor->animation_key = (actor->animation_key + 1) % 2;
		}
		actor->src_rect.x %= actor->texture_width;
	}
}

bool load_actor(app_t *app, actor_t *actor, config_t config, const char *actor_src) {
	// load and initialize actor
	actor->texture = IMG_LoadTexture(app->renderer, actor_src);
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
	#ifdef INIT_ACTOR // Needs fix
	actor->dest_rect.x = (config.window_width - actor->dest_rect.w) / 2;
	actor->dest_rect.y = (config.window_height - actor->dest_rect.h) / 2;
	#else
	(void)config;
	#endif
	actor->speed = 500.0f;

	return true;
}

bool add_actor(game_t *game, actor_t *actor) {
	game->actors = realloc(game->actors, sizeof(actor_t *) * (game->actor_count + 1));
	if (!game->actors) { 
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Not enough memory to reallocate.\n");
		return false;
	}
	game->actors[game->actor_count++] = actor;

	return true;
}

void handle_input(app_t *app, config_t config) {
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

			case SDLK_c:
				switch (++app->game.animal % 6) {
				case CAT_GRAY:
					if (!load_actor(app, app->game.actors[0], config, "player/CAT_GRAY.png")) exit(EXIT_FAILURE);
					app->game.animal = CAT_GRAY;
					break;
				case CAT_ORANGE:
					if (!load_actor(app, app->game.actors[0], config, "player/CAT_ORANGE.png")) exit(EXIT_FAILURE);
					app->game.animal = CAT_ORANGE;
					break;
				case FOX:
					if (!load_actor(app, app->game.actors[0], config, "player/FOX.png")) exit(EXIT_FAILURE);
					app->game.animal = FOX;
					break;
				case BIRD_BLUE:
					if (!load_actor(app, app->game.actors[0], config, "player/BIRD_BLUE.png")) exit(EXIT_FAILURE);
					app->game.animal = BIRD_BLUE;
					break;
				case BIRD_WHITE:
					if (!load_actor(app, app->game.actors[0], config, "player/BIRD_WHITE.png")) exit(EXIT_FAILURE);
					app->game.animal = BIRD_WHITE;
					break;
				case RACOON:
					if (!load_actor(app, app->game.actors[0], config, "player/RACOON.png")) exit(EXIT_FAILURE);
					app->game.animal = RACOON;
					break;
				break;
			}

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

	// Initialize game
	game_t game = {0};
	app.game = game;
	app.game.actor_count = 0;
	app.game.animal = CAT_GRAY;				// Default is gray cat :/

	// Load player texture into the game
	actor_t actor = {.state = IDLE, .animation_key = 0};
	if (!load_actor(&app, &actor, config, "player/CAT_GRAY.png")) exit(EXIT_FAILURE);
	if (!add_actor(&app.game, &actor)) exit(EXIT_FAILURE);

	// Game Loop
	while (app.state != QUIT) {

		app.prev_time = app.current_time;
		app.current_time = SDL_GetTicks();
		app.delta_time = (app.current_time - app.prev_time) / 1000.0f;  // Miliseconds passed
		
		// Handle input
		handle_input(&app, config);

		if (app.state == PAUSED) continue;

		handle_continuous_input(&app, app.game.actors[0], config);

		SDL_RenderClear(app.renderer);																					// Clear the screen
		SDL_RenderCopy(app.renderer, app.game.actors[0]->texture, &app.game.actors[0]->src_rect, &app.game.actors[0]->dest_rect);		
		SDL_RenderPresent(app.renderer);																				// Trigger the double buffers for multiple rendering

		// 60 fps
		// SDL_Delay(1000 / FPS);
	}


	// Shutdown and cleanup
	cleanup(&app);
	exit(EXIT_SUCCESS);
}