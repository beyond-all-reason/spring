#version 430 core

uniform ivec2 arraySizes;
uniform vec3 frameInfo;

uniform mat4 camDirPos; // not a matrix, but convinient collection of xDir, yDir, zDir, Pos vectors
uniform vec4 frustumPlanes[6];

// Placeholer for the struct InputData
%s

// Placeholer for definitions
%s

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

layout(std430, binding = IDCS_SSBO_BINDING_IDX) writeonly restrict buffer OUT2
{
    uint indicesData[];
};

layout(std430, binding = ATOM_SSBO_BINDING_IDX) coherent restrict buffer ATOMIC
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

/*
float3 rotate(float angle, vec3 axis, inout vec3[]) const {
	float ca = cos(angle);
	float sa = sin(angle);

	//Rodrigues' rotation formula
	return (*this) * ca + axis.cross(*this) * sa + axis * axis.dot(*this) * (1.0f - ca);
}
*/

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
	vec3 animPrms,
	vec3 tlPos, vec2 tlUV, vec4 tlCol,
	vec3 trPos, vec2 trUV, vec4 trCol,
	vec3 brPos, vec2 brUV, vec4 brCol,
	vec3 blPos, vec2 blUV, vec4 blCol
) {

#define FRUSTUM_CULLING
#ifdef FRUSTUM_CULLING
{
	vec3 m = min(tlPos, min(trPos, min(brPos, blPos)));
	vec3 M = max(tlPos, max(trPos, max(brPos, blPos)));
	vec3 spherePos = (M + m) * 0.5;
	vec3 scales = (M - m) * 0.5;
	if (!SphereInView(vec4(spherePos, length(scales)))) {
		atomicAdd(atomicCounters[ATOM_SSBO_CULL_IDX], 1u);
		return;
	}
}
#endif
	//barrier();
    //memoryBarrierBuffer();
	//groupMemoryBarrier();

	uint thisQuadIndex = atomicAdd(atomicCounters[ATOM_SSBO_QUAD_IDX], 1u);

	// sanity check
	if (thisQuadIndex >= uint(arraySizes.y)) {
		atomicAdd(atomicCounters[ATOM_SSBO_OOBC_IDX], 1u);
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


	/////////////////
	// Triangles
	/////////////////
	// TL
	triangleData[triIndex].pos      = vec4(tlPos, drawOrder);
	triangleData[triIndex].uvw      = vec4(tlUV, textureLayer, 0.0);
	triangleData[triIndex].uvInfo   = uvInfo;
	triangleData[triIndex].apAndCol = vec4(animPrms, uintBitsToFloat(PackColor(tlCol)));
	triIndex++;

	// TR
	triangleData[triIndex].pos      = vec4(trPos, drawOrder);
	triangleData[triIndex].uvw      = vec4(trUV, textureLayer, 0.0);
	triangleData[triIndex].uvInfo   = uvInfo;
	triangleData[triIndex].apAndCol = vec4(animPrms, uintBitsToFloat(PackColor(trCol)));
	triIndex++;

	// BR
	triangleData[triIndex].pos      = vec4(brPos, drawOrder);
	triangleData[triIndex].uvw      = vec4(brUV, textureLayer, 0.0);
	triangleData[triIndex].uvInfo   = uvInfo;
	triangleData[triIndex].apAndCol = vec4(animPrms, uintBitsToFloat(PackColor(brCol)));
	triIndex++;

	// BL
	triangleData[triIndex].pos      = vec4(blPos, drawOrder);
	triangleData[triIndex].uvw      = vec4(blUV, textureLayer, 0.0);
	triangleData[triIndex].uvInfo   = uvInfo;
	triangleData[triIndex].apAndCol = vec4(animPrms, uintBitsToFloat(PackColor(blCol)));
}

void AddEffectsQuad(
	vec3 animPrms,
	vec3 tlPos, vec2 tlUV,
	vec3 trPos, vec2 trUV,
	vec3 brPos, vec2 brUV,
	vec3 blPos, vec2 blUV,
	vec4 quadColor
) {
	// TODO write optimized code
	AddEffectsQuad(
		animPrms,
		tlPos, tlUV, quadColor,
		trPos, trUV, quadColor,
		brPos, brUV, quadColor,
		blPos, blUV, quadColor
	);
}

void AddEffectsQuadCamera(
	vec3 animPrms,
	vec3 centerPos,
	vec2 quadDims,
	vec4 texCrds,
	vec4 quadColor
) {
	// TODO write optimized code
	AddEffectsQuad(
		animPrms,
		centerPos - camDirPos[0].xyz * quadDims.x - camDirPos[1].xyz * quadDims.y, texCrds.xy,
		centerPos + camDirPos[0].xyz * quadDims.x - camDirPos[1].xyz * quadDims.y, texCrds.zy,
		centerPos + camDirPos[0].xyz * quadDims.x + camDirPos[1].xyz * quadDims.y, texCrds.zw,
		centerPos - camDirPos[0].xyz * quadDims.x + camDirPos[1].xyz * quadDims.y, texCrds.xw,
		quadColor
	);
}

void main()
{
	if (gl_GlobalInvocationID.x >= arraySizes.x)
		return;

	// Placeholer for early exit and other init code
%s

	// Placeholer to define the number of quads
	//quadStartIndex = atomicAdd(localQuadsCounter, (%s) * uint(gl_GlobalInvocationID.x < arraySizes.x));

	//barrier();
    //memoryBarrierShared();

	// Placeholer for the rest of the main code
%s
}