#version 130
/**
 * @project Spring RTS
 * @file bumpWaterVS.glsl
 * @brief An extended bumpmapping water shader
 * @author jK
 *
 * Copyright (C) 2008,2009.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */

//////////////////////////////////////////////////
// runtime defined constants are:
// #define SurfaceColor       vec4
// #define DiffuseColor       vec3
// #define PlaneColor         vec4  (unused)
// #define AmbientFactor      float
// #define DiffuseFactor      float   (note: it is the map defined value multipled with 15x!)
// #define SpecularColor      vec3
// #define SpecularPower      float
// #define SpecularFactor     float
// #define PerlinStartFreq    float
// #define PerlinFreq         float
// #define PerlinAmp          float
// #define Speed              float
// #define FresnelMin         float
// #define FresnelMax         float
// #define FresnelPower       float
// #define ScreenInverse      vec2
// #define ViewPos            vec2
// #define MapMid             vec3
// #define SunDir             vec3
// #define ReflDistortion     float
// #define BlurBase           vec2
// #define BlurExponent       float
// #define PerlinStartFreq    float
// #define PerlinLacunarity   float
// #define WaveOffsetFactor   float
// #define WaveLength         float
// #define WaveFoamDistortion float
// #define WaveFoamIntensity  float
// #define CausticsResolution float
// #define CausticsStrength   float

// #define TexGenPlane      vec4
// #define ShadingPlane     vec4

//////////////////////////////////////////////////
// possible flags are:
// //#define opt_heightmap
// #define opt_reflection
// #define opt_refraction
// #define opt_shorewaves
// #define opt_depth
// #define opt_blurreflection
// #define opt_texrect
// #define opt_endlessocean

#line 10061

in vec3 pos;

uniform float frame;
uniform vec3 eyePos;
uniform vec2 windVector;

out float eyeVertexZ;
out vec3 eyeVec;
out vec3 ligVec;
out vec3 worldPos;
out vec4 texCoords[6];

void main()
{
	vec4 pos4 = vec4(pos, 1.0);
	// COMPUTE TEXCOORDS
	texCoords[0] = TexGenPlane * pos4.xzxz;
	texCoords[5].st = ShadingPlane.xy * pos4.xz;

	// COMPUTE WAVE TEXTURE COORDS
	float fstart = PerlinStartFreq;
	float f      = PerlinLacunarity;
	texCoords[1].st = (vec2(-1.0,-1.0) + texCoords[0].pq + 0.75) * fstart       + frame * windVector;
	texCoords[1].pq = (vec2(-1.0, 1.0) + texCoords[0].pq + 0.50) * fstart*f     - frame * windVector;
	texCoords[2].st = (vec2( 1.0,-1.0) + texCoords[0].pq + 0.25) * fstart*f*f   + frame * windVector;
	texCoords[2].pq = (vec2( 1.0, 1.0) + texCoords[0].pq + 0.00) * fstart*f*f*f + frame * windVector;

	texCoords[3].st = texCoords[0].pq * 160.0 + frame * 2.5;
	texCoords[3].pq = texCoords[0].pq * 90.0  - frame * 2.0;
	texCoords[4].st = texCoords[0].pq * 2.0;
	texCoords[4].pq = texCoords[0].pq * 6.0 + frame * 0.37;

	// SIMULATE WAVES
	// TODO:
	//   restrict amplitude to less than shallow water depth
	//   decrease cycling speed of caustics when zoomed out?
	vec4 waveVertex;
	waveVertex.xzw = pos4.xzw;
	waveVertex.y = 3.0 * (cos(frame * 500.0 + pos4.z) * sin(frame * 500.0 + pos4.x / 1000.0));

	// COMPUTE LIGHT VECTORS
	eyeVec = eyePos - waveVertex.xyz;
	ligVec = normalize(SunDir * 20000.0 + MapMid - waveVertex.xyz);

	// FOG
	worldPos = waveVertex.xyz;
	gl_FogFragCoord = (gl_ModelViewMatrix * waveVertex).z;

	gl_Position = gl_ModelViewProjectionMatrix * waveVertex;

	#if 0
	// distance to unperturbed vertex
	eyeVertexZ = (gl_ModelViewMatrix * pos4).z;
	#endif
}
