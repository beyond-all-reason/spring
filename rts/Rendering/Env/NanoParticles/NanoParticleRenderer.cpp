/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "NanoParticleRenderer.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>

#include "NanoParticleConfig.h"
#include "NanoParticleSystem.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/GL/SubState.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "Rendering/Colors.h"
#include "System/Log/ILog.h"
#include "System/SafeUtil.h"

#include "System/Misc/TracyDefs.h"

namespace NanoParticles {

Renderer* renderer = nullptr;

namespace {
	constexpr const char* SHADER_POOL = "[NanoParticles]";
	constexpr const char* SHADER_NAME_GEOM = "Nano Particles (geometry shader)";
	constexpr const char* SHADER_NAME_NOGEOM = "Nano Particles (instanced)";

	/// Attribute slots. The no-geometry path prefixes the template mesh attributes.
	constexpr GLuint ATTRIB_TEMPLATE_COUNT = 4;

	/// One vertex of the static mesh the no-geometry path instances.
	struct TemplateVertex {
		float3 position;
		float3 normal;
		float2 glowUV;
		float isGlow;
	};

	/*
	 * A unit cube (6 quads) plus a camera-facing quad for the halo. The
	 * geometry shader builds the same thing on the fly; this is only needed
	 * where geometry shaders are not an option.
	 */
	const std::vector<TemplateVertex>& GetTemplateVertices()
	{
		static const std::vector<TemplateVertex> vertices = [] {
			std::vector<TemplateVertex> result;
			result.reserve(6 * 6 + 6);

			const auto addTriangle = [&result](const float3& p0, const float3& p1, const float3& p2, const float3& normal) {
				result.emplace_back(TemplateVertex{p0, normal, {0.0f, 0.0f}, 0.0f});
				result.emplace_back(TemplateVertex{p1, normal, {0.0f, 0.0f}, 0.0f});
				result.emplace_back(TemplateVertex{p2, normal, {0.0f, 0.0f}, 0.0f});
			};
			const auto addQuad = [&addTriangle](const float3& p0, const float3& p1, const float3& p2, const float3& p3, const float3& normal) {
				addTriangle(p0, p1, p2, normal);
				addTriangle(p0, p2, p3, normal);
			};
			const auto addGlowVertex = [&result](float x, float y) {
				result.emplace_back(TemplateVertex{ZeroVector, ZeroVector, {x, y}, 1.0f});
			};

			addQuad({ 1.0f, -1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f}, { 1.0f,  0.0f,  0.0f});
			addQuad({-1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f});
			addQuad({-1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f, -1.0f}, { 0.0f,  1.0f,  0.0f});
			addQuad({-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f,  1.0f}, {-1.0f, -1.0f,  1.0f}, { 0.0f, -1.0f,  0.0f});
			addQuad({-1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f});
			addQuad({-1.0f, -1.0f, -1.0f}, {-1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f});

			addGlowVertex(-1.0f, -1.0f);
			addGlowVertex( 1.0f, -1.0f);
			addGlowVertex( 1.0f,  1.0f);
			addGlowVertex(-1.0f, -1.0f);
			addGlowVertex( 1.0f,  1.0f);
			addGlowVertex(-1.0f,  1.0f);
			return result;
		}();

		return vertices;
	}

	/// All six planes; the effect never draws in the shadow pass.
	constexpr std::uint8_t FRUSTUM_TEST_MASK = 0x3F;
} // namespace


void Renderer::InstanceBuffer::Kill()
{
	vbo.Release();
	vao.Delete();
	capacity = 0;
	count = 0;
}


void Renderer::InitStatic()
{
	RECOIL_DETAILED_TRACY_ZONE;
	KillStatic();

	renderer = new Renderer();
	renderer->Init();
}

void Renderer::KillStatic()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (renderer == nullptr)
		return;

	renderer->Kill();
	spring::SafeDelete(renderer);
}

void Draw(bool drawAboveWater, bool drawBelowWater, bool drawReflection, bool drawRefraction)
{
	if (renderer == nullptr)
		return;

	renderer->Draw(drawAboveWater, drawBelowWater, drawReflection, drawRefraction);
}

void DrawOnMinimap()
{
	if (renderer == nullptr)
		return;

	renderer->DrawOnMinimap();
}

void Renderer::Init()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (GetConfig().enabled)
		InitShader();
}

void Renderer::Kill()
{
	RECOIL_DETAILED_TRACY_ZONE;
	KillShader();

	templateVBO.Release();
	instanceBuffer.Kill();

	persistentVertices.clear();
	transientVertices.clear();
	enemyParticles.clear();
	enemyCells.clear();
	enemyCellIndices.clear();
	enemyCellCount = 0;

	uploadedGeneration = 0;
	persistentDirty = true;
	nextSyncFrame = 0;
	syncedAllyTeam = -2;
	syncedFullView = false;
	gatheredValid = false;
}

bool Renderer::Available() const
{
	return shader != nullptr && shader->IsValid();
}

void Renderer::ConfigChanged()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const Config& cfg = GetConfig();

	// only rebuild when the config actually asks for a different path than the
	// one we built; a missing geometry shader is a driver fact, not a config change
	if (shader != nullptr && builtForceNoGeometryShader != cfg.forceNoGeometryShader)
		KillShader();

	if (cfg.enabled && shader == nullptr)
		InitShader();

	if (!cfg.enabled && shader != nullptr)
		KillShader();
}


bool Renderer::InitShader()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const Config& cfg = GetConfig();

	usesGeometryShader = false;
	builtForceNoGeometryShader = cfg.forceNoGeometryShader;
	std::string geometryLog;

	if (!cfg.forceNoGeometryShader) {
		shader = shaderHandler->CreateProgramObject(SHADER_POOL, SHADER_NAME_GEOM);
		shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/NanoParticleVertProg.glsl", "", GL_VERTEX_SHADER));
		shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/NanoParticleGeomProg.glsl", "", GL_GEOMETRY_SHADER));
		shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/NanoParticleFragProg.glsl", "", GL_FRAGMENT_SHADER));
		shader->BindAttribLocation("particleStartPos", 0);
		shader->BindAttribLocation("particleVelocity", 1);
		shader->BindAttribLocation("particleFrames",   2);
		shader->BindAttribLocation("particleColor",    3);
		shader->Link();

		if (shader->IsValid()) {
			shader->Enable();
			shader->Disable();

			if (shader->Validate()) {
				usesGeometryShader = true;
				shaderGeneration = cfg.generation;
				SetShaderConfigUniforms();
				LOG_L(L_INFO, "[NanoParticles] geometry shader path initialized");
				return true;
			}
		}

		geometryLog = shader->GetLog();
		shaderHandler->ReleaseProgramObject(SHADER_POOL, SHADER_NAME_GEOM);
		shader = nullptr;
	}

	shader = shaderHandler->CreateProgramObject(SHADER_POOL, SHADER_NAME_NOGEOM);
	shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/NanoParticleNoGeomVertProg.glsl", "", GL_VERTEX_SHADER));
	shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/NanoParticleFragProg.glsl", "", GL_FRAGMENT_SHADER));
	shader->BindAttribLocation("templatePosition",  0);
	shader->BindAttribLocation("templateNormal",    1);
	shader->BindAttribLocation("templateGlowUV",    2);
	shader->BindAttribLocation("templateIsGlow",    3);
	shader->BindAttribLocation("particleStartPos",  4);
	shader->BindAttribLocation("particleVelocity",  5);
	shader->BindAttribLocation("particleFrames",    6);
	shader->BindAttribLocation("particleColor",     7);
	shader->Link();

	bool instancedValid = shader->IsValid();
	if (instancedValid) {
		shader->Enable();
		shader->Disable();
		instancedValid = shader->Validate();
	}

	if (!instancedValid) {
		LOG_L(L_WARNING,
			"[NanoParticles] no usable shader path, falling back to legacy nano projectiles."
			" geometry log:\n%s\ninstanced log:\n%s",
			geometryLog.c_str(), shader->GetLog().c_str());
		shaderHandler->ReleaseProgramObject(SHADER_POOL, SHADER_NAME_NOGEOM);
		shader = nullptr;
		return false;
	}

	if (cfg.forceNoGeometryShader)
		LOG_L(L_INFO, "[NanoParticles] instanced path selected by NanoParticlesNoGeometryShader");
	else
		LOG_L(L_WARNING, "[NanoParticles] geometry shader unavailable, using instanced path. log:\n%s", geometryLog.c_str());

	shaderGeneration = cfg.generation;
	SetShaderConfigUniforms();
	return true;
}

void Renderer::KillShader()
{
	if (shader != nullptr) {
		shaderHandler->ReleaseProgramObject(SHADER_POOL, usesGeometryShader ? SHADER_NAME_GEOM : SHADER_NAME_NOGEOM);
		shader = nullptr;
	}

	usesGeometryShader = false;
	uniformGeneration = 0;

	// the VAO encodes which attribute layout the dead program expected
	instanceBuffer.vao.Delete();

	uploadedGeneration = 0;
	persistentDirty = true;
	gatheredValid = false;
}

void Renderer::SetShaderConfigUniforms()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const AppearanceConfig& ap = GetConfig().appearance;

	shader->Enable();
	shader->SetUniform("drawRadius",            ap.drawRadius);
	shader->SetUniform("sizeVariation",         ap.sizeVariation);
	shader->SetUniform("baseAlpha",             ap.baseAlpha);
	shader->SetUniform("alphaVariation",        ap.alphaVariation);
	shader->SetUniform("glowScale",             ap.glowScale);
	shader->SetUniform("glowIntensity",         ap.glowIntensity);
	shader->SetUniform("glowFalloff",           ap.glowFalloff);
	shader->SetUniform("colorEqualize",         ap.colorEqualize);
	shader->SetUniform("colorTargetLuma",       ap.colorTargetLuma);
	shader->SetUniform("hueJitter",             ap.hueJitter);
	shader->SetUniform("coreBoost",             ap.coreBoost);
	shader->SetUniform("showInside",            ap.showInside);
	shader->SetUniform("noiseAmount",           ap.noiseAmount);
	shader->SetUniform("noiseScale",            ap.noiseScale);
	shader->SetUniform("whiteHotspot",          ap.whiteHotspot);
	shader->SetUniform("whiteHotspotThreshold", ap.whiteHotspotThreshold);
	shader->SetUniform("rotationRange",         ap.rotationRange);
	// stated per second in the config, consumed per frame by the shader
	shader->SetUniform("rotationRatePerFrame",  ap.rotationRate / GAME_SPEED);
	shader->SetUniform("noiseSpeedPerFrame",    ap.noiseSpeed / GAME_SPEED);
	shader->Disable();

	uniformGeneration = GetConfig().generation;
}


void Renderer::EnsureTemplateBuffer()
{
	if (templateVBO.GetIdRaw() != 0)
		return;

	templateVBO.Bind();
	templateVBO.New(GetTemplateVertices(), GL_STATIC_DRAW);
	templateVBO.Unbind();
}

void Renderer::SetupInstanceVAO()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const GLuint instanceBase = usesGeometryShader ? 0 : ATTRIB_TEMPLATE_COUNT;
	const GLuint attributeCount = instanceBase + 4;

	instanceBuffer.vao.Bind();

	if (!usesGeometryShader) {
		EnsureTemplateBuffer();
		templateVBO.Bind();

		for (GLuint index = 0; index < ATTRIB_TEMPLATE_COUNT; ++index) {
			glEnableVertexAttribArray(index);
			glVertexAttribDivisor(index, 0);
		}

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TemplateVertex), VA_TYPE_OFFSET(TemplateVertex, position));
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TemplateVertex), VA_TYPE_OFFSET(TemplateVertex, normal));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TemplateVertex), VA_TYPE_OFFSET(TemplateVertex, glowUV));
		glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(TemplateVertex), VA_TYPE_OFFSET(TemplateVertex, isGlow));
	}

	instanceBuffer.vbo.Bind();

	// one instance per particle in the instanced path, one vertex per particle otherwise
	const GLuint divisor = usesGeometryShader ? 0 : 1;
	for (GLuint index = instanceBase; index < attributeCount; ++index) {
		glEnableVertexAttribArray(index);
		glVertexAttribDivisor(index, divisor);
	}

	glVertexAttribPointer (instanceBase + 0, 3, GL_FLOAT,         GL_FALSE, sizeof(InstanceVertex), VA_TYPE_OFFSET(InstanceVertex, startPos));
	glVertexAttribPointer (instanceBase + 1, 3, GL_FLOAT,         GL_FALSE, sizeof(InstanceVertex), VA_TYPE_OFFSET(InstanceVertex, velocity));
	glVertexAttribPointer (instanceBase + 2, 4, GL_FLOAT,         GL_FALSE, sizeof(InstanceVertex), VA_TYPE_OFFSET(InstanceVertex, frames));
	glVertexAttribPointer (instanceBase + 3, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(InstanceVertex), VA_TYPE_OFFSET(InstanceVertex, color));

	instanceBuffer.vbo.Unbind();

	if (!usesGeometryShader)
		templateVBO.Unbind();

	instanceBuffer.vao.Unbind();

	for (GLuint index = 0; index < attributeCount; ++index)
		glDisableVertexAttribArray(index);
}

void Renderer::Upload(bool persistentChanged)
{
	ZoneScopedN("NanoParticles::Draw:Upload");
	RECOIL_DETAILED_TRACY_ZONE;
	const std::size_t persistentCount = persistentVertices.size();
	const std::size_t transientCount = transientVertices.size();

	instanceBuffer.count = persistentCount + transientCount;

	if (instanceBuffer.count == 0)
		return;

	// a reallocation drops what was already in the buffer, so both halves go again
	bool rewriteAll = persistentChanged;

	if (instanceBuffer.capacity < instanceBuffer.count) {
		instanceBuffer.capacity = std::bit_ceil(instanceBuffer.count);
		rewriteAll = true;
	}

	instanceBuffer.vbo.Bind();

	if (instanceBuffer.vbo.GetSize() < instanceBuffer.capacity * sizeof(InstanceVertex)) {
		instanceBuffer.vbo.New(instanceBuffer.capacity * sizeof(InstanceVertex), GL_STREAM_DRAW);
		rewriteAll = true;
	}

	if (rewriteAll && persistentCount > 0)
		instanceBuffer.vbo.SetBufferSubData(0, persistentCount * sizeof(InstanceVertex), persistentVertices.data());

	// the enemy half trails the own/allied one so a single draw covers both
	if (transientCount > 0)
		instanceBuffer.vbo.SetBufferSubData(persistentCount * sizeof(InstanceVertex), transientCount * sizeof(InstanceVertex), transientVertices.data());

	instanceBuffer.vbo.Unbind();

	// the VAO records the buffer binding, so it has to come after the first New()
	if (instanceBuffer.vao.GetIdRaw() == 0)
		SetupInstanceVAO();
}

void Renderer::DrawOnMinimap() const
{
	ZoneScopedN("NanoParticles::DrawOnMinimap");
	RECOIL_DETAILED_TRACY_ZONE;

	if (!Available())
		return;

	/* Reuse what the world pass already filtered: both halves have had the
	 * ally/LOS work done, so this costs one walk and no visibility tests. The
	 * data can be a frame stale, which a minimap cannot show. */
	const RenderConfig& rc = GetConfig().render;
	const float animationFrame = gs->frameNum + globalRendering->timeOffset;
	const bool drawStreaks = (rc.minimapStreakFrames > 0.0f);

	auto& pointsRB = CProjectile::GetMiniMapPointsRB();

	const auto addParticles = [&](const std::vector<InstanceVertex>& vertices) {
		for (const InstanceVertex& vertex: vertices) {
			if (animationFrame >= vertex.frames.y)
				continue;

			const float3 pos = vertex.startPos + vertex.velocity * (animationFrame - vertex.frames.z);

			if (drawStreaks)
				CProjectile::AddMiniMapVertices({pos, color4::green}, {pos + vertex.velocity * rc.minimapStreakFrames, color4::green});

			// a point survives any minimap scale; a sub-pixel streak does not
			if (rc.minimapPoints)
				pointsRB.AddVertex({pos, color4::green});
		}
	};

	addParticles(persistentVertices);
	addParticles(transientVertices);
}

void Renderer::DrawInstances() const
{
	if (instanceBuffer.count == 0 || instanceBuffer.vao.GetIdRaw() == 0)
		return;

	instanceBuffer.vao.Bind();

	if (usesGeometryShader) {
		glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(instanceBuffer.count));
	} else {
		glDrawArraysInstanced(GL_TRIANGLES, 0, static_cast<GLsizei>(GetTemplateVertices().size()), static_cast<GLsizei>(instanceBuffer.count));
	}

	instanceBuffer.vao.Unbind();
}


Renderer::InstanceVertex Renderer::MakeVertex(const Particle& particle)
{
	return InstanceVertex{
		particle.startPos,
		particle.velocity,
		{
			static_cast<float>(particle.createFrame),
			static_cast<float>(particle.deathFrame),
			static_cast<float>(particle.baseFrame),
			particle.fadeFrames,
		},
		particle.color,
	};
}

void Renderer::RebuildAllyVisibility()
{
	const bool validViewer = teamHandler.IsValidAllyTeam(syncedAllyTeam);

	everythingVisible = syncedFullView || (validViewer && losHandler->GetGlobalLOS(syncedAllyTeam));

	allyVisible.assign(std::max(0, teamHandler.ActiveAllyTeams()), std::uint8_t{0});

	if (!validViewer)
		return;

	for (std::size_t allyTeam = 0; allyTeam < allyVisible.size(); ++allyTeam)
		allyVisible[allyTeam] = teamHandler.Ally(static_cast<int>(allyTeam), syncedAllyTeam);
}

void Renderer::SyncPersistentBuffer()
{
	ZoneScopedN("NanoParticles::Draw:Sync");
	RECOIL_DETAILED_TRACY_ZONE;
	const std::uint32_t generation = system.GetGeneration();
	const int allyTeam = gu->myAllyTeam;
	const bool fullView = gu->spectatingFullView;
	const bool viewChanged = (syncedAllyTeam != allyTeam || syncedFullView != fullView);

	if (!viewChanged && uploadedGeneration == generation)
		return;

	const int frame = gs->frameNum;
	if (!viewChanged && frame < nextSyncFrame)
		return;

	syncedAllyTeam = allyTeam;
	syncedFullView = fullView;

	RebuildAllyVisibility();

	const auto& particles = system.GetParticles();

	persistentVertices.clear();
	persistentVertices.reserve(particles.size());
	enemyParticles.clear();

	for (const Particle& particle : particles) {
		const bool visible = everythingVisible
			|| (static_cast<unsigned>(particle.allyTeam) < allyVisible.size() && allyVisible[particle.allyTeam] != 0);

		if (visible)
			persistentVertices.emplace_back(MakeVertex(particle));
		else
			enemyParticles.emplace_back(particle);
	}

	BuildEnemyCells(frame);

	persistentDirty = true;
	uploadedGeneration = generation;
	nextSyncFrame = frame + GetConfig().render.bufferSyncIntervalFrames;
}

void Renderer::BuildEnemyCells(int frame)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const RenderConfig& rc = GetConfig().render;

	enemyCellIndices.clear();
	enemyCellCount = 0;

	if (enemyParticles.empty())
		return;

	enemyCellIndices.reserve(enemyParticles.size());

	/* The bins have to stay valid until the next resync, so each cell's bounds
	 * cover where its particles will have travelled to by then. */
	const float lookaheadFrames = static_cast<float>(rc.bufferSyncIntervalFrames + 1);

	for (std::uint32_t particleIndex = 0; particleIndex < enemyParticles.size(); ++particleIndex) {
		const Particle& particle = enemyParticles[particleIndex];
		const float3 currentPos = particle.startPos + particle.velocity * static_cast<float>(frame - particle.baseFrame);
		const int cellX = static_cast<int>(std::floor(currentPos.x / rc.enemyCellSize));
		const int cellZ = static_cast<int>(std::floor(currentPos.z / rc.enemyCellSize));
		const auto cellKey = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cellX)) << 32u) | static_cast<std::uint32_t>(cellZ);

		std::size_t cellIndex;
		if (const auto it = enemyCellIndices.find(cellKey); it != enemyCellIndices.end()) {
			cellIndex = it->second;
		} else {
			cellIndex = enemyCellCount++;
			enemyCellIndices[cellKey] = cellIndex;

			if (cellIndex == enemyCells.size())
				enemyCells.emplace_back();

			EnemyCell& newCell = enemyCells[cellIndex];
			newCell.particleIndices.clear();
			newCell.minPos = currentPos;
			newCell.maxPos = currentPos;
		}

		EnemyCell& cell = enemyCells[cellIndex];
		const float3 endPos = currentPos + particle.velocity * lookaheadFrames;

		cell.minPos = float3::min(cell.minPos, float3::min(currentPos, endPos));
		cell.maxPos = float3::max(cell.maxPos, float3::max(currentPos, endPos));
		cell.particleIndices.emplace_back(particleIndex);
	}
}

void Renderer::GatherVisibleEnemies(int frame)
{
	ZoneScopedN("NanoParticles::Draw:Enemies");
	transientVertices.clear();

	if (enemyParticles.empty() || gu->spectatingFullView)
		return;

	const int allyTeam = gu->myAllyTeam;
	if (!teamHandler.IsValidAllyTeam(allyTeam))
		return;

	const float cullRadius = GetConfig().render.cullRadius;
	const bool globalLos = losHandler->GetGlobalLOS(allyTeam);
	const ILosType& los = losHandler->los;
	const CLosMap& losMap = los.losMaps[allyTeam];
	const CCamera::Frustum& frustum = camera->GetFrustum();

	for (std::uint32_t cellIndex = 0; cellIndex < enemyCellCount; ++cellIndex) {
		const EnemyCell& cell = enemyCells[cellIndex];
		const float3 cellCenter = (cell.minPos + cell.maxPos) * 0.5f;
		const float cellRadius = (cell.maxPos - cell.minPos).Length() * 0.5f + cullRadius;

		if (!frustum.IntersectSphere(cellCenter, cellRadius, FRUSTUM_TEST_MASK))
			continue;

		for (const std::uint32_t particleIndex : cell.particleIndices) {
			const Particle& particle = enemyParticles[particleIndex];

			if (frame >= particle.deathFrame)
				continue;

			const float3 simPos = particle.startPos + particle.velocity * static_cast<float>(frame - particle.baseFrame);
			if (!frustum.IntersectSphere(simPos, cullRadius, FRUSTUM_TEST_MASK))
				continue;

			if (!globalLos && losMap.At(los.PosToSquare(simPos)) == 0 && losMap.At(los.PosToSquare(simPos + particle.velocity)) == 0)
				continue;

			transientVertices.emplace_back(MakeVertex(particle));
		}
	}
}


void Renderer::Draw(bool drawAboveWater, bool drawBelowWater, bool drawReflection, bool drawRefraction)
{
	ZoneScopedN("NanoParticles::Draw");
	RECOIL_DETAILED_TRACY_ZONE;

	if (!Available())
		return;

	const Config& cfg = GetConfig();

	/* Bail before any state setup: the reflection and refraction passes are half
	 * of the draws per frame, and the per-pass overhead dwarfs what the particles
	 * add to a water surface. */
	if ((drawReflection || drawRefraction) && !cfg.render.drawInWaterPasses)
		return;

	if (uniformGeneration != cfg.generation)
		SetShaderConfigUniforms();

	const int frame = gs->frameNum;
	const int camType = camera->GetCamType();

	SyncPersistentBuffer();

	/* Reuse the gathered enemy set across passes that share a camera: the
	 * above- and below-water passes differ only by clip plane and would
	 * otherwise redo the cull and the upload for identical vertex data. */
	if (!gatheredValid || gatheredDrawFrame != globalRendering->drawFrame || gatheredCamType != camType) {
		GatherVisibleEnemies(frame);
		Upload(persistentDirty);

		persistentDirty = false;
		gatheredDrawFrame = globalRendering->drawFrame;
		gatheredCamType = camType;
		gatheredValid = true;
	} else if (persistentDirty) {
		Upload(true);
		persistentDirty = false;
	}

	if (instanceBuffer.count == 0)
		return;

	static constexpr std::array<float, 4> clipPlanes[] {
		{ 0.0f,  0.0f, 0.0f, 0.0f}, // never used
		{ 0.0f, -1.0f, 0.0f, 0.0f},
		{ 0.0f,  1.0f, 0.0f, 0.0f},
		{ 0.0f,  0.0f, 0.0f, 1.0f}
	};
	const auto& clipPlane = clipPlanes[1U * drawBelowWater + 2U * drawAboveWater];

	using namespace GL::State;
	auto state = GL::SubState(
		Blending(GL_TRUE),
		BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA),
		DepthTest(GL_TRUE),
		DepthMask(GL_FALSE),
		ClipDistance<0>(GL_TRUE),
		// the shape is drawn from both sides; back faces are dimmed, not culled
		Culling(GL_FALSE)
	);

	const float3& camPos = camera->GetPos();
	const float3& camRight = camera->GetRight();
	const float3& camUp = camera->GetUp();

	/* Own and allied particles are not culled on the CPU - doing so would force
	 * a buffer rebuild every time the camera moves. The shader rejects them
	 * instead, before the expensive stage that expands one particle into a
	 * shape and a halo. */
	const CCamera::Frustum& frustum = camera->GetFrustum();

	shader->Enable();
	shader->SetUniform("animationFrame", frame + globalRendering->timeOffset);
	shader->SetUniform("cameraPos",   camPos.x,   camPos.y,   camPos.z);
	shader->SetUniform("cameraRight", camRight.x, camRight.y, camRight.z);
	shader->SetUniform("cameraUp",    camUp.x,    camUp.y,    camUp.z);
	shader->SetUniform("clipPlane", clipPlane[0], clipPlane[1], clipPlane[2], clipPlane[3]);
	shader->SetUniform4v("frustumPlanes", static_cast<GLsizei>(frustum.planes.size()), &frustum.planes[0].x);

	{
		ZoneScopedN("NanoParticles::Draw:Submit");
		DrawInstances();
	}

	shader->Disable();
}

} // namespace NanoParticles
