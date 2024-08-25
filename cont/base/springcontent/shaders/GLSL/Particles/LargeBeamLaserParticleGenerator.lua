return {
	MainCode =
[[
	vec3 midPos = (targetPos + startPos) * 0.5;
	vec3 cameraDir = normalize(midPos - camPos);

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
				drawOrder,
				vec3(1.0),
				pos1 - (xdir * beamEdgeSize), tex.xy,
				pos2 - (xdir * beamEdgeSize), tex.zy,
				pos2 + (xdir * beamEdgeSize), tex.zw,
				pos1 + (xdir * beamEdgeSize), tex.xw,
				ecsColor
			);

			AddEffectsQuad(
				drawOrder,
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
				drawOrder,
				vec3(1.0),
				pos1 - (xdir * beamEdgeSize), tex.xy,
				pos2 - (xdir * beamEdgeSize), tex.zy,
				pos2 + (xdir * beamEdgeSize), tex.zw,
				pos1 + (xdir * beamEdgeSize), tex.xw,
				ecsColor
			);

			AddEffectsQuad(
				drawOrder,
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
					drawOrder,
					vec3(1.0),
					pos1 - (xdir * beamEdgeSize), tex.xy,
					pos2 - (xdir * beamEdgeSize), tex.zy,
					pos2 + (xdir * beamEdgeSize), tex.zw,
					pos1 + (xdir * beamEdgeSize), tex.xw,
					ecsColor
				);

				AddEffectsQuad(
					drawOrder,
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
				drawOrder,
				vec3(1.0),
				pos1 - (xdir * beamEdgeSize), tex.xy,
				pos2 - (xdir * beamEdgeSize), tex.zy,
				pos2 + (xdir * beamEdgeSize), tex.zw,
				pos1 + (xdir * beamEdgeSize), tex.xw,
				ecsColor
			);

			AddEffectsQuad(
				drawOrder,
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
			drawOrder,
			vec3(1.0),
			pos2 - (xdir * beamEdgeSize)                        , texCoord2.xy,
			pos2 - (xdir * beamEdgeSize) + (ydir * beamEdgeSize), texCoord2.zy,
			pos2 + (xdir * beamEdgeSize) + (ydir * beamEdgeSize), texCoord2.zw,
			pos2 + (xdir * beamEdgeSize)                        , texCoord2.xw,
			ecsColor
		);

		AddEffectsQuad(
			drawOrder,
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
			drawOrder,
			vec3(1.0),
			pos1 + (ydir * muzzleEdgeSize)                          , texCoord3.xy,
			pos1 + (ydir * muzzleEdgeSize) + (zdir * muzzleEdgeSize), texCoord3.zy,
			pos1 - (ydir * muzzleEdgeSize) + (zdir * muzzleEdgeSize), texCoord3.zw,
			pos1 - (ydir * muzzleEdgeSize)                          , texCoord3.xw,
			edgeColor
		);

		AddEffectsQuad(
			drawOrder,
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
			drawOrder,
			vec3(1.0),
			pos1 + (ydir * muzzleEdgeSize)                          , texCoord3.xy,
			pos1 + (ydir * muzzleEdgeSize) + (zdir * muzzleEdgeSize), texCoord3.zy,
			pos1 - (ydir * muzzleEdgeSize) + (zdir * muzzleEdgeSize), texCoord3.zw,
			pos1 - (ydir * muzzleEdgeSize)                          , texCoord3.xw,
			edgeColor
		);

		AddEffectsQuad(
			drawOrder,
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
		pos1 = startPos - (camDir[2].xyz * 3.0);

		AddEffectsQuadCamera(
			drawOrder,
			vec3(1.0),
			pos1, vec2(flareEdgeSize), texCoord4,
			ecsColor
		);

		AddEffectsQuadCamera(
			drawOrder,
			vec3(1.0),
			pos1, vec2(flareCoreSize), texCoord4,
			ccsColor
		);
	}
]]
}