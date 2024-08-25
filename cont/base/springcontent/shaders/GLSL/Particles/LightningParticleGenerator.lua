return {
	MainCode =
[[
	vec4 col = GetPackedColor(color);

	vec3 ddir = normalize(targetPos - startPos);
	vec3 dif  = normalize(startPos - camPos);
	vec3 dir1 = normalize(cross(dif, ddir));

	vec3 tempPos;

	tempPos = startPos;
	for (uint d = 1u; d < 12u - 1u; ++d) {
		float f = (d + 1u) * (1.0 / (12u - 1u));
		vec3 tempPosO = tempPos;
		tempPos = (startPos * (1.0 - f)) + (targetPos * f);

		AddEffectsQuad(
			drawOrder,
			vec3(1.0),
			tempPosO + (dir1 * (Displacement1(d    ) + thickness)), texCoord.xy,
			tempPos  + (dir1 * (Displacement1(d + 1) + thickness)), texCoord.zy,
			tempPos  + (dir1 * (Displacement1(d + 1) - thickness)), texCoord.zw,
			tempPosO + (dir1 * (Displacement1(d    ) - thickness)), texCoord.xw,
			col
		);
	}
	
	tempPos = startPos;
	for (uint d = 1u; d < 12u - 1u; ++d) {
		float f = (d + 1u) * (1.0 / (12u - 1u));
		vec3 tempPosO = tempPos;
		tempPos = (startPos * (1.0 - f)) + (targetPos * f);

		AddEffectsQuad(
			drawOrder,
			vec3(1.0),
			tempPosO + (dir1 * (Displacement2(d    ) + thickness)), texCoord.xy,
			tempPos  + (dir1 * (Displacement2(d + 1) + thickness)), texCoord.zy,
			tempPos  + (dir1 * (Displacement2(d + 1) - thickness)), texCoord.zw,
			tempPosO + (dir1 * (Displacement2(d    ) - thickness)), texCoord.xw,
			col
		);
	}
]]
}