#version 430

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 Tex;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tang;
layout(location = 4) in vec3 Bitang;

out VS_OUT{
	vec3 pos;
	vec3 n;
	vec2 tex;
	mat3 TBN;
	vec3 t;
	vec3 b;
} vs_out;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

void main() {

	vs_out.pos = vec3(M * vec4(Pos, 1.0));
	vs_out.tex = Tex;	

	mat3 normalMat = transpose(inverse(mat3(M)));
	//vec3 N = normalize(normalMat*Normal);
	//vec3 T = normalize(normalMat*Tang);
	//T = normalize(T - dot(T, N) * N);
	//vec3 B = normalize(cross(N, T));
	//vec3 B = normalize(normalMat * Bitang);

	vec3 N = normalize(normalMat * Normal);
	vec3 T = normalize(normalMat * Tang);
	T = normalize(T - dot(T, N) * N);	
	vec3 B = normalize(normalMat * Bitang);
	mat3 TBN = mat3(T, B, N);

	vs_out.n = N;
	vs_out.t = T;
	vs_out.b = B;
	
	vs_out.TBN = TBN;
	
	gl_Position = P * V * M * vec4(Pos, 1.0);	
}