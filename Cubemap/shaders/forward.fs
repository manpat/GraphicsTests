#version 330

in vec3 vpos;
in vec3 vnorm;

uniform vec3 color;

layout(location=0) out vec4 outcolor;
layout(location=1) out vec3 outposition;
layout(location=2) out vec3 outnormal;

void main() {
	outcolor = vec4(color, 1);
	outposition = vpos;
	outnormal = vnorm*0.5+0.5;
}