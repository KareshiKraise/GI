#version 430

layout(local_size_x = 1) in;

layout(r32f, binding = 0) uniform image2D BRSM_CDF_TBO;
layout(r32f, binding = 1) uniform image1D BRSM_CDF_ROW_TBO;

uniform int size;

void main() {

	ivec2 load_pos = ivec2( 0 ,(size-1));
	int store_pos  = 0;

	float cdf = imageLoad(BRSM_CDF_TBO, load_pos).x;
	imageStore(BRSM_CDF_ROW_TBO, store_pos, vec4(cdf));
	for (int i = 1; i < size; i++)
	{
		load_pos.x += 1;
		store_pos += 1;
		cdf += imageLoad(BRSM_CDF_TBO, load_pos).x;
		imageStore(BRSM_CDF_ROW_TBO, store_pos, vec4(cdf));
	}
}
