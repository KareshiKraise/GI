#version 430

layout(local_size_x = 8, local_size_y = 8) in;

layout(binding = 0) uniform samplerCube cubePos;
layout(binding = 1) uniform samplerCube cubeNormal;
layout(binding = 2) uniform samplerCube cubeColor;

uniform vec4 refPos; //vec4(eyePos + eyeDir, angle_in_radians)
uniform int NumLights;
uniform int MaxVPLs;
uniform int vpl_factor;
#define PI 3.1415926f

layout(std430, binding = 4) buffer num_light {
	uint nlights;
};


struct plight {
	vec4 p;
	vec4 n;
	vec4 c;
};

layout(std430, binding = 5) buffer light_buffer {
	plight list[];
};


layout(rg32f, binding = 0) uniform imageBuffer samples;

//create orthonormal basis from inigo quilez 
mat3 makeBase(in vec3 w) 
{
	float k = inversesqrt(1.0 - w.y*w.y);
	return mat3(vec3(-w.z, 0.0, w.x)*k,
		        vec3(-w.x*w.y, 1.0 - w.y*w.y, -w.y*w.z)*k,
		        w);
}

vec3 generate_offset_vector(vec3 dir, vec2 rand) 
{	
	float ang = refPos.w;
	
	//float k = inversesqrt(1.0 - dir.y*dir.y);
	//vec3 xaxis = vec3(-dir.z, 0.0f, dir.x)*k;
	//vec3 yaxis = vec3(-dir.x*dir.y, 1.0f - dir.y*dir.y, -dir.y*dir.z)*k;
	//vec3 zaxis = dir;

	vec3 xaxis = normalize(cross(dir, vec3(0.0,0.0,1.0)));
	vec3 yaxis = normalize(cross(dir, xaxis));
	vec3 zaxis = dir;

	float rot = 2.0 * PI * rand.x;
	float the = acos(1.0 - rand.y*(1.0 - cos(ang)));
	float sinThe = sin(the);

	vec3 vec = xaxis * sinThe*cos(rot) + yaxis * sinThe*sin(rot) + zaxis * cos(the);
	//float term = (1.0 - rand.y*(1.0 - cos(ang)));
	//vec3 vec = vec3(cos(rot)*sqrt(1.0 - pow(term, 2)),
	//	            sin(rot)*sqrt(1.0 - pow(term, 2)),
	//				    1.0 - rand.y*(1.0 - cos(ang)));
	return vec;
}



//TODO Fix  for loop, ammount of max vpls has to be max size of light buffer
void main()
{
	if (gl_LocalInvocationIndex == 0)
	{
		nlights = NumLights;
	}

	barrier();	
	memoryBarrierShared();

	uint thread_num = gl_WorkGroupSize.x * gl_WorkGroupSize.y;
	if (vpl_factor > 0)
	{
		for (uint i = (gl_LocalInvocationIndex*vpl_factor + NumLights); i < MaxVPLs; i += (thread_num*vpl_factor))
		{
			uint lbuff_idx = i;
			for (uint j = 0; j < vpl_factor; ++j)
			{				
				vec2 samp = imageLoad(samples, int(gl_LocalInvocationIndex*vpl_factor + j)).xy;
				vec4 pos = list[gl_LocalInvocationIndex].p;
				vec3 dir = list[gl_LocalInvocationIndex].n.xyz;
				vec4 _c = list[gl_LocalInvocationIndex].c;

				vec3 offset_dir = normalize(generate_offset_vector(dir, samp));

				vec4 P = vec4(texture(cubePos, offset_dir).xyz, 20.0f);
				vec4 N = vec4(texture(cubeNormal, offset_dir).xyz, 1.0f);
				vec4 C = vec4(texture(cubeColor, offset_dir).xyz, 1.0f);

				//if (length(P.xyz - pos.xyz) < pos.w)
				//{
					vec4 col = clamp(_c * C, vec4(0.0f), vec4(1.0f));
					plight val = plight(P, N, col);
					list[lbuff_idx++] = val;
					atomicAdd(nlights, 1);
				//}

			}

			//vec2 samp = imageLoad(samples, int(gl_LocalInvocationIndex)).xy;
			////vec3 dir = normalize((refPos.xyz) - list[gl_LocalInvocationIndex].p.xyz);
			//vec4 pos = list[gl_LocalInvocationIndex].p;
			//vec3 dir = list[gl_LocalInvocationIndex].n.xyz;
			//
			//vec3 offset_dir = normalize(generate_offset_vector(dir, samp));
			//
			//vec4 P = vec4(texture(cubePos,    offset_dir).xyz, 20.0f);
			//vec4 N = vec4(texture(cubeNormal, offset_dir).xyz, 1.0f);
			//vec4 C = vec4(texture(cubeColor,  offset_dir).xyz, 1.0f);
			//
			//if (length(P.xyz - pos.xyz) < pos.w)
			//{
			//	plight val = plight(P, N, C);
			//	list[i] = val;
			//	atomicAdd(nlights, 1);
			//}
			//plight val = plight(P, N, C);
			//list[i] = val;
			//atomicAdd(nlights, 1);
		}
	}

	
}


