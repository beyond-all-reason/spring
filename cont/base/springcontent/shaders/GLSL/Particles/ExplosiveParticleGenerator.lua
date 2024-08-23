return {
	InputData =
[[
struct InputData {
	vec4  info0; // .xyz pos, .w radius
	vec4  info1; // .xyz dir, .w drawOrder(as int, but pass as is)
	uvec4 info2; // .x color0, .y color1, .z numStages, .w noGap;
	vec4  info3; //	.xyz animParams, .w alphaDecay;
	vec4  info4; // .xyz rotParams, .w sizeDecay;
	vec4  info5; // .x separation, .y edge0, .z edge1, .w - curTime
	vec4  info6; // texCoord
};
]],
	InputDefs =
[[
#define drawPos    dataIn[gl_GlobalInvocationID.x].info0.xyz
#define drawRadius dataIn[gl_GlobalInvocationID.x].info0.w

#define dir        dataIn[gl_GlobalInvocationID.x].info1.xyz
#define drawOrder  dataIn[gl_GlobalInvocationID.x].info1.w

#define color0     dataIn[gl_GlobalInvocationID.x].info2.x
#define color1     dataIn[gl_GlobalInvocationID.x].info2.y
#define numStages  dataIn[gl_GlobalInvocationID.x].info2.z
#define noGap      floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info2.w)

#define animParams dataIn[gl_GlobalInvocationID.x].info3.xyz
#define alphaDecay dataIn[gl_GlobalInvocationID.x].info3.w

#define rotParams  dataIn[gl_GlobalInvocationID.x].info4.xyz
#define sizeDecay  dataIn[gl_GlobalInvocationID.x].info4.w

#define separation dataIn[gl_GlobalInvocationID.x].info5.x
#define edge0      dataIn[gl_GlobalInvocationID.x].info5.y
#define edge1      dataIn[gl_GlobalInvocationID.x].info5.z
#define curTime    dataIn[gl_GlobalInvocationID.x].info5.w

#define texCoord   dataIn[gl_GlobalInvocationID.x].info6
]],
	EarlyExit =
[[
	if ((texCoord.z - texCoord.x) * (texCoord.w - texCoord.y) <= 0.0)
		return;
]],
	NumQuads =
[[
	numStages
]],
	MainCode =
[[
	vec4 col0 = GetPackedColor(color0);
	vec4 col1 = GetPackedColor(color1);

	float colMixRate = clamp((edge1 - curTime)/(edge1 - edge0), 0.0, 1.0);
	vec4 col = mix(col0, col1, colMixRate);

	float invStages  = 1.0 / max(1u, numStages);
	vec3 ndir = dir * separation * 0.6f;

	//ignore animation and rotation for now

	for (uint stage = 0u; stage < numStages; ++stage) {
		float stageDecay = (numStages - (stage * alphaDecay)) * invStages;
		float stageSize  = drawRadius * (1.0f - (stage * sizeDecay));

		vec3 xdirCam  = camDirPos[0].xyz * stageSize;
		vec3 ydirCam  = camDirPos[1].xyz * stageSize;

		vec3 stageGap = (noGap > 0) ? (ndir * stageSize * stage) : (ndir * drawRadius * stage);
		vec3 stagePos = drawPos - stageGap;

		col *= stageDecay;

		AddEffectsQuad(
			animParams,
			stagePos - xdirCam - ydirCam,
			stagePos + xdirCam - ydirCam,
			stagePos + xdirCam + ydirCam,
			stagePos - xdirCam + ydirCam,
			texCoord,
			col
		);
	}
]]
}