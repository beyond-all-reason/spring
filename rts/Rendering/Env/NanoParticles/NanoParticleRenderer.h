/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "NanoParticleDefs.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VAO.h"
#include "Rendering/GL/VBO.h"
#include "System/UnorderedMap.hpp"

namespace Shader {
	struct IProgramObject;
}

namespace NanoParticles {

/*
 * Draws the particles the System owns.
 *
 * Two shader paths produce the same picture:
 *   - a geometry shader expands one point per particle into the shape and its
 *     halo. One vertex per particle, so it is the cheaper of the two;
 *   - where geometry shaders are unavailable (or NanoParticlesNoGeometryShader
 *     is set), the same maths runs in a vertex shader over an instanced
 *     template mesh.
 *
 * Particles are split by visibility rather than re-uploaded wholesale each
 * frame. Own and allied particles cannot become invisible, so they live in a
 * persistent buffer that is rebuilt on a fixed cadence; particles of other
 * allyteams have to be LOS-tested continuously, so they are gathered into a
 * transient buffer per draw.
 */
class Renderer {
public:
	/*
	 * Heap-allocated after the GL context exists, like every other drawer in the
	 * engine, and for the same reason: a VBO's constructor calls
	 * VBO::IsSupported(), which latches the GLAD extension flags into
	 * function-local statics on its first call. Constructing one before GLAD has
	 * loaded would latch them all to false and silently turn every VBO in the
	 * process into a no-op.
	 */
	static void InitStatic();
	static void KillStatic();

	/// True once a shader path is up and particles can actually be drawn.
	bool Available() const;

	/// Re-reads the config-derived shader state; called after a live config change.
	void ConfigChanged();

	void Draw(bool drawAboveWater, bool drawBelowWater, bool drawReflection, bool drawRefraction);

	/// Adds this frame's visible particles to the shared projectile minimap buffer.
	void DrawOnMinimap() const;

private:
	void Init();
	void Kill();

	/// One particle as uploaded. Motion is reconstructed in the shader.
	struct InstanceVertex {
		float3 startPos;
		float3 velocity;
		/// x = createFrame (hash seed), y = deathFrame, z = baseFrame (motion origin), w = fadeFrames (ramp before death).
		float4 frames;
		SColor color;
	};

	/*
	 * One buffer holds both halves of the live set: the own/allied particles
	 * first, then the LOS-filtered enemy ones. They are refreshed on different
	 * cadences but live back to back, so the whole thing draws in a single call
	 * instead of one per half.
	 */
	struct InstanceBuffer {
		VBO vbo{GL_ARRAY_BUFFER};
		VAO vao;
		/// Vertices the buffer can hold.
		std::size_t capacity = 0;
		/// Vertices to draw: own/allied followed by enemy.
		std::size_t count = 0;

		void Kill();
	};

	/// Bucket of enemy particles sharing a map cell, so the frustum test is done per cell.
	struct EnemyCell {
		std::vector<std::uint32_t> particleIndices;
		float3 minPos;
		float3 maxPos;
	};

	bool InitShader();
	void KillShader();
	void SetShaderConfigUniforms();

	void EnsureTemplateBuffer();
	void SetupInstanceVAO();
	void Upload(bool persistentChanged);
	void DrawInstances() const;

	static InstanceVertex MakeVertex(const Particle& particle);
	void RebuildAllyVisibility();

	void SyncPersistentBuffer();
	void BuildEnemyCells(int frame);
	void GatherVisibleEnemies(int frame);

	Shader::IProgramObject* shader = nullptr;
	bool usesGeometryShader = false;
	/// What NanoParticlesNoGeometryShader said when the program was built.
	bool builtForceNoGeometryShader = false;
	/// Config generation the shader uniforms were last set from.
	std::uint32_t uniformGeneration = 0;
	/// Config generation the shader program itself was built for.
	std::uint32_t shaderGeneration = 0;

	/// Static shape+halo mesh the no-geometry path instances.
	VBO templateVBO{GL_ARRAY_BUFFER};

	InstanceBuffer instanceBuffer;

	std::vector<InstanceVertex> persistentVertices;
	std::vector<InstanceVertex> transientVertices;

	std::vector<Particle> enemyParticles;
	std::vector<EnemyCell> enemyCells;
	spring::unordered_map<std::uint64_t, std::size_t> enemyCellIndices;
	std::size_t enemyCellCount = 0;

	/// Set when the own/allied half changed and has to be rewritten.
	bool persistentDirty = true;
	/// Particle-set generation the persistent half was built from.
	std::uint32_t uploadedGeneration = 0;
	int nextSyncFrame = 0;
	/// Allyteam/spectator state the split was made for; a change forces a resync.
	int syncedAllyTeam = -2;
	bool syncedFullView = false;

	/*
	 * The own/enemy split is decided per particle over the whole live set, so the
	 * test has to be a single lookup rather than a walk through teamHandler and
	 * losHandler. Rebuilt once per sync.
	 */
	bool everythingVisible = false;
	std::vector<std::uint8_t> allyVisible;

	/*
	 * Draw() runs up to four times per rendered frame: above- and below-water
	 * from CWorldDrawer, plus the water reflection and refraction passes. The
	 * above/below pair differ only by clip plane and share a camera, so the
	 * gathered enemy set and its upload are reused rather than rebuilt. The
	 * water passes use a different camera and do rebuild.
	 */
	std::uint32_t gatheredDrawFrame = 0;
	int gatheredCamType = -1;
	bool gatheredValid = false;
};

/// Null until InitStatic(); headless and pre-GL code must tolerate that.
extern Renderer* renderer;

/// Null-safe wrappers for the draw call sites.
void Draw(bool drawAboveWater, bool drawBelowWater, bool drawReflection, bool drawRefraction);
void DrawOnMinimap();

} // namespace NanoParticles
