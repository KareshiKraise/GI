#version 430

layout(binding = 0) uniform sampler2D texImage;
layout(binding = 1) uniform sampler2D depthImage;

in vec2 tex;

uniform float near;
uniform float far;

out vec4 color;


float LinearizeDepth(float depth)
{
	float z = depth * 2.0 - 1.0; // Back to NDC 		
	return (2.0 * near) / (far + near - z * (far - near));
}

//float LinearizeDepth(float depth)
//{
//	float z_ndc = (depth - 0.5)  * 2; // Back to NDC
//	float z_eye = 1.0 / ((z_ndc *(far - near) - far - near) * far * near * 0.5);	
//	return z_eye;
//}


void main(){
	
	vec3 d = vec3(texture(texImage, tex).rgb);
	color = vec4(d, 1.0);
	

	//see depth
	//float z = texture(depthImage, tex).r;
	//z = LinearizeDepth(z);
	//vec3 d = vec3(z);
	//color = vec4(d, 1.0f);
}