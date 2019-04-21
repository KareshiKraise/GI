#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0) uniform sampler2D gposition;
layout(binding = 1) uniform sampler2D gnormal;
layout(binding = 2) uniform sampler2D galbedo;
layout(binding = 3) uniform sampler2D gdepth;
layout(binding = 4) uniform sampler2D directionalLightMap;
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

//directional light - sun direction
uniform vec3 sun_dir;
uniform vec3 eye_pos;
uniform mat4 lightspacemat;

//global model matrix 
uniform mat4 M;

#define DIRECTIONAL_LIGHT_SIZE 1

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

#define EPSILON 0.05f
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
	ret = ((fscene_depth - EPSILON) > fdepth) ? 0.0f : 1.0f;

	return ret;
}

const vec3 kd = vec3(0.15, 0.15, 0.15);
vec3 shade_point(plight light, vec3 ppos, vec3 pnormal)
{
	vec3 col = vec3(0.0f, 0.0f, 0.0f);

	vec3 lightdir = normalize(light.p.xyz - ppos);

	float dist = length(ppos - light.p.xyz);

	float atten = get_attenuation(light.p.w, length(ppos - light.p.xyz));

	vec3 ltop = normalize(light.p.xyz - ppos); //light to pos
	vec3 ptol = normalize(ppos - light.p.xyz); //pos to light

	col = light.c.xyz * kd * max(0.0, dot(pnormal, lightdir)) * atten;

	return col;
}


vec2 PCSS_blocker_search(int light_size, float curr_depth, vec2 curr_xy)
{
	int blockers = 0;
	float avg_blk_dist = 0.0f;
	//texel coordinates of receiver point
	vec2 coords = curr_xy;
	//pixel scale
	vec2 tex_size = textureSize(directionalLightMap, 0).xy;
	vec2 px_size = 0.5 / tex_size;//vec2(0.5 / float(wid), 0.5 / float(hei));	
	//vec2 px_size = vec2(1.0/float(wid), 1.0/float(hei));

	for (int i = -light_size; i <= light_size; i++)
	{
		for (int j = -light_size; j <= light_size; j++)
		{
			//offset
			float z = texture(directionalLightMap, coords + vec2(i, j)*px_size).r;
			if (z < curr_depth)
			{
				blockers++;
				avg_blk_dist += z;
			}
		}
	}

	avg_blk_dist /= float(blockers);

	return vec2(avg_blk_dist, float(blockers));
}


float PCSS_penumbra(int filter_size, vec2 curr_xy, float curr_depth, vec3 frag_normal)
{
	//pixel scale
	vec2 tex_size = textureSize(directionalLightMap, 0).xy;
	vec2 px_size = 0.5/tex_size;//vec2(0.5 / float(wid), 0.5 / float(hei));	
	float shadow = 0.0;

	vec3 N = normalize(frag_normal);
	float bias = max(0.05 * (1.0 - dot(N, sun_dir)), 0.005);

	for (int i = -filter_size; i <= filter_size; i++)
	{
		for (int j = -filter_size; j <= filter_size; j++)
		{
			float pcfdepth = texture(directionalLightMap, curr_xy + vec2(i, j)*px_size).r;
			shadow += ((curr_depth - bias) > pcfdepth) ? 0.0 : 1.0;
		}
	}

	shadow /=  (2 * filter_size + 1)*(2 * filter_size + 1);
	return shadow;
}

//light size and light space position
float do_PCSS(int light_size, vec4 ls_pos, vec3 frag_normal)
{
	vec3 proj = ls_pos.xyz / ls_pos.w;
	proj = proj * 0.5 + 0.5;
	float closest = texture(directionalLightMap, proj.xy).r;
	
	float curr_depth = proj.z;

	//blocker search
	vec2 blocker_info = PCSS_blocker_search(light_size, curr_depth, proj.xy);
	//penumbra calculation
	float size = 1;

	if(blocker_info.y > 1)
		size = light_size * (curr_depth - blocker_info.x) / blocker_info.x;

	return PCSS_penumbra(int(size), proj.xy, curr_depth, frag_normal);
}

layout(rgba16f, binding = 0) uniform image2D imageBuffer;

void main() {

	vec2 uv = vec2((float(gl_GlobalInvocationID.x) + 0.5f) / float(wid), (float(gl_GlobalInvocationID.y) + 0.5f) / float(hei));	

	int total_lights = num_first_bounce + num_second_bounce;
	int lights_per_tile = num_first_bounce / (num_rows * num_cols);
	int x_dim = wid / num_cols;
	int y_dim = hei / num_rows;

	//flatten workgroup id
	uint idx = (gl_GlobalInvocationID.x/x_dim);
	uint idy = (gl_GlobalInvocationID.y/y_dim);
	uint flat_id = idy * num_cols + idx;

	vec3 output_color = vec3(0.0);

	vec4 frag_pos    = texture(gposition, uv);
	vec4 frag_n      = texture(gnormal,   uv);
	vec4 frag_albedo = texture(galbedo,   uv);
	float spec = frag_albedo.a;
		
	//directional pass
	vec3 ambient = kd * frag_albedo.xyz;

	float diffuse = max(0.0, dot(sun_dir, frag_n.xyz));

	vec3 eye_dir = normalize(frag_pos.xyz - eye_pos);
	vec3 half_vector = normalize(eye_dir + sun_dir);

	float spec_val = pow(max(dot(frag_n.xyz, half_vector), 0.0), 16);
	vec3 specular = vec3(spec_val * spec);

	vec4 light_space_pos = lightspacemat * vec4(frag_pos.xyz, 1.0);
	//float in_shadow = DirectionalShadow(light_space_pos, frag_n.xyz);
	float in_shadow = do_PCSS(DIRECTIONAL_LIGHT_SIZE, light_space_pos, frag_n.xyz);

	vec3 directional = (in_shadow * (diffuse + specular)) * vec3(1.0);
	
	output_color += directional;

	for (int i = 0; i < lights_per_tile; i++)
	{
		plight curr_light = list[flat_id + i];	
	
		float layer = curr_light.n.w;
		float ret = ParabolicShadow(frag_pos, parabolic_mats[int(layer)], layer);
	
		//if (ret > 0.0f)
		output_color += ret* shade_point(curr_light, frag_pos.xyz, frag_n.xyz);
	}	

	int lights_per_tile2 = num_second_bounce/ (num_rows * num_cols);
	for (int i = 0; i < lights_per_tile2; i++)
	{
		plight curr_light = back_vpl_list[flat_id + i];
		output_color += shade_point(curr_light, frag_pos.xyz, frag_n.xyz);
	}

	ivec2 fpos = ivec2(gl_GlobalInvocationID.xy);
	imageStore(imageBuffer, fpos, vec4(output_color, 1.0));

}