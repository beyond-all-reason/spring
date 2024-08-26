return {
	MainCode =
[[
	vec3 drawPos = pos + speed * frameInfo.y;

	vec4 color = GetCurrentColor(color0, color1, colEdge0, colEdge1, curTime);

	float invStages  = 1.0 / max(1u, numStages);
	vec3 ndir = dir * separation * 0.6f;

	//ignore animation and rotation for now

	for (uint stage = 0u; stage < numStages; ++stage) {
		float stageDecay = (numStages - (stage * alphaDecay)) * invStages;
		float stageSize  = drawRadius * (1.0f - (stage * sizeDecay));

		vec3 stageGap = (noGap > 0) ? (ndir * stageSize * stage) : (ndir * drawRadius * stage);
		vec3 stagePos = drawPos - stageGap;

		color *= stageDecay;

		AddEffectsQuad(
			drawOrder,
			animParams,
			stagePos + (-camDir[0] - camDir[1]) * stageSize,
			stagePos + ( camDir[0] - camDir[1]) * stageSize,
			stagePos + ( camDir[0] + camDir[1]) * stageSize,
			stagePos + (-camDir[0] + camDir[1]) * stageSize,
			texCoord,
			color
		);
	}
]]
}