return {
	EarlyExit =
[[
	int syncedTime = int(frameInfo.x - createFrame);
	float currTime = syncedTime + frameInfo.y;
	float age = ageRate * syncedTime;
	if (age >= 1.0)
		return;
]],
	MainCode =
[[
	bool doUpdate = (frameInfo.x > frameInfo.w);
	if (doUpdate) {
		pos += speed;
		pos += windVec * age * 0.05;
		size += sizeExpansion;
		size += ((startSize - size) * 0.2 * float(size < startSize));
		
		SavePos(pos);
		SaveSize(size);
	}

	float alpha = (1.0 - age);
	color *= alpha;

	SetCurrentAnimation(animParams, currTime);
	vec3 drawPos = pos + speed * frameInfo.y;
	float drawSize = size + sizeExpansion * frameInfo.y;
	
	// rotParams do we need it?

	AddEffectsQuadCamera(
		drawOrder,
		animParams,
		drawPos, vec2(drawSize), texCoord,
		color
	);
]]
}