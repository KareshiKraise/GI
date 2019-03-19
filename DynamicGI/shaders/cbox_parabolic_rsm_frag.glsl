#version 430

layout(location = 0) out vec3 rsm_position;
layout(location = 1) out vec3 rsm_normal;
layout(location = 2) out vec4 rsm_flux;

in vec3 po;
in vec3 n;
in vec3 c;

in float clipDepth;


void main() {
	if (clipDepth < 0)
		discard;

	rsm_position = po;
	rsm_normal   = n;
	rsm_flux     = vec4(c, 1.0);
}