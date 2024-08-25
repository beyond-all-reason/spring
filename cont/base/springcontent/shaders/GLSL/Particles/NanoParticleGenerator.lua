return {
	MainCode =
[[
	float currTime = frameInfo.x + frameInfo.y - createFrame;

	float rotVal  = GetCurrentRotation(rotParams, currTime);
	float animVal = GetCurrentAnimation(animParams, currTime);
	
	vec4 partColor = GetPackedColor(partCol);

	vec3 bounds[4] = vec3[4](
		vec3(-camDir[0] - camDir[1]) * partSize,
		vec3( camDir[0] - camDir[1]) * partSize,
		vec3( camDir[0] + camDir[1]) * partSize,
		vec3(-camDir[0] + camDir[1]) * partSize
	);

	vec2 sc = vec2(sin(rotVal), cos(rotVal));	
	for (uint i = 0u; i < uint(bounds.length()); ++i) {
		bounds[i] = Rotate(sc, camDir[2], bounds[i]);
	}
	
	vec3 drawPos = partPos + partSpeed * frameInfo.y;

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