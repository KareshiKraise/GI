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

#define DIRECTIONAL_LIGHT_SIZE 4
#define VAL_LIGHT_SIZE 3

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
	dist += (0.25*dist);
	float denom = dist / light_radius + 1.0;
	float attenuation = 1.0 / (denom * denom);

	attenuation = (attenuation - cutoff) / (1 - cutoff);
	attenuation = max(attenuation, 0.0);
	return (attenuation);
}

vec4 computeDiscontinuity(vec4 shadowCoord, float sceneDepth, float layer = -1)
{

	vec4 dir = vec4(0.0, 0.0, 0.0, 0.0);
	//x = left; y = right; z = bottom; w = top
	vec2 shadowMapStep;
	if (layer == -1) shadowMapStep = vec2(1.0 / textureSize(directionalLightMap, 0).xy);
	else shadowMapStep = vec2(1.0 / textureSize(val_shadowmaps, 0).xy);


	shadowCoord.x -= shadowMapStep.x;
	float distanceFromLight;
	if (layer == -1) distanceFromLight = texture(directionalLightMap, shadowCoord.xy).r;
	else distanceFromLight = texture(val_shadowmaps, vec3(shadowCoord.xy, layer)).r;
	dir.x = sceneDepth > distanceFromLight ? 0.0f : 1.0f;

	shadowCoord.x += 2.0 * shadowMapStep.x;
	if (layer == -1) distanceFromLight = texture(directionalLightMap, shadowCoord.xy).r;
	else distanceFromLight = texture(val_shadowmaps, vec3(shadowCoord.xy, layer)).r;
	dir.y = sceneDepth > distanceFromLight ? 0.0f : 1.0f;

	shadowCoord.x -= shadowMapStep.x;
	shadowCoord.y += shadowMapStep.y;
	if (layer == -1) distanceFromLight = texture(directionalLightMap, shadowCoord.xy).r;
	else distanceFromLight = texture(val_shadowmaps, vec3(shadowCoord.xy, layer)).r;
	dir.z = sceneDepth > distanceFromLight ? 0.0f : 1.0f;

	shadowCoord.y -= 2.0 * shadowMapStep.y;
	if (layer == -1) distanceFromLight = texture(directionalLightMap, shadowCoord.xy).r;
	else distanceFromLight = texture(val_shadowmaps, vec3(shadowCoord.xy, layer)).r;
	dir.w = sceneDepth > distanceFromLight ? 0.0f : 1.0f;

	return abs(dir - 1.0);

}

float computeRelativeDistance(vec4 shadowCoord, vec2 dir, float c, float sceneDepth, float layer)
{

	vec4 tempShadowCoord = shadowCoord;
	float foundSilhouetteEnd = 0.0;
	float distance = 0.0;
	vec2 shadowMapStep;

	if (layer == -1) shadowMapStep = vec2(1.0 / textureSize(directionalLightMap, 0).xy);
	else shadowMapStep = vec2(1.0 / textureSize(val_shadowmaps, 0).xy);

	vec2 step = dir * shadowMapStep;
	tempShadowCoord.xy += step;

	float distanceFromLight;
	for (int it = 0; it < 256; it++) {

		if (layer == -1) distanceFromLight = texture(directionalLightMap, tempShadowCoord.xy).r;
		else distanceFromLight = texture(val_shadowmaps, vec3(tempShadowCoord.xy, layer)).r;

		float center = sceneDepth > distanceFromLight ? 0.0f : 1.0f;
		bool isCenterUmbra = !bool(center);

		if (isCenterUmbra) {
			foundSilhouetteEnd = 1.0;
			break;
		}
		else {
			vec4 d = computeDiscontinuity(tempShadowCoord, sceneDepth, layer);
			if ((d.r + d.g + d.b + d.a) == 0.0) break;
		}

		distance++;
		tempShadowCoord.xy += step;

	}

	distance = distance + (1.0 - c);
	return mix(-distance, distance, foundSilhouetteEnd);

}

vec4 computeRelativeDistance(vec4 shadowCoord, vec2 c, float sceneDepth, float layer = -1)
{

	float dl = computeRelativeDistance(shadowCoord, vec2(-1, 0), (1.0 - c.x), sceneDepth, layer);
	float dr = computeRelativeDistance(shadowCoord, vec2(1, 0), c.x, sceneDepth, layer);
	float db = computeRelativeDistance(shadowCoord, vec2(0, -1), (1.0 - c.y), sceneDepth, layer);
	float dt = computeRelativeDistance(shadowCoord, vec2(0, 1), c.y, sceneDepth, layer);
	return vec4(dl, dr, db, dt);

}

float normalizeRelativeDistance(vec2 dist) {

	float T = 1;
	if (dist.x < 0.0 && dist.y < 0.0) T = 0;
	if (dist.x > 0.0 && dist.y > 0.0) T = -2;

	float length = min(abs(dist.x) + abs(dist.y), float(256));
	return abs(max(T * dist.x, T * dist.y)) / length;

}

vec2 normalizeRelativeDistance(vec4 dist)
{

	vec2 r;
	r.x = normalizeRelativeDistance(vec2(dist.x, dist.y));
	r.y = normalizeRelativeDistance(vec2(dist.z, dist.w));
	return r;

}

float revectorizeShadow(vec2 r)
{

	if ((r.x * r.y > 0) && (1.0 - r.x > r.y)) return 0.0;
	else return 1.0;

}

float DirectionalRBSM(vec4 fragPosLS, vec3 norm) {

	vec3 proj = fragPosLS.xyz / fragPosLS.w;
	proj = proj * 0.5 + 0.5;
	vec4 shadowCoord = vec4(proj, 1.0);

	vec3 N = normalize(norm);
	float bias = max(0.05 * (1.0 - dot(N, sun_dir)), 0.005);
	float distanceFromLight = texture(directionalLightMap, shadowCoord.xy).r;
	float shadow = (shadowCoord.z - bias) > distanceFromLight ? 0.0f : 1.0f;
	if (shadow == 0.0f) return shadow;

	vec4 d = computeDiscontinuity(shadowCoord, shadowCoord.z - bias);
	if ((d.r + d.g + d.b + d.a) == 0.0) return 1.0;
	else if ((d.r + d.g) == 2.0 || (d.b + d.a) == 2.0) return 0.0;

	vec2 c = fract(vec2(shadowCoord.x * float(textureSize(directionalLightMap, 0).x), shadowCoord.y * float(textureSize(directionalLightMap, 0).y)));
	vec4 dist = computeRelativeDistance(shadowCoord, c, shadowCoord.z - bias);
	vec2 r = normalizeRelativeDistance(dist);
	return revectorizeShadow(r);

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


#define EPSILON 0.1f
float ParabolicRBSM(vec4 worldPos, mat4 lightV, float layer) {

	vec4 lpos = lightV * M * worldPos;
	float L = length(lpos.xyz);
	lpos /= L;

	vec4 shadowCoord = vec4(0.0);
	shadowCoord.x = (lpos.x / (1.0f + lpos.z))*0.5f + 0.5f;
	shadowCoord.y = (lpos.y / (1.0f + lpos.z))*0.5f + 0.5f;

	float sceneDepth = (L - parabola_near) / (parabola_far - parabola_near);
	float distanceFromLight = texture(val_shadowmaps, vec3(shadowCoord.xy, layer)).r;
	float shadow = (sceneDepth - EPSILON) > distanceFromLight ? 0.0f : 1.0f;
	if (shadow == 0.0f) return shadow;

	vec4 d = computeDiscontinuity(shadowCoord, sceneDepth - EPSILON, layer);
	if ((d.r + d.g + d.b + d.a) == 0.0) return 1.0;
	else if ((d.r + d.g) == 2.0 || (d.b + d.a) == 2.0) return 0.0;

	vec2 c = fract(vec2(shadowCoord.x * float(textureSize(val_shadowmaps, 0).x), shadowCoord.y * float(textureSize(val_shadowmaps, 0).y)));
	vec4 dist = computeRelativeDistance(shadowCoord, c, sceneDepth - EPSILON, layer);
	vec2 r = normalizeRelativeDistance(dist);
	return revectorizeShadow(r);

}

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

//const vec3 kd = vec3(0.15, 0.15, 0.15);
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


vec2 PCSS_blocker_search(int light_size, float curr_depth, vec2 curr_xy, vec2 px_size)
{
	int blockers = 0;
	float avg_blk_dist = 0.0f;
	//texel coordinates of receiver point
	vec2 coords = curr_xy;

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

float PCSS_penumbra(int filter_size, vec2 curr_xy, float curr_depth, float bias, vec2 px_size)
{

	float shadow = 0.0;

	for (int i = -filter_size; i <= filter_size; i++)
	{
		for (int j = -filter_size; j <= filter_size; j++)
		{
			float pcfdepth = texture(directionalLightMap, curr_xy + vec2(i, j)*px_size).r;
			shadow += ((curr_depth - bias) > pcfdepth) ? 0.0 : 1.0;
		}
	}

	shadow /= (2 * filter_size + 1)*(2 * filter_size + 1);
	return shadow;
}

//light size and light space position
float do_directionalPCSS(int light_size, vec4 ls_pos, vec3 frag_normal)
{
	vec3 proj = ls_pos.xyz / ls_pos.w;
	proj = proj * 0.5 + 0.5;
	float closest = texture(directionalLightMap, proj.xy).r;

	float curr_depth = proj.z;

	vec2 px_size = vec2(1.0 / textureSize(directionalLightMap, 0).xy);

	//blocker search
	vec2 blocker_info = PCSS_blocker_search(light_size, curr_depth, proj.xy, px_size);
	//penumbra calculation
	float size = 1;

	if (blocker_info.y > 1)
		size = light_size * (curr_depth - blocker_info.x) / blocker_info.x;

	vec3 N = normalize(frag_normal);
	float bias = max(0.05 * (1.0 - dot(N, sun_dir)), 0.005);

	return PCSS_penumbra(int(size), proj.xy, curr_depth, bias, px_size);
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
	vec2 px_size = vec2(1.0 / 1024.f, 1.0 / 1024.f);

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

	//penumbra calculation
	float size = 1;

	if (blockers > 1)
		size = light_size * (curr_depth - avg_blk_dist) / avg_blk_dist;

	float bias = 0.05f;

	float shadow = 0.0;
	int filter_size = int(size);

	vec2 curr_xy = tex_coord;
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
	
	float spec = frag_albedo.a;

	//directional pass
	vec3 ambient = kd * frag_albedo.xyz;

	float diffuse = max(0.0, dot(sun_dir, frag_n.xyz));

	vec3 eye_dir = normalize(eye_pos - frag_pos.xyz);
	vec3 half_vector = normalize(eye_dir + sun_dir);

	float spec_val = pow(max(dot(frag_n.xyz, half_vector), 0.0), 16);
	vec3 specular = vec3(spec_val * spec);

	vec4 light_space_pos = lightspacemat * vec4(frag_pos.xyz, 1.0);
	float in_shadow = DirectionalRBSM(light_space_pos, frag_n.xyz);
	//float in_shadow = DirectionalShadow(light_space_pos, frag_n.xyz);
	//float in_shadow = do_directionalPCSS(DIRECTIONAL_LIGHT_SIZE, light_space_pos, frag_n.xyz);

	vec3 directional = (in_shadow * (diffuse + specular)) * vec3(1.0);

	output_color += directional;

	for (int i = 0; i < lights_per_tile; i++)
	{
		plight curr_light = list[flat_id + i];

		float layer = curr_light.n.w;
		float ret = ParabolicRBSM(vec4(frag_pos.xyz, 1.0), parabolic_mats[int(layer)], layer);
		//float ret = ParabolicShadow(frag_pos, parabolic_mats[int(layer)], layer);
		//float ret = do_parabolicPCSS(VAL_LIGHT_SIZE, frag_pos, layer);

		//if (ret > 0.0f)
		output_color += ret * shade_point(curr_light, frag_pos.xyz, frag_n.xyz) ;
	}

	if (see_bounce)
	{
		int lights_per_tile2 = num_second_bounce / (num_rows * num_cols);
		for (int i = 0; i < lights_per_tile2; i++)
		{
			plight curr_light = back_vpl_list[flat_id + i];
			output_color += (shade_point(curr_light, frag_pos.xyz, frag_n.xyz));
		}
	}

	output_color *= 0.85;

	ivec2 fpos = ivec2(gl_GlobalInvocationID.xy);
	imageStore(imageBuffer, fpos, vec4(output_color, 1.0));

}