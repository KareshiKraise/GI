#version 430

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec3 Color;

out VS_OUT{
	vec3 pos;
	vec3 norm;
	vec3 col;
} vs_out;

void main()
{
	vs_out.pos = Pos;
	vs_out.norm = Normal;
	vs_out.col = Color;
}