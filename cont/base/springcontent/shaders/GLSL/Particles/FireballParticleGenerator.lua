return {
	MainCode =
[[
	int numFire = min(10, numSparks);

	if (validTextures.x) {
		for (int i = 0; i < numSparks; ++i) {
			vec4 sparkColor = GetColorFromIntegers(uvec4(
				(numSparks - i) * 12,
				(numSparks - i) *  6,
				(numSparks - i) *  4,
				1
			));

			AddEffectsQuadCamera(
				drawOrder,
				animParams1,
				SparkPos(i), vec2(SparkSize(i)), texCoord1,
				sparkColor
			);
		}
	}

	if (validTextures.y) {
		vec3 interPos = mix(dgunPos, dgunPos + frameInfo.y * speed, checkCol);

		int maxCol = int(mix(float(numFire), 10.0, checkCol));

		for (int i = 0; i < numFire; ++i) {
			vec4 dgunColor = GetColorFromIntegers(uvec4(
				(maxCol - i) * 25,
				(maxCol - i) * 15,
				(maxCol - i) * 10,
				1
			));

			AddEffectsQuadCamera(
				drawOrder,
				animParams2,
				interPos - (speed * 0.5 * i), vec2(dgunSize), texCoord2,
				dgunColor
			);
		}
	}
]]
}