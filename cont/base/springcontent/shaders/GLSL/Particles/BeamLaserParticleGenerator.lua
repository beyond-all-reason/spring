return {
	InputData =
[[
struct InputData {
	vec4 info0; // .xyz startPos, .w coreColStart
	vec4 info1; // .xyz targetPos, .w coreColEnd
	vec4 info2; // .xyz animParams1, .w edgeColStart
	vec4 info3; // .xyz animParams2, .w edgeColEnd
	vec4 info4; // .xyz animParams3, .w drawOrder(as int)
	vec4 info5; // texCoord1
	vec4 info6; // texCoord2
	vec4 info7; // texCoord3
	vec4 info8; // .x thickness, .y coreThickness, .z flareSize, .w midTexX2
};
]],
	InputDefs =
[[
#define startPos      dataIn[gl_GlobalInvocationID.x].info0.xyz
#define coreColStart  floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info0.w)

#define targetPos     dataIn[gl_GlobalInvocationID.x].info1.xyz
#define coreColEnd    floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info1.w)

#define animParams1   dataIn[gl_GlobalInvocationID.x].info2.xyz
#define edgeColStart  floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info2.w)

#define animParams2   dataIn[gl_GlobalInvocationID.x].info3.xyz
#define edgeColEnd    floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info3.w)

#define animParams3   dataIn[gl_GlobalInvocationID.x].info4.xyz
#define drawOrder     dataIn[gl_GlobalInvocationID.x].info4.w

#define texCoord1     dataIn[gl_GlobalInvocationID.x].info5
#define texCoord2     dataIn[gl_GlobalInvocationID.x].info6
#define texCoord3     dataIn[gl_GlobalInvocationID.x].info7

#define thickness     dataIn[gl_GlobalInvocationID.x].info8.x
#define coreThickness dataIn[gl_GlobalInvocationID.x].info8.y
#define flareSize     dataIn[gl_GlobalInvocationID.x].info8.z
#define midTexX2      dataIn[gl_GlobalInvocationID.x].info8.w
]],
	EarlyExit =
[[
	bvec3 validTextures = bvec3(
		(texCoord1.z - texCoord1.x) * (texCoord1.w - texCoord1.y) > 0.0,
		(texCoord2.z - texCoord2.x) * (texCoord2.w - texCoord2.y) > 0.0,
		(texCoord3.z - texCoord3.x) * (texCoord3.w - texCoord3.y) > 0.0
	);

	if (!any(validTextures))
		return;
]],
	NumQuads =
[[
	2 * uint(validTextures.x) + 4 * uint(validTextures.y) + 2 * uint(validTextures.z)
]],
	MainCode =
[[
	vec4 ccsColor = GetPackedColor(coreColStart);
	vec4 cceColor = GetPackedColor(coreColEnd);
	vec4 ecsColor = GetPackedColor(edgeColStart);
	vec4 eceColor = GetPackedColor(edgeColEnd);

	vec3 midPos = (targetPos + startPos) * 0.5f;
	vec3 cameraDir = normalize(midPos - camView[3].xyz);

	vec3 zdir = normalize(targetPos - startPos);
	vec3 xdir = normalize(cross(cameraDir, zdir));
	vec3 ydir = normalize(cross(cameraDir, xdir));

	float beamEdgeSize = thickness;
	float beamCoreSize = beamEdgeSize * coreThickness;
	float flareEdgeSize = thickness * flareSize;
	float flareCoreSize = flareEdgeSize * coreThickness;

	if (validTextures.y) {
		AddEffectsQuad(
			quadStartIndex++,
			animParams2,
			startPos - xdir * beamEdgeSize                      , vec2(midTexX2, texCoord2.y),
			startPos - xdir * beamEdgeSize - ydir * beamEdgeSize, texCoord2.zy,
			startPos + xdir * beamEdgeSize - ydir * beamEdgeSize, texCoord2.zw,
			startPos + xdir * beamEdgeSize                      , vec2(midTexX2, texCoord2.w),
			ecsColor
		);

		AddEffectsQuad(
			quadStartIndex++,
			animParams2,
			startPos - xdir * beamCoreSize                      , vec2(midTexX2, texCoord2.y),
			startPos - xdir * beamCoreSize - ydir * beamCoreSize, texCoord2.zy,
			startPos + xdir * beamCoreSize - ydir * beamCoreSize, texCoord2.zw,
			startPos + xdir * beamCoreSize                      , vec2(midTexX2, texCoord2.w),
			ccsColor
		);
	}

	if (validTextures.x) {
		AddEffectsQuad(
			quadStartIndex++,
			animParams1,
			startPos  - xdir * beamEdgeSize, texCoord1.xy, ecsColor,
			targetPos - xdir * beamEdgeSize, texCoord1.zy, eceColor,
			targetPos + xdir * beamEdgeSize, texCoord1.zw, eceColor,
			startPos  + xdir * beamEdgeSize, texCoord1.xw, ecsColor
		);

		AddEffectsQuad(
			quadStartIndex++,
			animParams1,
			startPos  - xdir * beamCoreSize, texCoord1.xy, ecsColor,
			targetPos - xdir * beamCoreSize, texCoord1.zy, eceColor,
			targetPos + xdir * beamCoreSize, texCoord1.zw, eceColor,
			startPos  + xdir * beamCoreSize, texCoord1.xw, ecsColor
		);
	}

	if (validTextures.y) {
		AddEffectsQuad(
			quadStartIndex++,
			animParams2,
			targetPos - xdir * beamEdgeSize                      , vec2(midTexX2, texCoord2.y),
			targetPos - xdir * beamEdgeSize + ydir * beamEdgeSize, texCoord2.zy,
			targetPos + xdir * beamEdgeSize + ydir * beamEdgeSize, texCoord2.zw,
			targetPos + xdir * beamEdgeSize                      , vec2(midTexX2, texCoord2.w),
			ecsColor
		);

		AddEffectsQuad(
			quadStartIndex++,
			animParams2,
			targetPos - xdir * beamCoreSize                      , vec2(midTexX2, texCoord2.y),
			targetPos - xdir * beamCoreSize + ydir * beamCoreSize, texCoord2.zy,
			targetPos + xdir * beamCoreSize + ydir * beamCoreSize, texCoord2.zw,
			targetPos + xdir * beamCoreSize                      , vec2(midTexX2, texCoord2.w),
			ccsColor
		);
	}

	if (validTextures.z) {
		AddEffectsQuadCamera(
			quadStartIndex++,
			animParams3,
			startPos, vec2(flareEdgeSize), texCoord3,
			ecsColor
		);
		AddEffectsQuadCamera(
			quadStartIndex++,
			animParams3,
			startPos, vec2(flareCoreSize), texCoord3,
			ccsColor
		);
	}
]]
}