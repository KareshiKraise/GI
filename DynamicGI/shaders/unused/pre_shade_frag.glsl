#version 430

uniform sampler2D gposition;
uniform sampler2D gnormal;
uniform sampler2D galbedo;

uniform sampler2D shadow_map;

uniform sampler2D rsm_pos;
uniform sampler2D rsm_normal;
uniform sampler2D rsm_flux;

uniform mat4 LightSpaceMat;

uniform int N_samples;
uniform float radius;
uniform int turn;

#define PI 3.14159
#define TWO_PI 6.28318

in vec2 tex;

out vec4 frag_color;

/*float inShadow(vec4 fragPosLS, vec3 norm) {
	vec3 proj = fragPosLS.xyz / fragPosLS.w;
	proj = proj * 0.5 + 0.5;
	float closest = texture(shadow_map, proj.xy).r;
	float curr = proj.z;

	vec3 N = normalize(norm);
	float bias = max(0.05 * (1.0 - dot(N, lightDir)), 0.005);
	float shadow = 0.0f;
	shadow = curr - bias > closest ? 0.0f : 1.0f;
	return shadow;
}*/

float calculate_hashed_angle(ivec2 pos) {
	return ((3 * pos.x ^ pos.y + pos.x * pos.y) * 10);
}

vec2 get_spiral_texel(vec2 cur, int i) {
	float ki = float(i + 0.5f) / N_samples;
	ivec2 ssC = ivec2(gl_FragCoord.xy);
	float ang = (TWO_PI * ki * turn) + calculate_hashed_angle(ssC);
	vec2 a_sin_cos = vec2(cos(ang), sin(ang));
	float hi = radius * ki;
	vec2 sp_tex = cur + (hi * a_sin_cos);
	return sp_tex;
}

vec3 calculate_indirect(vec3 P, vec3 N, vec2 projected) {
	int M = 0;	

	vec3 PX = P;
	vec3 NX = N;
	vec3 EX = vec3(0.0f);

	for (int i = 0; i < N_samples; i++) {
		vec2 n_tex = get_spiral_texel(projected, i);
		
		vec3 PY = texture(rsm_pos, n_tex).xyz;
		//vec3 NY = texture(rsm_normal, n_tex).xyz;
		vec3 W = normalize(PY - PX);
		//Lambertian->radiosity
		vec3 BY = texture(rsm_flux, n_tex).rgb;

		float dot_W_NX = dot(W, NX);
		//float dot_W_NY = dot(W, NY);
		
		if (dot_W_NX > 0.0f) {
			EX += BY * max(dot_W_NX, 0.0f);
			M += 1;
		}		
	}
	vec3 IRR = (TWO_PI / M) * EX; //IRRADIANCE
	float maxChannel = max(IRR.r, max(IRR.g, IRR.b));
	float minChannel = min(IRR.r, min(IRR.g, IRR.b));
	float boost = (maxChannel - minChannel) / maxChannel;
	float rho = 0.32f;
	return (IRR * rho * boost);		
}



void main() {

	vec3 pos = texture(gposition, tex).xyz;
	vec3 normal = texture(gnormal, tex).xyz;
			
	//vec4 l_space_pos = LightSpaceMat * vec4(pos, 1.0);
	//vec3 proj = l_space_pos.xyz / l_space_pos.w;
	//proj = proj * 0.5 + 0.5;

	vec3 indirect = calculate_indirect(pos, normal, gl_FragCoord.xy);
	frag_color = vec4(indirect , 1.0);
}