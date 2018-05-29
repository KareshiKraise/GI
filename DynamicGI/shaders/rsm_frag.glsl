#version 430

layout(location = 0) out vec3 rsm_position;
layout(location = 1) out vec3 rsm_normal;
layout(location = 2) out vec4 rsm_flux;

in VS_OUT{
	vec3 pos;
	vec3 normal;
	vec2 tex;
} fs_in;

uniform float lightColor;
uniform sampler2D texture_diffuse1;

void main() {
	rsm_position = fs_in.pos;
	rsm_normal = fs_in.normal;
	rsm_flux = vec4(texture(texture_diffuse1, fs_in.tex).rgb  * lightColor, 1.0);		
}