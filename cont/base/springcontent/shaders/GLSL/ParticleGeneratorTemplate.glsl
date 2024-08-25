#version 430 core

uniform ivec2 arraySizes;
uniform vec3 frameInfo; // gs->frameNum, globalRendering->timeOffset, gu->modGameTime

uniform vec3 camPos;
uniform vec3 camDir[3]; // right, up, forward
uniform vec4 frustumPlanes[6];

// Placeholer for the struct InputData
%s

shared uint localNumCulled;
shared uint localNumOutOfB;

struct TriangleData
{
    vec4 pos;
    vec4 uvw;
    vec4 uvInfo;
	vec4 apAndCol;
};

layout(local_size_x = WORKGROUP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = DATA_SSBO_BINDING_IDX) readonly restrict buffer IN
{
    InputData dataIn[];
};

layout(std430, binding = VERT_SSBO_BINDING_IDX) writeonly restrict buffer OUT1
{
    TriangleData triangleData[];
};

/*
layout(std430, binding = IDCS_SSBO_BINDING_IDX) writeonly restrict buffer OUT2
{
    uint indicesData[];
};
*/

layout(std430, binding = SIZE_SSBO_BINDING_IDX) coherent restrict buffer SIZE
{
    uint atomicCounters[];
};

uint GetUnpackedValue(uint packedValue, uint byteNum) {
	return (packedValue >> (8u * byteNum)) & 0xFFu;
}

vec4 GetColorFromIntegers(uvec4 bytes) {
	return vec4(
		float(bytes[0] / 255.0),
		float(bytes[1] / 255.0),
		float(bytes[2] / 255.0),
		float(bytes[3] / 255.0)
	);
}

vec4 GetPackedColor(uint packedColor) {
	return vec4(
		float(GetUnpackedValue(packedColor, 0)) / 255.0,
		float(GetUnpackedValue(packedColor, 1)) / 255.0,
		float(GetUnpackedValue(packedColor, 2)) / 255.0,
		float(GetUnpackedValue(packedColor, 3)) / 255.0
	);
}

uint PackColor(vec4 unpackedColor) {
	return
		(uint(0xFFu * clamp(unpackedColor.x, 0.0, 1.0)) << 0 ) |
		(uint(0xFFu * clamp(unpackedColor.y, 0.0, 1.0)) << 8 ) |
		(uint(0xFFu * clamp(unpackedColor.z, 0.0, 1.0)) << 16) |
		(uint(0xFFu * clamp(unpackedColor.w, 0.0, 1.0)) << 24);
}

vec3 Rotate(vec2 sc, vec3 axis, vec3 input) {
	//Rodrigues' rotation formula
	return input * sc.y + cross(axis, input) * sc.x + axis * dot(axis, input) * (1.0 - sc.y);
}

vec3 Rotate(float angle, vec3 axis, vec3 input) {
	vec2 sc = vec2(sin(angle), cos(angle));
	return Rotate(sc, axis, input);
}

float GetCurrentRotation(vec3 rotationParameters, float currTime) {
	// rotationParameters.y is acceleration in angle per frame^2
	float rotVel = rotationParameters.x + rotationParameters.y * currTime;
	float rotVal = rotationParameters.z + rotVel               * currTime;

	return rotVal;
}

float GetCurrentAnimation(vec3 animationParameters, float currTime) {
	if (animationParameters.x <= 1.0 && animationParameters.y <= 1.0)
		return 0.0;

	float animProgress;
	float animSpeed = abs(animationParameters.z);
	if (animationParameters.z < 0.0) {
		animProgress = 1.0 - abs(mod(currTime, 2.0 * animSpeed) / animSpeed - 1.0);
	} else {
		animProgress = mod(currTime, animSpeed) / animSpeed;
	}

	return animProgress;
}

vec4 GetCurrentColor(vec4 unpackedColorEdge0, vec4 unpackedColorEdge1, float lifeEdge0, float lifeEdge1, float currTime) {
	float colMixRate = clamp((lifeEdge1 - currTime)/(lifeEdge1 - lifeEdge0), 0.0, 1.0);
	return mix(unpackedColorEdge0, unpackedColorEdge1, colMixRate);
}

vec4 GetCurrentColor(uint colorEdge0, uint colorEdge1, float lifeEdge0, float lifeEdge1, float currTime) {
	vec4 unpackedColorEdge0 = GetPackedColor(colorEdge0);
	vec4 unpackedColorEdge1 = GetPackedColor(colorEdge1);

	return GetCurrentColor(unpackedColorEdge0, unpackedColorEdge1, lifeEdge0, lifeEdge1, currTime);
}

float GetParticleTime(int creationFrame) {
	return frameInfo.x + frameInfo.y - creationFrame;
}

vec3 GetParticleDrawPos(vec3 particlePosition, vec3 particleSpeed) {
	return particlePosition + particleSpeed * frameInfo.y;
}

bool SphereInView(vec4 posRad) {
	for (uint i = 0u; i < 6u; ++i) {
		float dist = dot(frustumPlanes[i].xyz, posRad.xyz) + frustumPlanes[i].w;
		if (dist < -posRad.w)
			return false; // outside
		/*
		else if (dist < posRad.w)
			return true;  // intersect
		*/
	}
	return true;
}

void AddEffectsQuad(
	int drawOrder,
	vec3 animPrms,
	vec3 tlPos, vec2 tlUV, vec4 tlCol,
	vec3 trPos, vec2 trUV, vec4 trCol,
	vec3 brPos, vec2 brUV, vec4 brCol,
	vec3 blPos, vec2 blUV, vec4 blCol
) {

//#undef FRUSTUM_CULLING
#ifdef FRUSTUM_CULLING
{
	vec3 m = min(tlPos, min(trPos, min(brPos, blPos)));
	vec3 M = max(tlPos, max(trPos, max(brPos, blPos)));
	vec3 spherePos = (M + m) * 0.5;
	vec3 scales    = (M - m) * 0.5;

	if (!SphereInView(vec4(spherePos, length(scales)))) {
		atomicAdd(localNumCulled, 1u);
		return;
	}
}
#endif
	uint thisQuadIndex = atomicAdd(atomicCounters[SIZE_SSBO_QUAD_IDX], 1u);

	// sanity check
	if (thisQuadIndex >= uint(arraySizes.y)) {
		atomicAdd(localNumOutOfB, 1u);
		return;
	}

	uint triIndex = 4u * thisQuadIndex;
	uint idxIndex = 6u * thisQuadIndex;

	vec4 minMaxUV = vec4(
		min(min(min(tlUV, trUV), brUV), blUV),
		max(max(max(tlUV, trUV), brUV), blUV)
	);

	vec4 uvInfo = vec4(minMaxUV.x, minMaxUV.y, minMaxUV.z - minMaxUV.x, minMaxUV.w - minMaxUV.y);
	const float textureLayer = 0.0; //for future

	// Proper indices will be produced after sorting
	/*
	/////////////////
	// Indices
	/////////////////
	//triangle 1 {tl, tr, bl}
	indicesData[idxIndex++] = triIndex + 3u;
	indicesData[idxIndex++] = triIndex + 0u;
	indicesData[idxIndex++] = triIndex + 1u;

	//triangle 2 {bl, tr, br}
	indicesData[idxIndex++] = triIndex + 3u;
	indicesData[idxIndex++] = triIndex + 1u;
	indicesData[idxIndex  ] = triIndex + 2u;
	*/

	/////////////////
	// Triangles
	/////////////////
	// TL
	triangleData[triIndex].pos      = vec4(tlPos, intBitsToFloat(drawOrder));
	triangleData[triIndex].uvw      = vec4(tlUV, textureLayer, 0.0);
	triangleData[triIndex].uvInfo   = uvInfo;
	triangleData[triIndex].apAndCol = vec4(animPrms, uintBitsToFloat(PackColor(tlCol)));
	triIndex++;

	// TR
	triangleData[triIndex].pos      = vec4(trPos, intBitsToFloat(drawOrder));
	triangleData[triIndex].uvw      = vec4(trUV, textureLayer, 0.0);
	triangleData[triIndex].uvInfo   = uvInfo;
	triangleData[triIndex].apAndCol = vec4(animPrms, uintBitsToFloat(PackColor(trCol)));
	triIndex++;

	// BR
	triangleData[triIndex].pos      = vec4(brPos, intBitsToFloat(drawOrder));
	triangleData[triIndex].uvw      = vec4(brUV, textureLayer, 0.0);
	triangleData[triIndex].uvInfo   = uvInfo;
	triangleData[triIndex].apAndCol = vec4(animPrms, uintBitsToFloat(PackColor(brCol)));
	triIndex++;

	// BL
	triangleData[triIndex].pos      = vec4(blPos, intBitsToFloat(drawOrder));
	triangleData[triIndex].uvw      = vec4(blUV, textureLayer, 0.0);
	triangleData[triIndex].uvInfo   = uvInfo;
	triangleData[triIndex].apAndCol = vec4(animPrms, uintBitsToFloat(PackColor(blCol)));
}

void AddEffectsQuad(
	int drawOrder,
	vec3 animPrms,
	vec3 tlPos, vec2 tlUV,
	vec3 trPos, vec2 trUV,
	vec3 brPos, vec2 brUV,
	vec3 blPos, vec2 blUV,
	vec4 quadColor
) {
	// TODO write optimized code
	AddEffectsQuad(
		drawOrder,
		animPrms,
		tlPos, tlUV, quadColor,
		trPos, trUV, quadColor,
		brPos, brUV, quadColor,
		blPos, blUV, quadColor
	);
}

void AddEffectsQuad(
	int drawOrder,
	vec3 animPrms,
	vec3 tlPos,
	vec3 trPos,
	vec3 brPos,
	vec3 blPos,
	vec4 texCrds,
	vec4 quadColor
) {
	// TODO write optimized code
	AddEffectsQuad(
		drawOrder,
		animPrms,
		tlPos, texCrds.xy, quadColor,
		trPos, texCrds.zy, quadColor,
		brPos, texCrds.zw, quadColor,
		blPos, texCrds.xw, quadColor
	);
}

void AddEffectsQuadCamera(
	int drawOrder,
	vec3 animPrms,
	vec3 centerPos,
	vec2 quadDims,
	vec4 texCrds,
	vec4 quadColor
) {
	// TODO write optimized code
	AddEffectsQuad(
		drawOrder,
		animPrms,
		centerPos - camDir[0] * quadDims.x - camDir[1] * quadDims.y, texCrds.xy,
		centerPos + camDir[0] * quadDims.x - camDir[1] * quadDims.y, texCrds.zy,
		centerPos + camDir[0] * quadDims.x + camDir[1] * quadDims.y, texCrds.zw,
		centerPos - camDir[0] * quadDims.x + camDir[1] * quadDims.y, texCrds.xw,
		quadColor
	);
}

void main()
{
	if (gl_LocalInvocationID.x == 0u) {
		localNumOutOfB = 0u;
		localNumCulled = 0u;
	}

	barrier();
	memoryBarrierShared();

	if (gl_GlobalInvocationID.x >= arraySizes.x)
		return;

	// Placeholer for automatic early exit
%s
	// Placeholer for custom early exit and other init code
%s
	// Automatically formed input data
%s
	// Placeholer for the rest of the main code
%s

	barrier();
	memoryBarrierShared();

	if (gl_LocalInvocationID.x == 0u) {
		atomicAdd(atomicCounters[SIZE_SSBO_OOBC_IDX], localNumOutOfB);
		atomicAdd(atomicCounters[SIZE_SSBO_CULL_IDX], localNumCulled);
	}
}