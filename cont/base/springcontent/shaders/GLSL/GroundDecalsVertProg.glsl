#version 130

in vec4 posT; //posTL, posTR
in vec4 posB; //posBR, posBL
in vec4 uvMain; // L, T, R, B
in vec4 uvNorm; // L, T, R, B
in vec4 info; // alpha, alphaFalloff, rot, height
in uvec2 createFrame; //min, max (left, right) side
in vec2 uvParams; // .x - uvWrapDistance, .y - traveled distance - used for tracks,

uniform sampler2D heightTex;
uniform sampler2D groundNormalTex;
uniform vec4 mapDims; //mapxy; 1.0 / mapxy
uniform float curAdjustedFrame;

flat out vec4 vPosT;
flat out vec4 vPosB;
flat out vec4 vuvMain;
flat out vec4 vuvNorm;
flat out vec4 midPoint;
flat out vec4 misc; //misc.x - alpha & glow, misc.y - height, misc.z - uvWrapDistance, misc.w - uvOffset
flat out vec4 misc2; //misc2.x - sin(rot), misc2.y - cos(rot);

#define posTL vPosT.xy
#define posTR vPosT.zw
#define posBR vPosB.xy
#define posBL vPosB.zw

#define NORM2SNORM(value) (value * 2.0 - 1.0)
#define SNORM2NORM(value) (value * 0.5 + 0.5)

const vec3 CUBE_VERT[36] = vec3[36](
	//RIGHT
	vec3( 1.0f, -1.0f, -1.0f),
	vec3( 1.0f, -1.0f,  1.0f),
	vec3( 1.0f,  1.0f,  1.0f),
	vec3( 1.0f,  1.0f,  1.0f),
	vec3( 1.0f,  1.0f, -1.0f),
	vec3( 1.0f, -1.0f, -1.0f),
	//LEFT
	vec3(-1.0f, -1.0f,  1.0f),
	vec3(-1.0f, -1.0f, -1.0f),
	vec3(-1.0f,  1.0f, -1.0f),
	vec3(-1.0f,  1.0f, -1.0f),
	vec3(-1.0f,  1.0f,  1.0f),
	vec3(-1.0f, -1.0f,  1.0f),
	//TOP
	vec3(-1.0f,  1.0f, -1.0f),
	vec3( 1.0f,  1.0f, -1.0f),
	vec3( 1.0f,  1.0f,  1.0f),
	vec3( 1.0f,  1.0f,  1.0f),
	vec3(-1.0f,  1.0f,  1.0f),
	vec3(-1.0f,  1.0f, -1.0f),
	//BOTTOM
	vec3(-1.0f, -1.0f, -1.0f),
	vec3(-1.0f, -1.0f,  1.0f),
	vec3( 1.0f, -1.0f, -1.0f),
	vec3( 1.0f, -1.0f, -1.0f),
	vec3(-1.0f, -1.0f,  1.0f),
	vec3( 1.0f, -1.0f,  1.0f),
	//FRONT
	vec3(-1.0f, -1.0f,  1.0f),
	vec3(-1.0f,  1.0f,  1.0f),
	vec3( 1.0f,  1.0f,  1.0f),
	vec3( 1.0f,  1.0f,  1.0f),
	vec3( 1.0f, -1.0f,  1.0f),
	vec3(-1.0f, -1.0f,  1.0f),
	//BACK
	vec3(-1.0f,  1.0f, -1.0f),
	vec3(-1.0f, -1.0f, -1.0f),
	vec3( 1.0f, -1.0f, -1.0f),
	vec3( 1.0f, -1.0f, -1.0f),
	vec3( 1.0f,  1.0f, -1.0f),
	vec3(-1.0f,  1.0f, -1.0f)
);

#line 100075

const vec2 HM_TEXEL = vec2(8.0, 8.0);
float HeightAtWorldPos(vec2 wxz) {
	// Some texel magic to make the heightmap tex perfectly align:
	wxz +=  -HM_TEXEL * (wxz * mapDims.zw) + 0.5 * HM_TEXEL;

	vec2 uvhm = clamp(wxz, HM_TEXEL, mapDims.xy - HM_TEXEL);
	uvhm *= mapDims.zw;

	return textureLod(heightTex, uvhm, 0.0).x;
}

vec3 GetFragmentNormal(vec2 wxz) {
	vec3 normal;
	normal.xz = texture2D(groundNormalTex, wxz * mapDims.zw).ra;
	normal.y  = sqrt(1.0 - dot(normal.xz, normal.xz));
	return normal;
}

mat2 GetRotMat2D(){
	//return mat2(1.0);
	float ca = misc2.y;
	float sa = misc2.x;
    return mat2(ca, -sa,
                sa,  ca);
}

// see the engine's rotateByUpVector()
vec3 RotateByNormalVector(vec3 p, vec3 newUpDir, vec3 rotAxis) {
	#define px (p.x)
	#define py (p.y)
	#define pz (p.z)

	#define yx (newUpDir.x)
	#define yy (newUpDir.y)
	#define yz (newUpDir.z)

	#define nx (rotAxis.x)
	#define ny (rotAxis.y)
	#define nz (rotAxis.z)

	vec3 cm = vec3(
		yy,
		sqrt(yx * yx + yz * yz),
		(nx * px + ny * py + nz * pz)* (1.0 - yy)
	);

	return vec3(
		dot(cm, vec3(px, (ny * pz - nz * py), nx)),
		dot(cm, vec3(py, (nz * px - nx * pz), ny)),
		dot(cm, vec3(pz, (nx * py - ny * px), nz))
	);

	#undef px
	#undef py
	#undef pz

	#undef yx
	#undef yy
	#undef yz

	#undef nx
	#undef ny
	#undef nz
}

// uv ==> L, T, R, B
void main() {
	vec3 relPos = CUBE_VERT[gl_VertexID];

	// alpha
	float thisVertexCreateFrame = mix(float(createFrame.x), float(createFrame.y), float(relPos.x) > 0.0);
	misc.x         = info.x - (curAdjustedFrame - thisVertexCreateFrame) * info.y;
	float alphaMax = info.x - (curAdjustedFrame -  float(createFrame.y)) * info.y;

	#if 1
	if (alphaMax <= 0.0f) {
		vPosT = vec4(0.0);
		vPosB = vec4(0.0);
		gl_Position = vec4(2.0, 2.0, 2.0, 1.0); //place outside of [-1;1]^3 NDC, basically cull out from the further rendering
		return;
	}
	#endif

	vec3 worldPos = vec3(0.0);

	misc2.x = sin(info.z);
	misc2.y = cos(info.z);
	mat2 rotMat = GetRotMat2D();

	vPosT = posT;
	vPosB = posB;

	midPoint.xz = (posTL + posTR + posBR + posBL) * 0.25;

	posTL = midPoint.xz + rotMat * (posTL - midPoint.xz);
	posTR = midPoint.xz + rotMat * (posTR - midPoint.xz);
	posBR = midPoint.xz + rotMat * (posBR - midPoint.xz);
	posBL = midPoint.xz + rotMat * (posBL - midPoint.xz);

	// max flat distance
	midPoint.w =                 distance(midPoint.xz, posTL);
	midPoint.w = max(midPoint.w, distance(midPoint.xz, posTR));
	midPoint.w = max(midPoint.w, distance(midPoint.xz, posBR));
	midPoint.w = max(midPoint.w, distance(midPoint.xz, posBL));

	vuvMain = uvMain;
	vuvNorm = uvNorm;

	vec4 testResults = vec4(
		float(all( equal(vec2(-1.0, -1.0), relPos.xz) )),
		float(all( equal(vec2( 1.0, -1.0), relPos.xz) )),
		float(all( equal(vec2( 1.0,  1.0), relPos.xz) )),
		float(all( equal(vec2(-1.0,  1.0), relPos.xz) ))
	);

	worldPos.xz += testResults.x * posTL;
	worldPos.xz += testResults.y * posTR;
	worldPos.xz += testResults.z * posBR;
	worldPos.xz += testResults.w * posBL;

	#if 1
	vec3 avgGroundNormal = normalize(
		GetFragmentNormal(midPoint.xz) +
		GetFragmentNormal(posTL) +
		GetFragmentNormal(posTR) +
		GetFragmentNormal(posBR) +
		GetFragmentNormal(posBL)
	);
	#else
	vec3 avgGroundNormal = GetFragmentNormal(midPoint.xz);
	#endif

	// mid-point height
	midPoint.y = HeightAtWorldPos(midPoint.xz);

	// effect's height
	misc.y = info.w;
	
	// store uvWrapDistance
	misc.z = uvParams.x;
	
	// store traveled distance
	misc.w = uvParams.y;

	worldPos.y = midPoint.y + relPos.y * misc.y;

	// do not rotate almost vertical cubes
	if (1.0 - avgGroundNormal.y > 0.1) {
		// rotAxis is cross(Upvector, N), but Upvector is known to be (0, 1, 0), so simplify
		vec3 rotAxis = normalize(vec3(avgGroundNormal.z, 0.0, -avgGroundNormal.x));
		// rotate the cube (through its vertices) to align with the terrain normal
		worldPos = midPoint.xyz + RotateByNormalVector(worldPos - midPoint.xyz, avgGroundNormal, rotAxis);
	}

	gl_Position = gl_ModelViewProjectionMatrix * vec4(worldPos, 1.0);
}
