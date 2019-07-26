#version 430

layout(location = 0) in vec2 Pos;

void main(){
	gl_PointSize = 15.f;
	gl_Position = vec4(Pos*2.0f - 1.0f, -1.0, 1.0);
}

