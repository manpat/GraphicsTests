#include "color.h"

namespace LAB_CONSTANTS {
	// Corresponds roughly to RGB brighter/darker
	static constexpr f32 Kn = 18;

	// D65 standard referent
	static constexpr f32 Xn = 0.950470;
	static constexpr f32 Yn = 1;
	static constexpr f32 Zn = 1.088830;

	static constexpr f32 t0 = 0.137931034;  // 4 / 29
	static constexpr f32 t1 = 0.206896552;  // 6 / 29
	static constexpr f32 t2 = 0.12841855;   // 3 * t1 * t1
	static constexpr f32 t3 = 0.008856452;  // t1 * t1 * t1
}

static constexpr f32 RADTODEG = 180.f/PI;
static constexpr f32 DEGTORAD = PI/180.f;

static f32 xyz_lab(f32 t) {
	if(t > LAB_CONSTANTS::t3)
		return std::pow(t, 1.f / 3.f);

	return t/LAB_CONSTANTS::t2 + LAB_CONSTANTS::t0;
}

static f32 rgb_xyz(f32 r) {
	if(r <= 0.04045f)
		return r / 12.92f;

	return std::pow((r + 0.055f) / 1.055f, 2.4f);
}

static f32 xyz_rgb(f32 r) {
	f32 v = (r <= 0.00304f)
		? 12.92f * r
		: 1.055f * std::pow(r, 1.f / 2.4f) - 0.055f
		;

	return v;
}

static f32 lab_xyz(f32 t) {
	if(t > LAB_CONSTANTS::t1)
		return t * t * t;

	return LAB_CONSTANTS::t2 * (t - LAB_CONSTANTS::t0);
}

LABColor Color::ToLAB() {
	f32 r2 = rgb_xyz(r);
	f32 g2 = rgb_xyz(g);
	f32 b2 = rgb_xyz(b);
	f32 x = xyz_lab((0.4124564f * r2 + 0.3575761f * g2 + 0.1804375f * b2) / LAB_CONSTANTS::Xn);
	f32 y = xyz_lab((0.2126729f * r2 + 0.7151522f * g2 + 0.0721750f * b2) / LAB_CONSTANTS::Yn);
	f32 z = xyz_lab((0.0193339f * r2 + 0.1191920f * g2 + 0.9503041f * b2) / LAB_CONSTANTS::Zn);

	return {116.f * y - 16.f, 500.f * (x - y), 200.f * (y - z)};
}

LCHColor Color::ToLCH() {
	auto lab = ToLAB();

	f32 c = std::sqrt(lab.a*lab.a + lab.b*lab.b);
	f32 h = std::fmod(std::atan2(lab.b, lab.a) * RADTODEG + 360.f, 360.f);

	// Necessary?
	if(std::round(c*10000.f) == 0.f)
		// h = std::numeric_limits<f32>::quiet_NaN();
		h = 0.f;

	return {lab.l, c, h};
}

HSVColor Color::ToHSV() {
	f32 min = std::min(std::min(r, g), b);
	f32 max = std::max(std::max(r, g), b);
	f32 delta = max - min;

	f32 v = max;
	f32 h, s;

	if (max == 0) {
		h = std::numeric_limits<f32>::quiet_NaN();
		s = 0.f;

	}else{
		s = delta / max;

		if(r == max) h = (g - b) / delta;
		else if(g == max) h = 2.f + (b - r) / delta;
		else h = 4.f + (r - g) / delta;

		h *= 60.f;
		if(h < 0.f) h += 360.f;
		if(h > 360.f) h -= 360.f;
	}

	return {h,s,v};
}


Color LABColor::ToRGB() {
	f32 y = (l + 16.f) / 116.f;
	f32 x = std::isnan(a)? y : (y + a / 500.f);
	f32 z = std::isnan(b)? y : (y - b / 200.f);

	y = LAB_CONSTANTS::Yn * lab_xyz(y);
	x = LAB_CONSTANTS::Xn * lab_xyz(x);
	z = LAB_CONSTANTS::Zn * lab_xyz(z);

	// D65 -> sRGB
	f32 r = xyz_rgb( 3.2404542f * x - 1.5371385f * y - 0.4985314f * z);
	f32 g = xyz_rgb(-0.9692660f * x + 1.8760108f * y + 0.0415560f * z);
	f32 b = xyz_rgb( 0.0556434f * x - 0.2040259f * y + 1.0572252f * z);

	r = clamp(r, 0, 1);
	g = clamp(g, 0, 1);
	b = clamp(b, 0, 1);

	return {r,g,b};
}

Color LCHColor::ToRGB() {
	f32 h2 = h * DEGTORAD;

	LABColor lab{
		l,
		std::cos(h2) * c,
		std::sin(h2) * c,
	};
  
	return lab.ToRGB();
}

Color HSVColor::ToRGB() {
	f32 r = 0, g = 0, b = 0;
	f32 h2 = h;

	if(s == 0.f){
		r = g = b = v;

	}else{
		while(h2 >= 360.f) h2 -= 360.f;
		while(h2 < 0.f) h2 += 360.f;

		h2 /= 60.f;
		s32 i = std::floor(h2);
		f32 f = h2 - i;
		f32 p = v * (1.f - s);
		f32 q = v * (1.f - s * f);
		f32 t = v * (1.f - s * (1.f - f));

		switch(i){
			case 0: r=v; g=t; b=p; break;
			case 1: r=q; g=v; b=p; break;
			case 2: r=p; g=v; b=t; break;
			case 3: r=p; g=q; b=v; break;
			case 4: r=t; g=p; b=v; break;
			case 5: r=v; g=p; b=q; break;
			default: r=1; g=1; b=1;
		}
	}

	return {r,g,b};
}