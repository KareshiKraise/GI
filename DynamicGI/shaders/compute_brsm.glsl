#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0) uniform sampler2D gbuffer_pos;
layout(binding = 1) uniform sampler2D gbuffer_normal;
layout(binding = 2) uniform sampler2D gbuffer_albedo;

layout(binding = 3) uniform sampler2D rsm_pos;
layout(binding = 4) uniform sampler2D rsm_normal;
layout(binding = 5) uniform sampler2D rsm_flux;

layout(std430, binding = 0) readonly buffer viewportSamples {
	vec2 samples[];
};

layout(r32f, binding = 0) uniform image2D BRSM_TBO;

uniform int num_samples;
uniform int dim;

//TODO - MOVE TO 2D CDF
void main(void) {

	//flatten index into brsm list
	int idx = int(gl_GlobalInvocationID.x * dim + gl_GlobalInvocationID.y);
	
	//compute average
	float avg = 0.f;
	ivec2 p = ivec2(gl_GlobalInvocationID.xy);
	vec2 uv = vec2((float(p.x) + 0.5f) / float(dim), (float(p.y) + 0.5f) / float(dim));
	
	vec3 vpl_pos    = texture2D(rsm_pos, uv).xyz;
	vec3 vpl_normal = texture2D(rsm_normal, uv).xyz;
	vec3 vpl_col    = texture2D(rsm_flux, uv).rgb;
	//float vpl_flux = max(vpl_col.x, max(vpl_col.y, vpl_col.z));
	float vpl_flux = 1.0f;

	for (int i = 0; i < num_samples; ++i)
	{		
		vec3 scrn_pos = texture2D(gbuffer_pos   , samples[i]).xyz;
		vec3 scrn_n   = texture2D(gbuffer_normal, samples[i]).xyz;
		//vec3 scrn_col = texture2D(gbuffer_albedo, samples[i]).xyz;
		
		vec3 val_to_vs = normalize(vpl_pos  - scrn_pos);
		vec3 vs_to_val = normalize(scrn_pos - vpl_pos);
		float dist = pow(length(vpl_pos-scrn_pos), 4);
		if (dist < 1.0f)
		{
			dist = 1.0f;
		}			
				
		avg += vpl_flux * max(dot(vpl_normal, vs_to_val), 0.0f) * max(dot(scrn_n, val_to_vs), 0.0f) / dist;
	}
	avg /= num_samples;

	//add to brsm
	vec4 data = vec4(avg);
	//debug
	//vec4 data = vec4(1.0,0.0,0.0,1.0);
	imageStore(BRSM_TBO, ivec2(gl_GlobalInvocationID.xy), data);
}

