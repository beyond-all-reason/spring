#version 130

uniform sampler2D atlasTex;
#ifdef SMOOTH_PARTICLES
	uniform sampler2D depthTex;
	uniform float softenThreshold;
	uniform vec2 softenExponent;
#endif
uniform vec4 alphaCtrl = vec4(0.0, 0.0, 0.0, 1.0); //always pass
uniform vec3 fogColor;

in vec4 vCol;
centroid in vec4 vUV;
in float vLayer; //for sampler2Darray (future)
in float vBF;
in float fogFactor;
#ifdef SMOOTH_PARTICLES
	in vec4 vsPos;
	noperspective in vec2 screenUV;
#endif

out vec4 fragColor;

#define projMatrix gl_ProjectionMatrix

#define NORM2SNORM(value) (value * 2.0 - 1.0)
#define SNORM2NORM(value) (value * 0.5 + 0.5)

#ifdef SMOOTH_PARTICLES
float GetViewSpaceDepth(float d) {
	#ifndef DEPTH_CLIP01
		d = NORM2SNORM(d);
	#endif
	return -projMatrix[3][2] / (projMatrix[2][2] + d);
}
#endif

bool AlphaDiscard(float a) {
	float alphaTestGT = float(a > alphaCtrl.x) * alphaCtrl.y;
	float alphaTestLT = float(a < alphaCtrl.x) * alphaCtrl.z;

	return ((alphaTestGT + alphaTestLT + alphaCtrl.w) == 0.0);
}

void main() {
	vec4 c0 = texture(atlasTex, vUV.xy);
	vec4 c1 = texture(atlasTex, vUV.zw);

	vec4 color = vec4(mix(c0, c1, vBF));
	fragColor = color * vCol;
	fragColor = vCol;

	fragColor.rgb = mix(fragColor.rgb, fogColor * fragColor.a, (1.0 - fogFactor));

	#ifdef SMOOTH_PARTICLES
	float depthZO = texture(depthTex, screenUV).x;
	float depthVS = GetViewSpaceDepth(depthZO);

	if (softenThreshold > 0.0) {
		float edgeSmoothness = smoothstep(0.0, softenThreshold, vsPos.z - depthVS); // soften edges
		fragColor *= pow(edgeSmoothness, softenExponent.x);
	} else {
		float edgeSmoothness = smoothstep(softenThreshold, 0.0, vsPos.z - depthVS); // follow the surface up
		fragColor *= pow(edgeSmoothness, softenExponent.y);
	}
	#endif

	if (AlphaDiscard(fragColor.a))
		discard;
}