return {
	MainCode =
[[
	vec4 lightYellow = GetColorFromIntegers(uvec4(255, 210, 180, 1));

	AddEffectsQuadCamera(
		vec3(1.0),
		partPos, vec2(fsize), texCoord,
		lightYellow
	);
]]
}