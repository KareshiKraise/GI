#version 430

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 Tex;

uniform mat4 parabolicModelView;

//near and far planes
const float near = 1.0f;
const float far = 80.0f;

out float dval;
out float clipDepth;

void main()
{
	vec4 p = parabolicModelView * vec4(Pos, 1.0);
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

	gl_Position = p;
}