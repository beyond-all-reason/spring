return {
	InputData =
[[
#define NUM_TRACER_PARTS 3u
#define MAX_NUM_AGEMODS 20u
struct InputData {
	vec4 info0; // .xyz - position, .w invttl
	vec4 info1; // .xyz - dir, .createFrame(int32)
	vec4 info2; // .xyz - rotParams, drawOrder
	vec4 info3; // .xyz - animParams, sizeGrowth
	vec4 info4; // .x size, .y len, .z frontOffset, .w unused
	vec4 info5; // .x col0, .y col1, .z edge0, .w edge1
	vec4 info6; // sideTexture
	vec4 info7; // frontTexture
};
]],
	InputDefs =
[[
#define position      dataIn[gl_GlobalInvocationID.x].info0.xyz
#define invttl        dataIn[gl_GlobalInvocationID.x].info0.w

#define dir           dataIn[gl_GlobalInvocationID.x].info1.xyz
#define createFrame   floatBitsToInt(dataIn[gl_GlobalInvocationID.x].info1.w)

#define rotParams     dataIn[gl_GlobalInvocationID.x].info2.xyz
#define drawOrder     dataIn[gl_GlobalInvocationID.x].info2.w

#define animParams    dataIn[gl_GlobalInvocationID.x].info3.xyz
#define sizeGrowth    dataIn[gl_GlobalInvocationID.x].info3.w

#define size          dataIn[gl_GlobalInvocationID.x].info4.x
#define len           dataIn[gl_GlobalInvocationID.x].info4.y
#define frontOffset   dataIn[gl_GlobalInvocationID.x].info4.z

#define color0        floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info5.x)
#define color1        floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info5.y)
#define edge0         dataIn[gl_GlobalInvocationID.x].info5.z
#define edge1         dataIn[gl_GlobalInvocationID.x].info5.w

#define sideTexture   dataIn[gl_GlobalInvocationID.x].info6
#define frontTexture  dataIn[gl_GlobalInvocationID.x].info7
]],
	EarlyExit =
[[
	bvec2 validTextures = bvec2(
		(sideTexture.z - sideTexture.x) * (sideTexture.w - sideTexture.y) > 0.0,
		(frontTexture.z - frontTexture.x) * (frontTexture.w - frontTexture.y) > 0.0
	);

	if (!any(validTextures))
		return;
]],
	NumQuads =
[[
	2 * uint(validTextures.x) + 1 * uint(validTextures.y)
]],
	MainCode =
[[
	float currTime = frameInfo.x + frameInfo.y - createFrame;

	float life = currTime * invttl;
	float rotVal  = GetCurrentRotation(rotParams, life);
	float animVal = GetCurrentAnimation(rotParams, life);

	float igrowth = sizeGrowth * (1.0 - (1.0 - life) * (1.0 - life));

	float isize = size * (igrowth + 1.0);
	float ilength = len * (igrowth + 1.0);

	vec4 currColor = GetCurrentColor(color0, color1, edge0, edge1, life);

	vec3 frontPosition = position + dir * frontOffset * ilength;

	vec3 zdir = (abs(dot(dir, vec3(0, 1, 0)/*UpVector*/)) >= 0.99) ? vec3(0, 0, 1)/*FwdVector*/: vec3(0, 1, 0)/*UpVector*/;
	vec3 xdir = normalize(cross(dir, zdir));
	vec3 ydir = normalize(cross(dir, xdir));

	vec2 sc = vec2(sin(rotVal), cos(rotVal));

	vec3 bounds[12] = vec3[12](
		  ydir * isize                ,
		  ydir * isize + dir * ilength,
		 -ydir * isize + dir * ilength,
		 -ydir * isize                ,

		  xdir * isize                ,
		  xdir * isize + dir * ilength,
		 -xdir * isize + dir * ilength,
		 -xdir * isize                ,

		 -xdir * isize + ydir * isize,
		  xdir * isize + ydir * isize,
		  xdir * isize - ydir * isize,
		 -xdir * isize - ydir * isize
	);

	for (uint i = 0u; i < 12u; ++i) {
		bounds[i] = Rotate(sc, dir, bounds[i]);
	}

	if (validTextures.x) {
		AddEffectsQuad(
			vec3(animParams.xy, animVal),
			position + bounds[0],
			position + bounds[1],
			position + bounds[2],
			position + bounds[3],
			sideTexture,
			currColor
		);

		AddEffectsQuad(
			vec3(animParams.xy, animVal),
			position + bounds[4],
			position + bounds[5],
			position + bounds[6],
			position + bounds[7],
			sideTexture,
			currColor
		);
	}

	if (validTextures.y) {
		AddEffectsQuad(
			vec3(animParams.xy, animVal),
			frontPosition + bounds[ 8],
			frontPosition + bounds[ 9],
			frontPosition + bounds[10],
			frontPosition + bounds[11],
			frontTexture,
			currColor
		);
	}
]]
}