#version 430

layout(binding = 0) uniform sampler2D lightingBuffer;
layout(binding = 1) uniform sampler2D edgeBuffer;

uniform float Wid;
uniform float Hei;
in vec2 tex;

//float offset[] = float[] (0.0, 1.0, 2.0, 3.0, 4.0, 5.0);
//float weight[] = float[] (0.382925, 0.24173, 0.060598, 0.005977, 0.000229, 0.000003 );

float offset[] = float[](0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
float weight[] = float[](0.13298, 0.125858, 0.106701, 0.081029, 0.055119, 0.033585, 0.018331, 0.008962, 0.003924);
#define K_SIZE 9
out vec4 color;

layout(rgba16f, binding = 0) uniform image2D imageBuffer;
layout(r32ui, binding = 1) uniform uimage2D sampBuffer;

void main()
{
	float edge = texture(edgeBuffer, vec2(gl_FragCoord) / vec2(Wid, Hei)).x;
	color = texture(lightingBuffer, vec2(gl_FragCoord) / vec2(Wid, Hei)) * weight[0];
	int num_samp = 1;
	float w = weight[0];
	
	if (edge == 0.0)
	{
		for (int i = 1; i < K_SIZE; ++i)
		{
			edge = texture(edgeBuffer, (vec2(gl_FragCoord) + vec2(0.0, offset[i])) / vec2(Wid, Hei)).x;
			if (edge == 1.0)
			{
				break;
			}
			else
			{
				num_samp += 1;
				color += texture(lightingBuffer, (vec2(gl_FragCoord) + vec2(0.0, offset[i])) / vec2(Wid, Hei)) * weight[i];
				w += weight[i];
			}
		}
	}
	for (int i = 1; i < K_SIZE; ++i)
	{
		edge = texture(edgeBuffer, (vec2(gl_FragCoord) - vec2(0.0, offset[i])) / vec2(Wid, Hei)).x;
		if (edge == 1.0)
		{
			break;
		}
		else
		{
			num_samp += 1;
			color += texture(lightingBuffer, (vec2(gl_FragCoord) - vec2(0.0, offset[i])) / vec2(Wid, Hei)) * weight[i];
			w += weight[i];
		}
		
	}
	
	imageStore(sampBuffer, ivec2(gl_FragCoord), ivec4(num_samp));
	imageStore(imageBuffer, ivec2(gl_FragCoord), color * (1.0/w));
}
