return {
	InputData =
[[
struct InputData {
	vec4 info0; // .xyz partPos, .w maxHeat
	vec4 info1; // .xyz speed, .w heat
	vec4 info2; // .xyz animParams, .w size
	vec4 info3; // .xyz rotParams, .w sizeGrowth
	vec4 info4; // .x sizeMod, .y drawOrder, .z createFrame, .w unused
	vec4 info5; // texCoord
};
]],
	InputDefs =
[[
#define partPos    dataIn[gl_GlobalInvocationID.x].info0.xyz
#define maxHeat    dataIn[gl_GlobalInvocationID.x].info0.w

#define partSpd    dataIn[gl_GlobalInvocationID.x].info1.xyz
#define heat       dataIn[gl_GlobalInvocationID.x].info1.w

#define animParams dataIn[gl_GlobalInvocationID.x].info2.xyz
#define size       dataIn[gl_GlobalInvocationID.x].info2.w

#define rotParams  dataIn[gl_GlobalInvocationID.x].info3.xyz
#define sizeGrowth dataIn[gl_GlobalInvocationID.x].info3.w

#define sizeMod     dataIn[gl_GlobalInvocationID.x].info4.x
#define drawOrder   dataIn[gl_GlobalInvocationID.x].info4.y
#define createFrame floatBitsToInt(dataIn[gl_GlobalInvocationID.x].info4.z)

#define texCoord   dataIn[gl_GlobalInvocationID.x].info5
]],
	EarlyExit =
[[
	if ((texCoord.z - texCoord.x) * (texCoord.w - texCoord.y) <= 0.0)
		return;
]],
	NumQuads =
[[
	1
]],
	MainCode =
[[
	float currTime = frameInfo.x + frameInfo.y - createFrame;

	float rotVal  = GetCurrentRotation(rotParams, currTime);
	float animVal = GetCurrentAnimation(animParams, currTime);

	float dheat = max(0.0, heat - frameInfo.y);
	float alpha = (dheat / maxHeat);

	vec4 partColor = vec4(vec3(alpha), 1.0 / 255.0);

	float drawSize = (size + sizeGrowth * frameInfo.y) * (1.0 - sizeMod);

	vec2 sc = vec2(sin(rotVal), cos(rotVal));

	vec3 bounds[4] = vec3[4](
		vec3(-camDir[0] - camDir[1]) * drawSize,
		vec3( camDir[0] - camDir[1]) * drawSize,
		vec3( camDir[0] + camDir[1]) * drawSize,
		vec3(-camDir[0] + camDir[1]) * drawSize
	);

	for (uint i = 0u; i < uint(bounds.length()); ++i) {
		bounds[i] = Rotate(sc, camDir[2], bounds[i]);
	}

	vec3 drawPos = partPos + partSpd * frameInfo.y;
	AddEffectsQuad(
		vec3(animParams.xy, animVal),
		drawPos + bounds[0],
		drawPos + bounds[1],
		drawPos + bounds[2],
		drawPos + bounds[3],
		texCoord,
		partColor
	);
]]
}