#version 430

out vec4 fragColor;
in vec2 tex;

uniform sampler2D screen_tex;
//uniform float near;
//uniform float far;

//float LinearizeDepth(float depth)
//{
//	float z = depth * 2.0 - 1.0; // Back to NDC 
//	return (2.0 * near * far) / (far + near - z * (far - near));
//}

void main() {
	float depthVal = texture(screen_tex, tex).b;
	fragColor = vec4(vec3(depthVal), 1.0f);
	//fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}







