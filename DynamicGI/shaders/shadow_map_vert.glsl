#version 430

layout(location = 0) in vec3 Pos;


uniform mat4 PV;
uniform mat4 M;

void main() {
	gl_Position = PV * M * vec4(Pos, 1.0f);
}