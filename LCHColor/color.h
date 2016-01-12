#ifndef COLOR_H
#define COLOR_H

#include "common.h"

// https://github.com/gka/chroma.js/blob/master/src/converter/out/rgb2lab.coffee

struct LABColor;
struct LCHColor;
struct HSVColor;

struct Color {
	f32 r;
	f32 g;
	f32 b; // 0 - 1

	HSVColor ToHSV();
	LABColor ToLAB();
	LCHColor ToLCH();
};

struct HSVColor {
	f32 h;
	f32 s;
	f32 v;

	Color ToRGB();
};

struct LABColor {
	f32 l; // 0 - 100
	f32 a;
	f32 b;

	Color ToRGB();
};

struct LCHColor {
	f32 l; // 0 - 100
	f32 c;
	f32 h;

	Color ToRGB();
};

#endif
