#version 430

layout(location = 0) out vec3 rsm_position;
layout(location = 1) out vec3 rsm_normal;
layout(location = 2) out vec4 rsm_flux;

uniform sampler2D texture_diffuse1;

in vec3 po;
in vec2 uv;
in vec3 n;

in float clipDepth;

void main() {
	if(clipDepth < 0)
		discard;

	rsm_position = po;
	rsm_normal   = n;
	rsm_flux = texture2D(texture_diffuse1, uv);
}