#version 330

in vec2 uv;
out vec4 outcolor;

uniform sampler2D colorBuffer;
uniform sampler2D positionBuffer;
uniform sampler2D normalBuffer;
uniform sampler2D depthBuffer;
uniform sampler2DShadow lightDepth;

uniform mat4 lightView;
uniform mat4 lightProj;

void main() {
	vec4 fragCol = texture2D(colorBuffer, uv);
	vec3 fragPos = texture2D(positionBuffer, uv).xyz;
	vec3 fragNorm = texture2D(normalBuffer, uv).xyz * 2.f - 1.f;
	vec3 lightDirection = normalize(inverse(lightView) * vec4(0,0,0,1)).xyz;
	float d = dot(lightDirection, fragNorm);

	vec4 lspace = lightProj * lightView * vec4(fragPos, 1);
	lspace /= lspace.w;
	lspace.xyz = lspace.xyz * .5 + .5;

	if(fragCol.a < 0.2) {
		outcolor = vec4(fragCol.xyz,1);
		return;
	}

	float bias = 0.005 * tan(acos(d));
	bias = clamp(bias, 0, 0.01);

	float fd = lspace.z-bias;
	vec2 texelSize = 1.f/textureSize(lightDepth, 0);
	float factor = 0.f;

	for (int y = -1 ; y <= 1 ; y++) {
		for (int x = -1 ; x <= 1 ; x++) {
			vec3 uvc = vec3(lspace.xy + vec2(x, y) * texelSize, fd);
			factor += texture(lightDepth, uvc, -2.f);
		}
	}

	factor /= 9.f;

	// vec3 col = fragNorm *.5+.5;
	vec3 col = fragCol.xyz;

	// col *= factor*0.4f + 0.6f;
	// col += vec3(1, .5, .1) * factor * 0.5f;
	// col *= 0.7;
	vec3 tone = vec3(.9, .6, .1);
	// col += mix(1.f-tone, tone, factor) * 0.3;
	col += tone * (factor*2-1) * 0.2;
	outcolor = vec4(col, 1);
}
