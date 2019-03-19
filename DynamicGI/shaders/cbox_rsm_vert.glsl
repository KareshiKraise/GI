#version 430

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec3 Color;

out vec3 p;
out vec3 n;
out vec3 c;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

void main()
{
	p = vec3(M * vec4(Pos, 1.0));
	n = Normal;
	c = Color;
	gl_Position = P * V * M * vec4(Pos, 1.0);
}