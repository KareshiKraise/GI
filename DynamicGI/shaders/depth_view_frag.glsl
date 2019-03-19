#version 430

//debug view of shadow map, to be used with a fullscreen quad

out vec4 fragColor;
in vec2 tex;

uniform sampler2D screen_tex;
uniform float near;
uniform float far;

float LinearizeDepth(float depth)
{
	float z = depth * 2.0 - 1.0; 
	return (2.0 * near * far) / (far + near - z * (far - near));
}


void main() {
	
	//view texture
	//fragColor = vec4(texture2D(screen_tex, tex).rgb, 1.0);
	
	//view standard depth map
	//float depthVal = LinearizeDepth(texture2D(screen_tex, tex).r);
	//fragColor = vec4(vec3(depthVal), 1.0f);
	

	//view parabolic depth map
	float val = texture2D(screen_tex, tex).r;
	fragColor = vec4(val, val, val, 1.0);

	
	
}







