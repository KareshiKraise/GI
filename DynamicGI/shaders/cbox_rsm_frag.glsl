#version 430

layout(location = 0) out vec3 rsm_position;
layout(location = 1) out vec3 rsm_normal;
layout(location = 2) out vec4 rsm_flux;

in vec3 p;
in vec3 n;
in vec3 c;

uniform float lightColor;
uniform vec3 lightDir;

void main()
{
	rsm_position = p;
	rsm_normal = n;	
    vec3 flux = c * lightColor * max(dot(n, lightDir), 0.0);
	rsm_flux = vec4(flux, 1.0);
}
