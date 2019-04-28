#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0) uniform sampler2D gposition;
layout(binding = 1) uniform sampler2D gnormal;
layout(binding = 2) uniform sampler2D galbedo;
layout(binding = 3) uniform sampler2D gdepth;
layout(binding = 4) uniform sampler2D directionalLightMap; //in this case, spotlight light map
layout(binding = 5) uniform sampler2DArray val_shadowmaps;

uniform float parabola_near;
uniform float parabola_far;

uniform int num_rows;
uniform int num_cols;
uniform int num_first_bounce;
uniform int num_second_bounce;
uniform int num_vals;
uniform int wid;
uniform int hei;

uniform bool see_bounce;

//main direction of light source
uniform vec3 sun_dir;
uniform vec3 lpos;
uniform vec3 eye_pos;
uniform mat4 lightspacemat;

//global model matrix 
uniform mat4 M;

const float cutoff = cos(radians(15.0f));

#define VAL_LIGHT_SIZE 1

#define NUM_CLUSTERS 4
layout(std140, binding = 0) uniform vpl_matrices{
	mat4 parabolic_mats[NUM_CLUSTERS];
};

struct plight {
	vec4 p; //position + radius
	vec4 n; //normal + cluster index
	vec4 c; //color	
};

//first bounce list
layout(std430, binding = 1) buffer lightSSBO {
	plight list[];
};

//val list
layout(std430, binding = 2) buffer direct_vals {
	plight val_list[];
};

//second bounce list
layout(std430, binding = 3) buffer backface_vpls {
	plight back_vpl_list[];
};

//second bounce count
layout(std430, binding = 4) buffer backface_vpl_count {
	unsigned int back_vpl_count;
};

float get_attenuation(float light_radius, float dist) {

	float cutoff = 0.3;
	float denom = dist / light_radius + 1.0;
	float attenuation = 1.0 / (denom * denom);

	attenuation = (attenuation - cutoff) / (1 - cutoff);
	attenuation = max(attenuation, 0.0);
	return (attenuation);
}

float DirectionalShadow(vec4 fragPosLS, vec3 norm) {
	vec3 proj = fragPosLS.xyz / fragPosLS.w;
	proj = proj * 0.5 + 0.5;
	float closest = texture(directionalLightMap, proj.xy).r;
	float curr = proj.z;

	vec3 N = normalize(norm);
	float bias = max(0.05 * (1.0 - dot(N, sun_dir)), 0.005);
	float shadow = 0.0f;
	shadow = (curr - bias) > closest ? 0.0f : 1.0f;
	return shadow;
}

#define BIAS 0.25f
float ParabolicShadow(vec4 worldPos, mat4 lightV, float layer) {

	vec4 lpos = lightV * M * worldPos;
	float L = length(lpos.xyz);
	lpos /= L;
	float ret = 0.0f;

	float fdepth;
	float fscene_depth;

	vec2 tex_coord;
	tex_coord.x = (lpos.x / (1.0f + lpos.z))*0.5f + 0.5f;
	tex_coord.y = (lpos.y / (1.0f + lpos.z))*0.5f + 0.5f;

	fscene_depth = (L - parabola_near) / (parabola_far - parabola_near);

	fdepth = texture(val_shadowmaps, vec3(tex_coord, layer)).r;
	ret = ((fscene_depth + BIAS) > fdepth) ? 0.0f : 1.0f;

	return ret;
}


const vec3 kd = vec3(0.15, 0.15, 0.15);
vec3 shade_point(plight light, vec3 ppos, vec3 pnormal)
{
	vec3 col = vec3(0.0f, 0.0f, 0.0f);

	vec3 lightdir = normalize(light.p.xyz - ppos);

	float dist = length(ppos - light.p.xyz);

	//attenuation coeficients from ogre wiki
	float atten = 1.0f / (1.0 + 0.6*(dist) + 1.9*(dist*dist));
	
	col = (light.c.xyz) * max(0.0, dot(pnormal, lightdir)) * atten * kd;

	return col;
}

vec2 blockerSearch(int light_size, float layer, float curr_depth, vec2 tex_coord, vec2 px_size)
{
	//blocker search
	int blockers = 0;
	float avg_blk_dist = 0.0f;
	//texel coordinates of receiver point
	vec2 coords = tex_coord;

	for (int i = -light_size; i <= light_size; i++)
	{
		for (int j = -light_size; j <= light_size; j++)
		{
			//offset
			float z = texture(val_shadowmaps, vec3(coords + vec2(i, j)*px_size, layer)).r;
			if (z < curr_depth)
			{
				blockers++;
				avg_blk_dist += z;
			}
		}
	}

	avg_blk_dist /= float(blockers);
	return vec2(avg_blk_dist, blockers);
}

float penumbra(int filter_size, vec2 curr_xy, float curr_depth, float bias, vec2 px_size, float layer)
{
	float shadow = 0.0;
	
	for (int i = -filter_size; i <= filter_size; i++)
	{
		for (int j = -filter_size; j <= filter_size; j++)
		{
			float pcfdepth = texture(val_shadowmaps, vec3(curr_xy + vec2(i, j)*px_size, layer)).r;
			shadow += ((curr_depth - bias) > pcfdepth) ? 0.0 : 1.0;
		}
	}

	shadow /= (2 * filter_size + 1)*(2 * filter_size + 1);
	return shadow;
}

float do_parabolicPCSS(int light_size, vec4 ws_pos, float layer)
{
	vec4 ls_pos = parabolic_mats[int(layer)] * ws_pos;
	float L = length(ls_pos.xyz);
	ls_pos /= L;

	vec2 tex_coord;
	tex_coord.x = (ls_pos.x / (1.0f + ls_pos.z))*0.5f + 0.5f;
	tex_coord.y = (ls_pos.y / (1.0f + ls_pos.z))*0.5f + 0.5f;

	float curr_depth = (L - parabola_near) / (parabola_far - parabola_near);
	vec2 px_size = vec2(1.0f / 1024.f, 1.0f / 1024.f);

	vec2 blocker_info = blockerSearch(light_size, layer, curr_depth, tex_coord.xy, px_size);
	
	//penumbra calculation
	float size = 1;
	
	if (int(blocker_info.y) > 1)
		size = light_size * (curr_depth - blocker_info.x) / blocker_info.x;
	
	float bias = BIAS;
	
	float shadow = 0.0;
	int filter_size = int(size);
	shadow = penumbra(filter_size, tex_coord, curr_depth,bias, px_size, layer);

	return shadow;
}

layout(rgba16f, binding = 0) uniform image2D imageBuffer;

void main() {

	vec2 uv = vec2((float(gl_GlobalInvocationID.x) + 0.5f) / float(wid), (float(gl_GlobalInvocationID.y) + 0.5f) / float(hei));

	int total_lights = num_first_bounce + num_second_bounce;
	int lights_per_tile = num_first_bounce / (num_rows * num_cols);
	int x_dim = wid / num_cols;
	int y_dim = hei / num_rows;

	//flatten workgroup id
	uint idx = (gl_GlobalInvocationID.x / x_dim);
	uint idy = (gl_GlobalInvocationID.y / y_dim);
	uint flat_id = idy * num_cols + idx;

	vec3 output_color = vec3(0.0);

	vec4 frag_pos = texture(gposition, uv);
	vec4 frag_n = texture(gnormal, uv);
	vec4 frag_albedo = texture(galbedo, uv);
	
	//main light source pass
	vec3 frag_to_light = normalize(frag_pos.xyz - lpos);
	float theta = dot(frag_to_light, -sun_dir);	
	
	float diffuse = max(dot(frag_n.xyz, -frag_to_light), 0.0);

	if (theta > cutoff)
	{
		output_color = vec3(diffuse);
	}
		
	for (int i = 0; i < lights_per_tile; i++)
	{
		plight curr_light = list[flat_id + i];

		float layer = curr_light.n.w;
		float ret = ParabolicShadow(frag_pos, parabolic_mats[int(layer)], layer);
		//float ret = do_parabolicPCSS(VAL_LIGHT_SIZE, frag_pos, layer);
				
		output_color += (ret * shade_point(curr_light, frag_pos.xyz, frag_n.xyz));
	}
	
	int lights_per_tile2 = num_second_bounce / (num_rows * num_cols);
	for (int i = 0; i < lights_per_tile2; i++)
	{
		plight curr_light = back_vpl_list[flat_id + i];
		output_color += shade_point(curr_light, frag_pos.xyz, frag_n.xyz);
	}
	
	ivec2 fpos = ivec2(gl_GlobalInvocationID.xy);
	imageStore(imageBuffer, fpos, vec4(output_color, 1.0));

}