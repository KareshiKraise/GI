#version 430

//entrada do vertex sempre em caps
layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 Tex;
layout(location = 2) in vec3 Normal;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

//saidas sempre em minusculo
out vec3 pos;
out vec2 tex;
out vec3 normal;

void main() {
	vec4 worldPos = M * vec4(Pos, 1.0);
	pos = worldPos.xyz;
	normal = Normal;
	tex = Tex;
	gl_Position = P * V * worldPos;
}

