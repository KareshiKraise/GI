#version 430

uniform sampler2D gposition;
uniform sampler2D gnormal;
uniform sampler2D galbedo;

out vec4 frag_color;

in vec2 tex;

void main()
{

	vec3 albedo = texture2D(galbedo, tex).xyz;
	frag_color = vec4(albedo, 1.0);
}