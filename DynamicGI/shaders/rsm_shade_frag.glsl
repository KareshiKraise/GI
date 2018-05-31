#version 430

//full shade with shadow mapping
uniform sampler2D gposition;
uniform sampler2D gnormal;
uniform sampler2D galbedo;

uniform sampler2D shadow_map;

uniform sampler2D rsm_position;
uniform sampler2D rsm_normal;
uniform sampler2D rsm_flux;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 eyePos;
uniform mat4 LightSpaceMat;
uniform float rmax;

const int SAMPLE_COUNT = 100;
uniform vec2 samples[SAMPLE_COUNT];

out vec4 frag_color;

in vec2 tex;

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
//equivalent to ambient
vec3 indirectLight(vec3 worldpos, vec3 worldnormal) {
	vec4 l_space_pos = LightSpaceMat * vec4(worldpos, 1.0);
	vec3 proj = l_space_pos.xyz / l_space_pos.w;
	proj = proj * 0.5 + 0.5;	

	vec3 indirect = vec3(0);

	for (int i = 0; i < SAMPLE_COUNT; ++i) {
		vec2 sig = samples[i].xy;
		vec2 sampling_coord = proj.xy + rmax * sig;

		vec3 vplPos  = texture(rsm_position, sampling_coord).xyz;
		vec3 vplNorm = texture(rsm_normal, sampling_coord).xyz;
		vec3 vplFlux = texture(rsm_flux, sampling_coord).rgb;

		vec3 res = vplFlux * (max(0, dot(vplNorm, worldpos - vplPos)) * max(0, dot(worldnormal, vplPos - worldpos))) / pow(length(worldpos - vplPos), 4);

		res *= sig.x * sig.x;
		indirect += res;			
	}	
	return clamp(indirect * lightColor, 0.0, 1.0);
}

void main() {
	vec3 fragpos = texture(gposition, tex).xyz;
	vec3 normal = texture(gnormal, tex).xyz;
	vec3 albedo = texture(galbedo, tex).rgb;

	vec4 pos_lightspace = LightSpaceMat * vec4(fragpos, 1.0);

	vec3 indirect = indirectLight(fragpos, normal);
		
	float diff = max(dot(lightDir, normal), 0.0f);
	vec3 diffuse = diff * lightColor;

	vec3 eyeDir = normalize(eyePos - fragpos);

	float spec = 0.0;
	vec3 half_v = normalize(lightDir + eyeDir);
	spec = pow(max(dot(normal, half_v), 0.0), 16);
	vec3 specular = spec * lightColor;
	
	float shadow = inShadow(pos_lightspace, normal);

	vec3 lighting = (indirect + (shadow) * (specular + diffuse)) * albedo;
	frag_color = vec4(lighting, 1.0);

}