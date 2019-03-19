#version 430

layout(local_size_x = 1) in;

layout(std430, binding = 4) buffer num_light {
	int nlights;
};

struct plight {
	vec4 p;
	vec4 n;
	vec4 c;
};

layout(std430, binding = 5) buffer light_buffer {
	plight list[];
};

//size is num_VALs
layout(std430, binding = 6) buffer val_buffer {
	plight val_list[];
};

uniform int num_vpls;
uniform int num_vals;

void main() {

	
	int size = nlights - num_vpls;

	
	vec3 avg_pos	 = vec3(0);
	vec3 avg_normal  = vec3(0);
	vec3 avg_flux	 = vec3(0);

	for (int i = num_vpls; i < nlights; i++)
	{
		plight curr = list[i];
		avg_pos		+= curr.p.xyz;
		avg_normal	+= curr.n.xyz;
		avg_flux	+= curr.c.rgb;
	}

	avg_pos /= size;
	avg_normal = normalize(avg_normal);
	avg_flux /= size;

	val_list[num_vals] = plight(vec4(avg_pos, 20.0f), vec4(avg_normal, 0.0), vec4(avg_flux, 1.0));

}