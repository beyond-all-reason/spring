#version 130

in vec4 posT; //posTL, posTR
in vec4 posB; //posBR, posBL
in vec4 uvMain; // L, T, R, B
in vec4 uvNorm; // L, T, R, B
in vec4 info; // alpha, alphaFalloff, rot, height
in vec4 createParams; //min, max (left, right) side, .z - uvWrapDistance, .w - traveled distance - used for tracks
in vec4 forcedNormalAndAlphaMult; // .xyz - forcedNormal, .w - alphaVisMult
in uint typeAndId; // { type:8; id:24; }

uniform sampler2D heightTex;
uniform sampler2D groundNormalTex;
uniform vec4 mapDims; //mapxy; 1.0 / mapxy
uniform float curAdjustedFrame;

flat out vec3 vPosTL;
flat out vec3 vPosTR;
flat out vec3 vPosBL;
flat out vec3 vPosBR;

flat out vec4 vuvMain;
flat out vec4 vuvNorm;
flat out vec4 midPoint;
     out vec4 misc; //misc.x - alpha & glow, misc.y - height, misc.z - uvWrapDistance, misc.w - uvOffset // can't be flat because of misc.x
flat out vec4 misc2; // groundNormal.xyz, decalTypeAsFloat
flat out vec3 xDir;

#define groundNormal misc2.xyz

#define NORM2SNORM(value) (value * 2.0 - 1.0)
#define SNORM2NORM(value) (value * 0.5 + 0.5)

const vec3 CUBE_VERT[36] = vec3[36](
	//RIGHT
	vec3( 1.0, -1.0, -1.0),
	vec3( 1.0, -1.0,  1.0),
	vec3( 1.0,  1.0,  1.0),
	vec3( 1.0,  1.0,  1.0),
	vec3( 1.0,  1.0, -1.0),
	vec3( 1.0, -1.0, -1.0),
	//LEFT
	vec3(-1.0, -1.0,  1.0),
	vec3(-1.0, -1.0, -1.0),
	vec3(-1.0,  1.0, -1.0),
	vec3(-1.0,  1.0, -1.0),
	vec3(-1.0,  1.0,  1.0),
	vec3(-1.0, -1.0,  1.0),
	//TOP
	vec3(-1.0,  1.0, -1.0),
	vec3( 1.0,  1.0, -1.0),
	vec3( 1.0,  1.0,  1.0),
	vec3( 1.0,  1.0,  1.0),
	vec3(-1.0,  1.0,  1.0),
	vec3(-1.0,  1.0, -1.0),
	//BOTTOM
	vec3(-1.0, -1.0, -1.0),
	vec3(-1.0, -1.0,  1.0),
	vec3( 1.0, -1.0, -1.0),
	vec3( 1.0, -1.0, -1.0),
	vec3(-1.0, -1.0,  1.0),
	vec3( 1.0, -1.0,  1.0),
	//FRONT
	vec3(-1.0, -1.0,  1.0),
	vec3(-1.0,  1.0,  1.0),
	vec3( 1.0,  1.0,  1.0),
	vec3( 1.0,  1.0,  1.0),
	vec3( 1.0, -1.0,  1.0),
	vec3(-1.0, -1.0,  1.0),
	//BACK
	vec3(-1.0,  1.0, -1.0),
	vec3(-1.0, -1.0, -1.0),
	vec3( 1.0, -1.0, -1.0),
	vec3( 1.0, -1.0, -1.0),
	vec3( 1.0,  1.0, -1.0),
	vec3(-1.0,  1.0, -1.0)
);

#line 100076

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
	normal.xz = textureLod(groundNormalTex, wxz * mapDims.zw, 0.0).ra;
	normal.y  = sqrt(1.0 - dot(normal.xz, normal.xz));
	return normal;
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
	float thisVertexCreateFrame = mix(createParams.x, createParams.y, float(relPos.x) > 0.0);
	misc.x         = info.x - (curAdjustedFrame - thisVertexCreateFrame) * info.y;
	float alphaMax = info.x - (curAdjustedFrame -        createParams.y) * info.y;
	alphaMax *= forcedNormalAndAlphaMult.w;

	misc2.w = float(typeAndId & 0xFu); //copy type only, don't care about ID

	// emulate explosion fade in for the first 6 frames, asjusted by the initial alpha (less fadein for already weak scars)
	misc.x *= mix(1.0, smoothstep(0.0, 6.0 * info.x, curAdjustedFrame - thisVertexCreateFrame), float(misc2.w == 2.0/*DECAL_EXPLOSION*/));

	#if 1
	if (alphaMax <= 0.0 || misc2.w <= 0.0) {
		vPosTL = vec3(0);
		vPosTR = vec3(0);
		vPosBL = vec3(0);
		vPosBR = vec3(0);
		gl_Position = vec4(2.0, 2.0, 2.0, 1.0); //place outside of [-1;1]^3 NDC, basically cull out from the further rendering
		return;
	}
	#endif

	float sa = sin(info.z);
	float ca = cos(info.z);

	midPoint.xz = (posT.xy + posT.zw + posB.xy + posB.zw) * 0.25;
	// mid-point height
	midPoint.y = HeightAtWorldPos(midPoint.xz);
	// flat distance from the center (only relevant for the explosion cube)
	midPoint.w = distance(midPoint.xz, posT.xy);
	// the actual check is in 3D space, so go from half diagonal in 2D to radius in 3D --> sqrt(3/2)
	midPoint.w *= 1.22474;

	// groundNormal
	if (dot(forcedNormalAndAlphaMult.xyz, forcedNormalAndAlphaMult.xyz) == 0.0) {
		groundNormal = vec3(0.0);
		groundNormal += 2.0 * GetFragmentNormal(midPoint.xz);
		groundNormal += 1.0 * GetFragmentNormal(posT.xy);
		groundNormal += 1.0 * GetFragmentNormal(posT.zw);
		groundNormal += 1.0 * GetFragmentNormal(posB.xy);
		groundNormal += 1.0 * GetFragmentNormal(posB.zw);
		groundNormal  = normalize(groundNormal);
	} else {
		groundNormal = forcedNormalAndAlphaMult.xyz;
	}

	// get 2D orthonormal system
	     xDir = vec3( ca, 0.0, sa);
	vec3 zDir = vec3(-sa, 0.0, ca);

	// don't rotate almost vertical cubes
	if (1.0 - groundNormal.y > 0.05) {
		// rotAxis is cross(Upvector, N), but Upvector is known to be (0, 1, 0), so simplify
		vec3 rotAxis = normalize(vec3(groundNormal.z, 0.0, -groundNormal.x));
		xDir = RotateByNormalVector(xDir, groundNormal, rotAxis);
		zDir = normalize(cross(xDir, groundNormal));
	}

	// orthonormal system
	mat3 ONS = mat3(xDir, groundNormal, zDir);
	//ONS = mat3(1);

	// absolute world coords, already rotated in all possible ways
	vPosTL = ONS * (vec3(posT.x, 0.0, posT.y) - vec3(midPoint.x, 0.0, midPoint.z)) + midPoint.xyz;
	vPosTR = ONS * (vec3(posT.z, 0.0, posT.w) - vec3(midPoint.x, 0.0, midPoint.z)) + midPoint.xyz;
	vPosBR = ONS * (vec3(posB.x, 0.0, posB.y) - vec3(midPoint.x, 0.0, midPoint.z)) + midPoint.xyz;
	vPosBL = ONS * (vec3(posB.z, 0.0, posB.w) - vec3(midPoint.x, 0.0, midPoint.z)) + midPoint.xyz;

	vuvMain = uvMain;
	vuvNorm = uvNorm;

	vec4 testResults = vec4(
		float(all( equal(vec2(-1.0, -1.0), relPos.xz) )),
		float(all( equal(vec2( 1.0, -1.0), relPos.xz) )),
		float(all( equal(vec2( 1.0,  1.0), relPos.xz) )),
		float(all( equal(vec2(-1.0,  1.0), relPos.xz) ))
	);

	vec3 worldPos = vec3(0);
	worldPos.xyz += testResults.x * vPosTL;
	worldPos.xyz += testResults.y * vPosTR;
	worldPos.xyz += testResults.z * vPosBR;
	worldPos.xyz += testResults.w * vPosBL;

	// effect's height
	misc.y = info.w;

	// store uvWrapDistance
	misc.z = createParams.z;

	// store traveled distance
	misc.w = createParams.w;

	worldPos += relPos.y * misc.y * groundNormal;

	gl_Position = gl_ModelViewProjectionMatrix * vec4(worldPos, 1.0);
}
