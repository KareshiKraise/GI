#version 430

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 Tex;
layout(location = 2) in vec3 Normal;

out VS_OUT {
	vec3 pos;
	vec3 norm;
	vec2 tex;
} vs_out;

void main()
{
	vs_out.pos = Pos;
	vs_out.norm = Normal;
	vs_out.tex = Tex;
}