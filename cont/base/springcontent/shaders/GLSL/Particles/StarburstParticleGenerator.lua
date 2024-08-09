return {
	InputData =
[[
#define NUM_TRACER_PARTS 3u
#define MAX_NUM_AGEMODS 20u
struct InputData {
	vec4 info0; // .xyz partPos, .w missileAge
	vec4 info1; // .xyz partSpeed, .w curTracerPart(uint)
	vec4 info2; // .x drawOrder
	vec4 info3[NUM_TRACER_PARTS]; // .xyz tracerPos, .w speedf
	vec4 info4[NUM_TRACER_PARTS]; // .xyz tracerDir, .w numAgeMods(unit)
	vec4 info5[NUM_TRACER_PARTS * MAX_NUM_AGEMODS]; // all age mods
	vec4 info6; // texCoord1
	vec4 info7; // texCoord3
};
]],
	InputDefs =
[[
#define partPos       dataIn[gl_GlobalInvocationID.x].info0.xyz
#define missileAge    dataIn[gl_GlobalInvocationID.x].info0.w

#define partSpeed     dataIn[gl_GlobalInvocationID.x].info1.xyz
#define curTracerPart floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info1.w)

#define drawOrder     dataIn[gl_GlobalInvocationID.x].info2.x

#define TracerPos(IDX) dataIn[gl_GlobalInvocationID.x].info3[IDX].xyz
#define Speedf(IDX)    dataIn[gl_GlobalInvocationID.x].info3[IDX].w

#define TracerDir(IDX)  dataIn[gl_GlobalInvocationID.x].info4[IDX].xyz
#define numAgeMods(IDX) floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info4[IDX].w)

#define AgeMod(IDX) dataIn[gl_GlobalInvocationID.x].info5[(IDX >> 2)][(IDX % 4)]

#define texCoord1   dataIn[gl_GlobalInvocationID.x].info6
#define texCoord3   dataIn[gl_GlobalInvocationID.x].info7
]],
	EarlyExit =
[[
	bvec2 validTextures = bvec2(
		(texCoord1.z - texCoord1.x) * (texCoord1.w - texCoord1.y) > 0.0,
		(texCoord3.z - texCoord3.x) * (texCoord3.w - texCoord3.y) > 0.0
	);

	if (!any(validTextures))
		return;
]],
	NumQuads =
[[
	20 * 3 * uint(validTextures.x) + 1 * uint(validTextures.y)
]],
	MainCode =
[[
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
			vec3(1.0),
			partPos, fsize2, texCoord1,
			lightRed
		);
	}
]]
}