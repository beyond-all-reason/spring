#version 430 core

layout(binding = 0) uniform sampler2D tex1;
layout(binding = 1) uniform sampler2D tex2;

in Data {
	vec4 uvCoord;
	vec4 teamCol;
};

uniform vec4 alphaCtrl = vec4(0.0, 0.0, 0.0, 1.0); //always pass
uniform vec4 colorMult = vec4(1.0);

bool AlphaDiscard(float a) {
	float alphaTestGT = float(a > alphaCtrl.x) * alphaCtrl.y;
	float alphaTestLT = float(a < alphaCtrl.x) * alphaCtrl.z;

	return ((alphaTestGT + alphaTestLT + alphaCtrl.w) == 0.0);
}

out vec4 fragColor;
void main(void)
{
	vec4 modelColor = texture(tex1, uvCoord.xy);
	vec4 modelProp  = texture(tex2, uvCoord.xy);


	modelColor.rgb = mix(modelColor.rgb, teamCol.rgb, modelColor.a);
	fragColor = colorMult * vec4(modelColor.rgb, modelProp.a);

	if (AlphaDiscard(modelProp.a))
		discard;
}