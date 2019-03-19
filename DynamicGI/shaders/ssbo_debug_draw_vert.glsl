#version 430

layout(location = 0) in vec4 Pos;
layout(location = 1) in vec4 Norm;
layout(location = 2) in vec4 Col;

uniform mat4 MVP;

out vec4 p;
out vec4 n;
out vec4 c;

uniform float size;

void main() {
	gl_PointSize = size;
	vec4 pos = vec4(Pos.xyz, 1.0);

	p = Pos;
	n = Norm;
	c = Col;

	gl_Position = MVP * pos;
}