#version 130

uniform sampler2D shadingTex;
uniform sampler2D minimapTex;
uniform sampler2D infomapTex;

in vec2 vTexCoords;

uniform vec2 uvMult;
uniform float infotexMul;

out vec4 fragColor;

void main()
{
	const float depthBias = -2.0;

	vec4 shadingColor = texture(shadingTex, vTexCoords                    );
	vec4 minimapColor = texture(minimapTex, vTexCoords         , depthBias);
	vec4 infomapColor = texture(infomapTex, vTexCoords * uvMult, depthBias) - vec4(0.5);

	fragColor = shadingColor * minimapColor + (infomapColor * infotexMul);
}