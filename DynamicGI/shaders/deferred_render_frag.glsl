#version 430

layout (location = 0) out vec3 gposition;
layout (location = 1) out vec3 gnormal;
layout (location = 2) out vec4 galbedo;

in VS_OUT{
	vec3 pos;
	vec3 normal;
	vec2 tex;	
} fs_in;

uniform sampler2D texture_diffuse1;

void main() {
	gposition = fs_in.pos;
	gnormal = normalize(fs_in.normal);
	galbedo = texture(texture_diffuse1, fs_in.tex);
}