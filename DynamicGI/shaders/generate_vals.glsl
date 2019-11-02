#version 430

//rel num val clusters
layout(local_size_x = 4) in;

layout(binding = 0) uniform sampler2D rsm_position;
layout(binding = 1) uniform sampler2D rsm_normal;
layout(binding = 2) uniform sampler2D rsm_flux;

uniform int num_vpls;
uniform int num_clusters;

//screen samples between 0.0 and 1.0 -- not being used as of now
layout(rg32f, binding = 0) uniform imageBuffer samples;

struct plight {
	vec4 p;
	vec4 n;
	vec4 c;
};

//first bounce val list
layout(std430, binding = 0) buffer val_buffer {
	plight first_val_list[];
};

//first bounce vpl list
layout(std430, binding = 1) buffer vpl_buffer {
	plight vpl_list[];
};


void main()
{	
	//vec2 samp = imageLoad(samples, int(gl_LocalInvocationIndex)).xy;
	//vec4 pos = texture2D(rsm_position, samp);
	//vec4 nor = texture2D(rsm_normal, samp);
	//vec4 col = texture2D(rsm_flux, samp);
	

	//the k-val is the (k*N/M)th vpl
	uint k = gl_LocalInvocationIndex;
	uint N = num_vpls;
	uint M = num_clusters;

	uint index = k * (N / M);
	
	plight curr = vpl_list[index];
	vec4 pos = curr.p;
	vec4 nor = curr.n;
	vec4 col = curr.c;

	first_val_list[gl_LocalInvocationIndex] = plight(pos, nor, col);
}