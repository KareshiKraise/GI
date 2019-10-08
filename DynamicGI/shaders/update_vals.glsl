#version 430

#define NUM_VALS 4

layout(local_size_x = NUM_VALS) in;

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

uniform int vpl_samples;

void main(){
	vec3 avg_p = vec3(0.0);
	vec3 avg_n = vec3(0.0);
	vec3 avg_c = vec3(0.0);

	unsigned int cluster_size = vpl_count[gl_LocalInvocationIndex];
	unsigned int index_to_vpl_array = gl_LocalInvocationIndex * vpl_samples;

	plight curr_light;
	for (int i = 0; i < cluster_size; i++)
	{
		unsigned int vpl_list_idx = vpl_to_val[index_to_vpl_array + i];
		curr_light = vpl_list[vpl_list_idx];

		avg_p += curr_light.p.xyz;
		avg_n += curr_light.n.xyz;
		avg_c += curr_light.c.xyz;
	}

	avg_p /= (cluster_size > 0 ? cluster_size : 1);
	avg_n = normalize(avg_n);
	avg_c /= (cluster_size > 0 ? cluster_size : 1);

	first_val_list[gl_LocalInvocationIndex] = plight(vec4(avg_p, 1.0), vec4(avg_n, 0.0), vec4(avg_c, 1.0));
}