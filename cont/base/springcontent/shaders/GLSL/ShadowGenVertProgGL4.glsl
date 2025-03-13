#version 430 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 T;
layout (location = 3) in vec3 B;
layout (location = 4) in vec4 uv;
layout (location = 5) in uvec3 bonesInfo; //boneIDsLow, boneWeights, boneIDsHigh

layout (location = 6) in uvec4 instData;
// u32 matOffset
// u32 uniOffset
// u32 {teamIdx, drawFlag, numPiecesH, numPiecesL}, note numPiecesH then numPiecesL
// u32 bposeMatOffset

layout(std140, binding = 0) uniform UniformMatrixBuffer {
	mat4 screenView;
	mat4 screenProj;
	mat4 screenViewProj;

	mat4 cameraView;
	mat4 cameraProj;
	mat4 cameraViewProj;
	mat4 cameraBillboardView;

	mat4 cameraViewInv;
	mat4 cameraProjInv;
	mat4 cameraViewProjInv;

	mat4 shadowView;
	mat4 shadowProj;
	mat4 shadowViewProj;

	mat4 reflectionView;
	mat4 reflectionProj;
	mat4 reflectionViewProj;

	mat4 orthoProj01;

	// transforms for [0] := Draw, [1] := DrawInMiniMap, [2] := Lua DrawInMiniMap
	mat4 mmDrawView; //world to MM
	mat4 mmDrawProj; //world to MM
	mat4 mmDrawViewProj; //world to MM

	mat4 mmDrawIMMView; //heightmap to MM
	mat4 mmDrawIMMProj; //heightmap to MM
	mat4 mmDrawIMMViewProj; //heightmap to MM

	mat4 mmDrawDimView; //mm dims
	mat4 mmDrawDimProj; //mm dims
	mat4 mmDrawDimViewProj; //mm dims
};

layout(std140, binding = 1) uniform UniformParamsBuffer {
	vec3 rndVec3; //new every draw frame.
	uint renderCaps; //various render booleans

	vec4 timeInfo;     //gameFrame, drawSeconds, interpolated(unsynced)GameSeconds(synced), frameTimeOffset
	vec4 viewGeometry; //vsx, vsy, vpx, vpy
	vec4 mapSize;      //xz, xzPO2
	vec4 mapHeight;    //height minCur, maxCur, minInit, maxInit

	vec4 fogColor;  //fog color
	vec4 fogParams; //fog {start, end, 0.0, scale}

	vec4 sunDir;

	vec4 sunAmbientModel;
	vec4 sunAmbientMap;
	vec4 sunDiffuseModel;
	vec4 sunDiffuseMap;
	vec4 sunSpecularModel;
	vec4 sunSpecularMap;

	vec4 shadowDensity; // {ground, units, 0.0, 0.0}

	vec4 windInfo; // windx, windy, windz, windStrength
	vec2 mouseScreenPos; //x, y. Screen space.
	uint mouseStatus; // bits 0th to 32th: LMB, MMB, RMB, offscreen, mmbScroll, locked
	uint mouseUnused;
	vec4 mouseWorldPos; //x,y,z; w=0 -- offmap. Ignores water, doesn't ignore units/features under the mouse cursor

	vec4 teamColor[255]; //all team colors
};

struct Transform {
	vec4 quat;
	vec4 trSc;
};

layout(std140, binding = 0) readonly buffer TransformBuffer {
	Transform transforms[];
};

uniform vec4 clipPlane0 = vec4(0.0, 0.0, 0.0, 1.0); //upper construction clip plane
uniform vec4 clipPlane1 = vec4(0.0, 0.0, 0.0, 1.0); //lower construction clip plane

out Data {
	vec4 uvCoord;
};
out float gl_ClipDistance[2];

void TransformShadowCam(vec4 worldPos, vec3 worldNormal) {
	vec4 lightVertexPos = shadowView * worldPos;
	vec3 lightVertexNormal = normalize(mat3(shadowView) * worldNormal);

	float NdotL = clamp(lightVertexNormal.z, 0.0, 1.0);

	//use old bias formula from GetShadowPCFRandom(), but this time to write down shadow depth map values
	const float cb = 1e-5;
	float bias = cb * clamp(tan(acos(NdotL)), 0.0, 30.0);

	lightVertexPos.xy += vec2(0.5);
	lightVertexPos.z  += bias;

	gl_Position = shadowProj * lightVertexPos;
}

#line 1119

uint GetUnpackedValue(uint packedValue, uint byteNum) {
	return (packedValue >> (8u * byteNum)) & 0xFFu;
}

vec4 MultiplyQuat(vec4 a, vec4 b)
{
    return vec4(a.w * b.w - dot(a.w, b.w), a.w * b.xyz + b.w * a.xyz + cross(a.xyz, b.xyz));
}

vec3 RotateByQuaternion(vec4 q, vec3 v) {
	return 2.0 * dot(q.xyz, v) * q.xyz + (q.w * q.w - dot(q.xyz, q.xyz)) * v + 2.0 * q.w * cross(q.xyz, v);
}

vec4 RotateByQuaternion(vec4 q, vec4 v) {
	return vec4(RotateByQuaternion(q, v.xyz), v.w);
}

vec4 InvertNormalizedQuaternion(vec4 q) {
	return vec4(-q.x, -q.y, -q.z, q.w);
}

vec3 ApplyTransform(Transform tra, vec3 v) {
	return RotateByQuaternion(tra.quat, v * tra.trSc.w) + tra.trSc.xyz;
}

vec4 ApplyTransform(Transform tra, vec4 v) {
	return vec4(ApplyTransform(tra, v.xyz), v.w);
}

Transform ApplyTransform(Transform parentTra, Transform childTra) {
	return Transform(
		MultiplyQuat(parentTra.quat, childTra.quat),
		vec4(
			parentTra.trSc.xyz + RotateByQuaternion(parentTra.quat, parentTra.trSc.w * childTra.trSc.xyz),
			parentTra.trSc.w * childTra.trSc.w
		)
	);
}

Transform InvertTransformAffine(Transform tra) {
	vec4 invR = InvertNormalizedQuaternion(tra.quat);
	float invS = 1.0 / tra.trSc.w;
	return Transform(
		invR,
		vec4(
			RotateByQuaternion(invR, -tra.trSc.xyz * invS),
			invS
		)
	);
}

vec4 SLerp(vec4 qa, vec4 qb, float t) {
	// Calculate angle between them.
	float cosHalfTheta = dot(qa, qb);

	// Unfortunately every rotation can be represented by two quaternions: (++++) or (----)
	// avoid taking the longer way: choose one representation
	float s = sign(cosHalfTheta);
	qb *= s;
	cosHalfTheta *= s;

	// if qa = qb or qa = -qb then theta = 0 and we can return qa
	if (abs(cosHalfTheta) >= 1.0) // greater-sign necessary for numerical stability
		return qa;

	// Calculate temporary values.
	float halfTheta = acos(cosHalfTheta);
	float sinHalfTheta = sqrt(1.0 - cosHalfTheta * cosHalfTheta); // NOTE: we checked above that |cosHalfTheta| < 1

	// if theta = pi then result is not fully defined
	// we could rotate around any axis normal to qa or qb
	if (sinHalfTheta < 1e-3)
		return normalize(mix(qa, qb, 0.5));

	// both should be divided by sinHalfTheta, but makes no sense to do it due to follow up normalization
	float ratioA = sin((1.0 - t) * halfTheta);
	float ratioB = sin((      t) * halfTheta);

	return normalize(qa * ratioA + qb * ratioB);
}

Transform Lerp(Transform t0, Transform t1, float a) {
	return Transform(
		SLerp(t0.quat, t1.quat, a),
		mix(t0.trSc, t1.trSc, a)
	);
}

void GetModelSpaceVertex(out vec4 msPosition, out vec3 msNormal)
{
	vec4 piecePos = vec4(pos, 1.0);

	vec4 weights = vec4(
		float(GetUnpackedValue(bonesInfo.y, 0u)) / 255.0,
		float(GetUnpackedValue(bonesInfo.y, 1u)) / 255.0,
		float(GetUnpackedValue(bonesInfo.y, 2u)) / 255.0,
		float(GetUnpackedValue(bonesInfo.y, 3u)) / 255.0
	);

	uint bID0 = GetUnpackedValue(bonesInfo.x, 0u) + (GetUnpackedValue(bonesInfo.z, 0u) << 8u); //first boneID
	
	// do interpolation
	Transform tx = Lerp(
		transforms[instData.x + 2u * (1u + bID0) + 0u],
		transforms[instData.x + 2u * (1u + bID0) + 1u],
		timeInfo.w
	);

	//tx = transforms[instData.x + 2u + 2u * bID0 + 1u];

	weights[0] *= float(tx.trSc.w > 0.0);
	msPosition = ApplyTransform(tx, piecePos);
	tx.trSc = vec4(0, 0, 0, 1); //nullify the transform part
	msNormal = ApplyTransform(tx, normal);

	if (weights[0] == 1.0)
		return;

	float wSum = 0.0;

	msPosition *= weights[0];
	msNormal   *= weights[0];
	wSum       += weights[0];

	Transform bposeTra = transforms[instData.w + bID0];

	// Vertex[ModelSpace,BoneX] = PieceMat[BoneX] * InverseBindPosMat[BoneX] * BindPosMat[Bone0] * Vertex[Bone0]
	for (uint bi = 1; bi < 3; ++bi) {
		uint bID = GetUnpackedValue(bonesInfo.x, bi) + (GetUnpackedValue(bonesInfo.z, bi) << 8u);

		if (bID == 0xFFFFu || weights[bi] == 0.0)
			continue;

		Transform bposeInvTra = InvertTransformAffine(transforms[instData.w + bID]);
		Transform boneTx = Lerp(
			transforms[instData.x + 2u * (1u + bID) + 0u],
			transforms[instData.x + 2u * (1u + bID) + 1u],
			timeInfo.w
		);

		weights[bi] *= float(boneTx.trSc.w > 0.0);

		// emulate boneTx * bposeInvTra * bposeTra * piecePos
		vec4 txPiecePos = ApplyTransform(ApplyTransform(boneTx, ApplyTransform(bposeInvTra, bposeTra)), piecePos);

		tx.trSc = vec4(0, 0, 0, 1); //nullify the transform part

		// emulate boneTx * bposeInvTra * bposeTra * normal
		vec3 txPieceNormal = ApplyTransform(ApplyTransform(boneTx, ApplyTransform(bposeInvTra, bposeTra)), normal);

		msPosition += txPiecePos    * weights[bi];
		msNormal   += txPieceNormal * weights[bi];
		wSum       += weights[bi];
	}

	msPosition /= wSum;
	msNormal   /= wSum;
}

void main(void)
{
	vec4 modelPos;
	vec3 modelNormal;
	GetModelSpaceVertex(modelPos, modelNormal);

	// do interpolation
	Transform tx = Lerp(
		transforms[instData.x + 0u],
		transforms[instData.x + 1u],
		timeInfo.w
	);

	vec4 worldPos = ApplyTransform(tx, modelPos);
	tx.trSc = vec4(0, 0, 0, 1); //nullify the transform part
	vec3 worldNormal = ApplyTransform(tx, modelNormal);

	gl_ClipDistance[0] = dot(modelPos, clipPlane0); //upper construction clip plane
	gl_ClipDistance[1] = dot(modelPos, clipPlane1); //lower construction clip plane

	uvCoord = uv;

	TransformShadowCam(worldPos, worldNormal);
}