#version 430

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 Tex;
layout(location = 2) in vec3 Normal;

out VS_OUT {
	vec3 pos;
	vec3 normal;
	vec2 tex;
	vec4 FragPosLightSpace;
} vs_out;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform mat4 LightSpaceMat;

void main() {
	vs_out.pos = vec3(M * vec4(Pos, 1.0));
	vs_out.normal = transpose(inverse(mat3(M))) * Normal;
	vs_out.tex = Tex;
	vs_out.FragPosLightSpace = LightSpaceMat * vec4(vs_out.pos, 1.0);
	gl_Position = P * V * M * vec4(Pos, 1.0);

}