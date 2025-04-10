#version 130

in vec2 uv;

uniform sampler2D tex;
uniform vec4 ucolor;

out vec4 fragColor;

void main()
{
	fragColor = ucolor * texture(tex, uv);
}
