#version 430

layout(triangles) in;
layout(triangle_strip, max_vertices = 6) out;

in vec2 tex[];

out vec2 Tex;

uniform int LAYERS;


void main() {

	for (int l = 0; l < LAYERS; l++) {
		gl_Layer = l;
		for (int i = 0; i < 3; i++) {
			Tex = tex[i];

			gl_Position = gl_in[i].gl_Position;
			EmitVertex();
		}
		EndPrimitive();
	}
}