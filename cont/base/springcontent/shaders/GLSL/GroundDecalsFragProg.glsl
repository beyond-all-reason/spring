#version 130

uniform sampler2D depthTex;
uniform sampler2D decalMainTex;
uniform sampler2D decalNormTex;

uniform sampler2D groundNormalTex;

uniform vec4 mapDims; //mapxy; 1.0 / mapxy

uniform vec4 groundAmbientColor; // + groundShadowDensity
uniform vec3 groundDiffuseColor;
uniform vec3 sunDir;

#ifdef HAVE_SHADOWS
uniform sampler2DShadow shadowTex;
uniform sampler2D shadowColorTex;
uniform mat4 shadowMatrix;
#endif

uniform float curAdjustedFrame;

in vec4 vPosT;
in vec4 vPosB;
in vec4 vuvMain;
in vec4 vuvNorm;
in vec4 midPoint;
in vec4 misc; //misc.x - alpha & glow, misc.y - height, misc.z - uvWrapDistance, misc.w - distance from left
noperspective in vec2 screenUV;

out vec4 fragColor;

#define posTL vPosT.xy
#define posTR vPosT.zw
#define posBR vPosB.xy
#define posBL vPosB.zw

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

#line 200051

vec3 GetTriangleBarycentric(vec2 p, vec2 p0, vec2 p1, vec2 p2) {
	vec2 v0 = p2 - p0;
	vec2 v1 = p1 - p0;
	vec2 v2 = p  - p0;

	float dot00 = dot(v0, v0);
	float dot01 = dot(v0, v1);
	float dot02 = dot(v0, v2);
	float dot11 = dot(v1, v1);
	float dot12 = dot(v1, v2);

	float invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);

	float s = (dot11 * dot02 - dot01 * dot12) * invDenom;
	float t = (dot00 * dot12 - dot01 * dot02) * invDenom;
	float q = 1.0 - s - t;
	return vec3(s,t,q);
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
	normal.xz = texture2D(groundNormalTex, wxz * mapDims.zw).ra;
	normal.y  = sqrt(1.0 - dot(normal.xz, normal.xz));
	return normal;
}

vec3 GetShadowColor(vec3 worldPos) {
#ifdef HAVE_SHADOWS
	vec4 shadowPos = shadowMatrix * vec4(worldPos, 1.0);
	shadowPos.xy += vec2(0.5);

	vec3 shadowColor = texture(shadowColorTex, shadowPos.xy).rgb;
	return mix(vec3(1.0), textureProj(shadowTex, shadowPos) * shadowColor, groundAmbientColor.w);
#else
	return vec3(1.0);
#endif
}


// Calculate out of the fragment in screen space the view space position.
vec4 GetWorldPos(vec2 texCoord, float sampledDepth) {
	vec4 projPosition = vec4(0.0, 0.0, 0.0, 1.0);

	//texture space [0;1] to NDC space [-1;1]
	#ifdef DEPTH_CLIP01
		//don't transform depth as it's in the same [0;1] space
		projPosition.xyz = vec3(NORM2SNORM(texCoord), sampledDepth);
	#else
		projPosition.xyz = NORM2SNORM(vec3(texCoord, sampledDepth));
	#endif

	vec4 pos = gl_ModelViewProjectionMatrixInverse * projPosition;
	pos /= pos.w;

	return pos;
}

const vec3 all0 = vec3(0.0);
const vec3 all1 = vec3(1.0);
void main() {
	float depthZO = texture(depthTex, screenUV).x;
	vec4 worldPos = GetWorldPos(screenUV, depthZO);

	vec4 uvBL = vec4(uvMainBL, uvNormBL);
	vec4 uvTL = vec4(uvMainTL, uvNormTL);
	vec4 uvTR = vec4(uvMainTR, uvNormTR);
	vec4 uvBR = vec4(uvMainBR, uvNormBR);

	float u = 1.0;
	if (misc.z > 0.0) {
		u = distance((posTL + posBL) * 0.5, (posBR + posTR) * 0.5) / misc.z;
	}

	vec4 relUV;
	bool disc = true;
	{
		vec3 bc = GetTriangleBarycentric(worldPos.xz, posBL, posTL, posTR);
		if (all(greaterThanEqual(bc, all0)) && all(lessThanEqual(bc, all1))) {
			disc = false;
			//uv = bc.x * uvBL + bc.y * uvTL + bc.z * uvTR;
			relUV = bc.x * vec4(0, u*1, 0, 1) + bc.y * vec4(0, 0, 0, 0) + bc.z * vec4(1, 0, 1, 0);
		}
	}
	if (disc)
	{
		vec3 bc = GetTriangleBarycentric(worldPos.xz, posTR, posBR, posBL);
		if (all(greaterThanEqual(bc, all0)) && all(lessThanEqual(bc, all1))) {
			disc = false;
			//uv = bc.x * uvTR + bc.y * uvBR + bc.z * uvBL;
			relUV = bc.x * vec4(1, 0, 1, 0) + bc.y * vec4(1, u*1, 1, 1) + bc.z * vec4(0, u*1, 0, 1);
		}
	}

	if (disc) {
		fragColor = vec4(0.0);
		return;
		//discard;
	}

	//fragColor = vec4(vec3(GetShadowColor(worldPos.xyz)), 1.0);
	//return;

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


	/*
	if (misc.z > 0.0) {
		uv.x *= misc.w / misc.z;
		uv.x = mod(uv.x, (uvBL.x + uvTL.x) * 0.5);
	}
	*/

	float glow  = clamp(misc.x, 1.0, 2.0) - 1.0;
	float alpha = clamp(misc.x, 0.0, 1.0);

	// overglow
	glow += smoothstep(0.9, 1.0, glow) * 0.02 * abs(sin(0.08 * curAdjustedFrame));

	// distance based glow adjustment
	float relDistance = distance(worldPos.xz, midPoint.xz) / midPoint.w;
	glow *= 1.0 - smoothstep(0.1, 0.9, relDistance);

	//glow = 1.0;
	float t = mix(500.0, 4000.0, glow);


	vec4 mainCol = texture(decalMainTex, uv.xy);
	vec4 normVal = texture(decalNormTex, uv.zw);

	vec3 N = GetFragmentNormal(worldPos.xz);

	vec3 wp_dx = dFdx(worldPos.xyz);
	vec3 wp_dy = dFdy(worldPos.xyz);
	vec2 uv_dx = dFdx(uv.xy);
	vec2 uv_dy = dFdy(uv.xy);

	vec3 T = -normalize(wp_dx * uv_dy.t - wp_dy * uv_dx.t);
	//vec3 B = normalize( -wp_dx * uv_dy.s + wp_dy * uv_dx.s );
	vec3 B = cross(N, T);

	//vec3 T = normalize(cross(N, vec3(-1.0, 0.0, 0.0)));
	//vec3 T = normalize();

	//vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);

	//vec3 decalNormal = normalize(mix(N, normalize(TBN * NORM2SNORM(normVal.xyz)), alpha));
	vec3 decalNormal = normalize(TBN * NORM2SNORM(normVal.xyz));
	//decalNormal = N;

	fragColor.rgb = mainCol.rgb * (max(dot(sunDir, decalNormal), 0.0) + groundAmbientColor.rgb) * GetShadowColor(worldPos.xyz);
	fragColor.rgb += BlackBody(normVal.w * t) * glow;

	// alpha
	fragColor.a = mainCol.a;
	fragColor.a *= alpha;
	//fragColor.a *= pow(max(0.0, N.y), 2);

	if (misc.z == 0.0) {
		fragColor.a *= clamp(1.3 - abs(worldPos.y - midPoint.y) / misc.y, 0.0, 1.0); //height based elimination
	}


	//fragColor.rbg = vec3(mainCol.aaa);
	//fragColor.a = 1.0;
	//fragColor = mainCol;

	//fragColor = vec4(decalNormal, 1.0);
	//fragColor = vec4(N, 1.0);


	//fragColor = vec4(heightCol, 1);
	//fragColor = vec4(alpha, alpha, alpha, 1.0);
	//fragColor = vec4(uv.x, uv.x, uv.x, 1.0);
	//float x = mod(uv.x * misc.w / misc.z * 1.0, (uvBL.x + uvTL.x) * 0.5);
	//fragColor = vec4(x, x, x, 1.0);
	/*
	vec4 vertexShadowPos = shadowMatrix * vertexPos;
	vertexShadowPos.xy += vec2(0.5);

	float shadowCoeff = mix(1.0, shadow2DProj(shadowTex, vertexShadowPos).r, shadowDensity);

	vec4 shadeInt;
	vec4 decalInt;
	vec4 shadeCol;

	#if (HAVE_SHADING_TEX == 1)
	shadeInt = texture2D(shadeTex, gl_TexCoord[1].st);
	#else
	shadeInt = vec4(1.0, 1.0, 1.0, 1.0);
	#endif

	decalInt = texture2D(decalTex, gl_TexCoord[0].st);
	shadeCol = mix(groundAmbientColor, shadeInt, shadowCoeff * shadeInt.a);

	gl_FragColor = decalInt * shadeCol;
	gl_FragColor.a = decalInt.a * gl_Color.a;
	*/
	//fragColor = vec4(0.25, 0.25, 0.25, 1.0);
}
