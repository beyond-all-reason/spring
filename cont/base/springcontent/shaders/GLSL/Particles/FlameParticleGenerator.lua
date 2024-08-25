return {
	MainCode =
[[
	vec4 col0 = GetPackedColor(color0);
	vec4 col1 = GetPackedColor(color1);

	float colMixRate = clamp((colEdge1 - curTime)/(colEdge1 - colEdge0), 0.0, 1.0);
	vec4 col = mix(col0, col1, colMixRate);

	AddEffectsQuadCamera(
		drawOrder,
		animParams,
		drawPos, vec2(drawRadius), texCoord,
		col
	);
]]
}