#version 430
//dispatches num_VALs groups
#define NUM_VAL 2
layout(local_size_x = NUM_VAL) in;

uniform int num_VALs;
uniform int num_VPLs;

struct plight {
	vec4 p; //position + radius
	vec4 n;
	vec4 c; //color	
};

//sampled vpls size is num_VPLs
layout(std430, binding = 5) buffer light_buffer {
	plight list[];
};

//size is num_VALs + 1 , here we update only the first "num_VALs" lights
layout(std430, binding = 6) buffer val_buffer {
	plight val_list[];
};

//clusters(val) to generate
//size is (num_VALs) * num_VPLs + num_VALs
//the last num_VALs indexes save the number of light indices associated with the corresponding VAL
layout(std430, binding = 7) buffer vpl_index_buffer {
	unsigned int val_idx[];
};

void main()
{
	unsigned int size = val_idx[(num_VALs * num_VPLs) + gl_LocalInvocationIndex];
	unsigned int start_pos = gl_LocalInvocationIndex * num_VPLs;
	
	vec3 avg_pos =    vec3(0);
	vec3 avg_normal = vec3(0);
	vec3 avg_flux =   vec3(0);
	if (size > 0)
	{
		for (unsigned int i = start_pos; i < (start_pos + size); i++)
		{
			unsigned int j = val_idx[i];
			avg_pos += list[j].p.xyz;
			avg_normal += list[j].n.xyz;
			avg_flux += list[j].c.xyz;
		}
	
		avg_pos /= size;
		avg_normal = normalize(avg_normal);
		avg_flux /= size;
	
		val_list[gl_LocalInvocationIndex].p = vec4(avg_pos, 45.0f);
		val_list[gl_LocalInvocationIndex].n.xyz = avg_normal;
		val_list[gl_LocalInvocationIndex].c.xyz = avg_flux;
	}
}