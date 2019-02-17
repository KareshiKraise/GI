#version 430

//instanced draw vertex pass, scaling sphere vertices by  a factor of 15

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 Tex;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec2 offset;

uniform sampler2D rsmposition;
uniform sampler2D rsmnormal;
uniform sampler2D rsmflux;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

const float scale_factor = 30.0f;

void main() {

	vec4 p = M * vec4(Pos, 1.0) * scale_factor;
	vec4 worldPos = vec4(p.xyz + texture2D(rsmposition, offset).xyz, 1.0);
	gl_Position = P * V * worldPos;
}