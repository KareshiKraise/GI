#version 430

in VS_OUT{
	vec3 pos;
	vec3 normal;
	vec2 tex;
	vec4 FragPosLightSpace;
} fs_in;

uniform sampler2D texture_diffuse1;
uniform sampler2D shadowMap;

uniform vec3 lightPos;
uniform vec3 eyePos;

out vec4 fragColor;

float inShadow(vec4 fragPosLS) {
	vec3 proj = fragPosLS.xyz / fragPosLS.w;
	proj = proj * 0.5 + 0.5;
	float closest = texture(shadowMap, proj.xy).r;
	float curr = proj.z;
	float shadow = curr > closest ? 1.0 : 0.0;
	return shadow;
}


void main() {

	vec3 albedo = texture(texture_diffuse1, fs_in.tex).rgb;
	vec3 normal = normalize(fs_in.normal);
	vec3 lightCol = vec3(1.0);

	vec3 ambient = albedo * 0.15;
	vec3 lightDir = normalize(lightPos - fs_in.pos);
	vec3 eyeDir = normalize(eyePos - fs_in.pos);

	float diff = max(dot(lightDir, normal), 0.0f);
	vec3 diffuse = diff * lightCol;

	float spec = 0.0;
	vec3 half_v = normalize(lightDir + eyeDir);
	spec = pow(max(dot(normal, half_v), 0.0), 32);
	vec3 specular = spec * lightCol;
	float shadow = inShadow(fs_in.FragPosLightSpace);

	vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * albedo;
	fragColor = vec4(lighting, 1.0);
	
}
