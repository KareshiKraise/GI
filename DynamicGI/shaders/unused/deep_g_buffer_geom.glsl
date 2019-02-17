#version 430

layout(triangles) in;
layout(triangle_strip, max_vertices = 6) out;

in VS_OUT{
	vec3 pos;
	vec3 normal;
	vec2 tex;
	vec4 clipSpace;
}gs_in[];

out GS_OUT{
	vec3 pos;
	vec3 normal;
	vec2 tex;
	vec4 clipSpace;
}gs_out;

uniform int LAYERS;

void main() {
	for (int l = 0; l < LAYERS; l++) {
		gl_Layer = l;
		for (int i = 0; i < 3; i++) {
			gs_out.pos =    gs_in[i].pos;
			gs_out.tex =    gs_in[i].tex;
			gs_out.normal = gs_in[i].normal;
			gs_out.clipSpace = gs_in[i].clipSpace;

			gl_Position = gl_in[i].gl_Position;
			EmitVertex();
		}
		EndPrimitive();
	}



}
