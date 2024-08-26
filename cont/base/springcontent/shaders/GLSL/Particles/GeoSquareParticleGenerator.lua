return {
	MainCode =
[[
	color.rgb *= color.a;

	vec3 dif1 = normalize(p1 - camPos);
	vec3 dir1 = normalize(cross(dif1, vctr1));

	vec3 dif2 = normalize(p2 - camPos);
	vec3 dir2 = normalize(cross(dif2, vctr2));
	
	float u = (texCoord.x + texCoord.z) * 0.5;
	float v0 = texCoord.y;
	float v1 = texCoord.w;
	
	if (w2 != 0.0) {
		AddEffectsQuad(
			drawOrder,
			vec3(1.0),
			p1 - dir1 * w1, vec2(u, v1),
			p1 + dir1 * w1, vec2(u, v0),
			p2 + dir2 * w2, vec2(u, v0),
			p2 - dir2 * w2, vec2(u, v1),
			color
		);
	} else {
		AddEffectsQuad(
			drawOrder,
			vec3(1.0),
			p1 - dir1 * w1, vec2(u, v1                  ),
			p1 + dir1 * w1, vec2(u, v0                  ),
			p2                , vec2(u, v0 + (v1 - v0) * 0.5),
			p2                , vec2(u, v0 + (v1 - v0) * 1.5),
			color
		);
	}
]]
}