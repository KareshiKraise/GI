#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0) uniform sampler2D gnormal;
layout(binding = 1) uniform sampler2D gdepth;

layout(rgba8, binding = 0) uniform image2D imageBuffer;

/*-edge detection params*/
float n_thres = 25;   //0.15
float z_thres = 0.013;  //0.013

#define RAD_TO_DEGREE 57.295779513

uniform int Wid;
uniform int Hei;

void main()
{
	ivec2 ipos = ivec2(gl_GlobalInvocationID.xy);
	ivec2 uv;
	
	vec3 n  = normalize(texelFetch(gnormal, ipos, 0 ).xyz);
	float z = texelFetch(gdepth,  ipos, 0 ).r;
	vec4 packed_nz = vec4(n, z);
	
	vec4 neighbours[3];

	//x+1, y
	uv = ivec2(ipos.x + 1, ipos.y);
	n = normalize(texelFetch(gnormal, uv, 0).xyz);
	z = texelFetch(gdepth,  uv, 0).r;
	neighbours[0] = vec4(n , z);
	//x+1, y+1
	uv = ivec2(ipos.x + 1, ipos.y + 1);
	n = normalize(texelFetch(gnormal, uv, 0).xyz);
	z = texelFetch(gdepth, uv, 0).r;
	neighbours[1] = vec4(n, z);
	//x, y+1
	uv = ivec2(ipos.x, ipos.y + 1);
	n = normalize(texelFetch(gnormal, uv, 0).xyz);
	z = texelFetch(gdepth, uv, 0).r;
	neighbours[2] = vec4(n, z);

	// 0 - continuity
	// 1 - discontinuity
	int edge = 0;
	float deltan;
	float deltaz;
	for (int i = 0; i < 3; i++)
	{
		deltan = acos(dot(packed_nz.xyz, neighbours[i].xyz)) * RAD_TO_DEGREE;
		if (deltan > n_thres)
		{
			edge = 1;
			break;
		}		
	}
	vec4 val = vec4(edge);
	imageStore(imageBuffer, ipos, val);		
}