#version 130

#ifdef NOSPRING
	#define SMF_INTENSITY_MULT (210.0 / 255.0)
	#define SMF_TEXSQUARE_SIZE 1024.0
	#define GBUFFER_NORMTEX_IDX 0
	#define GBUFFER_DIFFTEX_IDX 1
	#define GBUFFER_SPECTEX_IDX 2
	#define GBUFFER_EMITTEX_IDX 3
	#define GBUFFER_MISCTEX_IDX 4
#endif

#if (GL_FRAGMENT_PRECISION_HIGH == 1)
// ancient GL3 ATI drivers confuse GLSL for GLSL-ES and require this
precision highp float;
#else
precision mediump float;
#endif

/***********************************************************************/
// Consts

const float SMF_SHALLOW_WATER_DEPTH     = 10.0;
const float SMF_SHALLOW_WATER_DEPTH_INV = 1.0 / SMF_SHALLOW_WATER_DEPTH;
const float SMF_DETAILTEX_RES           = 0.02;


/***********************************************************************/
// Uniforms + Varyings + Output

in vec3 halfDir;
in float fogFactor;
in vec4 vertexWorldPos;
in vec2 diffuseTexCoords;


uniform sampler2D diffuseTex;
uniform sampler2D normalsTex;
uniform sampler2D detailTex;
#ifndef SMF_ADV_SHADING
	uniform sampler2D shadingTex;
#endif

uniform vec2 specularTexGen; // 1.0/mapSize

#ifdef SMF_ADV_SHADING
	uniform vec2 normalTexGen;   // either 1.0/mapSize (when NPOT are supported) or 1.0/mapSizePO2
	uniform vec3 groundAmbientColor;
	uniform vec3 groundDiffuseColor;
	uniform vec3 groundSpecularColor;
	uniform float groundSpecularExponent;
	uniform float groundShadowDensity;

	uniform vec2 mapHeights; // min & max height on the map

	uniform vec4 lightDir;
	uniform vec3 cameraPos;
#endif

uniform sampler2D infoTex;
uniform float infoTexIntensityMul;
uniform vec2 infoTexGen;     // 1.0/(pwr2map{x,z} * SQUARE_SIZE)

#ifdef SMF_SPECULAR_LIGHTING
	uniform sampler2D specularTex;
#endif

#ifdef HAVE_SHADOWS
	uniform sampler2DShadow shadowTex;
	uniform sampler2D shadowColorTex;
	uniform mat4 shadowMat;
#endif

#ifdef SMF_WATER_ABSORPTION
	uniform vec3 waterMinColor;
	uniform vec3 waterBaseColor;
	uniform vec3 waterAbsorbColor;
#endif

#if defined(SMF_DETAIL_TEXTURE_SPLATTING) && !defined(SMF_DETAIL_NORMAL_TEXTURE_SPLATTING)
	uniform sampler2D splatDetailTex;
	uniform sampler2D splatDistrTex;
	uniform vec4 splatTexMults;  // per-channel splat intensity multipliers
	uniform vec4 splatTexScales; // defaults to SMF_DETAILTEX_RES per channel
#endif

#ifdef SMF_DETAIL_NORMAL_TEXTURE_SPLATTING
	uniform sampler2D splatDetailNormalTex1;
	uniform sampler2D splatDetailNormalTex2;
	uniform sampler2D splatDetailNormalTex3;
	uniform sampler2D splatDetailNormalTex4;
	uniform sampler2D splatDistrTex;
	uniform vec4 splatTexMults;  // per-channel splat intensity multipliers
	uniform vec4 splatTexScales; // defaults to SMF_DETAILTEX_RES per channel
#endif
#ifdef SMF_SKY_REFLECTIONS
	uniform samplerCube skyReflectTex;
	uniform sampler2D skyReflectModTex;
#endif

#ifdef SMF_BLEND_NORMALS
	uniform sampler2D blendNormalsTex;
#endif

#ifdef SMF_LIGHT_EMISSION
	uniform sampler2D lightEmissionTex;
#endif

#ifdef SMF_PARALLAX_MAPPING
	uniform sampler2D parallaxHeightTex;
#endif

#ifdef DEFERRED_MODE
	out vec4 fragData[GBUFFER_MISCTEX_IDX + 1];
#else
	out vec4 fragColor;
#endif


/***********************************************************************/
// Helper functions

#ifdef SMF_PARALLAX_MAPPING
vec2 GetParallaxUVOffset(vec2 uv, vec3 dir) {
	vec4 texel = texture2D(parallaxHeightTex, uv);

	// RG: height in [ 0.0, 1.0] (256^2 strata)
	//  B: scale  in [ 0.0, 1.0] (256   strata), eg.  0.04 (~10.0/256.0)
	//  A: bias   in [-0.5, 0.5] (256   strata), eg. -0.02 (~75.0/256.0)
	//
	const float RMUL = 255.0 * 256.0;
	const float GMUL = 256.0;
	const float HDIV = 65536.0;

	float heightValue  = dot(texel.rg, vec2(RMUL, GMUL)) / HDIV;
	float heightScale  = texel.b;
	float heightBias   = texel.a - 0.5;
	float heightOffset = heightValue * heightScale + heightBias;

	return ((dir.xy / dir.z) * heightOffset);
}
#endif


#ifdef SMF_ADV_SHADING
	vec3 GetFragmentNormal(vec2 uv) {
		vec3 normal;
		normal.xz = texture2D(normalsTex, uv).ra;
		normal.y  = sqrt(1.0 - dot(normal.xz, normal.xz));
		return normal;
	}
#endif // SMF_ADV_SHADING

#ifndef SMF_DETAIL_NORMAL_TEXTURE_SPLATTING
vec4 GetDetailTextureColor(vec2 uv) {
	#ifndef SMF_DETAIL_TEXTURE_SPLATTING
		vec2 detailTexCoord = vertexWorldPos.xz * vec2(SMF_DETAILTEX_RES);
		vec4 detailCol = (texture2D(detailTex, detailTexCoord) * 2.0) - 1.0;
	#else
		vec4 splatTexCoord0 = vertexWorldPos.xzxz * splatTexScales.rrgg;
		vec4 splatTexCoord1 = vertexWorldPos.xzxz * splatTexScales.bbaa;
		vec4 splatDetails;
			splatDetails.r = texture2D(splatDetailTex, splatTexCoord0.st).r;
			splatDetails.g = texture2D(splatDetailTex, splatTexCoord0.pq).g;
			splatDetails.b = texture2D(splatDetailTex, splatTexCoord1.st).b;
			splatDetails.a = texture2D(splatDetailTex, splatTexCoord1.pq).a;
			splatDetails   = (splatDetails * 2.0) - 1.0;
		vec4 splatCofac = texture2D(splatDistrTex, uv) * splatTexMults;
		vec4 detailCol = vec4(dot(splatDetails, splatCofac));
	#endif
	return detailCol;
}
#else // SMF_DETAIL_NORMAL_TEXTURE_SPLATTING is defined
vec4 GetSplatDetailTextureNormal(vec2 uv, out vec2 splatDetailStrength) {
	vec4 splatTexCoord0 = vertexWorldPos.xzxz * splatTexScales.rrgg;
	vec4 splatTexCoord1 = vertexWorldPos.xzxz * splatTexScales.bbaa;
	vec4 splatCofac = texture2D(splatDistrTex, uv) * splatTexMults;

	// dot with 1's to sum up the splat distribution weights
	splatDetailStrength.x = min(1.0, dot(splatCofac, vec4(1.0)));

	vec4 splatDetailNormal;
		splatDetailNormal  = ((texture2D(splatDetailNormalTex1, splatTexCoord0.st) * 2.0 - 1.0) * splatCofac.r);
		splatDetailNormal += ((texture2D(splatDetailNormalTex2, splatTexCoord0.pq) * 2.0 - 1.0) * splatCofac.g);
		splatDetailNormal += ((texture2D(splatDetailNormalTex3, splatTexCoord1.st) * 2.0 - 1.0) * splatCofac.b);
		splatDetailNormal += ((texture2D(splatDetailNormalTex4, splatTexCoord1.pq) * 2.0 - 1.0) * splatCofac.a);

	// note: y=0.01 (pointing up) in case all splat-cofacs are zero
	splatDetailNormal.y = max(splatDetailNormal.y, 0.01);

	#ifdef SMF_DETAIL_NORMAL_DIFFUSE_ALPHA
		splatDetailStrength.y = clamp(splatDetailNormal.a, -1.0, 1.0);
	#endif

	// note: .xyz is intentionally not normalized here
	// (the normal will be rotated to worldspace first)
	return splatDetailNormal;
}
#endif

#ifdef SMF_ADV_SHADING
	vec4 GetShadeInt(float groundLightInt, vec3 groundShadowCoeff, float groundDiffuseAlpha) {
		vec4 groundShadeInt = vec4(0.0, 0.0, 0.0, 1.0);

		groundShadeInt.rgb = groundAmbientColor + groundDiffuseColor * (groundLightInt * groundShadowCoeff);
		groundShadeInt.rgb *= vec3(SMF_INTENSITY_MULT);

	#ifdef SMF_VOID_WATER
		// cut out all underwater fragments indiscriminately
		groundShadeInt.a = float(vertexWorldPos.y >= 0.0);
	#endif

	#ifdef SMF_VOID_GROUND
		// assume the map(per)'s diffuse texture provides sensible alphas
		// note that voidground overrides voidwater if *both* are enabled
		// (limiting it to just above-water fragments would be arbitrary)
		groundShadeInt.a = groundDiffuseAlpha;
	#endif

	#ifdef SMF_WATER_ABSORPTION
		// use alpha of groundShadeInt cause:
		// allow voidground maps to create holes in the seabed
		// (SMF_WATER_ABSORPTION == 1 implies voidwater is not
		// enabled but says nothing about the voidground state)
		vec4 waterShadeInt = vec4(waterBaseColor.rgb, groundShadeInt.a);
		if (mapHeights.x <= 0.0) {
			float waterShadeAlpha  = abs(vertexWorldPos.y) * SMF_SHALLOW_WATER_DEPTH_INV;
			float waterShadeDecay  = 0.2 + (waterShadeAlpha * 0.1);
			float vertexStepHeight = min(1023.0, -vertexWorldPos.y);
			float waterLightInt    = min(groundLightInt * 2.0 + 0.4, 1.0);

			// vertex below shallow water depth --> alpha=1
			// vertex above shallow water depth --> alpha=waterShadeAlpha
			waterShadeAlpha = min(1.0, waterShadeAlpha + float(vertexWorldPos.y <= -SMF_SHALLOW_WATER_DEPTH));

			waterShadeInt.rgb -= (waterAbsorbColor.rgb * vertexStepHeight);
			waterShadeInt.rgb  = max(waterMinColor.rgb, waterShadeInt.rgb);
			waterShadeInt.rgb *= vec3(SMF_INTENSITY_MULT * waterLightInt);

			// make shadowed areas darker over deeper water
			waterShadeInt.rgb *= (1.0 - waterShadeDecay * (vec3(1.0) - groundShadowCoeff));

			// if depth is greater than _SHALLOW_ depth, select waterShadeInt
			// otherwise interpolate between groundShadeInt and waterShadeInt
			// (both are already cosine-weighted)
			waterShadeInt.rgb = mix(groundShadeInt.rgb, waterShadeInt.rgb, waterShadeAlpha);
		}
		return mix(groundShadeInt, waterShadeInt, float(vertexWorldPos.y < 0.0));
	#else
		return groundShadeInt;
	#endif
	}
#endif // SMF_ADV_SHADING

/***********************************************************************/
// main()

#line 10257

void main() {
	vec2 diffTexCoords = diffuseTexCoords;
	vec2 specTexCoords = vertexWorldPos.xz * specularTexGen;
	vec2 infoTexCoords = vertexWorldPos.xz * infoTexGen;
	#ifdef SMF_ADV_SHADING
		vec2 normTexCoords = vertexWorldPos.xz * normalTexGen;

		// not calculated in the vertex shader to save varying components (OpenGL2.0 allows just 32)
		vec3 cameraDir = vertexWorldPos.xyz - cameraPos;
		vec3 normal = GetFragmentNormal(normTexCoords);
	#endif

	#if defined(SMF_BLEND_NORMALS) || defined(SMF_PARALLAX_MAPPING) || defined(SMF_DETAIL_NORMAL_TEXTURE_SPLATTING)
		// detail-normals are (assumed to be) defined within STN space
		// (for a regular vertex normal equal to <0, 1, 0>, the S- and
		// T-tangents are aligned with Spring's +x and +z (!) axes)
		vec3 tTangent = normalize(cross(normal, vec3(-1.0, 0.0, 0.0)));
		vec3 sTangent = cross(normal, tTangent);
		mat3 stnMatrix = mat3(sTangent, tTangent, normal);
	#endif


	#ifdef SMF_PARALLAX_MAPPING
	{
		// use specular-texture coordinates to index parallaxHeightTex
		// (ie. specularTex and parallaxHeightTex must have equal size)
		// cameraDir does not need to be normalized, x/z and y/z ratios
		// do not change
		vec2 uvOffset = GetParallaxUVOffset(specTexCoords, transpose(stnMatrix) * cameraDir);

		// scale the parallax offset since it is in spectex-space
		diffTexCoords += (uvOffset / (SMF_TEXSQUARE_SIZE * specularTexGen));
		normTexCoords += (uvOffset * (normalTexGen / specularTexGen));
		specTexCoords += (uvOffset);

		normal = GetFragmentNormal(normTexCoords);
	}
	#endif

	#ifdef SMF_BLEND_NORMALS
	{
		vec4 dtSample = texture2D(blendNormalsTex, normTexCoords);
		vec3 dtNormal = (dtSample.xyz * 2.0) - 1.0;

		// convert dtNormal from TS to WS before mixing
		normal = normalize(mix(normal, stnMatrix * dtNormal, dtSample.a));
	}
	#endif

	vec4 detailCol;
	#if !defined(SMF_DETAIL_NORMAL_TEXTURE_SPLATTING) || !defined(SMF_ADV_SHADING)
	{
		detailCol = GetDetailTextureColor(specTexCoords);
	}
	#else
	{
		// x-component modulates mixing of normals
		// y-component contains the detail color (splatDetailNormal.a if SMF_DETAIL_NORMAL_DIFFUSE_ALPHA)
		vec2 splatDetailStrength = vec2(0.0, 0.0);

		// note: splatDetailStrength is an OUT-param
		vec4 splatDetailNormal = GetSplatDetailTextureNormal(specTexCoords, splatDetailStrength);

		detailCol = vec4(splatDetailStrength.y);

		// convert the splat detail normal to world-space, then
		// mix it with the regular one, then normalize it again
		// to get correct specular and diffuse highlights
		normal = normalize(mix(normal, normalize(stnMatrix * splatDetailNormal.xyz), splatDetailStrength.x));
	}
	#endif

#if !defined(DEFERRED_MODE) && defined(SMF_ADV_SHADING)
	float cosAngleDiffuse = clamp(dot(lightDir.xyz, normal), 0.0, 1.0);
	float cosAngleSpecular = clamp(dot(normalize(halfDir), normal), 0.001, 1.0);
#endif

	vec4 diffuseCol = texture2D(diffuseTex, diffTexCoords);
	vec4 specularCol = vec4(0.0, 0.0, 0.0, 1.0);
	vec4 emissionCol = vec4(0.0, 0.0, 0.0, 0.0);

	#if !defined(DEFERRED_MODE) && defined(SMF_SKY_REFLECTIONS)
	{
		// cameraDir does not need to be normalized for reflect()
		vec3 reflectDir = reflect(cameraDir, normal);
		vec3 reflectCol = textureCube(skyReflectTex, reflectDir).rgb;
		vec3 reflectMod = texture2D(skyReflectModTex, specTexCoords).rgb;

		diffuseCol.rgb = mix(diffuseCol.rgb, reflectCol, reflectMod);
	}
	#endif
	#if !defined(DEFERRED_MODE) && defined(HAVE_INFOTEX)
	{
		// increase contrast and brightness for the overlays
		// TODO: make the multiplier configurable by users?
		diffuseCol.rgb += (texture2D(infoTex, infoTexCoords).rgb * infoTexIntensityMul);
		diffuseCol.rgb -= (vec3(0.5, 0.5, 0.5) * float(infoTexIntensityMul == 1.0));
	}
	#endif



	vec3 shadowCoeff = vec3(1.0);

	#if !defined(DEFERRED_MODE) && defined(HAVE_SHADOWS)
	{
		vec4 vertexShadowPos = shadowMat * vertexWorldPos;
		vertexShadowPos.xyz /= vertexShadowPos.w;
		vertexShadowPos.xy = vertexShadowPos.xy * 0.5 + 0.5;

		// same as ARB shader: shadowCoeff = 1 - (1 - shadowCoeff) * groundShadowDensity
		vec3 shadowColor = texture(shadowColorTex, vertexShadowPos.xy).rgb;
		shadowCoeff = mix(vec3(1.0), shadow2DProj(shadowTex, vertexShadowPos).r * shadowColor, groundShadowDensity);
	}
	#endif

	#ifndef DEFERRED_MODE
		#ifdef SMF_ADV_SHADING
		{
			// GroundMaterialAmbientDiffuseColor * LightAmbientDiffuseColor
			vec4 shadeInt = GetShadeInt(cosAngleDiffuse, shadowCoeff, diffuseCol.a);

			fragColor.rgb = (diffuseCol.rgb + detailCol.rgb) * shadeInt.rgb;
			fragColor.a = shadeInt.a;
		}
		#else // SMF_ADV_SHADING
		{
			fragColor.rgb = (diffuseCol.rgb + detailCol.rgb) * texture2D(shadingTex, infoTexCoords).rgb;
			fragColor.a = diffuseCol.a;
		}
		#endif // SMF_ADV_SHADING
	#endif // DEFERRED_MODE

	#ifdef SMF_LIGHT_EMISSION
	{
		// apply self-illumination aka. glow, not masked by shadows
		emissionCol = texture2D(lightEmissionTex, specTexCoords);

		#ifndef DEFERRED_MODE
		fragColor.rgb = fragColor.rgb * (1.0 - emissionCol.a) + emissionCol.rgb;
		#endif
	}
	#endif

	#ifdef SMF_ADV_SHADING
		#ifdef SMF_SPECULAR_LIGHTING
			specularCol = texture2D(specularTex, specTexCoords);
		#else
			specularCol = vec4(groundSpecularColor, 1.0);
		#endif // SMF_SPECULAR_LIGHTING

		#ifndef DEFERRED_MODE
			// sun specular lighting contribution
			#ifdef SMF_SPECULAR_LIGHTING
				float specularExp  = specularCol.a * 16.0;
			#else
				float specularExp  = groundSpecularExponent;
			#endif
			float specularPow  = pow(cosAngleSpecular, specularExp);

			vec3  specularInt  = specularCol.rgb * specularPow;
				  specularInt *= shadowCoeff;

			fragColor.rgb += specularInt;
		#endif // DEFERRED_MODE
	#endif // SMF_ADV_SHADING


#ifdef DEFERRED_MODE
	fragData[GBUFFER_NORMTEX_IDX] = vec4((normal + vec3(1.0, 1.0, 1.0)) * 0.5, 1.0);
	fragData[GBUFFER_DIFFTEX_IDX] = diffuseCol + detailCol;
	fragData[GBUFFER_SPECTEX_IDX] = specularCol;
	fragData[GBUFFER_EMITTEX_IDX] = emissionCol;
	fragData[GBUFFER_MISCTEX_IDX] = vec4(0.0, 0.0, 0.0, 0.0);

	// linearly transform the eye-space depths, might be more useful?
	// gl_FragDepth = gl_FragCoord.z / gl_FragCoord.w;
#else
	fragColor.rgb = mix(gl_Fog.color.rgb, fragColor.rgb, fogFactor);
#endif
}

