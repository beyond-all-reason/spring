#version 130

in vec3 vertexPos;

uniform ivec2 texSquare;
uniform vec3 cameraPos;
uniform vec4 lightDir;       // mapInfo->light.sunDir
uniform vec2 specularTexGen; // 1.0/mapSize
uniform sampler2D heightMapTex;

out vec3 halfDir;
out float fogFactor;
out vec4 vertexWorldPos;
out vec2 diffuseTexCoords;

const float SMF_TEXSQR_SIZE = 1024.0;
const float SMF_DETAILTEX_RES = 0.02;

// specularTexGen is inverseMapSize
vec2 mapSize = vec2(1.0) / specularTexGen;
float HeightAtWorldPos(vec2 wxz){
	// Some texel magic to make the heightmap tex perfectly align:
	const vec2 HM_TEXEL = vec2(8.0, 8.0);
	wxz +=  -HM_TEXEL * (wxz * specularTexGen) + 0.5 * HM_TEXEL;

	vec2 uvhm = clamp(wxz, HM_TEXEL, mapSize.xy - HM_TEXEL);
	uvhm *= specularTexGen;

	return textureLod(heightMapTex, uvhm, 0.0).x;
}

void main() {
	// calc some lighting variables
	vec3 viewDir = vec3(gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0));

	vertexWorldPos = vec4(vertexPos, 1.0);
	vertexWorldPos.xz += vec2(texSquare) * SMF_TEXSQR_SIZE;
	vertexWorldPos.y = HeightAtWorldPos(vertexWorldPos.xz);

	viewDir = normalize(viewDir - vertexWorldPos.xyz);
	halfDir = normalize(lightDir.xyz + viewDir);

	// calc texcoords
	diffuseTexCoords = (vertexWorldPos.xz / SMF_TEXSQR_SIZE) - vec2(texSquare);

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
