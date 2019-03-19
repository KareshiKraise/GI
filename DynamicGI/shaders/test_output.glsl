#version 430

layout(local_size_x = 16, local_size_y = 16) in;

uniform mat4 parabolicMat1;
uniform mat4 parabolicMat2;
uniform mat4 parabolicMat3;
uniform mat4 parabolicMat4;

uniform mat4 dLSMat;

uniform float near;
uniform float far;

uniform vec3 sun_Dir;

layout(binding = 0) uniform sampler2D positionBuffer;
layout(binding = 1) uniform sampler2D normalBuffer;
layout(binding = 2) uniform sampler2D albedoBuffer;

layout(binding = 3) uniform sampler2D pSM1;
layout(binding = 4) uniform sampler2D pSM2;
layout(binding = 5) uniform sampler2D pSM3;
layout(binding = 6) uniform sampler2D pSM4;

layout(binding = 7) uniform sampler2D dSM;

layout(rgba32f, binding = 0) uniform image2D imageBuffer;

#define EPSILON 0.0005f
float ParabolicShadow(vec4 worldPos, mat4 lightMV, sampler2D samp) {

	float ret = 0.0f;
	vec4 wpos = lightMV * worldPos;
	vec3 lpos = wpos.xyz;
	float L = length(lpos.xyz);
	lpos /= L;
	float clipDepth = lpos.z;

	if (clipDepth > 0.f)
	{
		float fdepth;
		float fscene_depth;

		vec2 tex_coord;
		tex_coord.x = (lpos.x / (1.0f + lpos.z))*0.5f + 0.5f;
		tex_coord.y = (lpos.y / (1.0f + lpos.z))*0.5f + 0.5f;
		fdepth = texture2D(samp, tex_coord).r;
		fscene_depth = (L - near) / (far - near);
		ret = ((fscene_depth + EPSILON) > (fdepth)) ? 0.0f : 1.0f;
		
	}
	return ret;
}

float DirectionalShadow(vec4 fragPosLS, vec3 norm){
	vec3 proj = fragPosLS.xyz / fragPosLS.w;
	proj = proj * 0.5 + 0.5;
	float closest = texture(dSM, proj.xy).r;
	float curr = proj.z;

	vec3 N = normalize(norm);
	float bias = max(0.05 * (1.0 - dot(N, sun_Dir)), 0.005);
	float shadow = 0.0f;
	shadow = curr - bias > closest ? 0.0f : 1.0f;
	return shadow;
}

struct plight {
	vec4 p; //position + radius
	vec4 n;
	vec4 c; //color	
};

//first bounce val list
layout(std430, binding = 0) buffer val_buffer {
	plight first_val_list[];
};

float get_attenuation(float light_radius, float dist) {

	float cutoff = 0.3;
	float denom = dist / light_radius + 1.0;
	float attenuation = 1.0 / (denom * denom);

	attenuation = (attenuation - cutoff) / (1 - cutoff);
	attenuation = max(attenuation, 0.0);
	return (attenuation);
}

#define RAD 45.0f

void main()
{
	ivec2 texel_pos = ivec2(gl_GlobalInvocationID.xy);
	vec2 uv = vec2(float(texel_pos.x + 0.5f)/1280.0f, 
				   float(texel_pos.y + 0.5f)/720.0f);

	vec4 wpos = texture2D(positionBuffer, uv);
	vec3 norm = texture2D(normalBuffer, uv).xyz;
	vec4 albedo = texture2D(albedoBuffer, uv);	
	
	//Directional Map
	vec4 lspos = dLSMat  * wpos;

	float diff = max(0.0, dot(norm, sun_Dir));

	float shadow_test = DirectionalShadow(lspos, norm);
	vec4 out_color = (shadow_test * diff) *  albedo;

	//Parabolic Maps

	float res = ParabolicShadow(wpos, parabolicMat1, pSM1);
		
	float len = length(first_val_list[0].p.xyz - wpos.xyz);
	float atten = get_attenuation(RAD, len);
	vec3 ldir = normalize(first_val_list[0].p.xyz - wpos.xyz);
	vec4 col = first_val_list[0].c * atten * max(0.0, dot(ldir, norm));
	out_color += (res * col) * albedo;
	

	res = ParabolicShadow(wpos, parabolicMat2, pSM2);
	
	if (res > 0.f)
	{
		float len = length(first_val_list[1].p.xyz - wpos.xyz);
		float atten = get_attenuation(RAD, len);
		vec3 ldir = normalize(first_val_list[1].p.xyz - wpos.xyz);
		vec4 col = first_val_list[1].c * atten * max(0.0, dot(ldir, norm));
		out_color += (col)* albedo;
	}
	
	res = ParabolicShadow(wpos, parabolicMat3, pSM3);
	
	if (res > 0.f)
	{
		float len = length(first_val_list[2].p.xyz - wpos.xyz);
		float atten = get_attenuation(RAD, len);
		vec3 ldir = normalize(first_val_list[2].p.xyz - wpos.xyz);
		vec4 col = first_val_list[2].c * atten * max(0.0, dot(ldir, norm));
		out_color += (col)* albedo;
	}
	
	res = ParabolicShadow(wpos, parabolicMat4, pSM4);
	
	if (res > 0.f)
	{
		float len = length(first_val_list[3].p.xyz - wpos.xyz);
		float atten = get_attenuation(RAD, len);
		vec3 ldir = normalize(first_val_list[3].p.xyz - wpos.xyz);
		vec4 col = first_val_list[3].c * atten * max(0.0, dot(ldir, norm));
		out_color += (col)* albedo;
	}

	

	imageStore(imageBuffer, texel_pos, out_color);
}
