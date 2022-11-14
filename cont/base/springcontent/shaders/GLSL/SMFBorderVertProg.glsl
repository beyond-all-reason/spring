#version 130

in vec3 vertexPos;
in vec4 vertexCol;

uniform ivec2 texSquare; //TODO convert to texture array

const float SMF_TEXSQR_SIZE = 1024.0;
const vec4 detailPlaneS = vec4(0.005, 0.000, 0.005, 0.5);
const vec4 detailPlaneT = vec4(0.000, 0.005, 0.000, 0.5);

out vec3 vVertPos;
out vec4 vVertCol;
out vec2 vDiffuseUV;
out vec2 vDetailsUV;

void main() {
	vec4 vertexPos4 = vec4(vertexPos, 1.0);

	vVertPos = vertexPos;
	vVertCol = vertexCol;

	vDiffuseUV.s = dot(vertexPos4, vec4(1.0 / SMF_TEXSQR_SIZE, 0.0, 0.0                  , -texSquare.x * 1.0));
	vDiffuseUV.t = dot(vertexPos4, vec4(0.0                  , 0.0, 1.0 / SMF_TEXSQR_SIZE, -texSquare.y * 1.0));
	vDetailsUV.s = dot(vertexPos4, detailPlaneS);
	vDetailsUV.t = dot(vertexPos4, detailPlaneT);

	gl_Position = gl_ModelViewProjectionMatrix * vertexPos4;
}

