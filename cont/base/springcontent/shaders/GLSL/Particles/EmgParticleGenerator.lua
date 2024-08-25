return {
	MainCode =
[[
	vec4 col = GetPackedColor(color);
	
	AddEffectsQuadCamera(
		drawOrder,
		animParams,
		drawPos, vec2(drawRadius), texCoord,
		col
	);
]]
}