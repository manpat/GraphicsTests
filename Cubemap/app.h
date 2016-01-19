#ifndef APP_H
#define APP_H

#include "common.h"

#include <SDL2/SDL.h>

struct App {
	Log logger{"App"};
	SDL_Window* window;
	SDL_GLContext glctx;

	bool running = true;

	App();
	~App();

	void Run();
};

#endif