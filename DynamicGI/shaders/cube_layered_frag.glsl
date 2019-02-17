#version 430

layout(location = 0) out vec4 cube_position;
layout(location = 1) out vec3 cube_normal;
layout(location = 2) out vec4 cube_albedo;

uniform sampler2D texture_diffuse1;

in GS_out{
	vec4 p;
	vec3 n;
	vec2 uv;
}fs_in;



void main() {
	
	vec4 col = vec4(texture(texture_diffuse1, fs_in.uv));	
	cube_position = fs_in.p;
	cube_normal = fs_in.n;
	cube_albedo = col;
	
	

}