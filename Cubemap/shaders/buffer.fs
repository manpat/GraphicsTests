#version 330

in vec2 uv;
layout(location=0) out vec4 outcolor;

uniform sampler2D buffer;

void main() {
	vec3 c = texture2D(buffer, uv).rgb;
	c = fract(c*0.999f);
	outcolor = vec4(c, 1);
}