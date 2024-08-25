return {
	MainCode =
[[
	float alpha = max(0.0, 1.0 - (partAge / (4.0 + size * 30.0)));
	float fade  = clamp((1.0 - alpha) * (20.0 + aIndex) * 0.1, 0.0, 1.0);
	
	float modAge = sqrt(partAge + 2.0);
	vec2 drawSize = vec2(modAge * 3.0);

	vec3 interPos = pos + (aIndex + 2.0 + frameInfo.y) * randDir * modAge * 0.4;

	vec4 partColor1 = vec4(180, 180, 180, 255) * alpha * fade / 255.0;

	AddEffectsQuadCamera(
		drawOrder,
		vec3(1.0),
		interPos, drawSize, texCoord1,
		partColor1
	);
	
	if (fade < 1.0) {
		float ifade = 1.0f - fade;
		vec4 partColor2 = vec4(vec3(ifade), 1.0 / 255.0);
		
		AddEffectsQuadCamera(
			drawOrder,
			vec3(1.0),
			interPos, drawSize, texCoord2,
			partColor2
		);
	}
]]
}