return {
	EarlyExit =
[[
	int syncedTime = int(createFrame - frameInfo.x);
	float life = lifeDecayRate * syncedTime;
	if (life >= 1.0)
		return;
]],
	MainCode =
[[
	// update time
	if (frameInfo.x != frameInfo.w) {
		pos += syncedTime * speed;
		speed = ValAddMulSteps(speed, gravity, vec3(airDrag), syncedTime);
		size = ValMulAddSteps(size, sizeMod, sizeGrowth, syncedTime);
		float rotVel = rotParams.x + rotParams.y * syncedTime;
		float rotVal = rotParams.z + rotVel      * syncedTime;
	}
]]
}