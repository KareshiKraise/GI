#version 430

layout(binding = 0) uniform sampler2DArray texture1;

in vec2 tex;
out vec4 color;

uniform int layer;

void main(){	
	
	vec3 c = texture(texture1, vec3(tex, layer)).rrr;		
	color = vec4(c, 1.0f);
}