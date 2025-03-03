#version 130

in vec2 pos;

out vec2 mapUV;

uniform vec4 mapSizeP1;

#define NORM2SNORM(value) (value * 2.0 - 1.0)
#define SNORM2NORM(value) (value * 0.5 + 0.5)

void main()
{
	// pos is 0 --> 1
	mapUV = pos * mapSizeP1.zw;

	gl_Position = vec4(NORM2SNORM(mapUV), 0.0, 1.0);
}