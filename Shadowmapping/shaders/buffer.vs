#version 330

in vec2 vertex;
out vec2 uv;

void main() {
	gl_Position = vec4(vertex, 0, 1);

	uv = vertex*0.5 + 0.5;
}