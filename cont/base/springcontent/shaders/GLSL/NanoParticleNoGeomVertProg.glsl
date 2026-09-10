/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * Instanced path, stage 1 of 2. Used where geometry shaders are unavailable or
 * NanoParticlesNoGeometryShader is set.
 *
 * Runs the same maths as NanoParticleGeomProg.glsl, but per template-mesh
 * vertex instead of per emitted vertex: the mesh supplies the cube corners and
 * the halo quad, and this shader places them. The helpers below are a verbatim
 * copy of the geometry shader's; keep the two in step.
 */

#version 150 compatibility

// static template mesh (per vertex)
in vec3 templatePosition;
in vec3 templateNormal;
in vec2 templateGlowUV;
in float templateIsGlow;

// particle (per instance)
in vec3 particleStartPos;
in vec3 particleVelocity;
in vec4 particleFrames;  // x = createFrame, y = deathFrame, z = baseFrame, w = fadeFrames
in vec4 particleColor;

uniform float animationFrame;
uniform vec3 cameraRight;
uniform vec3 cameraUp;
uniform vec4 clipPlane;

uniform float drawRadius;
uniform float sizeVariation;
uniform float baseAlpha;
uniform float alphaVariation;
uniform float glowScale;
uniform float colorEqualize;
uniform float colorTargetLuma;
uniform float rotationRange;
uniform float rotationRatePerFrame;
/// World-space frustum planes of the active camera, same convention as CCamera::Frustum.
uniform vec4 frustumPlanes[6];

out vec4 g_color;
out vec3 g_normal;
out vec3 g_worldPos;
out vec3 g_localPos;
out vec3 g_noiseSeed;
out vec2 g_glowUV;
out float g_isGlow;
out float g_seed;
/// 1 = full strength, 0 = gone. Applied to the whole fragment, so the halo fades with the shape.
out float g_fade;
out float gl_ClipDistance[1];

const float TAU = 6.2831853;
const vec3 LUMA_WEIGHTS = vec3(0.2126, 0.7152, 0.0722);

bool outsideFrustum(vec3 center, float radius)
{
	for (int i = 0; i < 6; ++i) {
		if (dot(frustumPlanes[i].xyz, center) + frustumPlanes[i].w < -radius)
			return true;
	}

	return false;
}

/* There is no geometry stage to return from here, so a vertex that should not
 * exist is pushed outside clip space and degenerates instead. */
void discardVertex()
{
	g_color = vec4(0.0);
	g_normal = vec3(0.0);
	g_worldPos = vec3(0.0);
	g_localPos = vec3(0.0);
	g_noiseSeed = vec3(0.0);
	g_glowUV = vec2(0.0);
	g_isGlow = 0.0;
	g_seed = 0.0;
	g_fade = 0.0;
	gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
	gl_ClipDistance[0] = 1.0;
}

float hash11(float value)
{
	return fract(sin(value) * 43758.5453);
}

vec3 equalizeColor(vec3 color)
{
	float luma = dot(color, LUMA_WEIGHTS);
	if (luma < 0.001)
		return color;

	color *= pow(colorTargetLuma / luma, colorEqualize);

	float maxChannel = max(color.r, max(color.g, color.b));
	if (maxChannel > 1.0)
		color /= maxChannel;

	return color;
}

mat3 rotXYZ(vec3 angle)
{
	float cx = cos(angle.x), sx = sin(angle.x);
	float cy = cos(angle.y), sy = sin(angle.y);
	float cz = cos(angle.z), sz = sin(angle.z);
	mat3 rotateX = mat3(1, 0, 0, 0, cx, sx, 0, -sx, cx);
	mat3 rotateY = mat3(cy, 0, -sy, 0, 1, 0, sy, 0, cy);
	mat3 rotateZ = mat3(cz, sz, 0, -sz, cz, 0, 0, 0, 1);
	return rotateZ * rotateY * rotateX;
}

void main()
{
	float createFrame = particleFrames.x;
	float deathFrame = particleFrames.y;
	float baseFrame = particleFrames.z;
	float fadeFrames = particleFrames.w;

	// the buffer is rebuilt on a cadence, so a particle can outlive its last upload
	if (animationFrame >= deathFrame) {
		discardVertex();
		return;
	}

	float age = max(animationFrame - createFrame, 0.0);
	float motionAge = max(animationFrame - baseFrame, 0.0);
	float fade = clamp((deathFrame - animationFrame) / max(fadeFrames, 0.01), 0.0, 1.0);
	vec3 center = particleStartPos + particleVelocity * motionAge;

	float particleHash = dot(particleVelocity, vec3(12.9898, 78.233, 37.719)) + createFrame * 0.6180339;
	vec3 randomValues = vec3(
		hash11(particleHash + 1.7),
		hash11(particleHash + 3.3),
		hash11(particleHash + 5.9)
	);

	float sizeMultiplier = 1.0 + sizeVariation * (hash11(particleHash + 7.1) * 2.0 - 1.0);
	// shrinks all the way to nothing, so the end of a fade is never a pop
	float size = drawRadius * sizeMultiplier * fade;
	float alpha = baseAlpha * (1.0 + alphaVariation * (hash11(particleHash + 11.3) * 2.0 - 1.0));

	float haloSize = size * glowScale;

	if (outsideFrustum(center, haloSize)) {
		discardVertex();
		return;
	}

	g_color = vec4(equalizeColor(particleColor.rgb), max(alpha, 0.0));
	g_seed = randomValues.x * TAU;
	g_fade = fade;

	vec3 worldOffset;
	if (templateIsGlow > 0.5) {
		worldOffset = (cameraRight * templateGlowUV.x + cameraUp * templateGlowUV.y) * haloSize;
		g_normal = vec3(0.0, 1.0, 0.0);
		g_localPos = vec3(0.0);
		g_noiseSeed = vec3(0.0);
		g_glowUV = templateGlowUV;
		g_isGlow = 1.0;
	} else {
		float rotValue = mix(-rotationRange, rotationRange, hash11(particleHash + 13.7));
		float rotVelocity = mix(-rotationRatePerFrame, rotationRatePerFrame, hash11(particleHash + 17.9));
		float rotation = radians(rotValue + rotVelocity * age);
		vec3 phase = randomValues * TAU;
		mat3 rotationMatrix = rotXYZ(phase + vec3(rotation, rotation * 1.3, rotation * 0.7));

		worldOffset = rotationMatrix * (templatePosition * size);
		g_normal = rotationMatrix * templateNormal;
		g_localPos = worldOffset;
		g_noiseSeed = randomValues * (360.0 * 137.0) + vec3(11.0, 47.0, 83.0);
		g_glowUV = vec2(0.0);
		g_isGlow = 0.0;
	}

	g_worldPos = center + worldOffset;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(g_worldPos, 1.0);
	gl_ClipDistance[0] = dot(vec4(g_worldPos, 1.0), clipPlane);
}
