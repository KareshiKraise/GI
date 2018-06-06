#version 430

layout(location = 0) out vec3 glambertian;

in vec2 Tex;

uniform vec3 lightDir;
uniform vec3 lightColor;

layout(location = 1) uniform sampler2DArray gnormal;
layout(location = 2) uniform sampler2DArray galbedo;

void main() {
	vec3 albedo = vec3(0.0f);
	vec3 normal = vec3(0.0f);
	float diffuse = 0.0f;

	if (gl_Layer == 0) {

		albedo = texture(galbedo, vec3(Tex, 0)).rgb;
		normal = texture(galbedo, vec3(Tex, 0)).xyz;

		diffuse = max(dot(lightDir, normal), 0.0f);		
	}
	else if (gl_Layer == 1) {
		albedo = texture(galbedo, vec3(Tex, 1)).rgb;
		normal = texture(galbedo, vec3(Tex, 1)).xyz;

		diffuse = max(dot(lightDir, normal), 0.0f);		
	}

	glambertian = diffuse * lightColor * albedo;

}