#version 430

layout(location = 0) in vec3 Pos;

uniform mat4 P;
uniform mat4 V;

out vec3 tex;

void main() {
	tex = Pos;
	vec4 p = P * V * vec4(Pos, 1.0);
	gl_Position = p.xyww;
}