return {
	MainCode =
[[
	vec4 lightYellow = GetColorFromIntegers(uvec4(255, 210, 180, 1));

	AddEffectsQuadCamera(
		drawOrder,
		vec3(1.0),
		pos, vec2(fsize), texCoord,
		lightYellow
	);
]]
}