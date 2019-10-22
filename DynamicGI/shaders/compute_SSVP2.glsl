#version 430

layout(local_size_x = 8, local_size_y = 8) in;

//back gbuffer
layout(binding = 0)	uniform sampler2DArray prsm_pos;
layout(binding = 1)	uniform sampler2DArray prsm_norm;
layout(binding = 2)	uniform sampler2DArray prsm_flux;

//screen samples between 0.0 and 1.0
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

uniform float vpl_radius;


void main() {
     
	//if (gl_LocalInvocationIndex.x == 0)
	//{
	//	back_vpl_count = 0;
	//}
	//barrier();

	float SECOND_BOUNCE_RAD = vpl_radius;// - (vpl_radius * 0.15f);  
	
	vec2 uv = imageLoad(samples, int(gl_LocalInvocationIndex)).rg;

	uint id = gl_WorkGroupID.x;
	
	vec3 p = texture(prsm_pos, vec3(uv, id)).xyz;
	vec4 n = texture(prsm_norm, vec3(uv, id));
	vec4 c = texture(prsm_flux, vec3(uv, id));

	uint idx = atomicAdd(back_vpl_count, 1);
	back_vpl_list[idx] = plight(vec4(p, SECOND_BOUNCE_RAD), n, c);
	

}



