#version 430

layout(local_size_x = 1) in;

layout(binding = 0) uniform sampler2D rsm_position;
layout(binding = 1) uniform sampler2D rsm_flux;
layout(binding = 2) uniform sampler2D rsm_normal;

struct plight {
	vec4 p;
	vec4 n;
	vec4 c;	
};

layout(std430, binding = 5) writeonly buffer light_buffer {
	plight list[];
};

layout(rg32f, binding = 0) uniform imageBuffer samples;

uniform float vpl_radius;

void main(void) {	
			
	vec2 uv = imageLoad(samples, int(gl_GlobalInvocationID.x)).xy;

	vec4 pos = vec4(texture2D(rsm_position, uv).xyz, vpl_radius);
	vec4 norm = vec4(texture2D(rsm_normal, uv).xyz, 0.0);
	vec4 col = vec4(texture2D(rsm_flux, uv).xyz, 1.0);
	
	plight val = plight(pos,norm ,col);
	list[gl_GlobalInvocationID.x] = val;
	
}