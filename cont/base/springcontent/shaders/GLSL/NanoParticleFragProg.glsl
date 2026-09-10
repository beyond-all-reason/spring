/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * Shared final stage for both nano particle paths.
 *
 * Two kinds of fragment arrive here: the faces of the tumbling shape, and the
 * camera-facing additive halo (g_isGlow). Every tunable is a uniform fed from
 * NanoParticleConfig.
 */

#version 150 compatibility

uniform float animationFrame;
uniform vec3 cameraPos;

uniform float showInside;
uniform float noiseAmount;
uniform float noiseSpeedPerFrame;
uniform float noiseScale;
uniform float glowIntensity;
uniform float glowFalloff;
uniform float coreBoost;
uniform float hueJitter;
uniform float whiteHotspot;
uniform float whiteHotspotThreshold;

in vec4 g_color;
in vec3 g_normal;
in vec3 g_worldPos;
in vec3 g_localPos;
in vec3 g_noiseSeed;
in vec2 g_glowUV;
in float g_isGlow;
in float g_seed;
in float g_fade;

out vec4 fragColor;

const vec3 LUMA_WEIGHTS = vec3(0.2126, 0.7152, 0.0722);
/// Luma the halo tint is lifted toward, and the ceiling on that lift.
const float GLOW_TARGET_LUMA = 0.55;
const float GLOW_MAX_BOOST = 5.0;

float hash13(vec3 value)
{
	value = fract(value * 0.1031);
	value += dot(value, value.zyx + 31.32);
	return fract((value.x + value.y) * value.z);
}

float valueNoise3(vec3 value)
{
	vec3 cell = floor(value);
	vec3 fraction = fract(value);
	fraction = fraction * fraction * fraction * (fraction * (fraction * 6.0 - 15.0) + 10.0);

	float n000 = hash13(cell + vec3(0, 0, 0));
	float n100 = hash13(cell + vec3(1, 0, 0));
	float n010 = hash13(cell + vec3(0, 1, 0));
	float n110 = hash13(cell + vec3(1, 1, 0));
	float n001 = hash13(cell + vec3(0, 0, 1));
	float n101 = hash13(cell + vec3(1, 0, 1));
	float n011 = hash13(cell + vec3(0, 1, 1));
	float n111 = hash13(cell + vec3(1, 1, 1));
	vec4 xMix = mix(vec4(n000, n010, n001, n011), vec4(n100, n110, n101, n111), fraction.x);
	vec2 yMix = mix(xMix.xz, xMix.yw, fraction.y);
	return mix(yMix.x, yMix.y, fraction.z);
}

void main()
{
	vec3 tint = vec3(1.0) + hueJitter * vec3(
		sin(g_seed),
		sin(g_seed + 2.094),
		sin(g_seed + 4.188)
	);

	if (g_isGlow > 0.5) {
		float radialDistance = length(g_glowUV);
		if (radialDistance > 1.0)
			discard;

		float glow = pow(clamp(1.0 - radialDistance, 0.0, 1.0), glowFalloff) * glowIntensity;

		/* The halo carries the team colour at full saturation; normalising it
		 * first keeps a dark team's halo as bright as a light team's. */
		vec3 glowTint = g_color.rgb / max(max(g_color.r, max(g_color.g, g_color.b)), 0.001);
		float glowLuma = dot(glowTint, LUMA_WEIGHTS);
		float glowBoost = min(GLOW_TARGET_LUMA / max(glowLuma, 0.001), GLOW_MAX_BOOST);

		/* Output is premultiplied (blend is ONE, ONE_MINUS_SRC_ALPHA), so the
		 * fade has to scale colour as well as alpha or the light never dims. */
		fragColor = vec4(glowTint * tint * (glow * glowBoost), g_color.a * glow) * g_fade;
		return;
	}

	vec3 normal = normalize(g_normal);
	vec3 lightDirection = normalize(vec3(0.4, 1.0, 0.25));
	float directionalShade = 0.85 + 0.15 * max(dot(normal, lightDirection), 0.0);

	/* Back faces are dimmed rather than culled, so the shape reads as a
	 * translucent chunk instead of a flat silhouette. */
	vec3 viewDirection = normalize(cameraPos - g_worldPos);
	float normalDotView = dot(normal, viewDirection);
	float shade3D;
	float alpha3D;
	if (normalDotView >= 0.0) {
		shade3D = 0.80 + 0.45 * (1.0 - normalDotView);
		alpha3D = 1.0;
	} else {
		shade3D = 0.30 + 0.30 * (-normalDotView);
		alpha3D = 0.55;
	}

	float shade = mix(directionalShade, directionalShade * shade3D, showInside);
	float alphaMultiplier = mix(1.0, alpha3D, showInside);

	float noiseTime = animationFrame * noiseSpeedPerFrame;
	vec3 noisePosition = g_localPos * noiseScale + g_noiseSeed + vec3(noiseTime, noiseTime * 0.7, noiseTime * 1.3);
	float noiseValue = valueNoise3(noisePosition);
	shade *= 1.0 + noiseAmount * (noiseValue * 2.0 - 1.0);

	vec3 baseColor = g_color.rgb * tint * shade * coreBoost;
	float hotspot = smoothstep(whiteHotspotThreshold, 1.0, noiseValue) * whiteHotspot;
	baseColor = mix(baseColor, vec3(1.0) * max(shade, 0.6), hotspot);

	fragColor = vec4(baseColor, g_color.a * alphaMultiplier) * g_fade;
}
