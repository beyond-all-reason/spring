#version 150 compatibility

uniform sampler2D atlasTex;
uniform sampler2D depthTex;

in Data{
	vec4 vsPos;
	vec4 vCol;
	vec2 vUV;
};

#define projMatrix gl_ProjectionMatrix

uniform vec2 invScreenSize;
uniform float softenThreshold;
uniform vec2 softenExponent;

#define NORM2SNORM(value) (value * 2.0 - 1.0)
#define SNORM2NORM(value) (value * 0.5 + 0.5)

float GetViewSpaceDepth(float d) {
	#ifndef DEPTH_CLIP01
		d = NORM2SNORM(d);
	#endif
	return -projMatrix[3][2] / (projMatrix[2][2] + d);
}

void main() {
	vec4 color = texture(atlasTex, vUV);
	vec2 screenUV = gl_FragCoord.xy * invScreenSize;

	float depthZO = texture(depthTex, screenUV).x;
	float depthVS = GetViewSpaceDepth(depthZO);
	float edgeSmoothness;

	if (softenThreshold > 0.0) {
		edgeSmoothness = smoothstep(0.0, softenThreshold, vsPos.z - depthVS); // soften edges
		gl_FragColor  = color * vCol;
		gl_FragColor *= pow(edgeSmoothness, softenExponent.x);
	} else {
		edgeSmoothness = smoothstep(softenThreshold, 0.0, vsPos.z - depthVS); // follow the surface up
		gl_FragColor  = color * vCol;
		gl_FragColor *= pow(edgeSmoothness, softenExponent.y);
	}
}