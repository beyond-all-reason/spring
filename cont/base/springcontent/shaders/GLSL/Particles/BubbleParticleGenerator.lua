return {
	InputData =
[[
struct InputData {
	vec4 info0; // .xyz partPos, .w alpha
	vec4 info1; // .x size, .y sizeExpansion, .z drawOrder, .w unused
	vec4 info2; // texCoord
};
]],
	InputDefs =
[[
#define partPos    dataIn[gl_GlobalInvocationID.x].info0.xyz
#define alpha      dataIn[gl_GlobalInvocationID.x].info0.w

#define size           dataIn[gl_GlobalInvocationID.x].info1.x
#define sizeExpansion  dataIn[gl_GlobalInvocationID.x].info1.y
#define drawOrder      dataIn[gl_GlobalInvocationID.x].info1.z

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
	vec4 partColor = vec4(alpha);
	
	float interSize = size + sizeExpansion * frameInfo.y;

	AddEffectsQuadCamera(
		vec3(1.0),
		partPos, vec2(interSize), texCoord,
		partColor
	);
]]
}