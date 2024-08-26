return {
	MainCode =
[[
	float currTime = frameInfo.x + frameInfo.y - createFrame;

	SetCurrentAnimation(animParams, currTime);
	vec3 drawPos = pos + speed * frameInfo.y;
	float colMixRate = clamp((colEdge1 - curTime)/(colEdge1 - colEdge0), 0.0, 1.0);
	vec4 color = mix(color0, color1, colMixRate);

	AddEffectsQuadCamera(
		drawOrder,
		animParams,
		drawPos, vec2(size), texCoord,
		color
	);
]]
}