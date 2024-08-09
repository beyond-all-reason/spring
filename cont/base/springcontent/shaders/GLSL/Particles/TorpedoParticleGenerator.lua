return {
	InputData =
[[
struct InputData {
	vec4  info0; // .xyz partPos, .w drawOrder
	vec4  info1; //	.xyz partSpeed, .w unused
	vec4  info2; // texCoord
};
]],
	InputDefs =
[[
#define partPos    dataIn[gl_GlobalInvocationID.x].info0.xyz
#define drawOrder  dataIn[gl_GlobalInvocationID.x].info0.w

#define partSpeed  dataIn[gl_GlobalInvocationID.x].info1.xyz

#define texCoord   dataIn[gl_GlobalInvocationID.x].info2
]],
	EarlyExit =
[[
	if ((texCoord.z - texCoord.x) * (texCoord.w - texCoord.y) <= 0.0)
		return;
]],
	NumQuads =
[[
	8
]],
	MainCode =
[[
	vec2 texXY = vec2(
		(texCoord.z + texCoord.x) * 0.5,
		(texCoord.w + texCoord.y) * 0.5
	);

	vec3 dir = normalize(partSpeed);
	vec3 r = cross(dir, vec3(0.0, 1.0, 0.0)/*UpVector*/);

	if (length(r) < 0.01)
		r = vec3(1.0, 0.0, 0.0);
	else
		r = normalize(r);

	vec3 u = normalize(cross(dir, r));

	const float h = 12.0;
	const float w = 2.0;

	vec4 col = GetColorFromIntegers(uvec4(60, 60, 100, 255));
	
	vec3 drawPos = partPos + partSpeed * frameInfo.y;

	AddEffectsQuad(
		vec3(1.0),
		drawPos + (r * w)            , texXY,
		drawPos + (u * w)            , texXY,
		drawPos + (u * w) + (dir * h), texXY,
		drawPos + (r * w) + (dir * h), texXY,
		col
	);

	AddEffectsQuad(
		vec3(1.0),
		drawPos + (u * w)            , texXY,
		drawPos - (r * w)            , texXY,
		drawPos - (r * w) + (dir * h), texXY,
		drawPos + (u * w) + (dir * h), texXY,
		col
	);

	AddEffectsQuad(
		vec3(1.0),
		drawPos - (r * w)           ,  texXY,
		drawPos - (u * w)           ,  texXY,
		drawPos - (u * w) + (dir * h), texXY,
		drawPos - (r * w) + (dir * h), texXY,
		col
	);

	AddEffectsQuad(
		vec3(1.0),
		drawPos - (u * w)            , texXY,
		drawPos + (r * w)            , texXY,
		drawPos + (r * w) + (dir * h), texXY,
		drawPos - (u * w) + (dir * h), texXY,
		col
	);

	AddEffectsQuad(
		vec3(1.0),
		drawPos + (r * w) + (dir * h), texXY,
		drawPos + (u * w) + (dir * h), texXY,
		drawPos + (dir * h * 1.2)    , texXY,
		drawPos + (dir * h * 1.2)    , texXY,
		col
	);

	AddEffectsQuad(
		vec3(1.0),
		drawPos + (u * w) + (dir * h), texXY,
		drawPos - (r * w) + (dir * h), texXY,
		drawPos + (dir * h * 1.2)    , texXY,
		drawPos + (dir * h * 1.2)    , texXY,
		col
	);

	AddEffectsQuad(
		vec3(1.0),
		drawPos - (r * w) + (dir * h), texXY,
		drawPos - (u * w) + (dir * h), texXY,
		drawPos + (dir * h * 1.2)    , texXY,
		drawPos + (dir * h * 1.2)    , texXY,
		col
	);

	AddEffectsQuad(
		vec3(1.0),
		drawPos - (u * w) + (dir * h), texXY,
		drawPos + (r * w) + (dir * h), texXY,
		drawPos + (dir * h * 1.2)    , texXY,
		drawPos + (dir * h * 1.2)    , texXY,
		col
	);
]]
}