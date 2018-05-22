#version 430

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;

uniform mat4 MVP;

out vec3 col;

void main() {

	col = color;
	gl_Position = MVP * vec4(pos, 1.0);
}







