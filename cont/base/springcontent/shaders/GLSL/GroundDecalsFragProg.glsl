#ifdef HIGH_QUALITY
	uniform sampler2DMS depthTex;
#else
	uniform sampler2D   depthTex;
#endif

uniform sampler2D decalMainTex;
uniform sampler2D decalNormTex;

uniform sampler2D groundNormalTex;
uniform sampler2D miniMapTex;
uniform sampler2D infoTex;

uniform vec4 mapDims; //mapxy; 1.0 / mapxy
uniform vec4 mapDimsPO2; //mapxyPO2; 1.0 / mapxyPO2
uniform float infoTexIntensityMul;

uniform vec4 groundAmbientColor; // + groundShadowDensity
uniform vec3 groundDiffuseColor;

#ifdef SMF_WATER_ABSORPTION
uniform vec3 waterMinColor;
uniform vec3 waterBaseColor;
uniform vec3 waterAbsorbColor;
#endif

uniform vec3 sunDir;

uniform vec2 screenSizeInverse; // 1/X, 1/Y

#ifdef HAVE_SHADOWS
uniform sampler2DShadow shadowTex;
uniform sampler2D shadowColorTex;
uniform mat4 shadowMatrix;
#endif

uniform float curAdjustedFrame;

flat in vec3 vPosTL;
flat in vec3 vPosTR;
flat in vec3 vPosBL;
flat in vec3 vPosBR;

flat in vec4 vuvMain;
flat in vec4 vuvNorm;
flat in vec4 midPoint;
     in vec4 misc; //misc.x - alpha & glow, misc.y - height, misc.z - uvWrapDistance, misc.w - distance from left // can't be flat because of misc.x
flat in vec4 misc2; // groundNormal.xyz, decalTypeAsFloat
flat in vec3 xDir;

#define groundNormal misc2.xyz

out vec4 fragColor;

#define uvMainTL vuvMain.xy
#define uvMainTR vuvMain.zy
#define uvMainBR vuvMain.zw
#define uvMainBL vuvMain.xw

#define uvNormTL vuvNorm.xy
#define uvNormTR vuvNorm.zy
#define uvNormBR vuvNorm.zw
#define uvNormBL vuvNorm.xw

#define NORM2SNORM(value) (value * 2.0 - 1.0)
#define SNORM2NORM(value) (value * 0.5 + 0.5)

#line 200068

vec3 GetTriangleBarycentric(vec3 p, vec3 p0, vec3 p1, vec3 p2) {
    vec3 v0 = p2 - p0;
    vec3 v1 = p1 - p0;
    vec3 v2 = p - p0;

    float dot00 = dot(v0, v0);
    float dot01 = dot(v0, v1);
    float dot02 = dot(v0, v2);
    float dot11 = dot(v1, v1);
    float dot12 = dot(v1, v2);

    float invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);

    float s = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float t = (dot00 * dot12 - dot01 * dot02) * invDenom;
    float q = 1.0 - s - t;
    return vec3(s, t, q);
}

// https://www.shadertoy.com/view/MslSDl
vec3 BlackBody(float t)
{
	float u = (0.860117757 + 1.54118254e-4 * t + 1.28641212e-7 * t * t)
		/ (1.0 + 8.42420235e-4 * t + 7.08145163e-7 * t * t);

	float v = (0.317398726 + 4.22806245e-5 * t + 4.20481691e-8 * t * t)
		/ (1.0 - 2.89741816e-5 * t + 1.61456053e-7 * t * t);

	float x = 3.0 * u / (2.0 * u - 8.0 * v + 4.0);
	float y = 2.0 * v / (2.0 * u - 8.0 * v + 4.0);
	float z = 1.0 - x - y;

	float Y = 1.0;
	float X = (Y/y) * x;
	float Z = (Y/y) * z;

	mat3 XYZtosRGB = mat3(
		 3.2404542,-1.5371385,-0.4985314,
		-0.9692660, 1.8760108, 0.0415560,
		 0.0556434,-0.2040259, 1.0572252
	);

	vec3 RGB = vec3(X,Y,Z) * XYZtosRGB;
	return RGB * pow(0.0004 * t, 4.0);
}

vec3 GetFragmentNormal(vec2 wxz) {
	vec3 normal;
	normal.xz = textureLod(groundNormalTex, wxz * mapDims.zw, 0.0).ra;
	normal.y  = sqrt(1.0 - dot(normal.xz, normal.xz));
	return normal;
}

// Shadow mapping functions (HighQ)

//const float goldenAngle = PI * (3.0 - sqrt(5.0));
const float goldenAngle = 2.3999632297286533222315555066336;
//const float PI = acos(0.0) * 2.0;
const float PI = 3.1415926535897932384626433832795;

// http://blog.marmakoide.org/?p=1
vec2 SpiralSNorm(int i, int N) {
	float theta = float(i) * goldenAngle;
	float r = sqrt(float(i)) / sqrt(float(N));
	return vec2 (r * cos(theta), r * sin(theta));
}

float hash12L(vec2 p) {
	const float HASHSCALE1 = 0.1031;
	vec3 p3  = fract(vec3(p.xyx) * HASHSCALE1);
	p3 += dot(p3, p3.yzx + 19.19);
	return fract((p3.x + p3.y) * p3.z);
}
// Derivatives of light-space depth with respect to texture2D coordinates
vec2 DepthGradient(vec3 xyz) {
	vec2 dZduv = vec2(0.0, 0.0);

	vec3 dUVZdx = dFdx(xyz);
	vec3 dUVZdy = dFdy(xyz);

	dZduv.x  = dUVZdy.y * dUVZdx.z;
	dZduv.x -= dUVZdx.y * dUVZdy.z;

	dZduv.y  = dUVZdx.x * dUVZdy.z;
	dZduv.y -= dUVZdy.x * dUVZdx.z;

	float det = (dUVZdx.x * dUVZdy.y) - (dUVZdx.y * dUVZdy.x);

	return dZduv / det;
}

float BiasedZ(float z0, vec2 dZduv, vec2 offset) {
	//return z0 + dot(dZduv, offset);
	return z0;
}

vec3 GetShadowColor(vec3 worldPos, float NdotL) {
#ifdef HAVE_SHADOWS
	vec4 shadowPos = shadowMatrix * vec4(worldPos, 1.0);
	shadowPos.xy += vec2(0.5);
	shadowPos /= shadowPos.w;

	vec3 shadowColor = texture(shadowColorTex, shadowPos.xy).rgb;
	#ifndef HIGH_QUALITY
		float shadowFactor = texture(shadowTex, shadowPos.xyz);
	#else
		const int shadowSamples = 3;
		const float samplingRandomness = 0.4;
		const float samplingDistance = 1.0;

		vec2 dZduv = DepthGradient(shadowPos.xyz);

		float rndRotAngle = NORM2SNORM(hash12L(gl_FragCoord.xy)) * PI / 2.0 * samplingRandomness;

		vec2 vSinCos = vec2(sin(rndRotAngle), cos(rndRotAngle));
		mat2 rotMat = mat2(vSinCos.y, -vSinCos.x, vSinCos.x, vSinCos.y);

		vec2 filterSize = vec2(samplingDistance / vec2(textureSize(shadowTex, 0)));

		float shadowFactor = 0.0;
		for (int i = 0; i < shadowSamples; ++i) {
			// SpiralSNorm return low discrepancy sampling vec2
			vec2 offset = (rotMat * SpiralSNorm( i, shadowSamples )) * filterSize;

			vec3 shadowSamplingCoord = vec3(shadowPos.xy, 0.0) + vec3(offset, BiasedZ(shadowPos.z, dZduv, offset));
			shadowSamplingCoord.xy += offset;

			shadowFactor += texture(shadowTex, shadowSamplingCoord);
		}
		shadowFactor /= float(shadowSamples);
	#endif
	shadowFactor = min(shadowFactor, smoothstep(0.0, 0.35, NdotL));

	return mix(vec3(1.0), shadowFactor * shadowColor, groundAmbientColor.w);
#else
	return vec3(1.0);
#endif
}


// Calculate out of the fragment in screen space the view space position.
vec3 GetWorldPos(vec2 texCoord, float sampledDepth) {
	vec4 projPosition = vec4(0.0, 0.0, 0.0, 1.0);

	//texture space [0;1] to NDC space [-1;1]
	#ifdef DEPTH_CLIP01
		//don't transform depth as it's in the same [0;1] space
		projPosition.xyz = vec3(NORM2SNORM(texCoord), sampledDepth);
	#else
		projPosition.xyz = NORM2SNORM(vec3(texCoord, sampledDepth));
	#endif

	vec4 pos = gl_ModelViewProjectionMatrixInverse * projPosition;

	return pos.xyz / pos.w;
}

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

const float SMF_INTENSITY_MULT = 210.0 / 255.0;
const float SMF_SHALLOW_WATER_DEPTH     = 10.0;
const float SMF_SHALLOW_WATER_DEPTH_INV = 1.0 / SMF_SHALLOW_WATER_DEPTH;

const float EPS = 3e-3;
const vec3 all0 = vec3(0.0);
const vec3 all1 = vec3(1.0);
void main() {
	#ifdef HIGH_QUALITY
		float depthZO = texelFetch(depthTex, ivec2(gl_FragCoord.xy), gl_SampleID).x;
	#else
		float depthZO = texelFetch(depthTex, ivec2(gl_FragCoord.xy),           0).x;
	#endif

	//xyBias = vec2(0);
	vec3 worldPos = GetWorldPos(gl_FragCoord.xy * screenSizeInverse, depthZO);

	vec3 worldPosProj = worldPos - dot(worldPos - midPoint.xyz, groundNormal) * groundNormal;

	vec4 uvBL = vec4(uvMainBL, uvNormBL);
	vec4 uvTL = vec4(uvMainTL, uvNormTL);
	vec4 uvTR = vec4(uvMainTR, uvNormTR);
	vec4 uvBR = vec4(uvMainBR, uvNormBR);

	float u = 1.0;
	if (misc.z > 0.0) {
		u = distance((vPosTL + vPosBL) * 0.5, (vPosBR + vPosTR) * 0.5) / misc.z;
		//u = distance(vPosTL, vPosTR) / misc.z;
	}

	vec4 relUV;
	bool disc = true;
	{
		vec3 bc = GetTriangleBarycentric(worldPosProj, vPosBL, vPosTL, vPosTR);
		if (all(greaterThanEqual(bc, all0)) && all(lessThanEqual(bc, all1))) {
			disc = false;
			//uv = bc.x * uvBL + bc.y * uvTL + bc.z * uvTR;
			relUV = bc.x * vec4(0, u*1, 0, 1) + bc.y * vec4(0, 0, 0, 0) + bc.z * vec4(1, 0, 1, 0);
		}
	}
	if (disc)
	{
		vec3 bc = GetTriangleBarycentric(worldPosProj, vPosTR, vPosBR, vPosBL);
		if (all(greaterThanEqual(bc, all0)) && all(lessThanEqual(bc, all1))) {
			disc = false;
			//uv = bc.x * uvTR + bc.y * uvBR + bc.z * uvBL;
			relUV = bc.x * vec4(1, 0, 1, 0) + bc.y * vec4(1, u*1, 1, 1) + bc.z * vec4(0, u*1, 0, 1);
		}
	}

	if (disc) {
		//fragColor = vec4(0.0);
		//return;
		discard;
	}

	vec4 uv;
	if (misc.z > 0.0) {
		relUV.y = mod(misc.w / misc.z + relUV.y, 1.0);
	}

	uv.xy = mix(
		mix(uvTL.xy, uvBL.xy, relUV.x),
		mix(uvTR.xy, uvBR.xy, relUV.x),
	relUV.y);

	uv.zw = mix(
		mix(uvTL.zw, uvBL.zw, relUV.z),
		mix(uvTR.zw, uvBR.zw, relUV.z),
	relUV.w);


	float alpha = clamp(misc.x, 0.0, 1.0);

	// glow is from 1.05 to 1.0 as capped by the engine
	// transform to 0 - 1
	float glow = smoothstep(1.0, 1.05, misc.x);

	// overglow
	glow += smoothstep(0.75, 1.0, glow) * 0.2 * abs(sin(0.02 * curAdjustedFrame));

	// distance based glow adjustment
	float relDistance = distance(worldPos.xyz, midPoint.xyz) / midPoint.w;
	//float relDistance = distance(worldPos.xz, midPoint.xz) / midPoint.w;
	relDistance = smoothstep(0.9, 0.1, relDistance);
	glow *= pow(relDistance, 7.0); // artistic choice to keep the glow centered
	glow *= smoothstep(-SMF_SHALLOW_WATER_DEPTH, 0.0, worldPos.y);

	float t = mix(1.0, 3700.0, glow);

	const vec3 LUMA = vec3(0.2125, 0.7154, 0.0721);

	vec4 mainCol = texture(decalMainTex, uv.xy);
	vec4 normVal = texture(decalNormTex, uv.zw);
	vec3 mapDiffuse = textureLod(miniMapTex, worldPos.xz * mapDims.zw, 0.0).rgb;
	#if 0
		vec3 mapDecalMix = mix(mainCol.rgb, mapDiffuse.rgb, smoothstep(0.0, 0.6, dot(mainCol.rgb, LUMA)));
	#else
		vec3 mapDecalMix = 2.0 * mainCol.rgb * mapDiffuse.rgb;
	#endif
	mainCol.rgb = mix(mainCol.rgb, mapDecalMix, float(misc2.w == 2.0/*DECAL_EXPLOSION*/)); //only apply mapDecalMix for explosions

	vec3 N = GetFragmentNormal(worldPos.xz);

	#if 0
	// Expensive processing
	vec3 T = xDir; //tangent if N was (0,1,0)

	if (1.0 - N.y > 0.01) {
		// rotAxis is cross(Upvector, N), but Upvector is known to be (0, 1, 0), so simplify
		vec3 rotAxis = normalize(vec3(N.z, 0.0, -N.x));
		T = RotateByNormalVector(T, N, rotAxis);
	}
	#else
	// Cheaper Gramm-Schmidt
	vec3 T = normalize(xDir - N * dot(xDir,  N));
	#endif

	vec3 B = normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	vec3 decalNormal = normalize(TBN * NORM2SNORM(normVal.xyz));
	float NdotL = max(dot(sunDir, decalNormal), 0.0);
	vec3 diffuseTerm = NdotL * groundDiffuseColor;

	vec3 lightCol = diffuseTerm * GetShadowColor(worldPos.xyz, dot(sunDir, N)) + groundAmbientColor.rgb;

	fragColor.rgb = mainCol.rgb * lightCol;
	fragColor.rgb += BlackBody(normVal.w * t) * glow;

	// adaptation from SMFFragProg.glsl
	#ifdef SMF_WATER_ABSORPTION
	if (worldPos.y <= 0.0) {
		vec3 waterShadeInt = waterBaseColor;

		float waterShadeAlpha  = -worldPos.y * SMF_SHALLOW_WATER_DEPTH_INV;
		float waterShadeDecay  = 0.2 + (waterShadeAlpha * 0.1);
		float vertexStepHeight = min(1023.0, -worldPos.y);
		float waterLightInt    = min(NdotL * 2.0 + 0.4, 1.0);

		// vertex below shallow water depth --> alpha=1
		// vertex above shallow water depth --> alpha=waterShadeAlpha
		waterShadeAlpha = min(1.0, waterShadeAlpha + float(worldPos.y <= -SMF_SHALLOW_WATER_DEPTH));

		waterShadeInt -= (waterAbsorbColor * vertexStepHeight);
		waterShadeInt  = max(waterMinColor, waterShadeInt);
		waterShadeInt *= vec3(SMF_INTENSITY_MULT * waterLightInt);

		// make shadowed areas darker over deeper water
		waterShadeInt *= (1.0 - waterShadeDecay);

		// if depth is greater than _SHALLOW_ depth, select waterShadeInt
		// otherwise interpolate between groundShadeInt and waterShadeInt
		// (both are already cosine-weighted)
		fragColor.rgb = mix(fragColor.rgb, waterShadeInt, waterShadeAlpha);
	}
	#endif

	fragColor.a = mainCol.a * alpha;
	// artistic adjustments
	//fragColor  *= pow(max(dot(groundNormal, N), 0.0), 1.5); // MdotL^1.5 is arbitrary
	fragColor.a *=
		smoothstep(0.0, EPS, relUV.x) * (1.0 - smoothstep(1.0 - EPS, 1.0, relUV.x)) *
		smoothstep(0.0, EPS, relUV.y) * (1.0 - smoothstep(1.0 - EPS, 1.0, relUV.y));
}
