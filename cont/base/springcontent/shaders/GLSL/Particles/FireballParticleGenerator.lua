return {
	InputData =
[[
struct InputData {
	vec4  info0[12]; // .xyz sparkPos, .w sparkSize
	vec4  info1;     // .xyz dgunPos, .w dgunSize
	vec4  info2;     // .xyz animParams1, .w numSparks
	vec4  info3;     // .xyz animParams2, .w drawOrder
	vec4  info4;     // .xyz speed, .w checkCol
	vec4  info5;     // texCoord1
	vec4  info6;     // texCoord2
};
]],
	InputDefs =
[[
#define SparkPos(IDX) dataIn[gl_GlobalInvocationID.x].info0[IDX].xyz
#define SparkSize(IDX) dataIn[gl_GlobalInvocationID.x].info0[IDX].w

#define dgunPos  dataIn[gl_GlobalInvocationID.x].info1.xyz
#define dgunSize dataIn[gl_GlobalInvocationID.x].info1.w

#define animParams1 dataIn[gl_GlobalInvocationID.x].info2.xyz
#define numSparks   floatBitsToInt(dataIn[gl_GlobalInvocationID.x].info2.w)

#define animParams2 dataIn[gl_GlobalInvocationID.x].info3.xyz
#define drawOrder   dataIn[gl_GlobalInvocationID.x].info3.w

#define speed     dataIn[gl_GlobalInvocationID.x].info4.xyz
#define checkCol  dataIn[gl_GlobalInvocationID.x].info4.w

#define texCoord1   dataIn[gl_GlobalInvocationID.x].info5
#define texCoord2   dataIn[gl_GlobalInvocationID.x].info6
]],
	EarlyExit =
[[
	bvec2 validTextures = bvec2(
		(texCoord1.z - texCoord1.x) * (texCoord1.w - texCoord1.y) > 0.0,
		(texCoord2.z - texCoord2.x) * (texCoord2.w - texCoord2.y) > 0.0
	);

	if (!any(validTextures))
		return;
		
	int numFire = min(10, numSparks);
]],
	NumQuads =
[[
	numSparks * uint(validTextures.x) + numFire * uint(validTextures.y)
]],
	MainCode =
[[
	if (validTextures.x) {
		for (int i = 0; i < numSparks; ++i) {
			vec4 sparkColor = GetColorFromIntegers(uvec4(
				(numSparks - i) * 12,
				(numSparks - i) *  6,
				(numSparks - i) *  4,
				1
			));

			AddEffectsQuadCamera(
				quadStartIndex++,
				animParams1,
				SparkPos(i), vec2(SparkSize(i)), texCoord1,
				sparkColor
			);
		}
	}

	if (validTextures.y) {
		vec3 interPos = mix(dgunPos, dgunPos + fract(dgunPos) * speed, checkCol);

		int maxCol = int(mix(float(numFire), 10.0, checkCol));

		vec3 dgunDrawPos = dgunPos + fract(dgunPos) * speed * checkCol;
		for (int i = 0; i < numFire; ++i) {
			vec4 dgunColor = GetColorFromIntegers(uvec4(
				(maxCol - i) * 25,
				(maxCol - i) * 15,
				(maxCol - i) * 10,
				1
			));

			AddEffectsQuadCamera(
				quadStartIndex++,
				animParams2,
				interPos - (speed * 0.5 * i), vec2(dgunSize), texCoord2,
				dgunColor
			);
		}
	}
]]
}