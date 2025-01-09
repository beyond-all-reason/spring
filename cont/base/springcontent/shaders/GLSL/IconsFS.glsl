#version 150

uniform sampler2D mainTex;
uniform sampler2D custTex;

uniform vec4 ucolor = vec4(1.0);

// FS input attributes
in vec3 vuvw;
in vec4 vColor;

// FS output attributes
out vec4 outColor;

uniform vec4 alphaCtrl = vec4(0.0, 0.0, 0.0, 1.0); //always pass

bool AlphaDiscard(float a) {
	float alphaTestGT = float(a > alphaCtrl.x) * alphaCtrl.y;
	float alphaTestLT = float(a < alphaCtrl.x) * alphaCtrl.z;

	return ((alphaTestGT + alphaTestLT + alphaCtrl.w) == 0.0);
}

void main() {
	if (vuvw.z == 0.0)
		outColor = texture(mainTex, vuvw.xy);
	else
		outColor = texture(custTex, vuvw.xy);
	
	outColor *= vColor * ucolor;

	if (AlphaDiscard(outColor.a))
		discard;
}