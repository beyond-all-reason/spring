/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * Geometry-shader path, stage 1 of 3. One vertex per particle; the geometry
 * shader expands it into the shape and its halo.
 *
 * Position is reconstructed rather than uploaded per frame: the CPU only
 * rewrites a particle when it is re-aimed, and then moves `baseFrame` forward
 * with it. `createFrame` stays put so the per-particle hash - and therefore the
 * particle's size, alpha and tumble - does not jump when that happens.
 */

#version 150 compatibility

in vec3 particleStartPos;
in vec3 particleVelocity;
in vec4 particleFrames;  // x = createFrame, y = deathFrame, z = baseFrame, w = fadeFrames
in vec4 particleColor;

uniform float animationFrame;

out vec3 v_velocity;
out vec3 v_lifetime;  // x = createFrame, y = deathFrame, z = fadeFrames
out vec4 v_color;

void main()
{
	float motionAge = max(animationFrame - particleFrames.z, 0.0);

	gl_Position = vec4(particleStartPos + particleVelocity * motionAge, 1.0);
	v_velocity = particleVelocity;
	v_lifetime = particleFrames.xyw;
	v_color = particleColor;
}
