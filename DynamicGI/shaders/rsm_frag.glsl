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
uniform vec3 lightDir;
uniform sampler2D texture_diffuse1;

void main() {
	rsm_position = fs_in.pos;
	rsm_normal = normalize(fs_in.normal);
	vec3 flux = texture(texture_diffuse1, fs_in.tex).rgb * lightColor * clamp(dot(rsm_normal, lightDir), 0.0, 1.0);
	rsm_flux = vec4(flux, 1.0);		
}