#version 430

in vec2 tex;
out vec4 color;

layout(location = 0) uniform sampler2DArray gposition;
layout(location = 1) uniform sampler2DArray gnormal;
layout(location = 2) uniform sampler2DArray galbedo;
layout(location = 3) uniform sampler2D shadow_map;
layout(location = 4) uniform sampler2DArray gradiosity;

uniform vec3 eyePos;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform mat4 LightSpaceMat;


uniform int samples;
uniform float radius;
uniform float turn;
uniform int layers;

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

float calculate_hashed_angle(ivec2 pos){
	return ((3 * pos.x ^ pos.y + pos.x * pos.y)*10);
}

vec2 get_spiral_texel(vec2 cur, int i) {
	float ki = (i + 0.5f) / samples;
	ivec2 ssC = ivec2(gl_FragCoord.xy);
	float ang = TWO_PI * ki * turn +calculate_hashed_angle(ssC);
	vec2 a_sin_cos = vec2(cos(ang), sin(ang));
	float hi = radius * ki;
	vec2 sp_tex = cur + (hi * a_sin_cos);
	return sp_tex;
}

vec3 calculate_indirect(vec2 t, vec3 P, vec3 N) {
	int M = 0;

	vec3 PX = P;//texture(gposition, vec3(t, 0)).xyz;
	vec3 NX = N;//texture(gnormal, vec3(t, 0)).xyz;
	vec3 EX = vec3(0.0f);

	for (int i = 0; i < samples; i++) {
		
		vec2 n_tex = get_spiral_texel(t, i);

		for (int l = 0; l < layers; l++) {

			vec3 PY = texture(gposition, vec3(n_tex, l)).xyz;
			vec3 NY = texture(gnormal, vec3(n_tex, l)).xyz;
			vec3 W = normalize(PY - PX);
			//Lambertian->radiosity
			vec3 BY = texture(gradiosity, vec3(n_tex, l)).rgb;
			
			float dot_W_NX = dot(W, NX);	
									
			if (dot_W_NX > 0.0f) {
				EX += BY * max(dot_W_NX, 0.0f);
				M += 1;
			}
		}
	}
	vec3 IRR = (TWO_PI / M) * EX; //IRRADIANCE
	float maxChannel = max(IRR.r, max(IRR.g, IRR.b));
	float minChannel = min(IRR.r, min(IRR.g, IRR.b));
	float boost = (maxChannel - minChannel )/ maxChannel;
	float rho = 1.5f;

	return (IRR * rho * boost);
}


void main() {	
		
	vec3 pos    = texture(gposition, vec3(tex, 0)).xyz;
	vec3 n      = texture(gnormal, vec3(tex, 0)).xyz; 
	vec3 albedo = texture(galbedo, vec3(tex, 0)).rgb;
	float spec  = texture(galbedo, vec3(tex, 0)).a;
	
	vec3 indirect = calculate_indirect(tex, pos, n);

	float diffuse = max(dot(lightDir, n), 0.0f);
	vec3 lambertian = diffuse * lightColor;
	
	vec3 eyeDir = normalize(eyePos - pos);
	vec3 half_v = normalize(lightDir + eyeDir);
	float factor = pow(max(dot(n, half_v), 0.0), 32);
	vec3 specular = spec * factor * lightColor;
	
	vec4 pos_Light_Space = LightSpaceMat * vec4(pos, 1.0f);
	float occluded = inShadow(pos_Light_Space, n);
	
	vec3 lighting = (indirect + (occluded)*(lambertian + specular)) * albedo;
		
	color = vec4(lighting, 1.0f);
}