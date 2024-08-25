return {
	MainCode =
[[
	vec3 midPos = (targetPos + startPos) * 0.5f;
	vec3 cameraDir = normalize(midPos - camPos);

	vec3 zdir = normalize(targetPos - startPos);
	vec3 xdir = normalize(cross(cameraDir, zdir));
	vec3 ydir = normalize(cross(cameraDir, xdir));

	float beamEdgeSize = thickness;
	float beamCoreSize = beamEdgeSize * coreThickness;
	float flareEdgeSize = thickness * flareSize;
	float flareCoreSize = flareEdgeSize * coreThickness;

	if (validTextures.y) {
		AddEffectsQuad(
			drawOrder,
			animParams2,
			startPos - xdir * beamEdgeSize                      , vec2(midTexX2, texCoord2.y),
			startPos - xdir * beamEdgeSize - ydir * beamEdgeSize, texCoord2.zy,
			startPos + xdir * beamEdgeSize - ydir * beamEdgeSize, texCoord2.zw,
			startPos + xdir * beamEdgeSize                      , vec2(midTexX2, texCoord2.w),
			ecsColor
		);

		AddEffectsQuad(
			drawOrder,
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
			drawOrder,
			animParams1,
			startPos  - xdir * beamEdgeSize, texCoord1.xy, ecsColor,
			targetPos - xdir * beamEdgeSize, texCoord1.zy, eceColor,
			targetPos + xdir * beamEdgeSize, texCoord1.zw, eceColor,
			startPos  + xdir * beamEdgeSize, texCoord1.xw, ecsColor
		);

		AddEffectsQuad(
			drawOrder,
			animParams1,
			startPos  - xdir * beamCoreSize, texCoord1.xy, ecsColor,
			targetPos - xdir * beamCoreSize, texCoord1.zy, eceColor,
			targetPos + xdir * beamCoreSize, texCoord1.zw, eceColor,
			startPos  + xdir * beamCoreSize, texCoord1.xw, ecsColor
		);
	}

	if (validTextures.y) {
		AddEffectsQuad(
			drawOrder,
			animParams2,
			targetPos - xdir * beamEdgeSize                      , vec2(midTexX2, texCoord2.y),
			targetPos - xdir * beamEdgeSize + ydir * beamEdgeSize, texCoord2.zy,
			targetPos + xdir * beamEdgeSize + ydir * beamEdgeSize, texCoord2.zw,
			targetPos + xdir * beamEdgeSize                      , vec2(midTexX2, texCoord2.w),
			ecsColor
		);

		AddEffectsQuad(
			drawOrder,
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
			drawOrder,
			animParams3,
			startPos, vec2(flareEdgeSize), texCoord3,
			ecsColor
		);
		AddEffectsQuadCamera(
			drawOrder,
			animParams3,
			startPos, vec2(flareCoreSize), texCoord3,
			ccsColor
		);
	}
]]
}