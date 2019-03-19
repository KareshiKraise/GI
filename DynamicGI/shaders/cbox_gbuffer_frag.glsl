#version 430

layout(location = 0) out vec3 gposition;
layout(location = 1) out vec3 gnormal;
layout(location = 2) out vec4 galbedo;

in vec3 p;
in vec3 n;
in vec3 c;

void main()
{
	gposition = p;
	gnormal = n;
	galbedo = vec4(c, 1.0);
}