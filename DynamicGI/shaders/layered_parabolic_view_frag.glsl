#version 430

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

uniform mat4 M;

#define NUM_CLUSTERS 4
layout(std140, binding = 0) uniform vpl_matrices{
	mat4 parabolic_mats[NUM_CLUSTERS];
};

uniform float ism_near;
uniform float ism_far;

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
		pos.z = (L - ism_near) / (ism_far - ism_near);
		pos.w = 1.0f;
	
		rsm_position = fs_in.p;
		rsm_normal = normalize(fs_in.n);
		rsm_flux = texture2D(texture_diffuse1, fs_in.uv);
		gl_FragDepth = pos.z;
	}
	
}