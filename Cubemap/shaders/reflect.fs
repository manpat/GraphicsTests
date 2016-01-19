#version 330

in vec3 vpos;
in vec3 vnorm;

uniform vec3 color;

layout(location=0) out vec4 outcolor;

uniform samplerCube cubeMap;
uniform mat4 view;
uniform vec3 cameraPos;

void main() {
	vec3 dir = normalize(vpos - cameraPos);
	vec3 ray = reflect(dir, vnorm);

	vec3 cubeCol = texture(cubeMap, ray).xyz;
	outcolor = vec4(cubeCol, 1);
}
