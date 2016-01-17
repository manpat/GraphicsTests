#version 330

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

layout(location=0) in vec3 vertex;
layout(location=1) in vec3 normal;

out vec3 vpos;
out vec3 vnorm;

void main() {
	gl_Position = projection * view * model * vec4(vertex, 1);
	vpos = (model * vec4(vertex, 1)).xyz;
	vnorm = normalize(mat3(model) * normal);
}