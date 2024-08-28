return {
	EarlyExit =
[[
	int syncedTime = int(createFrame - frameInfo.x);
	float currTime = syncedTime + frameInfo.y;
	float life = lifeDecayRate * syncedTime;
	if (life >= 1.0)
		return;
]],
	MainCode =
[[
	// update time
	if (frameInfo.x != frameInfo.w) {
		speed = ValAddMulSteps(speed, gravity, vec3(airDrag), syncedTime);
		size = ValMulAddSteps(size, sizeMod, sizeGrowth, syncedTime);
	}

	vec4 color = GetCurrentColorFromColorMap(colMapOfft, colMapSize, life);

	pos += currTime * speed;
	vec3 drawPos = pos + speed * frameInfo.y;

	float rotVal = GetCurrentRotation(rotParams, currTime);
	vec2 sc = vec2(sin(rotVal), cos(rotVal));

	vec3 xDir;
	vec3 yDir;
	vec3 zDir;

	if (directional > 0) {
		zDir = normalize(pos - camPos);
		yDir = cross(zDir, speed);
		float yDirLen = length(yDir);
		if (yDirLen > 0.1) {
			yDir /= yDirLen;
			xDir = cross(yDir, zDir);
		} else {
			xDir = camDir[0];
			yDir = camDir[1];
			zDir = camDir[2];
		}
	} else {
		xDir = camDir[0];
		yDir = camDir[1];
		zDir = camDir[2];
	}

	vec3 bounds[4] = vec3[4](
		vec3(-yDir - xDir) * size,
		vec3(-yDir + xDir) * size,
		vec3( yDir + xDir) * size,
		vec3( yDir - xDir) * size
	);

	for (uint i = 0u; i < uint(bounds.length()); ++i) {
		bounds[i] = Rotate(sc, camDir[2], bounds[i]);
	}

	AddEffectsQuad(
		drawOrder,
		animParams,
		drawPos + bounds[0],
		drawPos + bounds[1],
		drawPos + bounds[2],
		drawPos + bounds[3],
		texCoord,
		color
	);
]]
}