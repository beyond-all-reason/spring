#version 130

in vec3 vertexPos;

uniform ivec2 texSquare;
uniform vec3 cameraPos;
uniform vec4 lightDir;       // mapInfo->light.sunDir

out vec3 halfDir;
out float fogFactor;
out vec4 vertexWorldPos;
out vec2 diffuseTexCoords;

const float SMF_TEXSQR_SIZE = 1024.0;
const float SMF_DETAILTEX_RES = 0.02;

void main() {
	// calc some lighting variables
	vec3 viewDir = vec3(gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0));

	viewDir = normalize(viewDir - vertexPos);
	halfDir = normalize(lightDir.xyz + viewDir);

	vertexWorldPos = vec4(vertexPos, 1.0);

	// calc texcoords
	diffuseTexCoords = (floor(vertexWorldPos.xz) / SMF_TEXSQR_SIZE) - vec2(texSquare);

	// transform vertex pos
	gl_Position = gl_ModelViewProjectionMatrix * vertexWorldPos;
	gl_ClipVertex = gl_ModelViewMatrix * vertexWorldPos;

#ifndef DEFERRED_MODE
	// emulate linear fog
	float fogCoord = length(gl_ClipVertex.xyz);
	fogFactor = (gl_Fog.end - fogCoord) * gl_Fog.scale; // gl_Fog.scale == 1.0 / (gl_Fog.end - gl_Fog.start)
	fogFactor = clamp(fogFactor, 0.0, 1.0);
#endif
}

