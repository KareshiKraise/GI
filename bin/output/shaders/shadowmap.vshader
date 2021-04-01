#version 430

layout(location = 0) in vec3 Pos;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

void main() {
	gl_Position = P * V * M * vec4(Pos, 1.0f);
}