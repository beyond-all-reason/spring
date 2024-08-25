return {
	MainCode =
[[
	float currTime = frameInfo.x + frameInfo.y - createFrame;

	float rotVal  = GetCurrentRotation(rotParams, currTime);
	float animVal = GetCurrentAnimation(animParams, currTime);

	float dheat = max(0.0, heat - frameInfo.y);
	float alpha = (dheat / maxHeat);

	vec4 partColor = vec4(vec3(alpha), 1.0 / 255.0);

	float drawSize = (size + sizeGrowth * frameInfo.y) * (1.0 - sizeMod);

	vec2 sc = vec2(sin(rotVal), cos(rotVal));

	vec3 bounds[4] = vec3[4](
		vec3(-camDir[0] - camDir[1]) * drawSize,
		vec3( camDir[0] - camDir[1]) * drawSize,
		vec3( camDir[0] + camDir[1]) * drawSize,
		vec3(-camDir[0] + camDir[1]) * drawSize
	);

	for (uint i = 0u; i < uint(bounds.length()); ++i) {
		bounds[i] = Rotate(sc, camDir[2], bounds[i]);
	}

	vec3 drawPos = partPos + partSpd * frameInfo.y;
	AddEffectsQuad(
		vec3(animParams.xy, animVal),
		drawPos + bounds[0],
		drawPos + bounds[1],
		drawPos + bounds[2],
		drawPos + bounds[3],
		texCoord,
		partColor
	);
]]
}