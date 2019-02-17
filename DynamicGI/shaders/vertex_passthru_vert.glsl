#version 430

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 Tex;
layout(location = 2) in vec3 Normal;

uniform mat4 M;
layout(std140, binding = 0) uniform Matrices
{
	mat4 V[6];
};
uniform mat4 P;

out VS_out{
	vec4 p;
	vec3 n;
	vec2 uv;
}vs_out;


void main()
{
	vs_out.p = M * vec4(Pos, 1.0f);
	vs_out.n = Normal;
	vs_out.uv = Tex;
	gl_Position = M * vec4(Pos, 1.0);
}