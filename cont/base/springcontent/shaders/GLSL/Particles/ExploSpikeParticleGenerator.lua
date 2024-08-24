return {
	InputData =
[[
struct InputData {
	vec4 info0; // .xyz partPos, .w alpha
	vec4 info1; // .xyz speed, .w alphaDecay
	vec4 info2; // .xyz dir, .w color
	vec4 info3; // .x length, .y lengthGrowth, .z width, .w drawOrder
	vec4 info4; // texCoord
};
]],
	InputDefs =
[[
#define partPos    dataIn[gl_GlobalInvocationID.x].info0.xyz
#define alpha      dataIn[gl_GlobalInvocationID.x].info0.w

#define partSpd    dataIn[gl_GlobalInvocationID.x].info1.xyz
#define alphaDecay floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info1.w)

#define partDir    dataIn[gl_GlobalInvocationID.x].info1.xyz
#define partCol    floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info1.w)

#define partLen        dataIn[gl_GlobalInvocationID.x].info2.x
#define partLenGrowth  dataIn[gl_GlobalInvocationID.x].info2.y
#define partWidth      dataIn[gl_GlobalInvocationID.x].info2.z
#define drawOrder      dataIn[gl_GlobalInvocationID.x].info2.w

#define texCoord   dataIn[gl_GlobalInvocationID.x].info4
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
	vec4 partColor = GetPackedColor(partCol);
	float a = max(0.0f, alpha - alphaDecay * frameInfo.y);
	partColor *= a;
	
	vec3 dif1 = normalize(partPos - camDirPos[0].xyz);
	vec3 dir2 = normalize(cross(dif1, partDir));
	
	vec3 l = (partDir * partLen) + (partLenGrowth * frameInfo.y);
	vec3 w = (dir2 * partWidth);
	
	vec3 drawPos = partPos + partSpd * frameInfo.y;

	AddEffectsQuad(
		vec3(1.0),
		drawPos - l - w,
		drawPos + l - w,
		drawPos + l + w,
		drawPos - l + w,
		texCoord,
		partColor
	);
]]
}