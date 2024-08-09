return {
	InputData =
[[
struct InputData {
	vec4 info0; // .xyz partPos, .w fsize
	vec4 info1; // .xyz speed, .w drawOrder
	vec4 info2; // texCoord
};
]],
	InputDefs =
[[
#define partPos    dataIn[gl_GlobalInvocationID.x].info0.xyz
#define fsize      dataIn[gl_GlobalInvocationID.x].info0.w

#define speed      dataIn[gl_GlobalInvocationID.x].info1.xyz
#define drawOrder  dataIn[gl_GlobalInvocationID.x].info1.w

#define texCoord   dataIn[gl_GlobalInvocationID.x].info2
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
	vec4 lightYellow = GetColorFromIntegers(uvec4(255, 210, 180, 1));

	AddEffectsQuadCamera(
		vec3(1.0),
		partPos, vec2(fsize), texCoord,
		lightYellow
	);
]]
}