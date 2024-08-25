return {
	MainCode =
[[
	vec4 partColor = GetPackedColor(partCol);
	partColor.rgb *= partColor.a;

	vec3 dif1 = normalize(partP1 - camPos);
	vec3 dir1 = normalize(cross(dif1, partV1));

	vec3 dif2 = normalize(partP2 - camPos);
	vec3 dir2 = normalize(cross(dif2, partV2));
	
	float u = (texCoord.x + texCoord.z) * 0.5;
	float v0 = texCoord.y;
	float v1 = texCoord.w;
	
	if (partW2 != 0.0) {
		AddEffectsQuad(
			drawOrder,
			vec3(1.0),
			partP1 - dir1 * partW1, vec2(u, v1),
			partP1 + dir1 * partW1, vec2(u, v0),
			partP2 + dir2 * partW2, vec2(u, v0),
			partP2 - dir2 * partW2, vec2(u, v1),
			partColor
		);
	} else {
		AddEffectsQuad(
			drawOrder,
			vec3(1.0),
			partP1 - dir1 * partW1, vec2(u, v1                  ),
			partP1 + dir1 * partW1, vec2(u, v0                  ),
			partP2                , vec2(u, v0 + (v1 - v0) * 0.5),
			partP2                , vec2(u, v0 + (v1 - v0) * 1.5),
			partColor
		);
	}
]]
}