return {
	MainCode =
[[
	vec4 col = GetPackedColor(color);
	
	AddEffectsQuadCamera(
		animParams,
		drawPos, vec2(drawRadius), texCoord,
		col
	);
]]
}