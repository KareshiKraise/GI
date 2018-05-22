#version 430

in VS_OUT{
	vec3 pos;
	vec3 normal;
	vec2 tex;
	vec4 FragPosLightSpace;
} fs_in;

layout(binding = 0) uniform sampler2D texture_diffuse1;
layout(binding = 5) uniform sampler2D shadow_Map;

uniform vec3 lightDir;
uniform vec3 eyePos;

out vec4 fragColor;

float inShadow(vec4 fragPosLS) {
	vec3 proj = fragPosLS.xyz / fragPosLS.w;
	proj = proj * 0.5 + 0.5;
	float closest = texture(shadow_Map, proj.xy).r;
	float curr = proj.z;

	vec3 normal = normalize(fs_in.normal);
	//vec3 lightDir = normalize(lightPos - fs_in.pos);
	float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
	float shadow = 0.0f;
	shadow = curr - bias > closest ? 0.0f : 1.0f; // correcao sugerida por marcio	
	return shadow;
}

void main() {

	vec3 albedo = texture(texture_diffuse1, fs_in.tex).rgb;
	vec3 normal = normalize(fs_in.normal);
	vec3 lightCol = vec3(1.0);

	vec3 ambient = albedo * 0.1;
	//vec3 lightDir = normalize(lightPos - fs_in.pos);
	vec3 eyeDir = normalize(eyePos - fs_in.pos);

	float diff = max(dot(lightDir, normal), 0.0f);
	vec3 diffuse = diff * lightCol;

	float spec = 0.0;
	vec3 half_v = normalize(lightDir + eyeDir);
	spec = pow(max(dot(normal, half_v), 0.0), 16);
	vec3 specular = spec * lightCol;

	float shadow = inShadow(fs_in.FragPosLightSpace);

	vec3 lighting = (ambient + (shadow) * (specular + diffuse)) * albedo;
	//vec3 lighting = ambient + ((shadow) * (diffuse + specular) * albedo);
	fragColor = vec4(lighting, 1.0);		
}
