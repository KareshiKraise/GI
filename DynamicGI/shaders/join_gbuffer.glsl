#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0) uniform sampler2D interleaved_image;

layout(rgba16f, binding = 0) uniform image2D imageBuffer;

uniform int Wid;
uniform int Hei;
uniform int n_rows;
uniform int n_cols;

void main()
{
	uint dim_x = Wid / n_cols;
	uint dim_y = Hei / n_rows;

	uint tile_id_x = gl_GlobalInvocationID.x % n_cols;
	uint tile_id_y = gl_GlobalInvocationID.y % n_rows;

	ivec2 id = ivec2(tile_id_x*dim_x + gl_GlobalInvocationID.x / n_cols, tile_id_y*dim_y + gl_GlobalInvocationID.y / n_rows);
	
	ivec2 uv = ivec2(gl_GlobalInvocationID.x,
		gl_GlobalInvocationID.y);	

	vec4 val = texelFetch(interleaved_image, id, 0);

	imageStore(imageBuffer, uv, val);

}






