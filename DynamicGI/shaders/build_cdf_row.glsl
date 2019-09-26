#version 430

layout(local_size_x = 1) in;

//texture approach
//layout(r32f, binding = 0) uniform image2D BRSM_CDF_TBO;
//layout(r32f, binding = 1) uniform image1D BRSM_CDF_ROW_TBO;

//ssbo approach
layout(std430, binding = 0) readonly buffer CDF_COLS {
	float cdf_cols[][256];
};
layout(std430, binding = 1) writeonly buffer CDF_ROW {
	float cdf_row[];
};

uniform int size;

void main() {

	ivec2 load_pos = ivec2( 0 ,(size-1));
	int store_pos  = 0;

	//float cdf = imageLoad(BRSM_CDF_TBO, load_pos).x;
	//imageStore(BRSM_CDF_ROW_TBO, store_pos, vec4(cdf));

	float cdf = cdf_cols[store_pos][size - 1];
	cdf_row[store_pos] = cdf;

	for (int i = 1; i < size; i++)
	{
		//ssbo approach
		store_pos += 1;
		cdf += cdf_cols[store_pos][size - 1];
		cdf_row[store_pos] = cdf;

		//texture approach
		//load_pos.x += 1;
		//store_pos += 1;
		//cdf += imageLoad(BRSM_CDF_TBO, load_pos).x;
		//imageStore(BRSM_CDF_ROW_TBO, store_pos, vec4(cdf));
	}
}
