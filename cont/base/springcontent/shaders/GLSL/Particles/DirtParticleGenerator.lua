return {
	InputData =
[[
struct InputData {
	vec4 info0; // .xyz partPos, .w alpha
	vec4 info1; // .xyz speed, .w color
	vec4 info2; // .x size, .y sizeExpansion, .z drawOrder, .w unused
	vec4 info3; // texCoord
};
]],
	InputDefs =
[[
#define partPos    dataIn[gl_GlobalInvocationID.x].info0.xyz
#define alpha      dataIn[gl_GlobalInvocationID.x].info0.w

#define partSpd    dataIn[gl_GlobalInvocationID.x].info1.xyz
#define partCol    floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info1.w)

#define size           dataIn[gl_GlobalInvocationID.x].info2.x
#define sizeExpansion  dataIn[gl_GlobalInvocationID.x].info2.y
#define drawOrder      dataIn[gl_GlobalInvocationID.x].info2.z

#define texCoord   dataIn[gl_GlobalInvocationID.x].info3
]],
	EarlyExit =
[[
	if ((texCoord.z - texCoord.x) * (texCoord.w - texCoord.y) <= 0.0)
		return;
	
	float partAbove = partPos.y / (size * camPos.y);
	
	if (partAbove < -1.0)
		return;
	
	partAbove = min(partAbove, 1.0);
]],
	NumQuads =
[[
	1
]],
	MainCode =
[[
	vec4 partColor =  GetPackedColor(partCol);
	partColor *= alpha;
	
	float interSize = size + sizeExpansion * frameInfo.y;
	float texx = texCoord.x + (texCoord.z - texCoord.x) * ((1.0 - partAbove) * 0.5);
	
	vec3 drawPos = partPos + partSpd * frameInfo.y;

	AddEffectsQuad(
		vec3(1.0),
		drawPos - camDir[0] * interSize - camDir[1] * interSize * partAbove, vec2(texx,       texCoord.y),
		drawPos - camDir[0] * interSize + camDir[1] * interSize            , vec2(texCoord.z, texCoord.y),
		drawPos + camDir[0] * interSize + camDir[1] * interSize            , vec2(texCoord.z, texCoord.w),
		drawPos + camDir[0] * interSize - camDir[1] * interSize * partAbove, vec2(texx,       texCoord.w),
		partColor
	);
]]
}