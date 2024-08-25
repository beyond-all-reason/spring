return {
	InputData =
[[
struct InputData {
	vec4 info0; // .xyz partPos, .w age
	vec4 info1; // .xyz speed, .w alphaDecay
	vec4 info2; // .x aIndex(int32), .y drawOrder, .zw unused
	vec4 info3; // texCoord1
	vec4 info4; // texCoord2
};
]],
	InputDefs =
[[
#define partPos    dataIn[gl_GlobalInvocationID.x].info0.xyz
#define partAge    dataIn[gl_GlobalInvocationID.x].info0.w

#define randDir    dataIn[gl_GlobalInvocationID.x].info1.xyz
#define partSize   dataIn[gl_GlobalInvocationID.x].info1.w

#define aIndex     floatBitsToInt(dataIn[gl_GlobalInvocationID.x].info2.x)
#define drawOrder  dataIn[gl_GlobalInvocationID.x].info2.y

#define texCoord1   dataIn[gl_GlobalInvocationID.x].info3
#define texCoord2   dataIn[gl_GlobalInvocationID.x].info4
]],
	EarlyExit =
[[
	bvec2 validTextures = bvec2(
		(texCoord1.z - texCoord1.x) * (texCoord1.w - texCoord1.y) > 0.0,
		(texCoord2.z - texCoord2.x) * (texCoord2.w - texCoord2.y) > 0.0
	);

	if (!any(validTextures))
		return;
]],
	NumQuads =
[[
	1 * uint(validTextures.x) + 1 * uint(validTextures.y)
]],
	MainCode =
[[
	float alpha = max(0.0, 1.0 - (partAge / (4.0 + partSize * 30.0)));
	float fade  = clamp((1.0 - alpha) * (20.0 + aIndex) * 0.1, 0.0, 1.0);
	
	float modAge = sqrt(partAge + 2.0);
	vec2 drawSize = vec2(modAge * 3.0);

	vec3 interPos = partPos + (aIndex + 2.0 + frameInfo.y) * randDir * modAge * 0.4;

	vec4 partColor1 = vec4(180, 180, 180, 255) * alpha * fade / 255.0;

	AddEffectsQuadCamera(
		vec3(1.0),
		interPos, drawSize, texCoord1,
		partColor1
	);
	
	if (fade < 1.0) {
		float ifade = 1.0f - fade;
		vec4 partColor2 = vec4(vec3(ifade), 1.0 / 255.0);
		
		AddEffectsQuadCamera(
			vec3(1.0),
			interPos, drawSize, texCoord2,
			partColor2
		);
	}
]]
}