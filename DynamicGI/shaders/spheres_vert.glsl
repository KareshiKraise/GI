#version 430

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

out vec3 lpos;
out vec3 lnormal;
out vec3 lcol;

void main() {	
	vec4 p = M * vec4(Pos, 1.0); ;
	vec4 worldPos = vec4(p.xyz + texture2D(rsmposition, offset).xyz, 1.0 );
	lpos = worldPos.xyz;
	lnormal = texture2D(rsmnormal, offset).xyz;
	lcol = texture2D(rsmflux, offset).xyz;
	
	gl_Position = P * V * worldPos;	
}