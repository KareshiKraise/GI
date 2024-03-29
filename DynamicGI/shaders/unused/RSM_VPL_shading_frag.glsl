#version 430

//full shade with shadow mapping
uniform sampler2D gposition;
uniform sampler2D gnormal;
uniform sampler2D galbedo;

uniform sampler2D shadow_map;

uniform sampler2D preshade_buffer;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 eyePos;
uniform mat4 LightSpaceMat;

out vec4 frag_color;
in vec2 tex;

#define PI 3.14159
#define TWO_PI 6.28318

float inShadow(vec4 fragPosLS, vec3 norm) {
	vec3 proj = fragPosLS.xyz / fragPosLS.w;
	proj = proj * 0.5 + 0.5;
	float closest = texture(shadow_map, proj.xy).r;
	float curr = proj.z;

	vec3 N = normalize(norm);
	float bias = max(0.05 * (1.0 - dot(N, lightDir)), 0.005);
	float shadow = 0.0f;
	shadow = curr - bias > closest ? 0.0f : 1.0f;
	return shadow;
}

void main() {
	vec3 fragpos = texture(gposition, tex).xyz;
	vec3 normal = texture(gnormal, tex).xyz;
	vec3 albedo = texture(galbedo, tex).rgb;

	vec4 pos_lightspace = LightSpaceMat * vec4(fragpos, 1.0);
	vec3 indirect = texture(preshade_buffer, tex).rgb;

	float diff = max(dot(lightDir, normal), 0.0f);
	vec3 diffuse = diff * lightColor;
	vec3 eyeDir = normalize(eyePos - fragpos);
	
	float spec = 0.0;
	vec3 half_v = normalize(lightDir + eyeDir);
	spec = pow(max(dot(normal, half_v), 0.0), 16);
	vec3 specular = spec * lightColor;	
	
	float shadow = inShadow(pos_lightspace, normal);
	vec3 lighting = (indirect + (shadow * (specular + diffuse)))* albedo;

	frag_color = vec4(lighting, 1.0);

}