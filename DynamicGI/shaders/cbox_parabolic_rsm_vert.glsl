#version 430

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec3 Color;

out vec3 po;
out vec3 n;
out vec3 c;

out float clipDepth;

uniform mat4 V;
uniform mat4 M;

//near and far planes
const float near = 1.0f;
const float far = 80.0f;


void main() {
	vec4 p = V * M * vec4(Pos, 1.0);
	p /= p.w;

	float L = length(p.xyz);
	p /= L;

	clipDepth = p.z;

	p.z = p.z + 1;
	p.x = p.x / p.z;
	p.y = p.y / p.z;

	p.z = (L - near) / (far - near);
	p.w = 1.0;

	vec4 aux = M * vec4(Pos, 1.0);
	po = aux.xyz;
	n = Normal;
	c = Color;

	gl_Position = p;
}