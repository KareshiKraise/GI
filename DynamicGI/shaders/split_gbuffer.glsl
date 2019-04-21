#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0) uniform sampler2D gposition;
layout(binding = 1) uniform sampler2D gnormal;
layout(binding = 2) uniform sampler2D gcol;
layout(binding = 3) uniform sampler2D gdepth;

layout(rgba16f, binding = 0) uniform image2D posBuffer;
layout(rgba16f, binding = 1) uniform image2D normalBuffer;
layout(rgba8,  binding = 2)  uniform image2D colBuffer;
layout(r32f ,  binding = 3)  uniform image2D depthBuffer;

uniform int Wid;
uniform int Hei;
uniform int n_rows;
uniform int n_cols;

void main()
{
	//uint nncol = 2;
	//uint nnrow = 2;
	//
	//uint dim_x = Wid / n_cols;//640 if 1280
	//uint dim_y = Hei / n_rows;//360 if 720
	//
	//uint posx = gl_GlobalInvocationID.x / dim_x;
	//uint posy = gl_GlobalInvocationID.y / dim_y;
	//
	//uint tid_x = gl_GlobalInvocationID.x % n_cols;
	//uint tid_y = gl_GlobalInvocationID.y % n_rows;
	//
	//uint offsetx = dim_x / nncol;
	//uint offsety = dim_y / nnrow;
	//
	//////unchanged
	//vec2 uv = vec2((float(gl_GlobalInvocationID.x) + 0.5f)/Wid,
	//	           (float(gl_GlobalInvocationID.y) + 0.5f)/Hei	   
	//);
	//
	//vec4  p = texture(gposition, uv);
	//vec4  n = texture(gnormal,   uv);
	//vec4  c = texture(gcol,      uv);
	//vec4  z = texture(gdepth,    uv);
	//
	//uint px = gl_GlobalInvocationID.x;
	//uint py = gl_GlobalInvocationID.y;
	//
	//if (gl_GlobalInvocationID.x >= dim_x)
	//{
	//	px = gl_GlobalInvocationID.x - dim_x;		
	//}	
	//
	//if (gl_GlobalInvocationID.y >= dim_y)
	//{
	//	py = gl_GlobalInvocationID.y - dim_y;
	//}
	//
	//
	////unchanged
	//ivec2 id = ivec2(posx*dim_x + tid_x * (dim_x/nncol) + (px)/n_cols ,
	//	             posy*dim_y + tid_y * (dim_y/nnrow) + (py)/n_rows 
	//);
	//
	//imageStore(posBuffer   , id, p);
	//imageStore(normalBuffer, id, n);
	//imageStore(colBuffer   , id, c);
	//imageStore(depthBuffer , id, z);

	//step 2
	uint dim_x = Wid / n_cols;
	uint dim_y = Hei / n_rows;
	
	uint tile_id_x = gl_GlobalInvocationID.x % n_cols;
	uint tile_id_y = gl_GlobalInvocationID.y % n_rows;
		
	ivec2 id = ivec2(tile_id_x*dim_x + gl_GlobalInvocationID.x/n_cols, tile_id_y*dim_y + gl_GlobalInvocationID.y/n_rows);
	
	vec2 uv = vec2((float(gl_GlobalInvocationID.x)+0.5f)/float(Wid),
		           (float(gl_GlobalInvocationID.y)+0.5f)/float(Hei)
	              );
	
	//vec4  p = texelFetch(gposition, ivec2(gl_GlobalInvocationID.xy), 0);
	//vec4  n = texelFetch(gnormal,   ivec2(gl_GlobalInvocationID.xy), 0);
	//vec4  c = texelFetch(gcol,      ivec2(gl_GlobalInvocationID.xy), 0);
    //vec4  z = texelFetch(gdepth,    ivec2(gl_GlobalInvocationID.xy), 0);

	vec4  p =texture(gposition, uv);
	vec4  n =texture(gnormal,   uv);
	vec4  c =texture(gcol,      uv);
	vec4  z =texture(gdepth,    uv);
	
	imageStore(posBuffer   , id, p);
	imageStore(normalBuffer, id, n);
	imageStore(colBuffer   , id, c);
	imageStore(depthBuffer , id, z);

		
}