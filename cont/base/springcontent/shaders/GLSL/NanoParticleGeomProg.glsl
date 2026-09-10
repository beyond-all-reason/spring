/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * Geometry-shader path, stage 2 of 3. Expands one point per particle into a
 * tumbling cube plus a camera-facing additive halo.
 *
 * Everything that shapes the look is a uniform fed from NanoParticleConfig, so
 * this file holds no tunables. The helper functions below are duplicated in
 * NanoParticleNoGeomVertProg.glsl, which has to produce an identical picture;
 * the engine's shader loader has no #include, so the two copies must be kept in
 * step by hand.
 */

#version 150 compatibility

layout(points) in;
layout(triangle_strip, max_vertices = 28) out;

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

in vec3 v_velocity[];
in vec3 v_lifetime[];  // x = createFrame, y = deathFrame, z = fadeFrames
in vec4 v_color[];

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

/* Output variables are undefined after EmitVertex(), so per-particle values
 * the emit helpers need have to live outside them. */
float particleFade = 1.0;

const float TAU = 6.2831853;
const vec3 LUMA_WEIGHTS = vec3(0.2126, 0.7152, 0.0722);

/* Rejecting here rather than on the CPU keeps the vertex buffer stable while
 * the camera moves, and this is the stage worth protecting: every surviving
 * particle costs 28 emitted vertices. */
bool outsideFrustum(vec3 center, float radius)
{
	for (int i = 0; i < 6; ++i) {
		if (dot(frustumPlanes[i].xyz, center) + frustumPlanes[i].w < -radius)
			return true;
	}

	return false;
}

float hash11(float value)
{
	return fract(sin(value) * 43758.5453);
}

/* Raw team colours span a wide brightness range; without this the darker teams
 * produce nano spray that is barely visible against terrain. */
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

void emitShapeVertex(
	vec3 center,
	vec3 worldOffset,
	vec3 localPos,
	vec3 normal,
	vec4 color,
	vec3 noiseSeed,
	vec2 glowUV,
	float isGlow,
	float seed
) {
	g_color = color;
	g_normal = normal;
	g_worldPos = center + worldOffset;
	g_localPos = localPos;
	g_noiseSeed = noiseSeed;
	g_glowUV = glowUV;
	g_isGlow = isGlow;
	g_seed = seed;
	g_fade = particleFade;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(g_worldPos, 1.0);
	gl_ClipDistance[0] = dot(vec4(g_worldPos, 1.0), clipPlane);
	EmitVertex();
}

void emitFace(
	vec3 corner0,
	vec3 corner1,
	vec3 corner2,
	vec3 corner3,
	vec3 normal,
	vec3 center,
	vec4 color,
	vec3 noiseSeed,
	float seed
) {
	emitShapeVertex(center, corner0, corner0, normal, color, noiseSeed, vec2(0.0), 0.0, seed);
	emitShapeVertex(center, corner1, corner1, normal, color, noiseSeed, vec2(0.0), 0.0, seed);
	emitShapeVertex(center, corner2, corner2, normal, color, noiseSeed, vec2(0.0), 0.0, seed);
	emitShapeVertex(center, corner3, corner3, normal, color, noiseSeed, vec2(0.0), 0.0, seed);
	EndPrimitive();
}

void emitGlow(vec3 center, vec4 color, float halfSize, float seed)
{
	vec3 right = cameraRight * halfSize;
	vec3 up = cameraUp * halfSize;
	vec3 normal = vec3(0.0, 1.0, 0.0);
	vec3 noiseSeed = vec3(0.0);

	emitShapeVertex(center, -right - up, vec3(0.0), normal, color, noiseSeed, vec2(-1.0, -1.0), 1.0, seed);
	emitShapeVertex(center,  right - up, vec3(0.0), normal, color, noiseSeed, vec2( 1.0, -1.0), 1.0, seed);
	emitShapeVertex(center, -right + up, vec3(0.0), normal, color, noiseSeed, vec2(-1.0,  1.0), 1.0, seed);
	emitShapeVertex(center,  right + up, vec3(0.0), normal, color, noiseSeed, vec2( 1.0,  1.0), 1.0, seed);
	EndPrimitive();
}

void main()
{
	vec3 center = gl_in[0].gl_Position.xyz;
	float createFrame = v_lifetime[0].x;
	float deathFrame = v_lifetime[0].y;
	float fadeFrames = v_lifetime[0].z;

	if (animationFrame >= deathFrame)
		return;

	float age = max(animationFrame - createFrame, 0.0);
	/* Per particle: the appearance default normally, or the whole remaining
	 * life once the target is lost, so the spray dissolves as it coasts. */
	float fade = clamp((deathFrame - animationFrame) / max(fadeFrames, 0.01), 0.0, 1.0);
	particleFade = fade;

	/* One hash per particle drives every random-looking property. Seeded from
	 * velocity and spawn frame so it survives a re-aim unchanged. */
	float particleHash = dot(v_velocity[0], vec3(12.9898, 78.233, 37.719)) + createFrame * 0.6180339;
	vec3 randomValues = vec3(
		hash11(particleHash + 1.7),
		hash11(particleHash + 3.3),
		hash11(particleHash + 5.9)
	);

	float sizeMultiplier = 1.0 + sizeVariation * (hash11(particleHash + 7.1) * 2.0 - 1.0);
	// shrinks all the way to nothing, so the end of a fade is never a pop
	float size = drawRadius * sizeMultiplier * fade;
	float alpha = baseAlpha * (1.0 + alphaVariation * (hash11(particleHash + 11.3) * 2.0 - 1.0));
	vec4 color = vec4(equalizeColor(v_color[0].rgb), max(alpha, 0.0));

	float haloSize = size * glowScale;

	if (outsideFrustum(center, haloSize))
		return;

	float rotValue = mix(-rotationRange, rotationRange, hash11(particleHash + 13.7));
	float rotVelocity = mix(-rotationRatePerFrame, rotationRatePerFrame, hash11(particleHash + 17.9));
	float rotation = radians(rotValue + rotVelocity * age);
	vec3 phase = randomValues * TAU;
	mat3 rotationMatrix = rotXYZ(phase + vec3(rotation, rotation * 1.3, rotation * 0.7));

	vec3 noiseSeed = randomValues * (360.0 * 137.0) + vec3(11.0, 47.0, 83.0);
	float seed = randomValues.x * TAU;

	vec3 xAxis = rotationMatrix * vec3(size, 0.0, 0.0);
	vec3 yAxis = rotationMatrix * vec3(0.0, size, 0.0);
	vec3 zAxis = rotationMatrix * vec3(0.0, 0.0, size);
	vec3 normalX = rotationMatrix[0];
	vec3 normalY = rotationMatrix[1];
	vec3 normalZ = rotationMatrix[2];

	emitFace( xAxis-yAxis-zAxis,  xAxis+yAxis-zAxis,  xAxis-yAxis+zAxis,  xAxis+yAxis+zAxis,  normalX, center, color, noiseSeed, seed);
	emitFace(-xAxis-yAxis-zAxis, -xAxis-yAxis+zAxis, -xAxis+yAxis-zAxis, -xAxis+yAxis+zAxis, -normalX, center, color, noiseSeed, seed);
	emitFace(-xAxis+yAxis-zAxis, -xAxis+yAxis+zAxis,  xAxis+yAxis-zAxis,  xAxis+yAxis+zAxis,  normalY, center, color, noiseSeed, seed);
	emitFace(-xAxis-yAxis-zAxis,  xAxis-yAxis-zAxis, -xAxis-yAxis+zAxis,  xAxis-yAxis+zAxis, -normalY, center, color, noiseSeed, seed);
	emitFace(-xAxis-yAxis+zAxis,  xAxis-yAxis+zAxis, -xAxis+yAxis+zAxis,  xAxis+yAxis+zAxis,  normalZ, center, color, noiseSeed, seed);
	emitFace(-xAxis-yAxis-zAxis, -xAxis+yAxis-zAxis,  xAxis-yAxis-zAxis,  xAxis+yAxis-zAxis, -normalZ, center, color, noiseSeed, seed);

	emitGlow(center, color, haloSize, seed);
}
