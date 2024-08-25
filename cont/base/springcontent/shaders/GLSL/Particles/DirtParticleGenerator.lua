return {
	EarlyExit =
[[
	float partAbove = partPos.y / (size * camPos.y);
	
	if (partAbove < -1.0)
		return;
	
	partAbove = min(partAbove, 1.0);
]],
	MainCode =
[[
	vec4 partColor =  GetPackedColor(partCol);
	partColor *= alpha;
	
	float interSize = size + sizeExpansion * frameInfo.y;
	float texx = texCoord.x + (texCoord.z - texCoord.x) * ((1.0 - partAbove) * 0.5);
	
	vec3 drawPos = partPos + partSpd * frameInfo.y;

	AddEffectsQuad(
		vec3(1.0),
		drawPos - camDir[0] * interSize - camDir[1] * interSize * partAbove, vec2(texx,       texCoord.y),
		drawPos - camDir[0] * interSize + camDir[1] * interSize            , vec2(texCoord.z, texCoord.y),
		drawPos + camDir[0] * interSize + camDir[1] * interSize            , vec2(texCoord.z, texCoord.w),
		drawPos + camDir[0] * interSize - camDir[1] * interSize * partAbove, vec2(texx,       texCoord.w),
		partColor
	);
]]
}