return {
	InputData =
[[
struct InputData {
	vec4 info0; // .xyz startPos, .w thickness
	vec4 info1; // .xyz targetPos, .w unused
	vec4 info2[(12 >> 2)]; // displacements1
	vec4 info3[(12 >> 2)]; // displacements2
	vec4 info4; // texCoord
	vec4 info5; // .x color, .y drawOrder, .zw - unused
};
]],
	InputDefs =
[[
#define startPos           dataIn[gl_GlobalInvocationID.x].info0.xyz
#define thickness          dataIn[gl_GlobalInvocationID.x].info0.w

#define targetPos          dataIn[gl_GlobalInvocationID.x].info1.xyz

#define Displacement1(IDX) dataIn[gl_GlobalInvocationID.x].info2[(IDX >> 2)][(IDX % 4)]
#define Displacement2(IDX) dataIn[gl_GlobalInvocationID.x].info3[(IDX >> 2)][(IDX % 4)]

#define texCoord           dataIn[gl_GlobalInvocationID.x].info4

#define color              floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info5.x)
#define drawOrder          dataIn[gl_GlobalInvocationID.x].info5.y
]],
	EarlyExit =
[[
	if ((texCoord.z - texCoord.x) * (texCoord.w - texCoord.y) <= 0.0)
		return;
]],
	NumQuads =
[[
	(12 + 12 - 2)
]],
	MainCode =
[[
	vec4 col = GetPackedColor(color);

	vec3 ddir = normalize(targetPos - startPos);
	vec3 dif  = normalize(startPos - camPos);
	vec3 dir1 = normalize(cross(dif, ddir));

	vec3 tempPos;

	tempPos = startPos;
	for (uint d = 1u; d < 12u - 1u; ++d) {
		float f = (d + 1u) * (1.0 / (12u - 1u));
		vec3 tempPosO = tempPos;
		tempPos = (startPos * (1.0 - f)) + (targetPos * f);

		AddEffectsQuad(
			vec3(1.0),
			tempPosO + (dir1 * (Displacement1(d    ) + thickness)), texCoord.xy,
			tempPos  + (dir1 * (Displacement1(d + 1) + thickness)), texCoord.zy,
			tempPos  + (dir1 * (Displacement1(d + 1) - thickness)), texCoord.zw,
			tempPosO + (dir1 * (Displacement1(d    ) - thickness)), texCoord.xw,
			col
		);
	}
	
	tempPos = startPos;
	for (uint d = 1u; d < 12u - 1u; ++d) {
		float f = (d + 1u) * (1.0 / (12u - 1u));
		vec3 tempPosO = tempPos;
		tempPos = (startPos * (1.0 - f)) + (targetPos * f);

		AddEffectsQuad(
			vec3(1.0),
			tempPosO + (dir1 * (Displacement2(d    ) + thickness)), texCoord.xy,
			tempPos  + (dir1 * (Displacement2(d + 1) + thickness)), texCoord.zy,
			tempPos  + (dir1 * (Displacement2(d + 1) - thickness)), texCoord.zw,
			tempPosO + (dir1 * (Displacement2(d    ) - thickness)), texCoord.xw,
			col
		);
	}
]]
}