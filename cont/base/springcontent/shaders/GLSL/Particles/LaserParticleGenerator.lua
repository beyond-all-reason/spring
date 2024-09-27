return {
	MainCode =
[[
	vec3 drawPos = pos + speed.xyz * frameInfo.y;
	vec3 dif = drawPos - camPos;
	float camDist = length(dif);
	dif /= camDist;

	vec3 dir1 = normalize(cross(dif, dir ));
	vec3 dir2 = normalize(cross(dif, dir1));

	float size = thickness;
	float coresize = size * coreThickness;
	float midtexx = 0.5 * (texCoord2.x + texCoord2.z);

	if (camDist < lodDistance) {
		vec3 pos2 = drawPos - (dir * curLength);

		float texStartOffset;
		float texEndOffset;

		if (checkCol == 1) { // expanding or contracting?
			texStartOffset = 0.0;
			texEndOffset   = ( 1.0 - (curLength / maxLength)) * (texCoord1.x - texCoord1.z);
		} else {
			texStartOffset = (-1.0 + (curLength / maxLength) + (stayTime * (speed.w / maxLength))) * (texCoord1.x - texCoord1.z);
			texEndOffset   = (stayTime * (speed.w / maxLength)) * (texCoord1.x - texCoord1.z);
		}

		if (validTextures.y) {
			AddEffectsQuad(
				drawOrder,
				vec3(1.0),
				drawPos - (dir1 * size) - (dir2 * size), vec2(texCoord2.x, texCoord2.y),
				drawPos - (dir1 * size)                , vec2(midtexx,     texCoord2.y),
				drawPos + (dir1 * size)                , vec2(midtexx,     texCoord2.w),
				drawPos + (dir1 * size) - (dir2 * size), vec2(texCoord2.x, texCoord2.w),
				color1
			);

			AddEffectsQuad(
				drawOrder,
				vec3(1.0),
				drawPos - (dir1 * coresize) - (dir2 * coresize), vec2(texCoord2.x, texCoord2.y),
				drawPos - (dir1 * coresize)                    , vec2(midtexx    , texCoord2.y),
				drawPos + (dir1 * coresize)                    , vec2(midtexx    , texCoord2.w),
				drawPos + (dir1 * coresize) - (dir2 * coresize), vec2(texCoord2.x, texCoord2.w),
				color2
			);
		}

		if (validTextures.x) {
			AddEffectsQuad(
				drawOrder,
				vec3(1.0),
				drawPos - (dir1 * size), vec2(texCoord1.x + texStartOffset, texCoord1.y),
				pos2    - (dir1 * size), vec2(texCoord1.z + texEndOffset  , texCoord1.y),
				pos2    + (dir1 * size), vec2(texCoord1.z + texEndOffset  , texCoord1.w),
				drawPos + (dir1 * size), vec2(texCoord1.x + texStartOffset, texCoord1.w),
				color1
			);

			AddEffectsQuad(
				drawOrder,
				vec3(1.0),
				drawPos - (dir1 * coresize), vec2(texCoord1.x + texStartOffset, texCoord1.y),
				pos2    - (dir1 * coresize), vec2(texCoord1.z + texEndOffset  , texCoord1.y),
				pos2    + (dir1 * coresize), vec2(texCoord1.z + texEndOffset  , texCoord1.w),
				drawPos + (dir1 * coresize), vec2(texCoord1.x + texStartOffset, texCoord1.w),
				color2
			);
		}

		if (validTextures.y) {
			AddEffectsQuad(
				drawOrder,
				vec3(1.0),
				pos2 - (dir1 * size)                , vec2(midtexx    , texCoord2.y),
				pos2 - (dir1 * size) + (dir2 * size), vec2(texCoord2.z, texCoord2.y),
				pos2 + (dir1 * size) + (dir2 * size), vec2(texCoord2.z, texCoord2.w),
				pos2 + (dir1 * size)                , vec2(midtexx    , texCoord2.w),
				color1
			);

			AddEffectsQuad(
				drawOrder,
				vec3(1.0),
				pos2 - (dir1 * coresize)                    , vec2(midtexx    , texCoord2.y),
				pos2 - (dir1 * coresize) + (dir2 * coresize), vec2(texCoord2.z, texCoord2.y),
				pos2 + (dir1 * coresize) + (dir2 * coresize), vec2(texCoord2.z, texCoord2.w),
				pos2 + (dir1 * coresize)                    , vec2(midtexx    , texCoord2.w),
				color2
			);
		}
	} else {
		vec3 pos1 = drawPos + (dir * (size * 0.5));
		vec3 pos2 = pos1 - (dir * (curLength + size));

		float texStartOffset;
		float texEndOffset;

		if (checkCol == 1) { // expanding or contracting?
			texStartOffset = 0.0;
			texEndOffset   = ( 1.0 - (curLength / maxLength)) * (texCoord1.x - texCoord1.z);
		} else {
			texStartOffset = (-1.0 + (curLength / maxLength) + (stayTime * (speed.w / maxLength))) * (texCoord1.x - texCoord1.z);
			texEndOffset   = (stayTime * (speed.w / maxLength)) * (texCoord1.x - texCoord1.z);
		}

		if (validTextures.x) {
			AddEffectsQuad(
				drawOrder,
				vec3(1.0),
				pos1 - (dir1 * size), vec2(texCoord1.x + texStartOffset, texCoord1.y),
				pos2 - (dir1 * size), vec2(texCoord1.z + texEndOffset  , texCoord1.y),
				pos2 + (dir1 * size), vec2(texCoord1.z + texEndOffset  , texCoord1.w),
				pos1 + (dir1 * size), vec2(texCoord1.x + texStartOffset, texCoord1.w),
				color1
			);

			AddEffectsQuad(
				drawOrder,
				vec3(1.0),
				pos1 - (dir1 * coresize), vec2(texCoord1.x + texStartOffset, texCoord1.y),
				pos2 - (dir1 * coresize), vec2(texCoord1.z + texEndOffset  , texCoord1.y),
				pos2 + (dir1 * coresize), vec2(texCoord1.z + texEndOffset  , texCoord1.w),
				pos1 + (dir1 * coresize), vec2(texCoord1.x + texStartOffset, texCoord1.w),
				color2
			);
		}
	}
]]
}