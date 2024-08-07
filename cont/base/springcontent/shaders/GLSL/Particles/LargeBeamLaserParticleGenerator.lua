return {
	InputData =
[[
struct InputData {
	vec4 info0; // .xyz startPos, .w drawOrder
	vec4 info1; // .xyz targetPos, .w unused
	vec4 info2; // .x thickness, .y coreThickness, .z flareSize, .w tileLength
	vec4 info3; // .x scrollSpeed, .y pulseSpeed, .z coreColStart, .w edgeColStart
	vec4 info4; // texCoord1
	vec4 info5; // texCoord2
	vec4 info6; // texCoord3
	vec4 info7; // texCoord4
};
]],
	InputDefs =
[[
#define startPos      dataIn[gl_GlobalInvocationID.x].info0.xyz
#define drawOrder     dataIn[gl_GlobalInvocationID.x].info0.w

#define targetPos     dataIn[gl_GlobalInvocationID.x].info1.xyz

#define thickness      dataIn[gl_GlobalInvocationID.x].info2.x
#define coreThickness  dataIn[gl_GlobalInvocationID.x].info2.y
#define flareSize      dataIn[gl_GlobalInvocationID.x].info2.z
#define tileLength     dataIn[gl_GlobalInvocationID.x].info2.w

#define scrollSpeed    dataIn[gl_GlobalInvocationID.x].info3.x
#define pulseSpeed     dataIn[gl_GlobalInvocationID.x].info3.y
#define coreColStart   floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info3.z)
#define edgeColStart   floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info3.w)

#define texCoord1    dataIn[gl_GlobalInvocationID.x].info4
#define texCoord2    dataIn[gl_GlobalInvocationID.x].info5
#define texCoord3    dataIn[gl_GlobalInvocationID.x].info6
#define texCoord4    dataIn[gl_GlobalInvocationID.x].info7
]],
	EarlyExit =
[[
	bvec4 validTextures = bvec4(
		(texCoord1.z - texCoord1.x) * (texCoord1.w - texCoord1.y) > 0.0,
		(texCoord2.z - texCoord2.x) * (texCoord2.w - texCoord2.y) > 0.0,
		(texCoord3.z - texCoord3.x) * (texCoord3.w - texCoord3.y) > 0.0,
		(texCoord4.z - texCoord4.x) * (texCoord4.w - texCoord4.y) > 0.0
	);

	if (!any(validTextures))
		return;

	vec3 midPos = (targetPos + startPos) * 0.5;
	vec3 cameraDir = normalize(midPos - camDirPos[3].xyz);

	// beam's coor-system; degenerate if targetPos == startPos
	vec3 zdir = normalize(targetPos - startPos);
	vec3 xdir = normalize(cross(cameraDir, zdir));
	vec3 ydir = normalize(cross(cameraDir, xdir));

	float startTex = 1.0 - fract(frameInfo.z * scrollSpeed);
	float texSizeX = texCoord1.z - texCoord1.x;

	float beamEdgeSize  = thickness;
	float beamCoreSize  = beamEdgeSize * coreThickness;
	float beamLength    = dot((targetPos - startPos), zdir);
	float flareEdgeSize = thickness * flareSize;
	float flareCoreSize = flareEdgeSize * coreThickness;

	float beamTileMinDst = tileLength * (1.0 - startTex);
	float beamTileMaxDst = beamLength - tileLength;

	// note: beamTileMaxDst can be negative, in which case we want numBeamTiles to equal zero
	float numBeamTiles = floor(((max(beamTileMinDst, beamTileMaxDst) - beamTileMinDst) / tileLength) + 0.5);

	int numQuads1 =
		(beamTileMinDst > beamLength) ?
		2 :
		4 + 2 * int(ceil((beamTileMaxDst - beamTileMinDst) / tileLength));
]],
	NumQuads =
[[
	numQuads1 * uint(validTextures.x) + 2 * uint(validTextures.y) + 4 * uint(validTextures.z) + 2 * uint(validTextures.w)
]],
	MainCode =
[[
	vec4 ccsColor = GetPackedColor(coreColStart);
	vec4 ecsColor = GetPackedColor(edgeColStart);

	vec3 pos1 = startPos;
	vec3 pos2 = targetPos;

	if (validTextures.x) {
		vec4 tex = texCoord1;
		if (beamTileMinDst > beamLength) {
			// beam short enough to be drawn by one polygon
			// draw laser start
			tex.x = texCoord1.x + startTex * texSizeX;

			AddEffectsQuad(
				quadStartIndex++,
				vec3(1.0),
				pos1 - (xdir * beamEdgeSize), tex.xy,
				pos2 - (xdir * beamEdgeSize), tex.zy,
				pos2 + (xdir * beamEdgeSize), tex.zw,
				pos1 + (xdir * beamEdgeSize), tex.xw,
				ecsColor
			);

			AddEffectsQuad(
				quadStartIndex++,
				vec3(1.0),
				pos1 - (xdir * beamCoreSize), tex.xy,
				pos2 - (xdir * beamCoreSize), tex.zy,
				pos2 + (xdir * beamCoreSize), tex.zw,
				pos1 + (xdir * beamCoreSize), tex.xw,
				ccsColor
			);
		} else {
			// beam longer than one polygon
			pos2 = pos1 + zdir * beamTileMinDst;

			// draw laser start
			tex.x = texCoord1.x + startTex * texSizeX;

			AddEffectsQuad(
				quadStartIndex++,
				vec3(1.0),
				pos1 - (xdir * beamEdgeSize), tex.xy,
				pos2 - (xdir * beamEdgeSize), tex.zy,
				pos2 + (xdir * beamEdgeSize), tex.zw,
				pos1 + (xdir * beamEdgeSize), tex.xw,
				ecsColor
			);

			AddEffectsQuad(
				quadStartIndex++,
				vec3(1.0),
				pos1 - (xdir * beamCoreSize), tex.xy,
				pos2 - (xdir * beamCoreSize), tex.zy,
				pos2 + (xdir * beamCoreSize), tex.zw,
				pos1 + (xdir * beamCoreSize), tex.xw,
				ccsColor
			);

			// draw continous beam
			tex.x = texCoord1.x;

			for (float i = beamTileMinDst; i < beamTileMaxDst; i += tileLength) {
				pos1 = startPos + zdir * (i             );
				pos2 = startPos + zdir * (i + tileLength);

				AddEffectsQuad(
					quadStartIndex++,
					vec3(1.0),
					pos1 - (xdir * beamEdgeSize), tex.xy,
					pos2 - (xdir * beamEdgeSize), tex.zy,
					pos2 + (xdir * beamEdgeSize), tex.zw,
					pos1 + (xdir * beamEdgeSize), tex.xw,
					ecsColor
				);

				AddEffectsQuad(
					quadStartIndex++,
					vec3(1.0),
					pos1 - (xdir * beamCoreSize), tex.xy,
					pos2 - (xdir * beamCoreSize), tex.zy,
					pos2 + (xdir * beamCoreSize), tex.zw,
					pos1 + (xdir * beamCoreSize), tex.xw,
					ccsColor
				);

			}

			// draw laser end
			pos1 = startPos + zdir * (beamTileMinDst + numBeamTiles * tileLength);
			pos2 = targetPos;
			tex.z = tex.x + (distance(pos1, pos2) / tileLength) * texSizeX;

			AddEffectsQuad(
				quadStartIndex++,
				vec3(1.0),
				pos1 - (xdir * beamEdgeSize), tex.xy,
				pos2 - (xdir * beamEdgeSize), tex.zy,
				pos2 + (xdir * beamEdgeSize), tex.zw,
				pos1 + (xdir * beamEdgeSize), tex.xw,
				ecsColor
			);

			AddEffectsQuad(
				quadStartIndex++,
				vec3(1.0),
				pos1 - (xdir * beamCoreSize), tex.xy,
				pos2 - (xdir * beamCoreSize), tex.zy,
				pos2 + (xdir * beamCoreSize), tex.zw,
				pos1 + (xdir * beamCoreSize), tex.xw,
				ccsColor
			);
		}
	}

	if (validTextures.y) {
		AddEffectsQuad(
			quadStartIndex++,
			vec3(1.0),
			pos2 - (xdir * beamEdgeSize)                        , texCoord2.xy,
			pos2 - (xdir * beamEdgeSize) + (ydir * beamEdgeSize), texCoord2.zy,
			pos2 + (xdir * beamEdgeSize) + (ydir * beamEdgeSize), texCoord2.zw,
			pos2 + (xdir * beamEdgeSize)                        , texCoord2.xw,
			ecsColor
		);

		AddEffectsQuad(
			quadStartIndex++,
			vec3(1.0),
			pos2 - (xdir * beamCoreSize)                        , texCoord2.xy,
			pos2 - (xdir * beamCoreSize) + (ydir * beamCoreSize), texCoord2.zy,
			pos2 + (xdir * beamCoreSize) + (ydir * beamCoreSize), texCoord2.zw,
			pos2 + (xdir * beamCoreSize)                        , texCoord2.xw,
			ccsColor
		);
	}

	if (validTextures.z) {
		float pulseStartTime = fract(frameInfo.z * pulseSpeed);
		float muzzleEdgeSize = thickness * flareSize * pulseStartTime;
		float muzzleCoreSize = muzzleEdgeSize * 0.6;

		vec4 coreColor; coreColor.a = 1.0 / 255.0;
		vec4 edgeColor; edgeColor.a = 1.0 / 255.0;

		coreColor.rgb = ccsColor.rgb * (1.0 - pulseStartTime);
		edgeColor.rgb = ecsColor.rgb * (1.0 - pulseStartTime);

		// draw muzzleflare
		pos1 = startPos - zdir * (thickness * flareSize) * 0.02;

		AddEffectsQuad(
			quadStartIndex++,
			vec3(1.0),
			pos1 + (ydir * muzzleEdgeSize)                          , texCoord3.xy,
			pos1 + (ydir * muzzleEdgeSize) + (zdir * muzzleEdgeSize), texCoord3.zy,
			pos1 - (ydir * muzzleEdgeSize) + (zdir * muzzleEdgeSize), texCoord3.zw,
			pos1 - (ydir * muzzleEdgeSize)                          , texCoord3.xw,
			edgeColor
		);

		AddEffectsQuad(
			quadStartIndex++,
			vec3(1.0),
			pos1 + (ydir * muzzleCoreSize)                          , texCoord3.xy,
			pos1 + (ydir * muzzleCoreSize) + (zdir * muzzleCoreSize), texCoord3.zy,
			pos1 - (ydir * muzzleCoreSize) + (zdir * muzzleCoreSize), texCoord3.zw,
			pos1 - (ydir * muzzleCoreSize)                          , texCoord3.xw,
			coreColor
		);

		pulseStartTime += 0.5;
		pulseStartTime -= 1.0 * float(pulseStartTime > 1.0);

		coreColor.rgb = ccsColor.rgb * (1.0 - pulseStartTime);
		edgeColor.rgb = ecsColor.rgb * (1.0 - pulseStartTime);

		muzzleEdgeSize = thickness * flareSize * pulseStartTime;

		AddEffectsQuad(
			quadStartIndex++,
			vec3(1.0),
			pos1 + (ydir * muzzleEdgeSize)                          , texCoord3.xy,
			pos1 + (ydir * muzzleEdgeSize) + (zdir * muzzleEdgeSize), texCoord3.zy,
			pos1 - (ydir * muzzleEdgeSize) + (zdir * muzzleEdgeSize), texCoord3.zw,
			pos1 - (ydir * muzzleEdgeSize)                          , texCoord3.xw,
			edgeColor
		);

		AddEffectsQuad(
			quadStartIndex++,
			vec3(1.0),
			pos1 + (ydir * muzzleCoreSize)                          , texCoord3.xy,
			pos1 + (ydir * muzzleCoreSize) + (zdir * muzzleCoreSize), texCoord3.zy,
			pos1 - (ydir * muzzleCoreSize) + (zdir * muzzleCoreSize), texCoord3.zw,
			pos1 - (ydir * muzzleCoreSize)                          , texCoord3.xw,
			coreColor
		);
	}
	
	if (validTextures.w) {
		// draw flare (moved slightly along the camera direction)
		pos1 = startPos - (camDirPos[2].xyz * 3.0);
		
		AddEffectsQuadCamera(
			quadStartIndex++,
			vec3(1.0),
			pos1, vec2(flareEdgeSize), texCoord4,
			ecsColor
		);
		
		AddEffectsQuadCamera(
			quadStartIndex++,
			vec3(1.0),
			pos1, vec2(flareCoreSize), texCoord4,
			ccsColor
		);
	}
]]
}