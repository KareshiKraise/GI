#version 430

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 Tex;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec2 offset;

uniform sampler2D rsmposition;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

out vec3 pos;

void main() {
	vec3 p = Pos + texture2D(rsmposition, offset).xyz;
	vec4 worldPos = M * vec4(p , 1.0);
	pos = worldPos.xyz;
	gl_Position = P * V * worldPos;
	
}