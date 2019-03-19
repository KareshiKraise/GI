#version 430

layout(local_size_x = 1) in;

//back gbuffer
layout(binding = 0)	uniform sampler2D buffer_position;
layout(binding = 1)	uniform sampler2D buffer_normal;
layout(binding = 2)	uniform sampler2D buffer_albedo;

layout(binding = 3)	uniform sampler2D parabolic_map1;
layout(binding = 4)	uniform sampler2D parabolic_map2;
layout(binding = 5)	uniform sampler2D parabolic_map3;
layout(binding = 6)	uniform sampler2D parabolic_map4;

uniform mat4 pMat1;
uniform mat4 pMat2;
uniform mat4 pMat3;
uniform mat4 pMat4;

uniform float near;
uniform float far;

uniform int num_back_samples;
uniform int num_vals;
uniform int diff;

layout(rg32f, binding = 0) uniform imageBuffer samples;

struct plight {
	vec4 p;
	vec4 n;
	vec4 c;
};

layout(std430, binding = 0) buffer backface_vpls {
	plight back_vpl_list[];
};

layout(std430, binding = 1) buffer backface_vpl_count {
	unsigned int back_vpl_count;
};

//first bounce val list
layout(std430, binding = 2) buffer val_buffer {
	plight first_val_list[];
};

#define EPSILON 0.005f
int ParabolicShadow(vec4 worldPos, mat4 lightMV, sampler2D samp) {

	int ret = 0;
	vec4 wpos = lightMV * worldPos;
	vec3 lpos = wpos.xyz;
	float L = length(lpos.xyz);
	lpos /= L;
	float clipDepth = lpos.z;

	if (clipDepth > 0.0f)
	{
		float fdepth;
		float fscene_depth;

		vec2 tex_coord;
		tex_coord.x = (lpos.x / (1.0f + lpos.z))*0.5f + 0.5f;
		tex_coord.y = (lpos.y / (1.0f + lpos.z))*0.5f + 0.5f;
		fdepth = texture2D(samp, tex_coord).r;
		fscene_depth = (L - near) / (far - near);
		ret = ((fscene_depth + EPSILON) > (fdepth)) ? 0 : 1;

	}
	return ret;
}


#define FIRST_VAL_RAD 40.0f
#define VPL_RAD 25.0f

void main() {
		
	vec2 uv = imageLoad(samples, int(gl_GlobalInvocationID.x)).xy;

	vec4 world_pos = texture2D(buffer_position, uv);

	int ret[4];
	ret[0] = ParabolicShadow(world_pos, pMat1, parabolic_map1);
	ret[1] = ParabolicShadow(world_pos, pMat2, parabolic_map2);
	ret[2] = ParabolicShadow(world_pos, pMat3, parabolic_map3);
	ret[3] = ParabolicShadow(world_pos, pMat4, parabolic_map4);

	float dist[4];
	dist[0] = length(world_pos.xyz - first_val_list[0].p.xyz);
	dist[1] = length(world_pos.xyz - first_val_list[1].p.xyz);
	dist[2] = length(world_pos.xyz - first_val_list[2].p.xyz);
	dist[3] = length(world_pos.xyz - first_val_list[3].p.xyz);

	if (((dist[0] < FIRST_VAL_RAD) && (ret[0] == 1)) ||
		((dist[1] < FIRST_VAL_RAD) && (ret[1] == 1)) ||
		((dist[2] < FIRST_VAL_RAD) && (ret[2] == 1)) ||
		((dist[3] < FIRST_VAL_RAD) && (ret[3] == 1))
		)
	{
		vec4 normal = texture2D(buffer_normal, uv);
		vec4 color = texture2D(buffer_albedo, uv);
		unsigned int idx = atomicAdd(back_vpl_count, 1);
		back_vpl_list[idx] = plight(vec4(world_pos.xyz, VPL_RAD), normal, color);
	}
	

}