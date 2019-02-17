#version 430

layout(location = 0) uniform sampler2DArray gposition;
layout(location = 1) uniform sampler2DArray gnormal;
layout(location = 2) uniform sampler2DArray galbedo;

layout(location = 3) uniform sampler2D shadow_map;

out vec4 color;

in vec2 tex;

uniform mat4 lightSpaceMat;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 eyePos;

void render_layer() {
	vec4 frag;
	if (gl_FragCoord.x <= 1240 / 2)
	{
		frag = texture(galbedo, vec3(tex, 0)).rgba;
	}
	else {
		frag = texture(galbedo, vec3(tex, 1)).rgba;
	}	
	color = vec4(vec3(frag), 1.0f);
}

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


void do_phong() {
	vec3 pos =		 texture(gposition,vec3(tex,0)).xyz;
	vec3 normal =	 texture(gnormal,  vec3(tex,0)).xyz;
	vec3 albedo =	 texture(galbedo,  vec3(tex,0)).rgb;
	float specular = texture(galbedo,  vec3(tex,0)).a;
	
	vec3 ambient = albedo * 0.1f;
	
	float diffuse = max(dot(lightDir, normal), 0.0f);
	vec3 lambertian = diffuse * lightColor;
	
	vec3 eyeDir = normalize(eyePos - pos);
	
	vec3 half_v = normalize(lightDir + eyeDir);
	float spec = pow(max(dot(normal, half_v), 0.0), 16);
	vec3 glossy = spec * lightColor * specular;
	
	vec4 pos_light_space = lightSpaceMat * vec4(pos, 1.0);

	float shadow = inShadow(pos_light_space, normal);

	vec3 lighting = (ambient + (shadow) * (glossy + diffuse)) * albedo;;
	color = vec4(lighting , 1.0f);
}

void main() {
	do_phong();
	
}