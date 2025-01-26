#if (GL_FRAGMENT_PRECISION_HIGH == 1)
// ancient GL3 ATI drivers confuse GLSL for GLSL-ES and require this
precision highp float;
#else
precision mediump float;
#endif

in vec3 vertexPos;

uniform sampler2D heightMapTex;
uniform float borderMinHeight;
uniform ivec2 texSquare;
uniform vec4 mapSize; // mapSize, 1.0/mapSize

const float SMF_TEXSQR_SIZE = 1024.0;

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
	gl_Position = gl_ModelViewProjectionMatrix * vertexWorldPos;

	gl_ClipVertex  = vertexWorldPos;
	gl_TexCoord[0] = gl_MultiTexCoord0;
}