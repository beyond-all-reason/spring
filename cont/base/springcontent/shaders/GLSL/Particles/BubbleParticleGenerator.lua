return {
	MainCode =
[[
	vec4 partColor = vec4(alpha);
	
	float interSize = size + sizeExpansion * frameInfo.y;

	AddEffectsQuadCamera(
		drawOrder,
		vec3(1.0),
		pos, vec2(interSize), texCoord,
		partColor
	);
]]
}