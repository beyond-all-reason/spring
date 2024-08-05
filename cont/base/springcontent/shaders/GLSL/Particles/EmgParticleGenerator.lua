return {
	InputData =
[[
struct InputData {
	vec4  info0; // .xyz pos, .w radius
	vec4  info1; //	.xyz animParams, .w color;
	vec4  info2; // .xyz rotParams, .w drawOrder(as int);	
	vec4  info3; // texCoord
};
]],
	InputDefs =
[[
#define drawPos    dataIn[gl_GlobalInvocationID.x].info0.xyz
#define drawRadius dataIn[gl_GlobalInvocationID.x].info0.w

#define animParams dataIn[gl_GlobalInvocationID.x].info1.xyz
#define color      floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info1.w)

#define rotParams  dataIn[gl_GlobalInvocationID.x].info2.xyz
#define drawOrder  dataIn[gl_GlobalInvocationID.x].info2.w

#define texCoord   dataIn[gl_GlobalInvocationID.x].info3
]],
	EarlyExit =
[[
	if ((texCoord.z - texCoord.x) * (texCoord.w - texCoord.y) <= 0.0)
		return;
]],
	NumQuads =
[[
	1 * uint(gl_GlobalInvocationID.x < arraySizes.x)
]],
	MainCode =
[[
	vec4 col = GetPackedColor(color);
	vec3 xDirCam = camView[0].xyz;
	vec3 yDirCam = camView[1].xyz;
	AddEffectsQuad(
		quadStartIndex,
		drawPos - xDirCam * drawRadius - yDirCam * drawRadius, texCoord.xy,
		drawPos + xDirCam * drawRadius - yDirCam * drawRadius, texCoord.zy,
		drawPos + xDirCam * drawRadius + yDirCam * drawRadius, texCoord.zw,
		drawPos - xDirCam * drawRadius + yDirCam * drawRadius, texCoord.xw,
		col
	);
]]
}