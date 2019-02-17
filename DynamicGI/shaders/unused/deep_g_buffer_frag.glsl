#version 430

layout(location = 0) out vec3 gposition;
layout(location = 1) out vec3 gnormal;
layout(location = 2) out vec4 galbedo;

in GS_OUT{
	vec3 pos;
	vec3 normal;
	vec2 tex;
	vec4 clipSpace;
}fs_in;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;

uniform sampler2DArray compare_buffer;

uniform float DELTA;
uniform float near;
uniform float far;


float linearize_and_compare() {
	//clamp between 0 and 1
	vec2 projected = (fs_in.clipSpace.xy / fs_in.clipSpace.w) * 0.5f + 0.5f;

	//check Z in comparison buffer
	float previousZ = texture(compare_buffer, vec3(projected, 0)).r;
	//Linearize it and bring back to world coords
	previousZ = 2.0 * previousZ - 1.0;
	previousZ = 2.0 * near * far / (far + near - previousZ * (far - near));

	//add delta
	previousZ += DELTA;

	//unnaply transform
	float threshold = (far + near - 2.0 * near * far / previousZ) / (far - near);
	return (threshold * 0.5f + 0.5f);
}

void main() {

	if (gl_Layer == 0) {
		gposition = fs_in.pos;
		gnormal = normalize(fs_in.normal);
		galbedo.rgb = texture(texture_diffuse1, fs_in.tex).rgb;
		galbedo.a = texture(texture_specular1, fs_in.tex).r;
	}
	else if (gl_Layer == 1) {

		float depth_threshold = linearize_and_compare();

		if (gl_FragCoord.z > depth_threshold) {
			gposition = fs_in.pos;
			gnormal = normalize(fs_in.normal);
			galbedo.rgb = texture(texture_diffuse1, fs_in.tex).rgb;
			galbedo.a =   texture(texture_specular1, fs_in.tex).r;
		}
		else {
			discard;
		}
	}




}







