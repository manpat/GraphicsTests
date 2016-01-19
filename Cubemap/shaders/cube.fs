#version 330

in vec2 uv;
layout(location=0) out vec4 outcolor;

uniform int face;
uniform samplerCube cubeMap;

void main() {
	vec3 f = vec3(uv*2.f-1.f, 0.f);
	switch(face) {
		case 0: f = vec3( 1.f, f.y,-f.x); break;
		case 1: f = vec3(-1.f, f.y, f.x); break;
		case 2: f = vec3(f.y,  1.f, f.x); break;
		case 3: f = vec3(f.y, -1.f, -f.x); break;
		case 4: f = vec3(f.x, f.y,  1.f); break;
		case 5: f = vec3(-f.x, f.y, -1.f); break;
		default: break;
	}

	outcolor = vec4(texture(cubeMap, f).xyz, 1);
}