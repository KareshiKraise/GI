#version 430

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 Tex;
layout(location = 2) in vec3 Normal;

uniform mat4 V;
uniform mat4 M;

//near and far planes
uniform float near;
uniform float far;

out float clipDepth;
out float dval;

out vec3 po;
out vec2 uv;
out vec3 n;

void main(){

	vec4 p = V*M * vec4(Pos, 1.0);
	p /= p.w;
	
	float L = length(p.xyz);
	p /= L;
	
	clipDepth = p.z;
	
	p.z = p.z + 1;
	p.x = p.x / p.z;
	p.y = p.y / p.z;
	
	p.z = (L - near) / (far - near);
	p.w = 1.0;
	
	dval = p.z;

	vec4 aux = M * vec4(Pos, 1.0);
	po = aux.xyz;
	n = Normal;
	uv = Tex;

	gl_Position = p;
}