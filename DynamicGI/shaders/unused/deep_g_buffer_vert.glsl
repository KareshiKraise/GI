#version 430

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 Tex;
layout(location = 2) in vec3 Normal;


out VS_OUT{
	vec3 pos;
	vec3 normal;
	vec2 tex;
	vec4 clipSpace;
} vs_out;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

//uniform mat3 nMat;

void main() {
	vec4 worldpos = M * vec4(Pos, 1.0f);
	vs_out.pos = vec3(worldpos);

	vs_out.clipSpace = P * V * worldpos;
	gl_Position = vs_out.clipSpace;
	
	vs_out.tex = Tex;

	mat3 nMat = transpose(inverse(mat3(M)));
	vs_out.normal = nMat * Normal;

}