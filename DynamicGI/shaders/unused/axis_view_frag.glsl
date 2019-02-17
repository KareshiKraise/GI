#version 430

in vec3 col;

out vec4 fragcolor;

void main() {
	fragcolor = vec4(col, 1.0);
}