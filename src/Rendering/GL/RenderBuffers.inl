static constexpr const char* vsRenderBufferSrc = R"(
// Version and extensions
%s

// VS input attributes
%s

//uniform mat4 transformMatrix = mat4(1.0);

// VS output attributes
%s

void main() {
%s
	gl_Position = gl_ModelViewProjectionMatrix * %s;
}
)";

static constexpr const char* fsRenderBufferSrc = R"(
// Version and extensions
%s

uniform sampler2D tex;
uniform vec4 ucolor = vec4(1.0);

// FS input attributes
%s

// FS output attributes
out vec4 outColor;

uniform vec4 alphaCtrl = vec4(0.0, 0.0, 0.0, 1.0); //always pass

bool AlphaDiscard(float a) {
	float alphaTestGT = float(a > alphaCtrl.x) * alphaCtrl.y;
	float alphaTestLT = float(a < alphaCtrl.x) * alphaCtrl.z;

	return ((alphaTestGT + alphaTestLT + alphaCtrl.w) == 0.0);
}

void main() {
%s
	outColor *= ucolor;
	if (AlphaDiscard(outColor.a))
		discard;

}
)";