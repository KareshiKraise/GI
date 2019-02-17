#version 430

//Lights are put in view space before being added to the buffer

layout(local_size_x = 1) in;

layout(binding = 0) uniform sampler2D rsm_position;
layout(binding = 1) uniform sampler2D rsm_flux;
layout(binding = 2) uniform sampler2D rsm_normal;

uniform mat4 V;

struct plight {
	vec4 p;
	vec4 n;
	vec4 c;	
};

layout(std430, binding = 5) writeonly buffer light_buffer {
	plight list[];
};

layout(rg32f, binding = 0) uniform imageBuffer samples;

#define RADIUS 30.0f

void main(void) {	
	//
	vec2 uv = imageLoad(samples, int(gl_GlobalInvocationID.x)).xy;

	vec4 pos = vec4(texture2D(rsm_position, uv).xyz, RADIUS);
	vec4 norm = vec4(texture2D(rsm_normal, uv).xyz, 0.0);
	vec4 col = vec4(texture2D(rsm_flux, uv).xyz, 1.0);
	
	plight val = plight(pos,norm ,col);
	list[gl_GlobalInvocationID.x] = val;
	
}