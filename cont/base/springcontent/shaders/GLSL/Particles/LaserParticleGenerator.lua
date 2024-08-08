return {
	InputData =
[[
struct InputData {
	vec4 info0; // .xyz drawPos, .w curLength
	vec4 info1; // .xyz dir, .w maxLength
	vec4 info2; // .x thickness, .y coreThickness, .z color1, .w color2
	vec4 info3; // .x lodDistance, .y drawOrder, .z checkCol .w stayTime
	vec4 info4; // .x speedf
	vec4 info5; // texCoord1
	vec4 info6; // texCoord2
};
]],
	InputDefs =
[[
#define drawPos        dataIn[gl_GlobalInvocationID.x].info0.xyz
#define curLength      dataIn[gl_GlobalInvocationID.x].info0.w

#define dir            dataIn[gl_GlobalInvocationID.x].info1.xyz
#define maxLength      dataIn[gl_GlobalInvocationID.x].info1.w

#define thickness      dataIn[gl_GlobalInvocationID.x].info2.x
#define coreThickness  dataIn[gl_GlobalInvocationID.x].info2.y
#define color1         floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info2.z)
#define color2         floatBitsToUint(dataIn[gl_GlobalInvocationID.x].info2.w)

#define lodDistance    dataIn[gl_GlobalInvocationID.x].info3.x
#define drawOrder      dataIn[gl_GlobalInvocationID.x].info3.y
#define checkCol       floatBitsToInt(dataIn[gl_GlobalInvocationID.x].info3.z)
#define stayTime       dataIn[gl_GlobalInvocationID.x].info3.w

#define speedf         dataIn[gl_GlobalInvocationID.x].info4.x

#define texCoord1      dataIn[gl_GlobalInvocationID.x].info5
#define texCoord2      dataIn[gl_GlobalInvocationID.x].info6
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
	2 * uint(validTextures.x) + 4 * uint(validTextures.y);
]],
	MainCode =
[[
	vec3 dif = drawPos - camDirPos[3].xyz;
	float camDist = length(dif);
	dif /= camDist;

	vec3 dir1 = normalize(cross(dif, dir ));
	vec3 dir2 = normalize(cross(dif, dir1));

	vec4 col1 = GetPackedColor(color1);
	vec4 col2 = GetPackedColor(color2);

	float size = thickness;
	float coresize = size * coreThickness;
	float midtexx = 0.5 * (texCoord2.x + texCoord2.z);

	if (camDist < lodDistance) {
		vec3 pos2 = drawPos - (dir * curLength);

		float texStartOffset;
		float texEndOffset;

		if (checkCol == 1) { // expanding or contracting?
			texStartOffset = 0.0;
			texEndOffset   = ( 1.0 - (curLength / maxLength)) * (texCoord1.x - texCoord1.z);
		} else {
			texStartOffset = (-1.0 + (curLength / maxLength) + (stayTime * (speedf / maxLength))) * (texCoord1.x - texCoord1.z);
			texEndOffset   = (stayTime * (speedf / maxLength)) * (texCoord1.x - texCoord1.z);
		}

		if (validTextures.y) {
			AddEffectsQuad(
				vec3(1.0),
				drawPos - (dir1 * size) - (dir2 * size), vec2(texCoord2.x, texCoord2.y),
				drawPos - (dir1 * size)                , vec2(midtexx,     texCoord2.y),
				drawPos + (dir1 * size)                , vec2(midtexx,     texCoord2.w),
				drawPos + (dir1 * size) - (dir2 * size), vec2(texCoord2.x, texCoord2.w),
				col1
			);

			AddEffectsQuad(
				vec3(1.0),
				drawPos - (dir1 * coresize) - (dir2 * coresize), vec2(texCoord2.x, texCoord2.y),
				drawPos - (dir1 * coresize)                    , vec2(midtexx    , texCoord2.y),
				drawPos + (dir1 * coresize)                    , vec2(midtexx    , texCoord2.w),
				drawPos + (dir1 * coresize) - (dir2 * coresize), vec2(texCoord2.x, texCoord2.w),
				col2
			);
		}

		if (validTextures.x) {
			AddEffectsQuad(
				vec3(1.0),
				drawPos - (dir1 * size), vec2(texCoord1.x + texStartOffset, texCoord1.y),
				pos2    - (dir1 * size), vec2(texCoord1.z + texEndOffset  , texCoord1.y),
				pos2    + (dir1 * size), vec2(texCoord1.z + texEndOffset  , texCoord1.w),
				drawPos + (dir1 * size), vec2(texCoord1.x + texStartOffset, texCoord1.w),
				col1
			);

			AddEffectsQuad(
				vec3(1.0),
				drawPos - (dir1 * coresize), vec2(texCoord1.x + texStartOffset, texCoord1.y),
				pos2    - (dir1 * coresize), vec2(texCoord1.z + texEndOffset  , texCoord1.y),
				pos2    + (dir1 * coresize), vec2(texCoord1.z + texEndOffset  , texCoord1.w),
				drawPos + (dir1 * coresize), vec2(texCoord1.x + texStartOffset, texCoord1.w),
				col2
			);
		}

		if (validTextures.y) {
			AddEffectsQuad(
				vec3(1.0),
				pos2 - (dir1 * size)                , vec2(midtexx    , texCoord2.y),
				pos2 - (dir1 * size) + (dir2 * size), vec2(texCoord2.z, texCoord2.y),
				pos2 + (dir1 * size) + (dir2 * size), vec2(texCoord2.z, texCoord2.w),
				pos2 + (dir1 * size)                , vec2(midtexx    , texCoord2.w),
				col1
			);

			AddEffectsQuad(
				vec3(1.0),
				pos2 - (dir1 * coresize)                    , vec2(midtexx    , texCoord2.y),
				pos2 - (dir1 * coresize) + (dir2 * coresize), vec2(texCoord2.z, texCoord2.y),
				pos2 + (dir1 * coresize) + (dir2 * coresize), vec2(texCoord2.z, texCoord2.w),
				pos2 + (dir1 * coresize)                    , vec2(midtexx    , texCoord2.w),
				col2
			);
		}
	} else {
		vec3 pos1 = drawPos + (dir * (size * 0.5));
		vec3 pos2 = pos1 - (dir * (curLength + size));

		float texStartOffset;
		float texEndOffset;

		if (checkCol == 1) { // expanding or contracting?
			texStartOffset = 0.0;
			texEndOffset   = ( 1.0 - (curLength / maxLength)) * (texCoord1.x - texCoord1.z);
		} else {
			texStartOffset = (-1.0 + (curLength / maxLength) + (stayTime * (speedf / maxLength))) * (texCoord1.x - texCoord1.z);
			texEndOffset   = (stayTime * (speedf / maxLength)) * (texCoord1.x - texCoord1.z);
		}

		if (validTextures.x) {
			AddEffectsQuad(
				vec3(1.0),
				pos1 - (dir1 * size), vec2(texCoord1.x + texStartOffset, texCoord1.y),
				pos2 - (dir1 * size), vec2(texCoord1.z + texEndOffset  , texCoord1.y),
				pos2 + (dir1 * size), vec2(texCoord1.z + texEndOffset  , texCoord1.w),
				pos1 + (dir1 * size), vec2(texCoord1.x + texStartOffset, texCoord1.w),
				col1
			);

			AddEffectsQuad(
				vec3(1.0),
				pos1 - (dir1 * coresize), vec2(texCoord1.x + texStartOffset, texCoord1.y),
				pos2 - (dir1 * coresize), vec2(texCoord1.z + texEndOffset  , texCoord1.y),
				pos2 + (dir1 * coresize), vec2(texCoord1.z + texEndOffset  , texCoord1.w),
				pos1 + (dir1 * coresize), vec2(texCoord1.x + texStartOffset, texCoord1.w),
				col2
			);
		}
	}
]]
}