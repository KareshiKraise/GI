#version 430

//vertical gaussian blur
layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0) uniform sampler2D colorBuffer;
layout(binding = 1) uniform sampler2D edgeBuffer;

float offset[3] = float[](0.0, 1.3846153846, 3.2307692308);
float weight[3] = float[](0.2270270270, 0.3162162162, 0.0702702703);

//float offset[2] = float[](0.0, 1.2);
//float weight[2] = float[](0.375, 0.3125);

layout(rgba16f, binding = 0) uniform image2D imageBuffer;

uniform float Wid;
uniform float Hei;

//OBS ACESSO AO DISCONTINUITY BUFFER TAMBEM USA LINEAR FILTERING
void main()
{
	//Screen Dimensions
	vec2 dims = vec2(Wid, Hei);
	//Current fragment coordinate centered 
	vec2 fragCoord = vec2(float(gl_GlobalInvocationID.x) + 0.5f, float(gl_GlobalInvocationID.y) + 0.5f);
	fragCoord = fragCoord / dims;

	vec4 fragColor = texture(colorBuffer, fragCoord) * weight[0];
	//accumulator for texel weights, for normalization
	float weight_accum = weight[0];
	//check pixel discontinuity
	float edge = texture(edgeBuffer, fragCoord).r;

	vec2 uv;
	
	if (edge == 0)
	{
		//blur in positive direction
		for (int i = 1; i < 3; i++)
		{
			uv = vec2(fragCoord.x, fragCoord.y + offset[i]);
			edge = texture(edgeBuffer, uv).r;
			weight_accum += weight[i];
			if (edge > 0)
				break;

			fragColor += texture(colorBuffer, uv) * weight[i];
			//weight_accum += weight[i];
		}
	}
	//blur in negative direction
	for (int i = 1; i < 3; i++)
	{
		uv = vec2(fragCoord.x, fragCoord.y - offset[i]);
		edge = texture(edgeBuffer, uv).r;
		weight_accum += weight[i];
		if (edge > 0)
			break;

		fragColor += texture(colorBuffer, uv) * weight[i];
		//weight_accum += weight[i];
	}

	vec4 col = fragColor * (1.0 / weight_accum);
	imageStore(imageBuffer, ivec2(gl_GlobalInvocationID.xy), vec4(col.rgb, 1.0));


}