return {
	MainCode =
[[
	vec4 partColor = GetPackedColor(partCol);
	float a = max(0.0f, alpha - alphaDecay * frameInfo.y);
	partColor *= a;
	
	vec3 dif1 = normalize(partPos - camPos);
	vec3 dir2 = normalize(cross(dif1, partDir));
	
	vec3 l = (partDir * partLen) + (partLenGrowth * frameInfo.y);
	vec3 w = (dir2 * partWidth);
	
	vec3 drawPos = partPos + partSpd * frameInfo.y;

	AddEffectsQuad(
		vec3(1.0),
		drawPos - l - w,
		drawPos + l - w,
		drawPos + l + w,
		drawPos - l + w,
		texCoord,
		partColor
	);
]]
}