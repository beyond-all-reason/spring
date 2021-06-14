#version 430 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 T;
layout (location = 3) in vec3 B;
layout (location = 4) in vec4 uv;
layout (location = 5) in uint pieceIndex;

layout (location = 6) in uvec4 instData;

layout(std140, binding = 0) uniform UniformMatrixBuffer {
	mat4 screenView;
	mat4 screenProj;
	mat4 screenViewProj;

	mat4 cameraView;
	mat4 cameraProj;
	mat4 cameraViewProj;
	mat4 cameraBillboard;

	mat4 cameraViewInv;
	mat4 cameraProjInv;
	mat4 cameraViewProjInv;

	mat4 shadowView;
	mat4 shadowProj;
	mat4 shadowViewProj;

	//TODO: minimap matrices
};

layout(std140, binding = 1) uniform UniformParamsBuffer {
	vec3 rndVec3; //new every draw frame.
	uint renderCaps; //various render booleans

	vec4 timeInfo; //gameFrame, gameSeconds, drawFrame, frameTimeOffset
	vec4 viewGeometry; //vsx, vsy, vpx, vpy
	vec4 mapSize; //xz, xzPO2

	vec4 fogColor; //fog color
	vec4 fogParams; //fog {start, end, 0.0, scale}

	vec4 pad[7];

	vec4 teamColor[255];
};

layout(std140, binding=0) readonly buffer MatrixBuffer {
	mat4 mat[];
};

uniform int drawMode = 0;
uniform mat4 staticModelMatrix = mat4(1.0);

out Data {
	vec4 uvCoord;
	vec4 teamCol;
};

mat4 mat4mix(mat4 a, mat4 b, float alpha) {
	return (a * (1.0 - alpha) + b * alpha);
}

void TransformShadowCam(vec4 worldPos, vec3 worldNormal) {
	vec4 lightVertexPos = shadowView * worldPos;
	vec3 lightVertexNormal = normalize(mat3(shadowView) * worldNormal);

	float NdotL = clamp(dot(lightVertexNormal, vec3(0.0, 0.0, 1.0)), 0.0, 1.0);

	//use old bias formula from GetShadowPCFRandom(), but this time to write down shadow depth map values
	const float cb = 5e-5;
	float bias = cb * tan(acos(NdotL));
	bias = clamp(bias, 0.0, 5.0 * cb);

	lightVertexPos.xy += vec2(0.5);
	lightVertexPos.z += bias;

	gl_Position = shadowProj * lightVertexPos;
}


void TransformPlayerCam(vec4 worldPos) {
	gl_Position = cameraViewProj * worldPos;
}

//TODO figure out?
void TransformPlayerReflCam(vec4 worldPos) {
	gl_Position = cameraViewProj * worldPos;
}

void TransformPlayerCamStaticMat(vec4 worldPos) {
	gl_Position = cameraViewProj * worldPos;
}


//unit, feature, projectile
mat4 GetWorldMatrixLocalModel() {
	uint baseIndex = instData.x; //ssbo offset
	mat4 modelMatrix = mat[baseIndex];

	mat4 pieceMatrix = mat4mix(mat4(1.0), mat[baseIndex + pieceIndex + 1u], modelMatrix[3][3]); //TODO: figure out why mat4mix() is needed
	return modelMatrix * pieceMatrix;
}

//static model
mat4 GetWorldMatrixModel() {
	uint baseIndex = instData.x; //ssbo offset

	mat4 pieceMatrix = mat[baseIndex + pieceIndex];
	return staticModelMatrix * pieceMatrix;
}

#line 1086

void main(void)
{
	mat4 worldMatrix = (drawMode >= 0) ? GetWorldMatrixLocalModel() : GetWorldMatrixModel();

	#if 0
		mat3 normalMatrix = mat3(transpose(inverse(worldMatrix)));
	#else
		mat3 normalMatrix = mat3(worldMatrix);
	#endif


	vec4 worldPos = worldMatrix * vec4(pos, 1.0);
	vec3 worldNormal = normalMatrix * normal;

	teamCol = teamColor[instData.y]; // team index
	uvCoord = uv;

	switch(drawMode) {
		case 1:  TransformShadowCam(worldPos, worldNormal); break; //shadow
		case 2:  TransformPlayerReflCam(worldPos);          break; //underwater reflection
		default: TransformPlayerCam(worldPos);              break; //player, corresponds to 0 and -1 modes
	};
}