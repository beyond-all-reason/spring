#version 430 compatibility

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
	vec4 teamColor[255];
};

layout(std140, binding=0) readonly buffer MatrixBuffer {
	mat4 mat[];
};

mat4 mat4mix(mat4 a, mat4 b, float alpha) {
	return (a * (1.0 - alpha) + b * alpha);
}

out Data {
	vec4 uvCoord;
	vec4 teamCol;
};

void main(void)
{
	uint baseIndex = instData.x; //ssbo offset
	mat4 modelMatrix = mat[baseIndex];
	
	mat4 pieceMatrix = mat4mix(mat4(1.0), mat[baseIndex + pieceIndex + 1u], modelMatrix[3][3]);

	vec4 worldPos = modelMatrix * pieceMatrix * vec4(pos, 1.0);	
	
	teamCol = teamColor[instData.y]; // team index
	uvCoord = uv;

	gl_Position = cameraViewProj * worldPos;	
}