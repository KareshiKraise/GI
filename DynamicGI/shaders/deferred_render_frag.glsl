#version 430

layout (location = 0) out vec3 gposition;
layout (location = 1) out vec3 gnormal;
layout (location = 2) out vec4 galbedo;

in VS_OUT{
	vec3 pos;
	vec3 n;
	vec2 tex;
	mat3 TBN;
	vec3 t;
	vec3 b;
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

vec3 bump_normal_derivative()
{
	vec3 bump = normalize(texture(texture_normal1, fs_in.tex).xyz);
    bump = 2.0f * bump - 1.0f;		

	vec3 q1 = dFdx(fs_in.pos);
	vec3 q2 = dFdy(fs_in.pos);
	vec2 st1 = dFdx(fs_in.tex);
	vec2 st2 = dFdy(fs_in.tex);

	vec3 N = normalize(fs_in.n);

	vec3 dp2perp = cross(q2, N);
	vec3 dp1perp = cross(N, q1);
		
	vec3 T = dp2perp * st1.x + dp1perp * st2.x;
	vec3 B = dp2perp * st1.y + dp1perp * st2.y;

	//vec3 T = normalize(q1*st2.t - q2*st1.t);
	//T = normalize(T - dot(T, N) * N);
	//vec3 B = normalize(cross(N, T));
	//vec3 B = normalize(-q1*st2.s - q2 * st1.s);
	//mat3 tbn = mat3(T, B, N);

	float invmax = inversesqrt(max(dot(T,T), dot(B,B)));
	mat3 tbn = mat3(T*invmax, B*invmax, N);

	vec3 output_normal =  normalize(tbn * bump);
	return output_normal;
}

vec3 bump_normal()
{
	vec3 bump = normalize(texture(texture_normal1, fs_in.tex).xyz);
	bump = 2.0f * bump - 1.0f;
	vec3 output_normal = normalize(fs_in.TBN * bump);
	return output_normal;
}

void main() {

	gposition = fs_in.pos;	
	galbedo.rgb = texture(texture_diffuse1, fs_in.tex).rgb;	
	
	//if (0 == useNormalMap)
	//{
	//	gnormal = normalize(fs_in.n);
	//}
	//else if (1 == useNormalMap )
	//{
		gnormal = normalize(V * vec4(bump_normal_derivative(), 0.0)).xyz;
	//}	    

	if (0 == useSpecMap)
	{
		galbedo.a = 1.0;
	}
	else if (1== useSpecMap)
	{
		galbedo.a = texture(texture_specular1, fs_in.tex).r;
	}
	
}
