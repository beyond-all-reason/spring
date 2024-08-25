return {
	MainCode =
[[
	vec4 partColor = vec4(alpha);
	
	float interSize = size + sizeExpansion * frameInfo.y;

	AddEffectsQuadCamera(
		vec3(1.0),
		partPos, vec2(interSize), texCoord,
		partColor
	);
]]
}