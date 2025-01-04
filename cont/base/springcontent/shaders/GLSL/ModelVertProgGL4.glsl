#version 430 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 T;
layout (location = 3) in vec3 B;
layout (location = 4) in vec4 uv;
layout (location = 5) in uvec2 bonesInfo; //boneIDs, boneWeights

layout (location = 6) in uvec4 instData;
// u32 matOffset
// u32 uniOffset
// u32 {teamIdx, drawFlag, unused, numPieces}
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

uniform int cameraMode = 0;
uniform int matrixMode = 0;

uniform mat4 staticModelMatrix = mat4(1.0);

uniform vec4 clipPlane0 = vec4(0.0, 0.0, 0.0, 1.0); //upper construction clip plane
uniform vec4 clipPlane1 = vec4(0.0, 0.0, 0.0, 1.0); //lower construction clip plane
uniform vec4 clipPlane2 = vec4(0.0, 0.0, 0.0, 1.0); //water clip plane

uniform float teamColorAlpha = 1.0;

out Data {
	centroid vec4 uvCoord;
	vec4 teamCol;

	vec4 worldPos;
	vec3 worldNormal;

	// main light vector(s)
	vec3 worldCameraDir;
	// shadowPosition
	vec4 shadowVertexPos;
	// Auxilary
	float fogFactor;
};
out float gl_ClipDistance[3];

#line 1122

void TransformPlayerCam(vec4 worldPos) {
	gl_Position = cameraViewProj * worldPos;
}

void TransformPlayerReflCam(vec4 worldPos) {
	gl_Position = reflectionViewProj * worldPos;
}

void TransformPlayerCamStaticMat(vec4 worldPos) {
	gl_Position = cameraViewProj * worldPos;
}

uint GetUnpackedValue(uint packedValue, uint byteNum) {
	return (packedValue >> (8u * byteNum)) & 0xFFu;
}

vec4 MultiplyQuat(vec4 a, vec4 b)
{
    return vec4(a.w * b.w - dot(a.w, b.w), a.w * b.xyz + b.w * a.xyz + cross(a.xyz, b.xyz));
}

vec3 RotateByQuaternion(vec4 q, vec3 v) {
	return 2.0f * dot(q.xyz, v) * q.xyz + (q.w * q.w - dot(q.xyz, q.xyz)) * v + 2.0 * q.w * cross(q.xyz, v);
}

vec4 RotateByQuaternion(vec4 q, vec4 v) {
	return vec4(RotateByQuaternion(q, v.xyz), v.w);
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

void GetModelSpaceVertex(out vec4 msPosition, out vec3 msNormal)
{
	bool staticModel = (matrixMode > 0);

	vec4 piecePos = vec4(pos, 1.0);

	vec4 weights = vec4(
		float(GetUnpackedValue(bonesInfo.y, 0)) / 255.0,
		float(GetUnpackedValue(bonesInfo.y, 1)) / 255.0,
		float(GetUnpackedValue(bonesInfo.y, 2)) / 255.0,
		float(GetUnpackedValue(bonesInfo.y, 3)) / 255.0
	);

	uint b0 = GetUnpackedValue(bonesInfo.x, 0); //first boneID
	Transform b0BoneTra = transforms[instData.x + b0 + uint(!staticModel)];

	weights[0] *= step(0.0, b0BoneTra.trSc.w);

	msPosition = ApplyTransform(b0BoneTra, piecePos);
	msNormal   = RotateByQuaternion(b0BoneTra.quat, normal);

	if (staticModel || weights[0] == 1.0)
		return;

	float wSum = 0.0;

	msPosition *= weights[0];
	msNormal   *= weights[0];
	wSum       += weights[0];

	uint numPieces = GetUnpackedValue(instData.z, 3);
	Transform bposeTra = transforms[instData.w + b0];

	// Vertex[ModelSpace,BoneX] = PieceMat[BoneX] * InverseBindPosMat[BoneX] * BindPosMat[Bone0] * Vertex[Bone0]
	for (uint bi = 1; bi < 3; ++bi) {
		uint bID = GetUnpackedValue(bonesInfo.x, bi);

		if (bID == 0xFFu || weights[bi] == 0.0)
			continue;

		Transform bposeInvTra = transforms[instData.w + numPieces + bID];
		Transform boneTra     = transforms[instData.x +        1u + bID];

		weights[bi] *= step(0.0, boneTra.trSc.w);

		Transform skinTra = ApplyTransform(ApplyTransform(boneTra, bposeInvTra), bposeTra);

		msPosition += ApplyTransform(skinTra, piecePos * weights[bi]);
		msNormal   += RotateByQuaternion(skinTra.quat, normal * weights[bi]);
		wSum       += weights[bi];
	}

	msPosition /= wSum;
	msNormal   /= wSum;
}

void main(void)
{
	bool staticModel = (matrixMode > 0);

	//mat4 worldMatrix = staticModel ? staticModelMatrix : mat[instData.x]; //don't cover ARRAY_MATMODE yet
	Transform worldTra = transforms[instData.x]; //don't cover ARRAY_MATMODE yet

	vec4 modelPos;
	vec3 modelNormal;
	GetModelSpaceVertex(modelPos, modelNormal);

	worldPos = ApplyTransform(worldTra, modelPos);
	worldNormal = RotateByQuaternion(worldTra.quat, modelNormal);

	gl_ClipDistance[0] = dot(modelPos, clipPlane0); //upper construction clip plane
	gl_ClipDistance[1] = dot(modelPos, clipPlane1); //lower construction clip plane
	gl_ClipDistance[2] = dot(worldPos, clipPlane2); //water clip plane

	uint teamIndex = (instData.z & 0x000000FFu); //leftmost ubyte is teamIndex
	teamCol = teamColor[teamIndex];
	teamCol.a = teamColorAlpha;

	uvCoord = uv;

	shadowVertexPos = shadowView * worldPos;
	shadowVertexPos.xy += vec2(0.5);  //no need for shadowParams anymore

	vec4 cameraPos = cameraViewInv * vec4(0, 0, 0, 1);
	worldCameraDir = cameraPos.xyz - worldPos.xyz; //from fragment to camera, world space, not normalized(!)

	#if (DEFERRED_MODE == 0)
		float fogDist = length(worldCameraDir);
		fogFactor = (fogParams.y - fogDist) * fogParams.w;
		fogFactor = clamp(fogFactor, 0.0, 1.0);
	#endif


	switch(cameraMode) {
		case  1: // water reflection
			TransformPlayerReflCam(worldPos);
			break;
		case  2: // water refraction
			TransformPlayerCam(worldPos);
			break;
		default: // player, (-1) static model, (0) normal rendering
			TransformPlayerCam(worldPos);
			break;
	};
}