return {
	MainCode =
[[
	float a = max(0.0f, alpha - alphaDecay * frameInfo.y);
	color *= a;
	
	vec3 dif1 = normalize(pos - camPos);
	vec3 dir2 = normalize(cross(dif1, dir));
	
	vec3 l = (dir * length) + (lengthGrowth * frameInfo.y);
	vec3 w = (dir2 * width);
	
	vec3 drawPos = pos + speed * frameInfo.y;

	AddEffectsQuad(
		drawOrder,
		vec3(1.0),
		drawPos - l - w,
		drawPos + l - w,
		drawPos + l + w,
		drawPos - l + w,
		texCoord,
		color
	);
]]
}