#version 430

in vec2 tex;
out vec4 color;

const int blur_kernel = 4;

uniform sampler2D source;

void main() {
	vec2 texelSize = 1.0 / vec2(textureSize(source, 0));

	vec3 result = vec3(0.0f);
	int blurOffset = blur_kernel / 2;
	for (int x = -blurOffset; x < blurOffset; x++)
	{
		for (int y = -blurOffset; y < blurOffset; y++)
		{
			vec2 offset = vec2(float(x), float(y)) * texelSize;
			result += texture(source, tex + offset).rgb;
		}
	}

	color = vec4(result / (blur_kernel * blur_kernel), 1.0);
}