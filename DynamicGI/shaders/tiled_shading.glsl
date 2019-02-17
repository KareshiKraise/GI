#version 430

layout(local_size_x = 16, local_size_y = 16) in;

//max lights per tile we can handle

#define MAX_LIGHTS 256
#define Wid 1280.0f
#define Hei 720.0f

layout(binding = 0) uniform sampler2D positionBuffer;
layout(binding = 1) uniform sampler2D normalBuffer;
layout(binding = 2) uniform sampler2D albedoBuffer;
layout(binding = 3) uniform sampler2D depthBuffer;

layout(binding = 4) uniform sampler2D shadow_map;

uniform mat4 P;
uniform mat4 invProj;
uniform mat4 ViewMat;
uniform mat4 lightSpaceMat;

uniform vec3 eyePos;

uniform int NumLights;

//directional light 
uniform vec3 sun_Dir;

layout(rgba32f, binding = 0) uniform image2D imageBuffer;

struct plight {
	vec4 p; //position + radius
	vec4 n;
	vec4 c; //color	
};

layout(std430, binding = 5) buffer light_buffer {
	plight list[];
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

float inShadow(vec4 fragPosLS, vec3 norm) {
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

float get_attenuation(float light_radius, float dist) {

	float cutoff = 0.3;
	float denom = dist / light_radius + 1.0;
	float attenuation = 1.0 / (denom * denom);

	attenuation = (attenuation - cutoff) / (1 - cutoff);
	attenuation = max(attenuation, 0.0);
	return (attenuation);
}

const vec3 kd = vec3(0.1, 0.1, 0.1);
vec3 shade_point(plight light, vec3 ppos, vec3 pnormal, vec3 pcol)
{
	vec3 col = vec3(0.0f, 0.0f, 0.0f);	
		
	vec3 lightdir = normalize(light.p.xyz - ppos);
		
	float atten = get_attenuation(light.p.w, length(ppos - light.p.xyz));
	
	vec3 ltop = light.p.xyz - ppos; //light to pos
	vec3 ptol = ppos - light.p.xyz; //pos to light

	vec3 I = light.c.xyz * max(0.0, dot(light.n.xyz,ptol));
	//vec3 E = kd * pcol * I *  atten; //*max(0.0, dot(pnormal, lightdir));

	col = light.c.xyz * atten * pcol * kd * max(0.0, dot(pnormal, lightdir));

	return col;
}


//left <-> right ;; bottom - top
shared vec4 frustum[4];
shared uint minDepth;
shared uint maxDepth;
shared uint lightCount;
shared uint lightIdx[MAX_LIGHTS];

void main(void){

	if (gl_LocalInvocationIndex == 0)
	{
		minDepth = 0xFFFFFFFF;
		maxDepth = 0;
		lightCount = 0;
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
	
	//Compute frustum		
	//min and max coords of screen space tile
	uint minX = gl_WorkGroupSize.x * gl_WorkGroupID.x;
	uint minY = gl_WorkGroupSize.y * gl_WorkGroupID.y;
	uint maxX = gl_WorkGroupSize.x * (gl_WorkGroupID.x + 1);
	uint maxY = gl_WorkGroupSize.y * (gl_WorkGroupID.y + 1);
	
	float c_minY = (float(minY) / Hei)  *  2.0f - 1.0f;
	float c_minX = (float(minX) / Wid)  *  2.0f - 1.0f;
	float c_maxX = (float(maxX) / Wid)  *  2.0f - 1.0f;
	float c_maxY = (float(maxY) / Hei)  *  2.0f - 1.0f;

	//Corners in NDC
	vec4 corners[4];
	
	//project corners to far plane
	corners[0] = unproject(vec4(c_minX, c_maxY, 1.0f, 1.0f));
	corners[1] = unproject(vec4(c_maxX, c_maxY, 1.0f, 1.0f));
	corners[2] = unproject(vec4(c_maxX, c_minY, 1.0f, 1.0f));
	corners[3] = unproject(vec4(c_minX, c_minY, 1.0f, 1.0f));
			
	for (int i = 0; i < 4; i++)
	{
		//create planes with normals pointing to the inside splace of the frustum
		frustum[i] = create_plane2(corners[i], corners[(i + 1) & 3]);
	}
		
	float maxZ = uintBitsToFloat(maxDepth);
	float minZ = uintBitsToFloat(minDepth);		
	

	barrier();
	memoryBarrierShared();

	uint max_threads = gl_WorkGroupSize.x * gl_WorkGroupSize.y;
	
	for(uint i = gl_LocalInvocationIndex; i < NumLights; i += max_threads)
	{				
		plight l = list[i];
		vec4 vs_light_pos = ViewMat * vec4(l.p.xyz, 1.0);

		if (lightCount < MAX_LIGHTS)
		{
			
			if ( 
				 (dist_point_plane(vs_light_pos, frustum[0]) < l.p.w)  &&
				 (dist_point_plane(vs_light_pos, frustum[1]) < l.p.w)  &&
				 (dist_point_plane(vs_light_pos, frustum[2]) < l.p.w)  &&
				 (dist_point_plane(vs_light_pos, frustum[3]) < l.p.w) 				 
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
	memoryBarrierShared();
		

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
	spec = pow(max(dot(pnormal, half_v), 0.0), 8);	
	vec3 glossy = vec3(spec) * spec_c;
	
	vec4 pos_lightspace = lightSpaceMat * vec4(mpos.xyz, 1.0);
	float shadow = inShadow(pos_lightspace, pnormal);
	
	vec3 lighting = ((shadow) * (glossy + vec3(diff))) * palbedo;
	
	col += vec4(lighting, 1.0);	
	
	//VPL shading
	for (uint i = 0; i < lightCount; ++i)
	{
		uint idx = lightIdx[i];
		plight l = list[idx];
		//color accumulation
		col += vec4(shade_point(l, ppos, pnormal, palbedo), 0.0);	
		
	}	
	
	barrier();	
	memoryBarrierShared();

	imageStore(imageBuffer, fpos , col);

	//if (gl_LocalInvocationID.x == 0 || gl_LocalInvocationID.y == 0 || gl_LocalInvocationID.x == 16 || gl_LocalInvocationID.y == 16)
	//	imageStore(imageBuffer, fpos, vec4(0.1f, 0.1f, 0.1f, 0.2f));


}