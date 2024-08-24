return {
	InputData =
[[
struct InputData {
	vec4 info0; // .xyz p1, .w w1
	vec4 info1; // .xyz p2, .w w2
	vec4 info2; // .xyz v1, .w drawOrder
	vec4 info3; // .xyz v2, .w color
	vec4 info4; // texCoord
};
]],
	InputDefs =
[[
#define partP1    dataIn[gl_GlobalInvocationID.x].info0.xyz
#define partW1    dataIn[gl_GlobalInvocationID.x].info0.w

#define partP2    dataIn[gl_GlobalInvocationID.x].info1.xyz
#define partW2    dataIn[gl_GlobalInvocationID.x].info1.w

#define partV1    dataIn[gl_GlobalInvocationID.x].info2.xyz
#define drawOrder dataIn[gl_GlobalInvocationID.x].info2.w

#define partV2    dataIn[gl_GlobalInvocationID.x].info2.xyz
#define partCol   floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info2.w)

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
	partColor.rgb *= partColor.a;

	vec3 dif1 = normalize(partP1 - camDirPos[0].xyz);
	vec3 dir1 = normalize(cross(dif1, partV1));

	vec3 dif2 = normalize(partP2 - camDirPos[0].xyz);
	vec3 dir2 = normalize(cross(dif2, partV2));
	
	float u = (texCoord.x + texCoord.z) * 0.5;
	float v0 = texCoord.y;
	float v1 = texCoord.w;
	
	if (partW2 != 0.0) {
		AddEffectsQuad(
			vec3(1.0),
			partP1 - dir1 * partW1, vec2(u, v1),
			partP1 + dir1 * partW1, vec2(u, v0),
			partP2 + dir2 * partW2, vec2(u, v0),
			partP2 - dir2 * partW2, vec2(u, v1),
			partColor
		);
	} else {
		AddEffectsQuad(
			vec3(1.0),
			partP1 - dir1 * partW1, vec2(u, v1                  ),
			partP1 + dir1 * partW1, vec2(u, v0                  ),
			partP2                , vec2(u, v0 + (v1 - v0) * 0.5),
			partP2                , vec2(u, v0 + (v1 - v0) * 1.5),
			partColor
		);
	}
]]
}