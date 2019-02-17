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

out struct Light {
	vec3 lpos;
	vec3 lnormal;
	vec3 lflux;
} sphere;

const float scale_factor = 30.0f;

void main() {	

	vec4 vpl_pos = texture2D(rsmposition, offset);
	vec4 p = M * vec4(Pos, 1.0) * scale_factor;
	vec4 worldPos = vec4(p.xyz + vpl_pos.xyz, 1.0 );
	sphere.lpos = vpl_pos.xyz;
	//sphere.lpos = worldPos.xyz ;
	sphere.lnormal = texture2D(rsmnormal, offset).xyz;
	sphere.lflux = texture2D(rsmflux, offset).xyz;
	
	gl_Position = P * V * worldPos;	
}