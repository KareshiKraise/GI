#version 430

layout(binding = 0) uniform sampler2D lightingBuffer;
layout(binding = 1) uniform sampler2D materialTexture;
layout(binding = 2) uniform sampler2D depthImage;

in vec2 tex;

uniform float Wid;
uniform float Hei;
uniform float near;
uniform float far;

out vec4 color;

float LinearizeDepth(float depth)
{
	float z = depth * 2.0 - 1.0; // Back to NDC 		
	return (2.0 * near) / (far + near - z * (far - near));
}

void main(){	

	const float gamma = 2.2f;	
	vec3 material = texture(materialTexture, tex).rgb;	
	vec3 radiance = texture(lightingBuffer, tex).rgb;
	material = material*radiance;
	material = material / (material + vec3(1.0));
	//material = vec3(1.0f) - exp(-material * 3.f);
	material = pow(material, vec3(1.0f/gamma));
		
	color = vec4(material, 1.0);	

	//float linear = texture(depthImage, tex).r;
	//float linear = LinearizeDepth(texture(depthImage, tex).r);
	//color = vec4(linear, linear, linear, 1.0);
	gl_FragDepth = texture(depthImage, tex).r;	
}