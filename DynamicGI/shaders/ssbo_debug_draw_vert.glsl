#version 430

layout(location = 0) in vec4 Pos;

uniform mat4 MVP;

void main() {
	gl_PointSize = 15.0;
	vec4 pos = vec4(Pos.xyz, 1.0);
	gl_Position = MVP * pos;
}