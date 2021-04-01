#version 430

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 Tex;
layout(location = 2) in vec3 Normal;

out VS_OUT{
	vec3 pos;
	vec3 normal;
	vec2 uv;
} vs_out;


uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

void main()
{
	vs_out.pos = vec3(M * vec4(Pos, 1.0));
	vs_out.normal = Normal;
	vs_out.uv = Tex;
	gl_Position = P * V * M * vec4(Pos, 1.0);
}