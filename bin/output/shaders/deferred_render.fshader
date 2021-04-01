#version 430

layout (location = 0) out vec3 gposition;
layout (location = 1) out vec3 gnormal;
layout (location = 2) out vec4 galbedo;

in VS_OUT{
	vec3 pos;
	vec3 normal;
	vec2 uv;
	mat3 TBN;
	vec3 tang;
	vec3 bitang;
} fs_in;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;
//uniform sampler2D texture_alphamask1;

uniform int useNormalMap;
uniform int useSpecMap;
//uniform int useAlphaMap;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

void main() {
	gposition = fs_in.pos;	
	galbedo.rgb = texture(texture_diffuse1, fs_in.uv).rgb;	//vec3(0.8, 0.8, 0.8);//
	if (0 == useSpecMap)
	{
		galbedo.a = 0.0;
	}
	else if (1==useSpecMap)
	{
		galbedo.a = texture(texture_specular1, fs_in.uv).r;
	}		
	vec3 normal;

	//if (0 == useNormalMap)
	//{
		gnormal = normalize(fs_in.normal);	
	//}
	//else if( 1 == useNormalMap)
	//{
	//	normal = texture(texture_normal1, fs_in.uv).rgb;
	//	normal = normalize(normal*2.0f - 1.0f);
	//	normal = normalize(fs_in.TBN * normal);
	//	gnormal = normal;
	//
	//	//gnormal = normalize(fs_in.normal);	
	//}
}
