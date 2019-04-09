#version 430

layout(location = 0) uniform sampler2DArray val_pos;
layout(location = 1) uniform sampler2DArray val_normal;
layout(location = 2) uniform sampler2DArray val_color;
layout(location = 3) uniform sampler2DArray val_depth;

in vec2 tex;
out vec4 fragColor;

uniform float near;
uniform float far;
uniform int layer;

void main()
{
	float d = texture(val_depth, vec3(tex, layer)).r;
	vec4 c = vec4(d, d, d, 1.0);
		
	fragColor = c;
}

