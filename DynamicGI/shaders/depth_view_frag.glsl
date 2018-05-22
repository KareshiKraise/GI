#version 430

//debug view of shadow map, to be used with a fullscreen quad

out vec4 fragColor;
in vec2 tex;

uniform sampler2D screen_tex;
uniform float near;
uniform float far;

float LinearizeDepth(float depth)
{
	float z = depth * 2.0 - 1.0; // Back to NDC 	
	return (2.0 * near * far) / (far + near - z * (far - near));
}

void main() {
	float depthVal = LinearizeDepth(texture(screen_tex, tex).r);
	fragColor = vec4(vec3(depthVal), 1.0f);
	//fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}







