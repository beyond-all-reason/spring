#version 150

out vec4 outColor;

uniform vec4 meshColor;

const float X = 0.05;

void main()
{
	outColor = meshColor;
}