#version 430

layout(local_size_x = 1) in;

uniform sampler2D rsm_position;
uniform sampler2D rsm_flux;

struct plight {
	vec4 p;
	vec4 c;
	float rad;
};

layout(std430, binding = 5) writeonly buffer light_buffer {
	plight list[];
};

layout(rg32f, binding = 0) uniform imageBuffer samples;

void main(void) {	
	//
	vec2 uv = imageLoad(samples, int(gl_GlobalInvocationID.x)).xy;
	vec4 pos = texture2D(rsm_position, uv);
	vec4 col = texture2D(rsm_flux, uv);
	float r = 15.0f;
	
	plight val = plight(pos, col, r);
	list[gl_GlobalInvocationID.x] = val;

	//list[gl_GlobalInvocationID.x].p = pos;
	//list[gl_GlobalInvocationID.x].c = col;
	//list[gl_GlobalInvocationID.x].rad = r;
}