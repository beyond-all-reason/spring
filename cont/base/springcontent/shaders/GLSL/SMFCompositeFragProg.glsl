#version 130

uniform sampler2D colorTex;
uniform sampler2D depthTex;

in vec2 texCoords;

out vec4 fragColor;

void main() {
	float depth = texture(depthTex, texCoords).r;
	vec4 color = texture(colorTex, texCoords);

	// only ground fragments carry a color; cleared, alpha-tested and Lua-drawn G-buffer pixels stay for later passes
	if (depth >= 1.0 || color.a == 0.0)
		discard;

	fragColor = color;
	gl_FragDepth = depth;
}
