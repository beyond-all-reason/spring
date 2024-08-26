return {
	MainCode =
[[
	float currTime = frameInfo.x + frameInfo.y - createFrame;

	SetCurrentAnimation(animParams, currTime);
	vec3 drawPos = pos + speed * frameInfo.y;
	
	// rotParams do we need it?

	AddEffectsQuadCamera(
		drawOrder,
		animParams,
		drawPos, vec2(drawRadius), texCoord,
		color
	);
]]
}