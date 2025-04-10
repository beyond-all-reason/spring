#version 130

in vec3 vertexPos;
in vec4 vertexCol;

uniform sampler2D heightMapTex;
uniform float borderMinHeight;
uniform ivec2 texSquare; //TODO convert to texture array
uniform vec4 mapSize; // mapSize, 1.0/mapSize

const float SMF_TEXSQR_SIZE = 1024.0;
const vec4 detailPlaneS = vec4(0.005, 0.000, 0.005, 0.5);
const vec4 detailPlaneT = vec4(0.000, 0.005, 0.000, 0.5);

out vec4 vVertCol;
out vec2 vDiffuseUV;
out vec2 vDetailsUV;

float HeightAtWorldPos(vec2 wxz){
	// Some texel magic to make the heightmap tex perfectly align:
	const vec2 HM_TEXEL = vec2(8.0, 8.0);
	wxz +=  -HM_TEXEL * (wxz * mapSize.zw) + 0.5 * HM_TEXEL;

	vec2 uvhm = clamp(wxz, HM_TEXEL, mapSize.xy - HM_TEXEL);
	uvhm *= mapSize.zw;

	return textureLod(heightMapTex, uvhm, 0.0).x;
}

void main() {
	vec4 vertexWorldPos = vec4(vertexPos, 1.0);
	vertexWorldPos.xz += vec2(texSquare) * SMF_TEXSQR_SIZE;
	vertexWorldPos.y = mix(borderMinHeight, HeightAtWorldPos(vertexWorldPos.xz), float(vertexWorldPos.y == 0.0));
	/*
	if (vertexWorldPos.y == 0.0)
		vertexWorldPos.y = HeightAtWorldPos(vertexWorldPos.xz);
	else
		vertexWorldPos.y = borderMinHeight;
	*/

	vVertCol = vertexCol;
	vDiffuseUV = (vertexWorldPos.xz * (1.0 / SMF_TEXSQR_SIZE)) - vec2(texSquare);
	vDetailsUV = vec2(
		dot(vertexWorldPos, detailPlaneS),
		dot(vertexWorldPos, detailPlaneT)
	);

	gl_Position = gl_ModelViewProjectionMatrix * vertexWorldPos;
}
