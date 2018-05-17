#version 430

layout(location = 0) in vec2 Pos;
layout(location = 1) in vec2 Tex;

out vec2 tex;

void main() {
	tex = Tex;
	gl_Position = vec4(Pos, 0.0, 1.0);
}

