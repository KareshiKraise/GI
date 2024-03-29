#version 430

layout(location = 0 ) uniform sampler2D rsmposition;
layout(location = 1 ) uniform sampler2D rsmnormal;
layout(location = 2 ) uniform sampler2D rsmflux;

layout(location = 3) uniform sampler2D gposition;
layout(location = 4) uniform sampler2D gnormal;
layout(location = 5) uniform sampler2D galbedo;

in struct Light {
	vec3 lpos;
	vec3 lnormal;
	vec3 lflux;
} sphere;

uniform vec2 Coords;

uniform mat4 V;

out vec4 col;

vec2 get_uv_coord() {
	float u = gl_FragCoord.x / Coords.x;
	float v = gl_FragCoord.y / Coords.y;
	return vec2(u, v);
}

void main() {
	
	vec2 uv = get_uv_coord();
	
	vec3 ppos = texture2D(gposition, uv).xyz;             //point position
	vec3 pnormal = normalize(texture2D(gnormal, uv).xyz); //point normal
	vec3 pcolor = texture2D(galbedo, uv).xyz;             //point color	
	
	vec3 pinview = (V * vec4(ppos, 1.0)).xyz;
	vec3 linview = (V * vec4(sphere.lpos, 1.0)).xyz;

	vec3 lightDir = normalize(linview - pinview);

	float diff = max(0, dot(pnormal, lightDir));

	float len = length(pinview - linview);

	vec3 c = pcolor * sphere.lflux * diff * (1.0f / max(1.0f, len));

	col = vec4(c, 1.0);
	
}