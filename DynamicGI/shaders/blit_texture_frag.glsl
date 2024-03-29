#version 430

layout(binding = 0) uniform sampler2D lightingBuffer;
layout(binding = 1) uniform sampler2D texAlbedo;
layout(binding = 2) uniform sampler2D depthImage;


in vec2 tex;

uniform float Wid;
uniform float Hei;
uniform float near;
uniform float far;
uniform int cfactor;

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
	
	//albedo for color correction
	vec3 albedo = texture(texAlbedo, tex).rgb;
	//spec
	float spec =  texture(texAlbedo, tex).a;
	
	//denoised lighting buffer
	vec3 d = texture(lightingBuffer, tex).rgb;
	
	d = d * albedo;
	const float gamma = 2.2f;	
	//vec3 mapped = d / (d + vec3(1.0f));	
	vec3 mapped = vec3(1.0f) - exp(-d * 3.f);
	mapped = pow(mapped, vec3(1.0f/gamma));

	color = vec4(mapped, 1.0);
	
	gl_FragDepth = texture(depthImage, tex).r;

	//see depth
	//float z = texture(depthImage, tex).r;
	//z = LinearizeDepth(z);
	//vec3 d = vec3(z);
	//color = vec4(d, 1.0f);
}