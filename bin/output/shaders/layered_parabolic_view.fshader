#version 430

#define NUM_CLUSTERS 4

layout(location = 0) out vec3 rsm_position;
layout(location = 1) out vec3 rsm_normal;
layout(location = 2) out vec4 rsm_flux;

in GS_OUT{
	vec3 p;
	vec2 uv;
	vec3 n;
	float dval;
	float clipDepth;	
}fs_in;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;

uniform mat4 M;

uniform float near;
uniform float far;

layout(std140, binding = 0) uniform vpl_matrices{
	mat4 parabolic_mats[NUM_CLUSTERS];
};


void main()
{
	//if (fs_in.clipDepth < 0.f)
	//	discard;
	
	vec4 pos = vec4(fs_in.p, 1.0);
	pos = parabolic_mats[gl_Layer] * M * pos;
	pos /= pos.w;
	
	float L = length(pos.xyz);
	pos /= L;
	
	if (pos.z < 0.f)
	{
		discard;
	}
	else
	{
		pos.x = pos.x / (pos.z + 1.0f);
		pos.y = pos.y / (pos.z + 1.0f);
		pos.z = (L - near) / (far - near);
		pos.w = 1.0f;
	
		rsm_position = fs_in.p;
		rsm_normal = texture2D(texture_normal1, fs_in.uv).xyz;
		rsm_flux = texture2D(texture_diffuse1, fs_in.uv);
		gl_FragDepth = pos.z;
	}
	
}