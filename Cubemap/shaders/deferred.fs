#version 330

in vec2 uv;
out vec4 outcolor;

uniform sampler2D colorBuffer;
uniform sampler2D positionBuffer;
uniform sampler2D normalBuffer;
uniform sampler2D depthBuffer;

void main() {
	vec4 fragCol = texture2D(colorBuffer, uv);
	vec3 fragPos = texture2D(positionBuffer, uv).xyz;
	vec3 fragNorm = texture2D(normalBuffer, uv).xyz * 2.f - 1.f;

	if(fragCol.a < 0.2) {
		outcolor = vec4(fragCol.xyz,1);
		return;
	}

	// vec3 col = fragNorm *.5+.5;
	vec3 col = fragCol.xyz;
	outcolor = vec4(col, 1);
}
