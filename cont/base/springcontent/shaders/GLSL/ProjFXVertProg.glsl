#version 130
//#extension GL_ARB_explicit_attrib_location : require

in vec4 pos;
in vec4 uvw;
in vec4 uvInfo;
in vec3 aparams;
in vec4 color;

out vec4 vCol;
centroid out vec4 vUV;
out float vLayer;
out float vBF;
out float fogFactor;
#ifdef SMOOTH_PARTICLES
	out vec4 vsPos;
	noperspective out vec2 screenUV;
#endif

out float gl_ClipDistance[1];

uniform vec2 fogParams;
uniform vec3 camPos;
uniform vec4 clipPlane = vec4(0.0, 0.0, 0.0, 1.0);

#define NORM2SNORM(value) (value * 2.0 - 1.0)
#define SNORM2NORM(value) (value * 0.5 + 0.5)

void main() {
	float ap = fract(aparams.z);

	float maxImgIdx = aparams.x * aparams.y - 1.0;
	ap *= maxImgIdx;

	float i0 = floor(ap);
	float i1 = i0 + 1.0;

	vBF = fract(ap); //blending factor

	if (maxImgIdx > 1.0) {
		vec2 uvDiff = (uvw.xy - uvInfo.xy);
		vUV = uvDiff.xyxy + vec4(
			floor(mod(i0 , aparams.x)),
			floor(   (i0 / aparams.x)),
			floor(mod(i1 , aparams.x)),
			floor(   (i1 / aparams.x))
		) * uvInfo.zwzw;
		vUV /= aparams.xyxy; //scale
		vUV += uvInfo.xyxy;
	} else {
		vUV = uvw.xyxy;
	}

	vLayer = uvw.z;
	vCol = color;

	float fogDist = length(pos.xyz - camPos);
	fogFactor = (fogParams.y - fogDist) / (fogParams.y - fogParams.x);
	fogFactor = clamp(fogFactor, 0.0, 1.0);

	gl_ClipDistance[0] = dot(vec4(pos.xyz, 1.0), clipPlane); //water clip plane

	#ifdef SMOOTH_PARTICLES
		// viewport relative UV [0.0, 1.0]
		vsPos = gl_ModelViewMatrix * vec4(pos.xyz, 1.0);
		gl_Position = gl_ProjectionMatrix * vsPos;
		screenUV = SNORM2NORM(gl_Position.xy / gl_Position.w);
	#else
		gl_Position = gl_ModelViewProjectionMatrix * vec4(pos.xyz, 1.0);
	#endif
}