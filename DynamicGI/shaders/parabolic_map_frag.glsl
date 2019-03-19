#version 430

in float dval;
in float clipDepth;

out vec4 fragColor;

void main()
{	
	if (clipDepth < 0)
		discard;

	fragColor = vec4(dval, dval, dval, 1.0);
}