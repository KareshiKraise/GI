#version 430

#define NUM_CLUSTERS 4

layout(triangles) in;
layout(triangle_strip, max_vertices = 12) out;

uniform mat4 M;

layout(std140, binding = 0) uniform vpl_matrices{
	mat4 parabolic_mats[NUM_CLUSTERS];
};

uniform float ism_near;
uniform float ism_far;

in VS_OUT{
	vec3 pos;
	vec3 norm;
	vec3 col;
}gs_in[];

out GS_OUT{

	vec3 p;
	vec3 c;
	vec3 n;
	float dval;
	float clipDepth;

}gs_out;


void main()
{
	for (int i = 0; i < NUM_CLUSTERS; i++)
	{
		gl_Layer = i;		

		for (int j = 0; j < 3; j++)
		{
			vec4 p = vec4(gs_in[j].pos, 1.0);
			p = parabolic_mats[i] * M * p;
			p /= p.w;

			float L = length(p.xyz);
			p /= L;

			gs_out.clipDepth = p.z;

			p.x = p.x / (p.z + 1.0);
			p.y = p.y / (p.z + 1.0);

			p.z = (L - ism_near) / (ism_far - ism_near);
			p.w = 1.0;

			gl_Position = p;

			gs_out.dval = p.z;

			vec4 aux = M * vec4(gs_in[j].pos, 1.0);
			gs_out.p = aux.xyz;
			gs_out.n = gs_in[j].norm;
			gs_out.c = gs_in[j].col;			

			EmitVertex();
		}
		EndPrimitive();
	}
}