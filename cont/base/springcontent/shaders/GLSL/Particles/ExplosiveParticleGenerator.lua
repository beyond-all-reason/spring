return {
	MainCode =
[[
	vec4 col0 = GetPackedColor(color0);
	vec4 col1 = GetPackedColor(color1);

	float colMixRate = clamp((edge1 - curTime)/(edge1 - edge0), 0.0, 1.0);
	vec4 col = mix(col0, col1, colMixRate);

	float invStages  = 1.0 / max(1u, numStages);
	vec3 ndir = dir * separation * 0.6f;

	//ignore animation and rotation for now

	for (uint stage = 0u; stage < numStages; ++stage) {
		float stageDecay = (numStages - (stage * alphaDecay)) * invStages;
		float stageSize  = drawRadius * (1.0f - (stage * sizeDecay));

		vec3 stageGap = (noGap > 0) ? (ndir * stageSize * stage) : (ndir * drawRadius * stage);
		vec3 stagePos = drawPos - stageGap;

		col *= stageDecay;

		AddEffectsQuad(
			drawOrder,
			animParams,
			stagePos + (-camDir[0] - camDir[1]) * stageSize,
			stagePos + ( camDir[0] - camDir[1]) * stageSize,
			stagePos + ( camDir[0] + camDir[1]) * stageSize,
			stagePos + (-camDir[0] + camDir[1]) * stageSize,
			texCoord,
			col
		);
	}
]]
}