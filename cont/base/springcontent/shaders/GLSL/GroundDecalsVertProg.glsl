#version 130

in vec4 forcedHeight;
in vec4 posT; //posTL, posTR
in vec4 posB; //posBR, posBL
in vec4 uvMain; // L, T, R, B
in vec4 uvNorm; // L, T, R, B
in vec4 createParams1;
in vec4 createParams2;
in vec4 createParams3;
in vec4 createParams4;
in uvec4 createParams5; // { type:8; id:24; }

uniform sampler2D heightTex;
uniform sampler2D groundNormalTex;
uniform vec4 mapDims; //mapxy; 1.0 / mapxy
uniform float curAdjustedFrame;

flat out vec4 vTranformedPos[5]; // midpos + 4 transformed vertices around midpos

flat out vec4 vuvMain;
flat out vec4 vuvNorm;

     out vec4 vData1;
flat out vec4 vData2;
flat out vec4 vData3;
flat out vec4 vData4;

flat out mat3 vRotMat;

/////////////////////////////////////////////////////////

#define posTL posT.xy
#define posTR posT.zw
#define posBR posB.xy
#define posBL posB.zw

#define refHeight          forcedHeight.x
#define minHeight          forcedHeight.y
#define maxHeight          forcedHeight.z
#define forceHeightMode    forcedHeight.w

#define alpha              createParams1.x
#define alphaFalloff       createParams1.y
#define glow               createParams1.z
#define glowFalloff        createParams1.w

#define rot                createParams2.x
#define height             createParams2.y
#define dotElimExp         createParams2.z
#define cmAlphaMult        createParams2.w

#define createFrameMin     createParams3.x
#define createFrameMax     createParams3.y
#define uvWrapDistance     createParams3.z
#define uvTraveledDistance createParams3.w

#define forcedNormal       createParams4.xyz
#define visMult            createParams4.w

#define decalType          ((createParams5.x >> 8u * 0u) & 0xFu)
#define decalId            ((createParams5.x >> 8u * 1u) & 0xFFFu)
#define texTint            vec4( \
							float((createParams5.y >> 8u * 0u) & 0xFFu) / 255.0,\
							float((createParams5.y >> 8u * 1u) & 0xFFu) / 255.0,\
							float((createParams5.y >> 8u * 2u) & 0xFFu) / 255.0,\
							float((createParams5.y >> 8u * 3u) & 0xFFu) / 255.0 \
						   )
#define glowTintMin        vec4( \
							float((createParams5.z >> 8u * 0u) & 0xFFu) / 255.0,\
							float((createParams5.z >> 8u * 1u) & 0xFFu) / 255.0,\
							float((createParams5.z >> 8u * 2u) & 0xFFu) / 255.0,\
							float((createParams5.z >> 8u * 3u) & 0xFFu) / 255.0 \
						   )
#define glowTintMax        vec4( \
							float((createParams5.w >> 8u * 0u) & 0xFFu) / 255.0,\
							float((createParams5.w >> 8u * 1u) & 0xFFu) / 255.0,\
							float((createParams5.w >> 8u * 2u) & 0xFFu) / 255.0,\
							float((createParams5.w >> 8u * 3u) & 0xFFu) / 255.0 \
						   )

/////////////////////////////////////////////////////////

#define midPoint          vTranformedPos[0]

#define vAlpha            vData1.x
#define vGlow             vData1.y
#define vDotElimExp       vData1.z
// vData1.w - empty
#define vHeight           vData2.x
#define vUVWrapDist       vData2.y
#define vUVOffset         vData2.z
#define vDecalType        vData2.w

#define vTintColor        vData3
#define vGlowColor        vData4

/////////////////////////////////////////////////////////

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

#line 100148

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

const float DECAL_EXPLOSION = 2.0;

// uv ==> L, T, R, B
void main() {
	vec3 relPos = CUBE_VERT[gl_VertexID];

	// alpha
	float thisVertexCreateFrame = mix(createFrameMin, createFrameMax, float(relPos.x) > 0.0);
	vAlpha         = alpha - (curAdjustedFrame - thisVertexCreateFrame) * alphaFalloff;
	vGlow          = glow  - (curAdjustedFrame - thisVertexCreateFrame) *  glowFalloff;
	float alphaMax = alpha - (curAdjustedFrame -        createFrameMax) * alphaFalloff;
	alphaMax *= visMult;

	vDecalType = float(decalType); //copy type only, don't care about ID

	#if 1
	if (alphaMax <= 0.0 || vDecalType <= 0.0) {
		vTranformedPos[0] = vec4(0);
		vTranformedPos[1] = vec4(0);
		vTranformedPos[2] = vec4(0);
		vTranformedPos[3] = vec4(0);
		vTranformedPos[4] = vec4(0);

		gl_Position = vec4(2.0, 2.0, 2.0, 1.0); //place outside of [-1;1]^3 NDC, basically cull out from the further rendering
		return;
	}
	#endif

	midPoint.xz = (posTL + posTR + posBL + posBR) * 0.25;

	// mid-point height
	midPoint.y = HeightAtWorldPos(midPoint.xz);

	// conditionally force relative height
	midPoint.y = mix(midPoint.y, refHeight + clamp(midPoint.y - refHeight, minHeight, maxHeight), float(forceHeightMode == 1.0));
	// conditionally force absolute height
	midPoint.y = mix(midPoint.y, clamp(midPoint.y, minHeight, maxHeight), float(forceHeightMode == 2.0));

	float sa = sin(rot);
	float ca = cos(rot);

	// groundNormal
	vec3 groundNormal = vec3(0);
	if (dot(forcedNormal.xyz, forcedNormal.xyz) == 0.0) {
		mat2 rotMat2d = mat2(ca, -sa, sa, ca);

		groundNormal += 2.0 * GetFragmentNormal(midPoint.xz);
		groundNormal += 1.0 * GetFragmentNormal(rotMat2d * (posTL - midPoint.xz) + midPoint.xz);
		groundNormal += 1.0 * GetFragmentNormal(rotMat2d * (posTR - midPoint.xz) + midPoint.xz);
		groundNormal += 1.0 * GetFragmentNormal(rotMat2d * (posBL - midPoint.xz) + midPoint.xz);
		groundNormal += 1.0 * GetFragmentNormal(rotMat2d * (posBR - midPoint.xz) + midPoint.xz);
		groundNormal  = normalize(groundNormal);
		//groundNormal  = GetFragmentNormal(midPoint.xz);
	} else {
		groundNormal = forcedNormal.xyz; //expected to be normalized by the CPU code
	}

	// get 2D orthonormal system
	vec3 xDir = vec3(ca, 0.0, sa);

	// don't rotate almost vertical cubes
	if (1.0 - groundNormal.y > 0.05) {
		// rotAxis is cross(Upvector, N), but Upvector is known to be (0, 1, 0), so simplify
		vec3 rotAxis = normalize(vec3(groundNormal.z, 0.0, -groundNormal.x));
		xDir = RotateByNormalVector(xDir, groundNormal, rotAxis);
	}
	vec3 zDir = normalize(cross(xDir, groundNormal));  // ex. (0,0,1)x(0,1,0)=(0,0,1) - righthanded coord system

	// orthonormal system
	vRotMat = mat3(xDir, groundNormal, zDir);

	//vTranformedPos[0] is handled by midPoint manipulations above

	// top cap (BL, TL, TR, BR)
	vTranformedPos[1].xyz = vRotMat * (vec3(posB.z, 0.0, posB.w) - vec3(midPoint.x, 0.0, midPoint.z)) + midPoint.xyz;
	vTranformedPos[2].xyz = vRotMat * (vec3(posT.x, 0.0, posT.y) - vec3(midPoint.x, 0.0, midPoint.z)) + midPoint.xyz;
	vTranformedPos[3].xyz = vRotMat * (vec3(posT.z, 0.0, posT.w) - vec3(midPoint.x, 0.0, midPoint.z)) + midPoint.xyz;
	vTranformedPos[4].xyz = vRotMat * (vec3(posB.x, 0.0, posB.y) - vec3(midPoint.x, 0.0, midPoint.z)) + midPoint.xyz;

	// distances
	vTranformedPos[1].w = distance(posBL, posBR) * 0.5;
	vTranformedPos[2].w = distance(posTL, posBL) * 0.5;
	vTranformedPos[3].w = distance(posTR, posTL) * 0.5;
	vTranformedPos[4].w = distance(posBR, posTR) * 0.5;

	// distance from the center (only relevant for the explosion cube)
	midPoint.w = sqrt(vTranformedPos[1].w * vTranformedPos[1].w + vTranformedPos[3].w * vTranformedPos[2].w + height * height);


	vuvMain = uvMain;
	vuvNorm = uvNorm;

	vec4 testResults = vec4(
		float(all( equal(vec2(-1.0,  1.0), relPos.xz) )),
		float(all( equal(vec2(-1.0, -1.0), relPos.xz) )),
		float(all( equal(vec2( 1.0, -1.0), relPos.xz) )),
		float(all( equal(vec2( 1.0,  1.0), relPos.xz) ))
	);

	vec3 worldPos = vec3(0);
	worldPos.xyz += testResults.x * vTranformedPos[1].xyz;
	worldPos.xyz += testResults.y * vTranformedPos[2].xyz;
	worldPos.xyz += testResults.z * vTranformedPos[3].xyz;
	worldPos.xyz += testResults.w * vTranformedPos[4].xyz;

	vTintColor = texTint;
	vGlowColor = mix(glowTintMin, glowTintMax, vAlpha * cmAlphaMult);

	// emulate explosion fade in for the first 6 frames, asjusted by the initial alpha (less fadein for already weak scars)
	vAlpha *= mix(1.0, smoothstep(0.0, 6.0 * alpha, curAdjustedFrame - thisVertexCreateFrame), float(vDecalType == DECAL_EXPLOSION));
	
	// vDotElimExp
	vDotElimExp = dotElimExp;

	// effect's height
	vHeight = height;

	// store uvWrapDistance
	vUVWrapDist = uvWrapDistance;

	// store traveled distance
	vUVOffset = uvTraveledDistance;

	worldPos += relPos.y * height * groundNormal;

	gl_Position = gl_ModelViewProjectionMatrix * vec4(worldPos, 1.0);
}
