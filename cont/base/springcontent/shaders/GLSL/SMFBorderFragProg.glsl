#version 130

uniform sampler2D diffuseTex;
uniform sampler2D detailsTex;

in vec4 vVertCol;
in vec2 vDiffuseUV;
in vec2 vDetailsUV;

// SMF_INTENSITY_MULT
const vec4 diffuseMult = vec4(110.0 / 255.0, 110.0 / 255.0, 110.0 / 255.0, 0.4);

out vec4 fragColor;

const float UV_BORDER_LEEWAY = 1e-2;
void main() {
	vec2 cDiffuseUV = clamp(vDiffuseUV, vec2(UV_BORDER_LEEWAY), vec2(1.0 - UV_BORDER_LEEWAY));
	vec4 diffuseCol = texture(diffuseTex, cDiffuseUV) * diffuseMult;
	vec4 detailsCol = texture(detailsTex, vDetailsUV) * 2.0 - 1.0;

	fragColor = (diffuseCol + detailsCol) * vVertCol;
}
