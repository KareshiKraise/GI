#version 430

layout(location = 0 ) uniform sampler2D rsmposition;
layout(location = 1 ) uniform sampler2D rsmnormal;
layout(location = 2 ) uniform sampler2D rsmflux;

out vec3 lpos;
out vec3 lnormal;
out vec3 lcol;

out vec4 col;

void main() {
	col = vec4(1.0, 0.0, 0.0, 1.0);
}