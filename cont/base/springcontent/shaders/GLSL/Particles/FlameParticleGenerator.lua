return {
	InputData =
[[
struct InputData {
	vec4  info0; // .xyz pos, .w radius
	vec4  info1; //	.xyz animParams, .w drawOrder(as float);
	vec4  info2; // .xyz rotParams, .w curTime
	vec4  info3; // .x color0(as float), .y color1(as float), .z colEdge0, .w colEdge1
	vec4  info4; // texCoord
};
]],
	InputDefs =
[[
#define drawPos    dataIn[gl_GlobalInvocationID.x].info0.xyz
#define drawRadius dataIn[gl_GlobalInvocationID.x].info0.w

#define animParams dataIn[gl_GlobalInvocationID.x].info1.xyz
#define drawOrder  dataIn[gl_GlobalInvocationID.x].info1.w

#define rotParams  dataIn[gl_GlobalInvocationID.x].info2.xyz
#define curTime    dataIn[gl_GlobalInvocationID.x].info2.w

#define color0     floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info3.x)
#define color1     floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info3.y)
#define colEdge0   dataIn[gl_GlobalInvocationID.x].info3.z
#define colEdge1   dataIn[gl_GlobalInvocationID.x].info3.w

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
	vec4 col0 = GetPackedColor(color0);
	vec4 col1 = GetPackedColor(color1);

	float colMixRate = clamp((colEdge1 - curTime)/(colEdge1 - colEdge0), 0.0, 1.0);
	vec4 col = mix(col0, col1, colMixRate);

	AddEffectsQuadCamera(
		animParams,
		drawPos, vec2(drawRadius), texCoord,
		col
	);
]]
}