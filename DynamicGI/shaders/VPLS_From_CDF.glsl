#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0) uniform sampler2D rsm_pos;
layout(binding = 1) uniform sampler2D rsm_normal;
layout(binding = 2) uniform sampler2D rsm_flux;

struct plight {
	vec4 p;
	vec4 n;
	vec4 c;
};

layout(std430, binding = 0) writeonly buffer light_buffer {
	plight list[];
};

layout(r32f, binding = 0) uniform image2D BRSM_CDF_TBO;
layout(r32f, binding = 1) uniform image1D BRSM_CDF_ROW_TBO;
layout(rg32f, binding = 2) uniform imageBuffer CDF_RAND_TBO; //cdf random numbers

uniform int num_vpls; //vpl budget, how many Im extracting from the brsm
uniform float vpl_radius; //radius of a vpl
uniform int dim; // dimension or resolution of a texture

//int upper_bound_bin_search(float val, int l, int r)
//{
//	while (l < r)
//	{
//		int m = l + (r - l) / 2;		
//		if (cdf[m] < val)
//		{
//			l = m + 1;
//		}
//		else
//		{
//			r = m;
//		}			
//	}
//	return l;	
//}

//TODO remove code dependency from 256 VPLs
void main(void)
{
	//number between [0 , 256*256)
	int idx = int(gl_GlobalInvocationID.x * 16 + gl_GlobalInvocationID.y); 
		
	//random number to sample CDFs
	vec2 st = imageLoad(CDF_RAND_TBO, idx).xy;	
	//to be assigned the random number to sample CDF after scaling	
	vec2 v;

	float row_p;
	float col_p;

	//upper bound bin search row in texel space [0, size) - will yield value in the variable l
	//scale random values by the maximum value present in cdf
	float max_row = imageLoad(BRSM_CDF_ROW_TBO, (dim - 1)).r;
	v.x = max_row * st.x;
	int l = 0; int r = dim - 1; int m;
	while (l < r)
	{
		int m = l + (r - l) / 2;
		float cdf_m = imageLoad(BRSM_CDF_ROW_TBO, m).r;
		if (cdf_m < v.x)
		{
			l = m + 1;
		}
		else
		{
			r = m;
		}
	}

	float pdf_row = (imageLoad(BRSM_CDF_ROW_TBO, l).r - imageLoad(BRSM_CDF_ROW_TBO, l-1).r) / max_row;
	
	//ivec2 to sample cols cdf
	ivec2 s_col_cdf; s_col_cdf.x = l;
	//scale random values by the maximum value present in cdf
	float max_col = imageLoad(BRSM_CDF_TBO, ivec2(l, dim - 1)).r;
	v.y = max_col * st.y;
	//upper bound bin search col in texel space [0, size) - will yield value in the variable l
	l = 0; r = dim - 1; 
	while (l < r)
	{
		int m = l + (r - l) / 2;
		s_col_cdf.y = m;
		float cdf_m = imageLoad(BRSM_CDF_TBO, s_col_cdf).r;
		if (cdf_m < v.y)
		{
			l = m + 1;
		}
		else
		{
			r = m;
		}
	}
	
	//s_col_cdf now holds column and row of chosen vpl
	s_col_cdf.y = l;

	float pdf_col = (imageLoad(BRSM_CDF_TBO, s_col_cdf).r - imageLoad(BRSM_CDF_TBO, ivec2(s_col_cdf.x, s_col_cdf.y-1)).r) / max_col;
	
	float pdf_ij = pdf_row * pdf_col;
	//to remove - old implementation
	//vec2 uv = vec2((float(ipos.x) + 0.5f)/float(dim), (float(ipos.y) + 0.5f)/float(dim));

	vec4 pos    = vec4(texelFetch(rsm_pos   , s_col_cdf, 0).xyz, pdf_ij);
	vec4 normal = vec4(texelFetch(rsm_normal, s_col_cdf, 0).xyz, 0.0);
	vec4 flux   = vec4(texelFetch(rsm_flux  , s_col_cdf, 0).xyz, 1.0);
	
	//sample vpl back and store
	plight vpl = plight(pos, normal, flux);
	list[idx] = vpl;

}