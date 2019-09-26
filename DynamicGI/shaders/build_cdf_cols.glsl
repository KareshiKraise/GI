#version 430

layout(local_size_x = 16) in;

//layout(binding = 0) uniform sampler2D BRSM;
//layout(r32f, binding = 0) uniform image2D BRSM_CDF_TBO;

layout(std430, binding = 0) readonly buffer BRSM {
	float brsm_map[][256];
};

layout(std430, binding = 1) writeonly buffer CDF_COLS {
	float cdf_cols[][256];
};

uniform int size;

void main() 
{
	float uvx = float(gl_GlobalInvocationID.x);
	float res = float(size);
	float step = 1.0f / 256.f;
		
	vec2 uv = vec2((uvx + 0.5f)/res, (0.5f)/res);
	ivec2 tf = ivec2(gl_GlobalInvocationID.x, 0);

	ivec2 store_pos = ivec2(gl_GlobalInvocationID.x, 0);
	
	//texture approach
	//float cdf = texelFetch(BRSM, tf, 0).r;	
	//imageStore(BRSM_CDF_TBO, store_pos, vec4(cdf));

	//ssbo approach
	float cdf = brsm_map[gl_GlobalInvocationID.x][0];
	cdf_cols[gl_GlobalInvocationID.x][0] = cdf;
	int yind = 0;

	for (int i = 1; i < size; ++i )
	{
		yind += 1;
		//ssbo approach
		cdf += brsm_map[gl_GlobalInvocationID.x][yind];
		cdf_cols[gl_GlobalInvocationID.x][yind] = cdf;

		//store_pos.y += 1;		
		//tf.y += 1;
		//texture approach
		//cdf += texelFetch(BRSM, tf, 0).r;		
		//imageStore(BRSM_CDF_TBO, store_pos, vec4(cdf));

	}
}