#version 430 core

uniform ivec2 arraySizes;
uniform float currFrame;
uniform mat4 camView;
uniform vec3 camPos;

#line 10008
// Placeholer for the struct InputData
%s

#line 20012
// Placeholer for definitions
%s

#line 30016

struct TriangleData
{
    vec4 pos;
    vec4 uvw;
    vec4 uvInfo;
	vec4 apAndCol;
};

layout(local_size_x = WORKGROUP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = DATA_SSBO_BINDING_IDX) readonly buffer IN
{
    InputData dataIn[];
};

layout(std430, binding = VERT_SSBO_BINDING_IDX) buffer OUT1
{
    TriangleData triangleData[];
};

layout(std430, binding = IDCS_SSBO_BINDING_IDX) buffer OUT2
{
    uint indicesData[];
};

layout(std430, binding = ATOM_SSBO_BINDING_IDX) buffer QUADSCOUNTER
{
    uint quadsCounter;
};

uint GetUnpackedValue(uint packedValue, uint byteNum) {
	return (packedValue >> (8u * byteNum)) & 0xFFu;
}

vec4 GetPackedColor(uint packedColor) {
	return  vec4(
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

shared uint localQuadsCounter;

void AddEffectsQuad(
	uint thisQuadIndex,
	vec3 tlPos, vec2 tlUV, vec4 tlCol,
	vec3 trPos, vec2 trUV, vec4 trCol,
	vec3 brPos, vec2 brUV, vec4 brCol,
	vec3 blPos, vec2 blUV, vec4 blCol
) {
	vec4 minMaxUV = vec4(
		min(min(min(tlUV, trUV), brUV), blUV),
		max(max(max(tlUV, trUV), brUV), blUV)
	);

	vec4 uvInfo = vec4(minMaxUV.x, minMaxUV.y, minMaxUV.z - minMaxUV.x, minMaxUV.w - minMaxUV.y);
	const float textureLayer = 0.0; //for future

	uint triIndex = 4u * thisQuadIndex;
	uint idxIndex = 6u * thisQuadIndex;

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
	triangleData[triIndex].apAndCol = vec4(animParams, uintBitsToFloat(PackColor(tlCol)));
	triIndex++;

	// TR
	triangleData[triIndex].pos      = vec4(trPos, drawOrder);
	triangleData[triIndex].uvw      = vec4(trUV, textureLayer, 0.0);
	triangleData[triIndex].uvInfo   = uvInfo;
	triangleData[triIndex].apAndCol = vec4(animParams, uintBitsToFloat(PackColor(trCol)));
	triIndex++;

	// BR
	triangleData[triIndex].pos      = vec4(brPos, drawOrder);
	triangleData[triIndex].uvw      = vec4(brUV, textureLayer, 0.0);
	triangleData[triIndex].uvInfo   = uvInfo;
	triangleData[triIndex].apAndCol = vec4(animParams, uintBitsToFloat(PackColor(brCol)));
	triIndex++;

	// BL
	triangleData[triIndex].pos      = vec4(blPos, drawOrder);
	triangleData[triIndex].uvw      = vec4(blUV, textureLayer, 0.0);
	triangleData[triIndex].uvInfo   = uvInfo;
	triangleData[triIndex].apAndCol = vec4(animParams, uintBitsToFloat(PackColor(blCol)));
}

void AddEffectsQuad(
	uint thisQuadIndex,
	vec3 tlPos, vec2 tlUV,
	vec3 trPos, vec2 trUV,
	vec3 brPos, vec2 brUV,
	vec3 blPos, vec2 blUV,
	vec4 quadColor
) {
	// TODO write optimized code
	AddEffectsQuad(
		thisQuadIndex,
		tlPos, tlUV, quadColor,
		trPos, trUV, quadColor,
		brPos, brUV, quadColor,
		blPos, blUV, quadColor
	);
}

void main()
{
	if (gl_LocalInvocationID.x == 0u)
		localQuadsCounter = 0u;

	barrier();
    memoryBarrierShared();
	
#line 40170
	// Placeholer for early exit
%s
#line 50173

#line 60175
	// Placeholer to define the number of quads
	uint quadStartIndex = atomicAdd(localQuadsCounter, %s);

	barrier();
    memoryBarrierShared();

	if (gl_LocalInvocationID.x == 0u)
		atomicAdd(quadsCounter, localQuadsCounter);

	if (gl_GlobalInvocationID.x >= arraySizes.x)
		return;
	
#line 70188
	// Placeholer for the rest of the main code
%s
}