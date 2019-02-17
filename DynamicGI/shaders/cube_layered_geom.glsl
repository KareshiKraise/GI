#version 430

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

in VS_out{
	vec4 p;
	vec3 n;
	vec2 uv;	
}gs_in[];


out GS_out{
	vec4 p;
	vec3 n;
	vec2 uv;
}gs_out;

const int faces = 6;

uniform mat4 M;
layout(std140, binding = 0) uniform Matrices
{
	mat4 V[6];
};

uniform mat4 P;

void main()
{
	for (int i = 0; i < faces; i++)
	{
		gl_Layer = i;

		for (int j = 0; j < 3; j++) {
			gs_out.p =  gs_in[j].p;
			gs_out.n =  gs_in[j].n;
			gs_out.uv = gs_in[j].uv;

			gl_Position = P * V[i] * gs_in[j].p;
			EmitVertex();
		}
		EndPrimitive();
	}
}