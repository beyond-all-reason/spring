return {
	InputData =
[[
struct InputData {
	vec4 info0; // .xyz partPos, .w createFrame(int32_t)
	vec4 info1; // .xyz partSpeed, .w partCol
	vec4 info2; // .xyz animParams, .w partSize
	vec4 info3; // .xyz rotParams, .w drawOrder
	vec4 info4; // texCoord
};
]],
	InputDefs =
[[
#define partPos      dataIn[gl_GlobalInvocationID.x].info0.xyz
#define createFrame  floatBitsToInt(dataIn[gl_GlobalInvocationID.x].info0.w)

#define partSpeed    dataIn[gl_GlobalInvocationID.x].info1.xyz
#define partCol      floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info1.w)

#define animParams    dataIn[gl_GlobalInvocationID.x].info2.xyz
#define partSize      dataIn[gl_GlobalInvocationID.x].info2.w

#define rotParams     dataIn[gl_GlobalInvocationID.x].info3.xyz
#define drawOrder     dataIn[gl_GlobalInvocationID.x].info3.w

#define texCoord   dataIn[gl_GlobalInvocationID.x].info3
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
	
	vec4 partColor = GetPackedColor(partCol);

	vec3 bounds[4] = vec3[4](
		vec3(-camDirPos[1].xyz - camDirPos[2].xyz) * partSize,
		vec3( camDirPos[1].xyz - camDirPos[2].xyz) * partSize,
		vec3( camDirPos[1].xyz + camDirPos[2].xyz) * partSize,
		vec3(-camDirPos[1].xyz + camDirPos[2].xyz) * partSize
	);

	vec2 sc = vec2(sin(rotVal), cos(rotVal));	
	for (uint i = 0u; i < uint(bounds.length()); ++i) {
		bounds[i] = Rotate(sc, camDirPos[3].xyz, bounds[i]);
	}
	
	vec3 drawPos = partPos + partSpeed * frameInfo.y;

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