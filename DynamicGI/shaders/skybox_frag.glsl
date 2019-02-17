#version 430

uniform samplerCube cubePos;
uniform samplerCube cubeNormal;
uniform samplerCube cubeColor;

in vec3 tex;
out vec4 col;


void main() {	
	
	col = texture(cubeColor, tex);
	 
	
}