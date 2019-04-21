#version 430

layout(local_size_x = 16, local_size_y = 16) in;

#define NUM_CLUSTERS 4
#define Wid 1280.0f
#define Hei 720.0f

layout(binding = 0) uniform sampler2D positionBuffer;
layout(binding = 1) uniform sampler2D normalBuffer;
layout(binding = 2) uniform sampler2D albedoBuffer;
layout(binding = 3) uniform sampler2D depthBuffer;

layout(binding = 4) uniform sampler2D shadow_map;

layout(binding = 5) uniform sampler2DArray val_shadowmaps;

//layout(binding = 5) uniform sampler2D pShadowMap1;
//layout(binding = 6) uniform sampler2D pShadowMap2;
//layout(binding = 7) uniform sampler2D pShadowMap3;
//layout(binding = 8) uniform sampler2D pShadowMap4;

uniform mat4 M;
uniform mat4 P;
uniform mat4 invProj;
uniform mat4 ViewMat;
uniform mat4 lightSpaceMat;

layout(std140, binding = 0) uniform vpl_matrices{
	mat4 parabolic_mats[NUM_CLUSTERS];
};

//ism near - far
uniform float near;
uniform float far;

uniform vec3 eyePos;

uniform int num_vpls;
uniform int num_vals;

//directional light 
uniform vec3 sun_Dir;

//do second bounce
uniform bool double_bounce;

layout(rgba32f, binding = 0) uniform image2D imageBuffer;

struct frustum {
	vec4 planes[4];
};

layout(std430, binding = 0) buffer frusta {
	frustum f[];
};

struct plight {
	vec4 p; //position + radius
	vec4 n;
	vec4 c; //color	
};

layout(std430, binding = 1) buffer lightSSBO {
	plight list[];
};

layout(std430, binding = 2) buffer direct_vals {
	plight val_list[];
};

layout(std430, binding = 3) buffer backface_vpls {
	plight back_vpl_list[];
};

layout(std430, binding = 4) buffer backface_vpl_count {
	unsigned int back_vpl_count;
};


float projtoview(float depth)
{
	return 1.0f / (depth * invProj[2][3] + invProj[3][3]);
}

//from screen to world
vec4 unproject(vec4 v) {
	v = invProj * v;
	v /= v.w;
	return v;
}

//create a plane from three non colinear points
vec4 create_plane(vec3 p0, vec3 p1, vec3 p2) {
	vec3 v1 = p1 - p0;
	vec3 v2 = p2 - p0;

	vec4 N;
	N.xyz = normalize(cross(v1, v2));
	N.w = dot(N.xyz, p0);
	
	return N;
}

//create a plane from two vectors (assuming one of three points is the origin 0,0,0)
vec4 create_plane2(vec4 a, vec4 b) {
	vec4 normal;
	normal.xyz = normalize(cross(a.xyz, b.xyz));
	normal.w = 0;
	return normal;
}

//plane over origin (0,0,0)
float dist_point_plane(vec4 p, vec4 plane){
	return dot(plane.xyz, p.xyz);
}

float dist_point_plane2(vec4 p, vec4 plane)
{
	return (dot(plane.xyz, p.xyz) - plane.w);
}

float DirectionalShadow(vec4 fragPosLS, vec3 norm) {
	vec3 proj = fragPosLS.xyz / fragPosLS.w;
	proj = proj * 0.5 + 0.5;
	float closest = texture(shadow_map, proj.xy).r;
	float curr = proj.z;

	vec3 N = normalize(norm);
	float bias = max(0.05 * (1.0 - dot(N, sun_Dir)), 0.005);
	float shadow = 0.0f;
	shadow = curr - bias > closest ? 0.0f : 1.0f;
	return shadow;
}

#define EPSILON 0.05f
float ParabolicShadow(vec4 worldPos, mat4 lightMV, float layer) {

	vec4 lpos = lightMV * M * worldPos;
	float L = length(lpos.xyz);
	lpos /= L;
	float ret = 0.0f;

	float fdepth;
	float fscene_depth;
		
	vec2 tex_coord;
	tex_coord.x = (lpos.x / (1.0f + lpos.z))*0.5f + 0.5f;
	tex_coord.y = (lpos.y / (1.0f + lpos.z))*0.5f + 0.5f;

	fscene_depth = (L - near) / (far - near);

	fdepth = texture(val_shadowmaps, vec3(tex_coord, layer)).r;	
	ret = ((fscene_depth - EPSILON) > fdepth) ? 0.0f : 1.0f;
	
	return ret;
}

float get_attenuation(float light_radius, float dist) {

	float cutoff = 0.3;
	float denom = dist / light_radius + 1.0;
	float attenuation = 1.0 / (denom * denom);

	attenuation = (attenuation - cutoff) / (1 - cutoff);
	attenuation = max(attenuation, 0.0);
	return (attenuation);
}

const vec3 kd = vec3(0.15, 0.15, 0.15);
vec3 shade_point(plight light, vec3 ppos, vec3 pnormal, vec3 pcol)
{
	vec3 col = vec3(0.0f, 0.0f, 0.0f);	
		
	vec3 lightdir = normalize(light.p.xyz - ppos);
		
	float dist = length(ppos - light.p.xyz);

	float atten = get_attenuation(light.p.w, length(ppos - light.p.xyz));
	
	vec3 ltop = normalize(light.p.xyz - ppos); //light to pos
	vec3 ptol = normalize(ppos - light.p.xyz); //pos to light
	
	col = light.c.xyz * kd * max(0.0, dot(pnormal, lightdir)) * atten ;

	return col;
}

//left <-> right ;; bottom - top

#define MAX_LIGHTS 512

shared uint minDepth;
shared uint maxDepth;
shared uint lightCount;
shared uint lightIdx[MAX_LIGHTS];

#define MAX_BACK_LIGHTS 512
shared uint backLightCount;
shared uint backLightIdx[MAX_BACK_LIGHTS];

void main(void){

	if (gl_LocalInvocationIndex == 0)
	{
		minDepth = 0xFFFFFFFF;
		maxDepth = 0;
		lightCount = 0;
		backLightCount = 0;
		
	}
	
	barrier();
	memoryBarrierShared();

	ivec2 fpos = ivec2(gl_GlobalInvocationID.xy);
	//adjusting pixel position to avoid banding artifacts
	vec2 uv = vec2( (float(fpos.x) + 0.5f) / Wid,
		            (float(fpos.y) + 0.5f) / Hei
				  );

	//float d = projtoview(texture(depthBuffer, uv).r * 2.0f - 1.0f);
	float d = (texture(depthBuffer, uv).r * 2.0f) - 1.0f;
	float v = unproject(vec4(0.0, 0.0, d, 1.0)).z;
		
	uint depth_uint = floatBitsToUint(v);
	
	atomicMin(minDepth, depth_uint);
	atomicMax(maxDepth, depth_uint);
	
	barrier();
	memoryBarrierShared();
				
	float maxZ = uintBitsToFloat(maxDepth);
	float minZ = uintBitsToFloat(minDepth);		
	
	barrier();
	memoryBarrierShared();

	uint max_threads = gl_WorkGroupSize.x * gl_WorkGroupSize.y;	

	//main lights
	for(uint i = gl_LocalInvocationIndex; i < num_vpls; i += max_threads)
	{				
		plight l = list[i];
		vec4 vs_light_pos = ViewMat * M * vec4(l.p.xyz, 1.0);
		unsigned int index = gl_WorkGroupID.y * gl_NumWorkGroups.x + gl_WorkGroupID.x;
		if (lightCount < MAX_LIGHTS)
		{
			
			if ( 
				 (abs(dist_point_plane(vs_light_pos, f[index].planes[0])) < l.p.w)  &&
				 (abs(dist_point_plane(vs_light_pos, f[index].planes[1])) < l.p.w)  &&
				 (abs(dist_point_plane(vs_light_pos, f[index].planes[2])) < l.p.w)  &&
				 (abs(dist_point_plane(vs_light_pos, f[index].planes[3])) < l.p.w) 				 
			   )
			{
				//opengl view space is right handed
				if (
					(vs_light_pos.z - l.p.w < minZ) &&
					(vs_light_pos.z + l.p.w > maxZ)
					)
				{
					uint id = atomicAdd(lightCount, 1);
					lightIdx[id] = i;
				}
			}				
		}		
	}

	barrier();
	if (double_bounce == true)
	{
		//second bounce lights
		for (uint i = gl_LocalInvocationIndex; i < back_vpl_count; i += max_threads)
		{
			plight l = back_vpl_list[i];
			vec4 vs_light_pos = ViewMat * vec4(l.p.xyz, 1.0);
			unsigned int index = gl_WorkGroupID.y * gl_NumWorkGroups.x + gl_WorkGroupID.x;
			if (backLightCount < MAX_BACK_LIGHTS)
			{

				if (
					(abs(dist_point_plane(vs_light_pos, f[index].planes[0])) < l.p.w) &&
					(abs(dist_point_plane(vs_light_pos, f[index].planes[1])) < l.p.w) &&
					(abs(dist_point_plane(vs_light_pos, f[index].planes[2])) < l.p.w) &&
					(abs(dist_point_plane(vs_light_pos, f[index].planes[3])) < l.p.w)
					)
				{
					//opengl view space is right handed
					if (
						(vs_light_pos.z - l.p.w < minZ) &&
						(vs_light_pos.z + l.p.w > maxZ)
						)
					{
						uint id = atomicAdd(backLightCount, 1);
						backLightIdx[id] = i;
					}
				}
			}
		}

		barrier();
	}
	//memoryBarrierShared();		

	//light calculation
	vec3 pnormal = texture(normalBuffer, uv).xyz;
	vec4 flux = texture(albedoBuffer, uv);	
	//position in model space
	vec4 mpos = texture(positionBuffer, uv);
	vec3 ppos = texture(positionBuffer, uv).xyz;
	
	vec3 palbedo = flux.rgb;	
	float spec_c = flux.a;
	

	vec4 col = vec4(0.0, 0.0, 0.0, 1.0);
	
	//Directional Shading
	float diff = max(0.0f, dot(pnormal, sun_Dir)) ;
	vec3 eyeDir = normalize(ppos - eyePos);
	float spec = 0.0f;
	vec3 half_v = normalize(sun_Dir + eyeDir);
	spec = pow(max(dot(pnormal, half_v), 0.0), 32);	
	vec3 glossy = vec3(spec) * spec_c;
	
	vec4 pos_lightspace = lightSpaceMat * vec4(mpos.xyz, 1.0);
	float shadow = DirectionalShadow(pos_lightspace, pnormal);
	vec3 diffuse = vec3(diff);
	vec3 lighting = (shadow * (glossy + diffuse)) * palbedo;
	
	col += vec4(lighting, 1.0);	
	
	//VPL shading
	for (uint i = 0; i < lightCount; ++i)
	{
		uint idx = lightIdx[i];
		plight l = list[idx];

		float ret;
		float layer = l.n.w;
		ret = ParabolicShadow(mpos, parabolic_mats[int(layer)], layer);

		//if (l.n.w == 0)
		//{			
		//	ret = ParabolicShadow(mpos, parabolic_mats[0], pShadowMap1);	
		//}
		//if (l.n.w == 1)
		//{			
		//	ret = ParabolicShadow(mpos, parabolic_mats[1], pShadowMap2);
		//}
		//if (l.n.w == 2)
		//{			
		//	ret = ParabolicShadow(mpos, parabolic_mats[2], pShadowMap3);
		//}
		//if (l.n.w == 3)
		//{
		//	ret = ParabolicShadow(mpos, parabolic_mats[3], pShadowMap4);
		//}		
		
		col += ret * vec4(shade_point(l, ppos, pnormal, palbedo), 0.0);	
		
	}

	if (double_bounce == true)
	{
		for (uint i = 0; i < backLightCount; ++i)
		{
			uint idx = backLightIdx[i];
			plight l = back_vpl_list[idx];
			col += vec4(shade_point(l, ppos, pnormal, palbedo), 0.0);
		}
	}

	col *= vec4(palbedo, 1.0);
		
	barrier();	
	memoryBarrierShared();

	imageStore(imageBuffer, fpos , col);

	
	//if (gl_LocalInvocationID.x == 0 || gl_LocalInvocationID.y == 0 || gl_LocalInvocationID.x == 16 || gl_LocalInvocationID.y == 16)
	//	imageStore(imageBuffer, fpos, vec4(1.f, 1.f, 1.f, 0.2f));


}