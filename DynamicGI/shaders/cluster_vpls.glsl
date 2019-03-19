#version 430

//dispatches 1 group of 8x8 = 64 (num_VPLs) threads

layout(local_size_x = 8, local_size_y = 8) in;

layout(binding = 0) uniform sampler2D rsm_position;
layout(binding = 1) uniform sampler2D rsm_normal;
layout(binding = 2) uniform sampler2D rsm_flux;

uniform int num_VALs;
uniform int num_VPLs;
uniform bool start_frame;

float wx = 0.7; //euclidean distance weight
float wa = 0.3; //angle diff weight

struct plight {
	vec4 p; //position + radius
	vec4 n;
	vec4 c; //color	
};

//sampled vpls size is num_VPLs
layout(std430, binding = 5) buffer light_buffer {
	plight list[];
};

layout(std430, binding = 6) buffer val_buffer {
	plight val_list[];
};

//clusters(val) to generate
//size is (num_VALs) * num_VPLs + num_VALs
//the last num_VALs indexes save the number of light indices associated with the corresponding VAL
layout(std430, binding = 7) buffer vpl_index_buffer {
	unsigned int val_idx[];
};

//uniform buffer
//size is num_VALs
layout(std140, binding = 8) uniform k_samples
{
	vec2 k_vals[2];
};

void main()
{
	//fill vals 
	if (start_frame == true)
	{
		if (gl_LocalInvocationIndex == 0)
		{
			for (int i = 0; i < num_VALs; i++)
			{


				//last num_VALs indexes are zeroed -- setting the light count per val to zero
				val_idx[(num_VALs * num_VPLs) + i] = 0;
				//vec4 p1 = texture2D(rsm_position, k_vals[i]);
				//vec4 n1 = texture2D(rsm_normal, k_vals[i]);
				//vec4 c1 = texture2D(rsm_flux, k_vals[i]);		
				//p1.w = 45.f;
				//val_list[i] = plight(p1, n1, c1);
				int idx = i * num_VPLs / num_VALs;
				plight aux = list[idx];
				aux.p.w = 45.f;
				val_list[i] = aux;


			}
		}
	}
	
	barrier();

	//kmeans
	plight current_light = list[gl_LocalInvocationIndex];

	float delta_d = length(val_list[0].p.xyz - current_light.p.xyz);
	float delta_a = dot(normalize(val_list[0].n.xyz), normalize(current_light.n.xyz));
	float dist = delta_d * wx + delta_a * wa;
	unsigned int closer_val_index = 0;
	
	for (unsigned int h = 1; h < num_VALs; h++)
	{
		delta_d = length(val_list[h].p.xyz - current_light.p.xyz);
		delta_a = dot(normalize(val_list[h].n.xyz), normalize(current_light.n.xyz));
		float aux = delta_d * wx + delta_a * wa; 

		if (dist > aux)
		{
			dist = aux;
			closer_val_index = h;			
		}
	}

	unsigned int index = (num_VALs * num_VPLs) + closer_val_index;
	
	unsigned int to_insert = atomicAdd(val_idx[index], 1);
	val_idx[to_insert] = gl_LocalInvocationIndex;
	barrier();
}