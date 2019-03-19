#version 430

layout(local_size_x = 4) in;

layout(binding = 0) uniform sampler2D rsm_position;
layout(binding = 1) uniform sampler2D rsm_normal;
layout(binding = 2) uniform sampler2D rsm_flux;

//screen samples between 0.0 and 1.0
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


void main()
{
	vec2 samp = imageLoad(samples, int(gl_LocalInvocationIndex)).xy;
	vec4 pos = texture2D(rsm_position, samp);
	vec4 nor = texture2D(rsm_normal, samp);
	vec4 col = texture2D(rsm_flux, samp);

	first_val_list[gl_LocalInvocationIndex] = plight(pos, nor, col);
}