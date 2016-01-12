#include "common.h"
#include "color.h"

#include <SDL2/SDL.h>

// http://www.alanzucconi.com/2016/01/06/secrets-colour-interpolation/#acp_wrapper

static Log logger{"Main"};

std::ostream& operator<<(std::ostream& o, const Color& c) {
	return o << "RGB{"
		<< c.r << ", "
		<< c.g << ", "
		<< c.b << "}";
}
std::ostream& operator<<(std::ostream& o, const LABColor& c) {
	return o << "LAB{"
		<< c.l << ", "
		<< c.a << ", "
		<< c.b << "}";
}
std::ostream& operator<<(std::ostream& o, const LCHColor& c) {
	return o << "LCH{"
		<< c.l << ", "
		<< c.c << ", "
		<< c.h << "}";
}

s32 main() {
	logger << "Init" << Log::NL;
	Color black{0,0,0};
	Color white{1,1,1};
	Color red  {1,0,0};

	logger << Log::NL << "RGB";
	logger << "Black: " << black;
	logger << "White: " << white;
	logger << "Red:   " << red;

	logger << Log::NL << "RGB -> LAB";
	auto blackLAB = black.ToLAB();
	auto whiteLAB = white.ToLAB();
	auto redLAB   = red.ToLAB();
	logger << "Black: " << blackLAB;
	logger << "White: " << whiteLAB;
	logger << "Red:   " << redLAB;

	logger << Log::NL << "LAB -> RGB";
	logger << "Black: " << blackLAB.ToRGB();
	logger << "White: " << whiteLAB.ToRGB();
	logger << "Red:   " << redLAB.ToRGB();

	logger << Log::NL << "RGB -> LCH";
	auto blackLCH = black.ToLCH();
	auto whiteLCH = white.ToLCH();
	auto redLCH   = red.ToLCH();
	logger << "Black: " << blackLCH;
	logger << "White: " << whiteLCH;
	logger << "Red:   " << redLCH;

	logger << Log::NL << "LCH -> RGB";
	logger << "Black: " << blackLCH.ToRGB();
	logger << "White: " << whiteLCH.ToRGB();
	logger << "Red:   " << redLCH.ToRGB();

	constexpr u32 wwidth = 800;
	constexpr u32 wheight = 200;

	if(SDL_Init(SDL_INIT_EVERYTHING) < 0){
		logger << "SDL init failed";
		return 1;
	}

	auto window = SDL_CreateWindow("Color interpolation", 
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, wwidth, wheight, 0);

	if(!window) {
		logger << "Window creation failed";
		return 1;
	}

	SDL_Surface* screen = SDL_GetWindowSurface(window);

	auto renderer = SDL_CreateSoftwareRenderer(screen);
	if(!renderer){
		logger << "Renderer creation failed";
		return 1;
	}

	constexpr u32 width = 16;
	constexpr u32 stride = 4;
	u8* rgbps = new u8[width * stride];
	u8* hsvps = new u8[width * stride];
	u8* labps = new u8[width * stride];
	u8* lchps = new u8[width * stride];

	SDL_Surface* rgbSurface = SDL_CreateRGBSurfaceFrom(rgbps, width, 1, 32, width*stride, 0,0,0,0);
	SDL_Surface* hsvSurface = SDL_CreateRGBSurfaceFrom(hsvps, width, 1, 32, width*stride, 0,0,0,0);
	SDL_Surface* labSurface = SDL_CreateRGBSurfaceFrom(labps, width, 1, 32, width*stride, 0,0,0,0);
	SDL_Surface* lchSurface = SDL_CreateRGBSurfaceFrom(lchps, width, 1, 32, width*stride, 0,0,0,0);

	bool running = true;
	while(running){
		SDL_Event e;
		while(SDL_PollEvent(&e)){
			if(e.type == SDL_QUIT) running = false;
			switch(e.type){
				case SDL_QUIT: running = false; break;
				case SDL_KEYDOWN: {
					if(e.key.keysym.sym == SDLK_ESCAPE) running = false;
					break;
				}
			}
		}

		Color beginColor{80./255, 30./255, 80./255};
		Color endColor  {239./255, 217./255, 217./255};

		auto beginHSV = beginColor.ToHSV();
		auto endHSV = endColor.ToHSV();

		auto beginLAB = beginColor.ToLAB();
		auto endLAB = endColor.ToLAB();

		auto beginLCH = beginColor.ToLCH();
		auto endLCH = endColor.ToLCH();

		for(u32 i = 0; i < width; i++) {
			f32 a = (f32)i / (width-1);
			f32 ia = 1.f-a;

			rgbps[i*stride+3] = 255;
			rgbps[i*stride+2] = (u8)((beginColor.r * ia + endColor.r * a)*255.f);
			rgbps[i*stride+1] = (u8)((beginColor.g * ia + endColor.g * a)*255.f);
			rgbps[i*stride+0] = (u8)((beginColor.b * ia + endColor.b * a)*255.f);

			auto rgb = HSVColor {
				beginHSV.h * ia + endHSV.h * a,
				beginHSV.s * ia + endHSV.s * a,
				beginHSV.v * ia + endHSV.v * a,
			}.ToRGB();

			hsvps[i*stride+3] = 255;
			hsvps[i*stride+2] = (u8)(rgb.r * 255.f);
			hsvps[i*stride+1] = (u8)(rgb.g * 255.f);
			hsvps[i*stride+0] = (u8)(rgb.b * 255.f);

			rgb = LABColor {
				beginLAB.l * ia + endLAB.l * a,
				beginLAB.a * ia + endLAB.a * a,
				beginLAB.b * ia + endLAB.b * a,
			}.ToRGB();

			labps[i*stride+3] = 255;
			labps[i*stride+2] = (u8)(rgb.r * 255.f);
			labps[i*stride+1] = (u8)(rgb.g * 255.f);
			labps[i*stride+0] = (u8)(rgb.b * 255.f);

			rgb = LCHColor {
				beginLCH.l * ia + endLCH.l * a,
				beginLCH.c * ia + endLCH.c * a,
				beginLCH.h * ia + endLCH.h * a,
			}.ToRGB();

			lchps[i*stride+3] = 255;
			lchps[i*stride+2] = (u8)(rgb.r * 255.f);
			lchps[i*stride+1] = (u8)(rgb.g * 255.f);
			lchps[i*stride+0] = (u8)(rgb.b * 255.f);
		}

		SDL_RenderClear(renderer);

		s32 w,h;
		SDL_GetWindowSize(window, &w, &h);
		SDL_Rect scrRect{0,0,w, h/4};
		SDL_BlitScaled(rgbSurface, nullptr, screen, &scrRect);

		scrRect.y += scrRect.h;
		SDL_BlitScaled(labSurface, nullptr, screen, &scrRect);

		scrRect.y += scrRect.h;
		SDL_BlitScaled(hsvSurface, nullptr, screen, &scrRect);

		scrRect.y += scrRect.h;
		SDL_BlitScaled(lchSurface, nullptr, screen, &scrRect);

		SDL_UpdateWindowSurface(window);
		SDL_RenderPresent(renderer);

		SDL_Delay(20);
	}

	delete[] rgbps;
	delete[] lchps;

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}