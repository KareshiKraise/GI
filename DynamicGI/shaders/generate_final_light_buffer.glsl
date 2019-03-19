#version 430

#define NUM_VAL 4
layout(local_size_x = NUM_VAL) in;

uniform int num_vpls;

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

//vpls indexes assigned to val
layout(std430, binding = 2) buffer vpls_per_val {
	unsigned int vpl_to_val[];
};

//num vpls_assigned to val
layout(std430, binding = 3) buffer count_vpl_per_val {
	unsigned int vpl_count[];
};



void main() {
	
	uint size = vpl_count[gl_LocalInvocationIndex];
	for (uint i = 0; i < size; i++)
	{
		uint into_array = gl_LocalInvocationIndex * num_vpls + i;
		uint id = vpl_to_val[into_array];
		vpl_list[id].n.w = gl_LocalInvocationIndex;
		//unsigned int val_idx = gl_LocalInvocationIndex;
		//indexed_vpl[id] = indexed_point_light(curr.p, curr.n, curr.c, val_idx);
	}
}