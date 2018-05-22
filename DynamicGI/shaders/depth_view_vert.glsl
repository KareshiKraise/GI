#version 430

//debug view of shadow map, to be used with a fullscreen quad

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 Tex;

out vec2 tex;

void main() {
	tex = Tex;
	gl_Position = vec4(Pos, 1.0);
}

