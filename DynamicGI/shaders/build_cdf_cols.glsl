#version 430

layout(local_size_x = 16) in;

layout(binding = 0) uniform sampler2D BRSM;

layout(r32f, binding = 0) uniform image2D BRSM_CDF_TBO;

uniform int size;

void main() 
{
	float uvx = float(gl_GlobalInvocationID.x);
	float res = float(size);
	float step = 1.0f / 256.f;
		
	vec2 uv = vec2((uvx + 0.5f)/res, (0.5f)/res);
	ivec2 tf = ivec2(gl_GlobalInvocationID.x, 0);

	ivec2 store_pos = ivec2(gl_GlobalInvocationID.x, 0);
	
	float cdf = texelFetch(BRSM, tf, 0).r;
	//float cdf = texture2D(BRSM, uv).r;
	imageStore(BRSM_CDF_TBO, store_pos, vec4(cdf));
	
	for (int i = 1; i < size; ++i )
	{
		store_pos.y += 1;
		//uv.y += step;
		tf.y += 1;
		cdf += texelFetch(BRSM, tf, 0).r;
		//cdf += texture2D(BRSM, uv).x;
		imageStore(BRSM_CDF_TBO, store_pos, vec4(cdf));
	}
}