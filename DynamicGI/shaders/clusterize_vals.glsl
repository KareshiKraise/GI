#version 430

#define BASE_NUM_VPLS 16
#define TOTAL_VPLS (BASE_NUM_VPLS*BASE_NUM_VPLS)

layout(local_size_x = BASE_NUM_VPLS, local_size_y = BASE_NUM_VPLS) in;

uniform int num_vals;
uniform int vpl_samples;
uniform bool first_cluster_pass;

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

//vpls assigned to val
layout(std430, binding = 2) buffer vpls_per_val {
	unsigned int vpl_to_val[];
};

//num vpls_assigned to val
layout(std430, binding = 3) buffer count_vpl_per_val {
	unsigned int vpl_count[];
};



const float wx = 0.6;
const float wa = 0.4;

void main()
{
	
	//if (first_cluster_pass == true)
	//{
		if (gl_LocalInvocationIndex < num_vals)
		{
			vpl_count[gl_LocalInvocationIndex] = 0;	
			
		}
	//}
			
	barrier();

	
	plight current_light = vpl_list[gl_LocalInvocationIndex];

	float delta_d = length(first_val_list[0].p.xyz - current_light.p.xyz);
	float delta_a = dot(normalize(first_val_list[0].n.xyz), normalize(current_light.n.xyz));
	float dist = delta_d * wx + delta_a * wa;
	unsigned int closer_val_index = 0;
	for (int i = 1; i < num_vals; i++) {
		delta_d = length(first_val_list[i].p.xyz - current_light.p.xyz);
		delta_a = dot(normalize(first_val_list[i].n.xyz), normalize(current_light.n.xyz));
		float aux_dist = delta_d * wx + delta_a * wa;
		if (dist > aux_dist)
		{
			dist = aux_dist;
			closer_val_index = i;
		}
	}
	unsigned int idx = atomicAdd(vpl_count[closer_val_index], 1);
	unsigned int to_insert = (vpl_samples * closer_val_index) + idx;
	vpl_to_val[to_insert] = gl_LocalInvocationIndex;
	
}


