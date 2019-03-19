#version 430

struct plight {
	vec4 p;
	vec4 n;
	vec4 c;
};

layout(std430, binding = 0) buffer backface_vpls {
	plight back_vpl_list[];
};

layout(std430, binding = 1) buffer backface_vpl_count {
	unsigned int back_vpl_count;
};

//layout(std430, binding = 2) buffer pass_vpl_count {
//	unsigned int pass_count;
//};

in vec4 p;
in vec4 n;
in vec4 c;

void main()
{
	uint index = atomicAdd(back_vpl_count, 1);
	back_vpl_list[index] = plight(p,n,c);
}