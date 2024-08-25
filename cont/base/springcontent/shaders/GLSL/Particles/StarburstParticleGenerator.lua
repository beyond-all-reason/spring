return {
	MainCode =
[[
	const uint NUM_TRACER_PARTS = 3u;
	const uint MAX_NUM_AGEMODS = 20u;

	vec4 lightYellow = GetColorFromIntegers(uvec4(255, 200, 150, 1));
	vec4 lightRed = GetColorFromIntegers(uvec4(255, 180, 180, 1));

	uint partNum = curTracerPart;

	const float TRACER_PARTS_STEP = 0.2;

	if (validTextures.y) {
		for (uint a = 0u; a < NUM_TRACER_PARTS; ++a) {
			vec3 opos = TracerPos(partNum);
			vec3 odir = TracerDir(partNum);
			float ospeed = Speedf(partNum);

			float curStep = 0.0;
			uint thisNumAgeMods = numAgeMods(partNum);
			for (int ageModIdx = 0; ageModIdx < thisNumAgeMods; ++ageModIdx) {
				curStep += TRACER_PARTS_STEP;
				float ageMod = AgeMod(partNum * MAX_NUM_AGEMODS + ageModIdx);
				float age2 = (a + (curStep / (ospeed + 0.01))) * 0.2;
				float drawsize = 1.0f + age2 * 0.8f * ageMod * 7;
				float alpha = (missileAge >= 20) ? ((1.0 - age2) * max(0.0, 1.0 - age2)) : (1.0 - age2) * (1.0 - age2);

				vec3 interPos = opos - (odir * ((a * 0.5) + curStep));

				vec4 col = lightYellow; col.rgb *= clamp(alpha, 0.0, 1.0);

				AddEffectsQuadCamera(
					drawOrder,
					vec3(1.0),
					interPos, vec2(drawsize), texCoord3,
					col
				);
			}
		}
		// unsigned, so LHS will wrap around to UINT_MAX
		partNum = min(partNum - 1, NUM_TRACER_PARTS - 1);
	}
	
	if (validTextures.x) {
		const vec2 fsize2 = vec2(25.0);

		AddEffectsQuadCamera(
			drawOrder,
			vec3(1.0),
			pos, fsize2, texCoord1,
			lightRed
		);
	}
]]
}