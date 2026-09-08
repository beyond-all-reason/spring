/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ProjectileDrawer.h"

#include <tuple>
#include <bit>

#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Game/LoadScreen.h"
#include "Lua/LuaParser.h"
#include "Rendering/GroundFlash.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/SubState.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Models/3DModelPiece.hpp"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Common/ModelDrawerHelpers.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/PieceProjectile.h"
#include "Rendering/Env/Particles/Classes/FlyingPiece.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/RadixSort.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/ScopedResource.h"

#include "System/Misc/TracyDefs.h"

CONFIG(int, SoftParticles).defaultValue(1).safemodeValue(0).description("Soften up CEG particles on clipping edges");
CONFIG(float, ProjectileReflectionMinRadius).defaultValue(0.0f).minimumValue(0.0f).description("Alpha particles with a draw radius (in elmos) smaller than this are skipped in water reflection passes; 0 draws all of them. Small particles are barely visible in reflections, so culling them there is cheap visually and saves a large part of the reflection pass on effect-heavy frames.");
CONFIG(bool, ProjectileDrawThreadedFill).defaultValue(true).safemodeValue(false).description("Generate alpha-particle geometry on the ThreadPool workers instead of the render thread. Draw order is preserved; the few particle types whose Draw is not thread-safe stay on the render thread.");
CONFIG(bool, ProjectileDrawReuseWaterPasses).defaultValue(true).safemodeValue(false).description("When water is visible, reuse the below-water pass' alpha particle geometry for the above-water pass instead of regenerating and re-uploading it. The two passes contain identical geometry by construction; disable to force a full refill per pass.");

// sort keys are snapshotted into SortableParticle at fill time, so sorting
// works on a contiguous array instead of dereferencing two CProjectile
// pointers (= two likely cache misses) per comparison
using SortableParticle = CProjectileDrawer::SortableParticle;

// maps a float to an unsigned int with the same ordering (for non-NaN),
// inverted so that LARGER distances get SMALLER keys (back-to-front when
// sorting ascending)
static inline uint32_t DistanceSortKey(float dist) noexcept
{
	uint32_t u = std::bit_cast<uint32_t>(dist);
	u ^= static_cast<uint32_t>(static_cast<int32_t>(u) >> 31) | 0x80000000u;
	return ~u;
}

// ascending == draw order: drawOrder asc (high 32 bits), distance desc
static inline uint64_t ParticleSortKey(const CProjectile* p, uint32_t camType, bool useDrawOrder) noexcept
{
	const uint64_t orderKey = useDrawOrder ? (static_cast<uint32_t>(p->drawOrder) ^ 0x80000000u) : 0u;
	return (orderKey << 32) | DistanceSortKey(p->GetSortDist(camType));
}

// The actual ordering is done by RadixSortByKey (System/RadixSort.h) on the
// packed key: linear, input-independent cost, and stable, so equal-key
// particles (exactly coincident ones) keep their fill order, which is frame
// coherent. The previous comparison sort tiebroke them by pointer address.

// with fewer particles per chunk the for_mt dispatch overhead exceeds the
// quad-generation work being distributed
static constexpr size_t MT_FILL_MIN_CHUNK_SIZE = 96;

// Runs p->Draw() over a particle list, generating quads on the ThreadPool
// workers: the list is split into contiguous chunks, each chunk fills its own
// scratch buffer, and the scratch buffers are merged into the primary buffer
// in chunk order, so the emitted geometry is byte-identical to a serial fill.
// Particles with mtDrawSafe == false (immediate GL calls, Lua callins) split
// the list into segments and are drawn serially, in their exact position.
template<typename GetParticle>
static void FillParticleGeometry(std::vector<TypedRenderBuffer<VA_TYPE_PROJ>>& scratchBufs, size_t count, bool threaded, GetParticle&& getParticle)
{
	auto& rb = CExpGenSpawnable::GetPrimaryRenderBuffer();

	size_t segStart = 0;
	for (size_t i = 0; i <= count; ++i) {
		CProjectile* boundaryProj = (i < count) ? getParticle(i) : nullptr;
		if (boundaryProj != nullptr && boundaryProj->mtDrawSafe)
			continue;

		// [segStart, i) is a contiguous run of MT-safe particles.
		// Per-particle draw cost varies wildly (a smoke trail emits dozens of
		// quads, a small flash one), so oversubscribe chunks 4x relative to
		// the worker count; the pool load-balances them and no single heavy
		// chunk gates the whole dispatch.
		const size_t segLen = i - segStart;
		const size_t numChunks = threaded ? std::min<size_t>(ThreadPool::GetNumThreads() * 4, segLen / MT_FILL_MIN_CHUNK_SIZE) : 1;

		if (numChunks < 2) {
			for (size_t j = segStart; j < i; ++j)
				getParticle(j)->Draw();
		} else {
			// main thread only: buffer (de)registration is not thread-safe
			if (scratchBufs.size() < numChunks)
				scratchBufs.resize(numChunks);

			const size_t chunkSize = (segLen + numChunks - 1) / numChunks;
			const size_t segEnd = i;

			for_mt(0, static_cast<int>(numChunks), [&scratchBufs, &getParticle, segStart, segEnd, chunkSize](int ci) {
				ZoneScopedN("ProjectileDrawer::DrawAlpha(MTChunk)");
				auto& scratch = scratchBufs[ci];
				scratch.Clear();

				CExpGenSpawnable::SetGeomFillTarget(&scratch);
				const size_t jb = segStart + ci * chunkSize;
				const size_t je = std::min(jb + chunkSize, segEnd);
				for (size_t j = jb; j < je; ++j)
					getParticle(j)->Draw();
				CExpGenSpawnable::SetGeomFillTarget(nullptr);
			});

			{
				// merge in chunk order to preserve the back-to-front particle order
				ZoneScopedN("ProjectileDrawer::DrawAlpha(MTMerge)");
				for (size_t ci = 0; ci < numChunks; ++ci) {
					auto& scratch = scratchBufs[ci];
					const auto& verts = scratch.GetElems();
					const auto& indcs = scratch.GetIndcs();

					if (verts.empty())
						continue;

					const auto vertBias = static_cast<int32_t>(rb.GetBaseVertex());
					rb.AddVertices(verts.begin(), verts.end());
					rb.AddIndices(indcs.begin(), indcs.end(), vertBias);
				}
			}
		}

		// the boundary particle itself is not MT-safe; draw it serially in its
		// exact sorted position
		if (boundaryProj != nullptr)
			boundaryProj->Draw();

		segStart = i + 1;
	}
}

CProjectileDrawer* projectileDrawer = nullptr;

// can not be a CProjectileDrawer; destruction in global
// scope might happen after ~EventHandler (referenced by
// ~EventClient)
alignas(CProjectileDrawer) static std::byte projectileDrawerMem[sizeof(CProjectileDrawer)];


void CProjectileDrawer::InitStatic() {
	RECOIL_DETAILED_TRACY_ZONE;
	if (projectileDrawer == nullptr)
		projectileDrawer = new (projectileDrawerMem) CProjectileDrawer();

	projectileDrawer->Init();
}
void CProjectileDrawer::KillStatic(bool reload) {
	RECOIL_DETAILED_TRACY_ZONE;
	projectileDrawer->Kill();

	if (reload)
		return;

	// minimapLinesRB and minimapPointsRB are now instance members,
	// destroyed by spring::SafeDestruct() below before OpenGL context is destroyed
	spring::SafeDestruct(projectileDrawer);
	memset(projectileDrawerMem, 0, sizeof(projectileDrawerMem));
}

TypedRenderBuffer<VA_TYPE_C>& CProjectileDrawer::GetMiniMapLinesRB() { return projectileDrawer->minimapLinesRB; }
TypedRenderBuffer<VA_TYPE_C>& CProjectileDrawer::GetMiniMapPointsRB() { return projectileDrawer->minimapPointsRB; }

void CProjectileDrawer::Init() {
	RECOIL_DETAILED_TRACY_ZONE;
	eventHandler.AddClient(this);

	loadscreen->SetLoadMessage("Creating Projectile Textures");

	textureAtlas  = new CTextureAtlas(CTextureAtlas::ATLAS_ALLOC_MP_LEGACY, 0, 0, "ExplosFXAtlas", true);
	groundFXAtlas = new CTextureAtlas(CTextureAtlas::ATLAS_ALLOC_MP_LEGACY, 0, 0, "GroundFXAtlas", true);

	LuaParser resourcesParser("gamedata/resources.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	LuaParser mapResParser("gamedata/resources_map.lua", SPRING_VFS_MAP_BASE, SPRING_VFS_ZIP);

	resourcesParser.Execute();

	const LuaTable& resTable = resourcesParser.GetRoot();
	const LuaTable& resGraphicsTable = resTable.SubTable("graphics");
	const LuaTable& resProjTexturesTable = resGraphicsTable.SubTable("projectileTextures");
	const LuaTable& resSmokeTexturesTable = resGraphicsTable.SubTable("smoke");
	const LuaTable& resGroundFXTexturesTable = resGraphicsTable.SubTable("groundfx");

	// used to block resources_map.* from overriding any of
	// resources.lua:{projectile, smoke, groundfx}textures,
	// as well as various defaults (repulsegfxtexture, etc)
	spring::unordered_set<std::string> blockedTexNames;

	ParseAtlasTextures(true, resProjTexturesTable, blockedTexNames, textureAtlas);
	ParseAtlasTextures(true, resGroundFXTexturesTable, blockedTexNames, groundFXAtlas);

	textureAtlas->SetMaxTexLevel(4);
	groundFXAtlas->SetMaxTexLevel(4);

	int smokeTexCount = -1;

	{
		// get the smoke textures, hold the count in 'smokeTexCount'
		if (resSmokeTexturesTable.IsValid()) {
			for (smokeTexCount = 0; true; smokeTexCount++) {
				const std::string& tex = resSmokeTexturesTable.GetString(smokeTexCount + 1, "");
				if (tex.empty())
					break;

				const std::string texName = "bitmaps/" + tex;
				const std::string smokeName = "ismoke" + IntToString(smokeTexCount, "%02i");

				textureAtlas->AddTexFromFile(smokeName, texName);
				blockedTexNames.insert(StringToLower(smokeName));
			}
		} else {
			// setup the defaults
			for (smokeTexCount = 0; smokeTexCount < 12; smokeTexCount++) {
				const std::string smokeNum = IntToString(smokeTexCount, "%02i");
				const std::string smokeName = "ismoke" + smokeNum;
				const std::string texName = "bitmaps/smoke/smoke" + smokeNum + ".tga";

				textureAtlas->AddTexFromFile(smokeName, texName);
				blockedTexNames.insert(StringToLower(smokeName));
			}
		}

		if (smokeTexCount <= 0) {
			// this needs to be an exception, other code
			// assumes at least one smoke-texture exists
			throw content_error("missing smoke textures");
		}
	}

	{
		// shield-texture memory
		std::array<char, 4 * perlinTexSize * perlinTexSize> perlinTexMem;
		perlinTexMem.fill(70);
		textureAtlas->AddTexFromMem("perlintex", perlinTexSize, perlinTexSize, CTextureAtlas::RGBA32, &perlinTexMem[0]);
		blockedTexNames.insert("perlintex");
	}

	blockedTexNames.insert("flare");
	blockedTexNames.insert("explo");
	blockedTexNames.insert("explofade");
	blockedTexNames.insert("heatcloud");
	blockedTexNames.insert("laserend");
	blockedTexNames.insert("laserfalloff");
	blockedTexNames.insert("randdots");
	blockedTexNames.insert("smoketrail");
	blockedTexNames.insert("wake");
	blockedTexNames.insert("flame");

	blockedTexNames.insert("sbtrailtexture");
	blockedTexNames.insert("missiletrailtexture");
	blockedTexNames.insert("muzzleflametexture");
	blockedTexNames.insert("repulsetexture");
	blockedTexNames.insert("dguntexture");
	blockedTexNames.insert("flareprojectiletexture");
	blockedTexNames.insert("sbflaretexture");
	blockedTexNames.insert("missileflaretexture");
	blockedTexNames.insert("beamlaserflaretexture");
	blockedTexNames.insert("bubbletexture");
	blockedTexNames.insert("geosquaretexture");
	blockedTexNames.insert("gfxtexture");
	blockedTexNames.insert("projectiletexture");
	blockedTexNames.insert("repulsegfxtexture");
	blockedTexNames.insert("sphereparttexture");
	blockedTexNames.insert("torpedotexture");
	blockedTexNames.insert("wrecktexture");
	blockedTexNames.insert("plasmatexture");

	if (mapResParser.Execute()) {
		// allow map-specified atlas textures (for gaia-projectiles and ground-flashes)
		const LuaTable& mapResTable = mapResParser.GetRoot();
		const LuaTable& mapResGraphicsTable = mapResTable.SubTable("graphics");
		const LuaTable& mapResProjTexturesTable = mapResGraphicsTable.SubTable("projectileTextures");
		const LuaTable& mapResGroundFXTexturesTable = mapResGraphicsTable.SubTable("groundfx");

		ParseAtlasTextures(false, mapResProjTexturesTable, blockedTexNames, textureAtlas);
		ParseAtlasTextures(false, mapResGroundFXTexturesTable, blockedTexNames, groundFXAtlas);
	}

	if (!textureAtlas->Finalize()) {
#ifndef HEADLESS
		LOG_L(L_ERROR, "Could not finalize projectile-texture atlas. Use fewer/smaller textures.");
#endif
	}


	flaretex        = &textureAtlas->GetTexture("flare");
	explotex        = &textureAtlas->GetTexture("explo");
	explofadetex    = &textureAtlas->GetTexture("explofade");
	heatcloudtex    = &textureAtlas->GetTexture("heatcloud");
	laserendtex     = &textureAtlas->GetTexture("laserend");
	laserfallofftex = &textureAtlas->GetTexture("laserfalloff");
	randdotstex     = &textureAtlas->GetTexture("randdots");
	smoketrailtex   = &textureAtlas->GetTexture("smoketrail");
	waketex         = &textureAtlas->GetTexture("wake");
	perlintex       = &textureAtlas->GetTexture("perlintex");
	flametex        = &textureAtlas->GetTexture("flame");

	smokeTextures.reserve(smokeTexCount);

	for (int i = 0; i < smokeTexCount; i++) {
		smokeTextures.push_back(&textureAtlas->GetTexture("ismoke" + IntToString(i, "%02i")));
	}

	sbtrailtex         = &textureAtlas->GetTextureWithBackup("sbtrailtexture",         "smoketrail"    );
	missiletrailtex    = &textureAtlas->GetTextureWithBackup("missiletrailtexture",    "smoketrail"    );
	muzzleflametex     = &textureAtlas->GetTextureWithBackup("muzzleflametexture",     "explo"         );
	repulsetex         = &textureAtlas->GetTextureWithBackup("repulsetexture",         "explo"         );
	dguntex            = &textureAtlas->GetTextureWithBackup("dguntexture",            "flare"         );
	flareprojectiletex = &textureAtlas->GetTextureWithBackup("flareprojectiletexture", "flare"         );
	sbflaretex         = &textureAtlas->GetTextureWithBackup("sbflaretexture",         "flare"         );
	missileflaretex    = &textureAtlas->GetTextureWithBackup("missileflaretexture",    "flare"         );
	beamlaserflaretex  = &textureAtlas->GetTextureWithBackup("beamlaserflaretexture",  "flare"         );
	bubbletex          = &textureAtlas->GetTextureWithBackup("bubbletexture",          "circularthingy");
	geosquaretex       = &textureAtlas->GetTextureWithBackup("geosquaretexture",       "circularthingy");
	gfxtex             = &textureAtlas->GetTextureWithBackup("gfxtexture",             "circularthingy");
	projectiletex      = &textureAtlas->GetTextureWithBackup("projectiletexture",      "circularthingy");
	repulsegfxtex      = &textureAtlas->GetTextureWithBackup("repulsegfxtexture",      "circularthingy");
	sphereparttex      = &textureAtlas->GetTextureWithBackup("sphereparttexture",      "circularthingy");
	torpedotex         = &textureAtlas->GetTextureWithBackup("torpedotexture",         "circularthingy");
	wrecktex           = &textureAtlas->GetTextureWithBackup("wrecktexture",           "circularthingy");
	plasmatex          = &textureAtlas->GetTextureWithBackup("plasmatexture",          "circularthingy");


	if (!groundFXAtlas->Finalize())
		LOG_L(L_ERROR, "Could not finalize groundFX texture atlas. Use fewer/smaller textures.");

	groundflashtex = &groundFXAtlas->GetTexture("groundflash");
	groundringtex = &groundFXAtlas->GetTexture("groundring");
	seismictex = &groundFXAtlas->GetTexture("seismic");


	for (int a = 0; a < 4; ++a) {
		perlinBlend[a] = 0.0f;
	}

	{
		glGenTextures(8, perlinBlendTex);
		for (int a = 0; a < 8; ++a) {
			glBindTexture(GL_TEXTURE_2D, perlinBlendTex[a]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, perlinBlendTexSize, perlinBlendTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		}
	}


	// ProjectileDrawer is no-op constructed, has to be initialized manually
	perlinFB.Init(false);

	if (perlinFB.IsValid()) {
		// we never refresh the full texture (just the perlin part), so reload it on AT
		perlinFB.reloadOnAltTab = true;

		perlinFB.Bind();
		perlinFB.AttachTexture(textureAtlas->GetTexID());
		drawPerlinTex = perlinFB.CheckStatus("PROJECTILE-DRAWER-PERLIN");
		perlinFB.Unbind();
	}


	renderProjectiles.reserve(projectileHandler.maxParticles + projectileHandler.maxNanoParticles);
	for (auto& mr : modelRenderers) { mr.Clear(); }

	LoadWeaponTextures();

	fxShadowShader = shaderHandler->CreateProgramObject("[ProjectileDrawer::VFS]", "FX Shader shadow");
	fxShadowShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/ProjFXVertShadowProg.glsl", "", GL_VERTEX_SHADER));
	fxShadowShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/ProjFXFragShadowProg.glsl", "", GL_FRAGMENT_SHADER));
	fxShadowShader->SetFlag("USE_TEXTURE_ARRAY", false);

	using VAT = std::decay_t<decltype(CProjectile::GetPrimaryRenderBuffer())>::VertType;
	fxShadowShader->BindAttribLocations<VAT>();

	fxShadowShader->Link();
	fxShadowShader->Enable();

	fxShadowShader->SetUniform("atlasTex", 0);
	fxShadowShader->SetUniform("alphaCtrl", 0.0f, 1.0f, 0.0f, 0.0f);
	fxShadowShader->SetUniform("shadowColorMode", shadowHandler.shadowColorMode > 0 ? 1.0f : 0.0f);

	fxShadowShader->Disable();
	fxShadowShader->Validate();


	fxShader = shaderHandler->CreateProgramObject("[ProjectileDrawer::VFS]", "FX Shader");
	fxShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/ProjFXVertProg.glsl", "", GL_VERTEX_SHADER));
	fxShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/ProjFXFragProg.glsl", "", GL_FRAGMENT_SHADER));
	fxShader->SetFlag("SMOOTH_PARTICLES", CheckSoftenExt());
	fxShader->SetFlag("DEPTH_CLIP01", globalRendering->supportClipSpaceControl);
	fxShader->SetFlag("USE_TEXTURE_ARRAY", false);

	using VAT = std::decay_t<decltype(CProjectile::GetPrimaryRenderBuffer())>::VertType;
	fxShader->BindAttribLocations<VAT>();

	fxShader->Link();
	fxShader->Enable();
	fxShader->SetUniform("atlasTex", 0);
	fxShader->SetUniform("depthTex", 15);
	fxShader->SetUniform("softenExponent", softenExponent[0], softenExponent[1]);
	fxShader->SetUniform("softenThreshold", softenThreshold[0]);

	fxShader->SetUniform("camPos", 0.0f, 0.0f, 0.0f);
	fxShader->SetUniform("fogColor", 0.0f, 0.0f, 0.0f);
	fxShader->SetUniform("fogParams", 0.0f, 0.0f);

	fxShader->Disable();

	fxShader->Validate();

	sdbc = std::make_unique<ScopedDepthBufferCopy>(false);

	EnableSoften(configHandler->GetInt("SoftParticles"));
}

void CProjectileDrawer::Kill() {
	RECOIL_DETAILED_TRACY_ZONE;
	eventHandler.RemoveClient(this);
	autoLinkedEvents.clear();

	glDeleteTextures(8, perlinBlendTex);
	spring::SafeDelete(textureAtlas);
	spring::SafeDelete(groundFXAtlas);

	smokeTextures.clear();

	renderProjectiles.clear();

	sortedParticles.clear();
	sortScratch.clear();
	unsortedParticles.clear();
	mtFillBuffers.clear();

	perlinFB.Kill();

	perlinTexObjects = 0;
	drawPerlinTex = false;

	drawSorted = true;

	shaderHandler->ReleaseProgramObjects("[ProjectileDrawer::VFS]");
	fxShader = nullptr;
	fxShadowShader = nullptr;
	sdbc = nullptr;

	configHandler->Set("SoftParticles", wantSoften);
}

void CProjectileDrawer::UpdateDrawFlags()
{
	ZoneScopedN("ProjectileDrawer::UpdateDrawFlags");

	// water reflections are distorted enough that small particles contribute
	// next to nothing visually; skipping them avoids most of the reflection
	// pass' fill/sort/quad-generation cost on effect-heavy frames
	const float reflMinRadius = configHandler->GetFloat("ProjectileReflectionMinRadius");

	// per-frame invariants, hoisted out of the per-particle loop (notably
	// IWater::GetWater()->CanDrawReflectionPass(), a virtual call that was
	// previously made once per particle)
	const bool drawReflPass = IWater::GetWater()->CanDrawReflectionPass();
	const bool drawShadowPass = (shadowHandler.shadowGenBits & CShadowHandler::SHADOWGEN_BIT_PROJ) != 0;
	const float timeOffset = globalRendering->timeOffset;

	const CCamera* camPlayer = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);
	const CCamera* camUWRefl = CCameraHandler::GetCamera(CCamera::CAMTYPE_UWREFL);
	const CCamera* camShadow = CCameraHandler::GetCamera(CCamera::CAMTYPE_SHADOW);

	for_mt(0, renderProjectiles.size(), [this, reflMinRadius, drawReflPass, drawShadowPass, timeOffset, camPlayer, camUWRefl, camShadow](int i) {
		CProjectile* p = renderProjectiles[i];
		const bool hasModel = (p->model != nullptr);

		p->drawPos = p->GetDrawPos(timeOffset);

		p->previousDrawFlag = p->drawFlag;
		p->ResetDrawFlag();

		if (!CanDrawProjectile(p, p->GetAllyteamID()))
			return;

		p->SetDrawFlag(DrawFlags::SO_DRICON_FLAG); //reuse as a minimap draw indication

		const float drawRadius = p->GetDrawRadius();

		if (camPlayer->InView(p->drawPos, drawRadius)) {
			p->SetSortDist(CCamera::CAMTYPE_PLAYER, camPlayer->ProjectedDistance(p->drawPos));

			if (hasModel)
				p->AddDrawFlag(DrawFlags::SO_OPAQUE_FLAG);
			else
				p->AddDrawFlag(DrawFlags::SO_ALPHAF_FLAG);

			if (p->drawPos.y - drawRadius < 0.0f)
				p->AddDrawFlag(DrawFlags::SO_REFRAC_FLAG);

			// Special case of piece projectile, since it has a model and fire particle
			if (p->piece)
				p->AddDrawFlag(DrawFlags::SO_ALPHAF_FLAG);
		}

		if (drawReflPass && (hasModel || drawRadius >= reflMinRadius) && camUWRefl->InView(p->drawPos, drawRadius)) {
			p->SetSortDist(CCamera::CAMTYPE_UWREFL, camUWRefl->ProjectedDistance(p->drawPos));

			if (CModelDrawerHelper::ObjectVisibleReflection(p->drawPos, camUWRefl->GetPos(), drawRadius))
				p->AddDrawFlag(DrawFlags::SO_REFLEC_FLAG);
		}

		if (drawShadowPass && p->castShadow && camShadow->InView(p->drawPos, drawRadius)) {
			p->SetSortDist(CCamera::CAMTYPE_SHADOW, camShadow->ProjectedDistance(p->drawPos));

			if unlikely(hasModel)
				p->AddDrawFlag(DrawFlags::SO_SHOPAQ_FLAG);
			else
				p->AddDrawFlag(DrawFlags::SO_SHTRAN_FLAG);

			// Special case of piece projectile, since it has a model and fire particle
			if (p->piece)
				p->AddDrawFlag(DrawFlags::SO_SHTRAN_FLAG);
		}
	});

}

bool CProjectileDrawer::CheckSoftenExt()
{
	RECOIL_DETAILED_TRACY_ZONE;
	static bool result =
		FBO::IsSupported() &&
		GLAD_GL_EXT_framebuffer_blit; //eval once
	return result;
}

void CProjectileDrawer::ParseAtlasTextures(
	const bool blockTextures,
	const LuaTable& textureTable,
	spring::unordered_set<std::string>& blockedTextures,
	CTextureAtlas* texAtlas
) {
	RECOIL_DETAILED_TRACY_ZONE;
	std::vector<std::string> subTables;
	spring::unordered_map<std::string, std::string> texturesMap;

	textureTable.GetMap(texturesMap);
	textureTable.GetKeys(subTables);

	// Sort texture names to ensure deterministic ordering across runs
	std::vector<std::string> sortedTexNames;
	sortedTexNames.reserve(texturesMap.size());
	for (const auto& [name, _] : texturesMap) {
		sortedTexNames.emplace_back(name);
	}
	std::sort(sortedTexNames.begin(), sortedTexNames.end());

	for (const auto& texName : sortedTexNames) {
		const std::string textureName = StringToLower(texName);

		// no textures added to this atlas are allowed
		// to be overwritten later by other textures of
		// the same name
		if (blockTextures)
			blockedTextures.insert(textureName);

		if (blockTextures || (blockedTextures.find(textureName) == blockedTextures.end()))
			texAtlas->AddTexFromFile(texName, "bitmaps/" + texturesMap[texName]);
	}

	texturesMap.clear();

	for (size_t i = 0; i < subTables.size(); i++) {
		const LuaTable& textureSubTable = textureTable.SubTable(subTables[i]);

		if (!textureSubTable.IsValid())
			continue;

		textureSubTable.GetMap(texturesMap);

		// Sort texture names to ensure deterministic ordering across runs
		sortedTexNames.clear();
		sortedTexNames.reserve(texturesMap.size());
		for (const auto& [name, _] : texturesMap) {
			sortedTexNames.emplace_back(name);
		}
		std::sort(sortedTexNames.begin(), sortedTexNames.end());

		for (const auto& texName : sortedTexNames) {
			const std::string textureName = StringToLower(texName);

			if (blockTextures)
				blockedTextures.insert(textureName);

			if (blockTextures || (blockedTextures.find(textureName) == blockedTextures.end()))
				texAtlas->AddTexFromFile(texName, "bitmaps/" + texturesMap[texName]);
		}

		texturesMap.clear();
	}
}

void CProjectileDrawer::LoadWeaponTextures() {
	RECOIL_DETAILED_TRACY_ZONE;
	// post-process the synced weapon-defs to set unsynced fields
	// (this requires CWeaponDefHandler to have been initialized)
	for (WeaponDef& wd: const_cast<std::vector<WeaponDef>&>(weaponDefHandler->GetWeaponDefsVec())) {
		wd.visuals.texture1 = nullptr;
		wd.visuals.texture2 = nullptr;
		wd.visuals.texture3 = nullptr;
		wd.visuals.texture4 = nullptr;

		if (!wd.visuals.colorMapStr.empty())
			wd.visuals.colorMap = CColorMap::LoadFromDefString(wd.visuals.colorMapStr);

		if (wd.type == "Cannon") {
			wd.visuals.texture1 = plasmatex;
		} else if (wd.type == "AircraftBomb") {
			wd.visuals.texture1 = plasmatex;
		} else if (wd.type == "Shield") {
			wd.visuals.texture1 = perlintex;
		} else if (wd.type == "Flame") {
			wd.visuals.texture1 = flametex;

			if (wd.visuals.colorMap == nullptr) {
				wd.visuals.colorMap = CColorMap::LoadFromDefString(
					"1.0 1.0 1.0 0.1 "
					"0.025 0.025 0.025 0.10 "
					"0.0 0.0 0.0 0.0"
				);
			}
		} else if (wd.type == "MissileLauncher") {
			wd.visuals.texture1 = missileflaretex;
			wd.visuals.texture2 = missiletrailtex;
		} else if (wd.type == "TorpedoLauncher") {
			wd.visuals.texture1 = plasmatex;
		} else if (wd.type == "LaserCannon") {
			wd.visuals.texture1 = laserfallofftex;
			wd.visuals.texture2 = laserendtex;
		} else if (wd.type == "BeamLaser") {
			if (wd.largeBeamLaser) {
				wd.visuals.texture1 = &textureAtlas->GetTexture("largebeam");
				wd.visuals.texture2 = laserendtex;
				wd.visuals.texture3 = &textureAtlas->GetTexture("muzzleside");
				wd.visuals.texture4 = beamlaserflaretex;
			} else {
				wd.visuals.texture1 = laserfallofftex;
				wd.visuals.texture2 = laserendtex;
				wd.visuals.texture3 = beamlaserflaretex;
			}
		} else if (wd.type == "LightningCannon") {
			wd.visuals.texture1 = laserfallofftex;
		} else if (wd.type == "EmgCannon") {
			wd.visuals.texture1 = plasmatex;
		} else if (wd.type == "StarburstLauncher") {
			wd.visuals.texture1 = sbflaretex;
			wd.visuals.texture2 = sbtrailtex;
			wd.visuals.texture3 = explotex;
		} else {
			wd.visuals.texture1 = plasmatex;
			wd.visuals.texture2 = plasmatex;
		}

		// override the textures if we have specified names for them
		if (!wd.visuals.texNames[0].empty()) { wd.visuals.texture1 = &textureAtlas->GetTexture(wd.visuals.texNames[0]); }
		if (!wd.visuals.texNames[1].empty()) { wd.visuals.texture2 = &textureAtlas->GetTexture(wd.visuals.texNames[1]); }
		if (!wd.visuals.texNames[2].empty()) { wd.visuals.texture3 = &textureAtlas->GetTexture(wd.visuals.texNames[2]); }
		if (!wd.visuals.texNames[3].empty()) { wd.visuals.texture4 = &textureAtlas->GetTexture(wd.visuals.texNames[3]); }

		// trails can only be custom EG's, prefix is not required game-side
		if (!wd.visuals.ptrailExpGenTag.empty())
			wd.ptrailExplosionGeneratorID = explGenHandler.LoadCustomGeneratorID(wd.visuals.ptrailExpGenTag.c_str());

		if (!wd.visuals.impactExpGenTag.empty())
			wd.impactExplosionGeneratorID = explGenHandler.LoadGeneratorID(wd.visuals.impactExpGenTag.c_str());

		if (!wd.visuals.bounceExpGenTag.empty())
			wd.bounceExplosionGeneratorID = explGenHandler.LoadGeneratorID(wd.visuals.bounceExpGenTag.c_str());
	}
}

bool CProjectileDrawer::CanDrawProjectile(const CProjectile* pro, int allyTeam)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto& th = teamHandler;
	auto& lh = losHandler;
	return (gu->spectatingFullView || (th.IsValidAllyTeam(allyTeam) && th.Ally(allyTeam, gu->myAllyTeam)) || lh->InLos(pro, gu->myAllyTeam));
}

bool CProjectileDrawer::ShouldDrawProjectile(const CProjectile* p, uint8_t thisPassMask)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(p);

	if (p->drawFlag == 0)
		return false;

	assert(std::popcount(thisPassMask) == 1);
	return p->HasDrawFlag(static_cast<DrawFlags>(thisPassMask));
}

void CProjectileDrawer::DrawProjectilesMiniMap()
{
	ZoneScopedN("ProjectileDrawer::DrawMiniMap");

	// draw opaque first
	for (CProjectile* p : renderProjectiles) {
		if (!p->model)
			continue;

		if (!ShouldDrawProjectile(p, DrawFlags::SO_DRICON_FLAG))
			continue;

		p->DrawOnMinimap();
	}

	// draw alpha second
	for (CProjectile* p : renderProjectiles) {
		if (p->model)
			continue;

		if (!ShouldDrawProjectile(p, DrawFlags::SO_DRICON_FLAG))
			continue;

		p->DrawOnMinimap();
	}

	auto& sh = TypedRenderBuffer<VA_TYPE_C>::GetShader();

	glLineWidth(1.0f);

	// Note: glPointSize(1.0f); doesn't work here on AMD drivers.
	// AMD drivers draw huge circles instead of small point for some reason
	// so disable GL_PROGRAM_POINT_SIZE
	const bool pntsz = glIsEnabled(GL_PROGRAM_POINT_SIZE);
	if (pntsz)
		glDisable(GL_PROGRAM_POINT_SIZE);

	sh.Enable();
	{
		ZoneScopedN("DrawProjectilesMiniMap::MiniMapLinesRB");
		GetMiniMapLinesRB().DrawArrays(GL_LINES);
	}
	{
		ZoneScopedN("DrawProjectilesMiniMap::MiniMapPointsRB");
		GetMiniMapPointsRB().DrawArrays(GL_POINTS);
	}
	sh.Disable();

	if (pntsz)
		glEnable(GL_PROGRAM_POINT_SIZE);
}

void CProjectileDrawer::DrawFlyingPieces(int modelType) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const FlyingPieceContainer& container = projectileHandler.flyingPieces[modelType];

	if (container.empty())
		return;

	FlyingPiece::BeginDraw();

	const FlyingPiece* last = nullptr;
	for (const FlyingPiece& fp: container) {
		const bool noLosTst = gu->spectatingFullView || teamHandler.AlliedTeams(gu->myTeam, fp.GetTeam());
		const bool inAirLos = noLosTst || losHandler->InAirLos(fp.GetPos(), gu->myAllyTeam);

		if (!inAirLos)
			continue;

		if (!camera->InView(fp.GetPos(), fp.GetRadius()))
			continue;

		fp.Draw(last);
		last = &fp;
	}

	FlyingPiece::EndDraw();
}

void CProjectileDrawer::DrawOpaque(bool drawReflection, bool drawRefraction)
{
	ZoneScopedN("ProjectileDrawer::DrawOpaque");

	using namespace GL::State;
	auto state = GL::SubState(
		Blending(GL_FALSE),
		DepthTest(GL_TRUE),
		DepthMask(GL_TRUE)
	);

	const uint8_t thisPassMask =
		(1 - (drawReflection || drawRefraction)) * DrawFlags::SO_OPAQUE_FLAG +
		(drawReflection * DrawFlags::SO_REFLEC_FLAG) +
		(drawRefraction * DrawFlags::SO_REFRAC_FLAG);

	ISky::GetSky()->SetupFog();
	ScopedModelDrawerImpl<CUnitDrawer> legacy(true, false);
	unitDrawer->SetupOpaqueDrawing(false);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; modelType++) {
		CModelDrawerHelper::PushModelRenderState(modelType);

		const auto& mdlRenderer = modelRenderers[modelType];

		for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
			if (mdlRenderer.GetObjectBin(i).empty())
				continue;

			CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

			for (CProjectile* p : mdlRenderer.GetObjectBin(i)) {
				if (!ShouldDrawProjectile(p, thisPassMask))
					continue;

				DrawProjectileModel(p);
			}

			CModelDrawerHelper::UnbindModelTypeTexture(modelType);
		}

		DrawFlyingPieces(modelType);

		CModelDrawerHelper::PopModelRenderState(modelType);
	}

	unitDrawer->ResetOpaqueDrawing(false);
	glDisable(GL_FOG);
}

void CProjectileDrawer::DrawAlpha(bool drawAboveWater, bool drawBelowWater, bool drawReflection, bool drawRefraction)
{
	ZoneScopedN("ProjectileDrawer::DrawAlpha");

	static constexpr std::array<float, 4> clipPlanes[] {
		{ 0.0f,  0.0f, 0.0f, 0.0f}, // never used
		{ 0.0f, -1.0f, 0.0f, 0.0f},
		{ 0.0f,  1.0f, 0.0f, 0.0f},
		{ 0.0f,  0.0f, 0.0f, 1.0f}
	};
	const auto& clipPlane = clipPlanes[1U * drawBelowWater + 2U * drawAboveWater];

	// The main view draws alpha particles twice per frame when water is
	// visible: a below-water pass, then an above-water pass once the water
	// surface has been drawn. Both passes contain the same particles viewed
	// from the same camera; only the clip plane differs. Fill and upload the
	// geometry once in the below-water pass and re-submit the saved range in
	// the above-water pass. The water reflection/refraction passes that run
	// in between fill the buffer for their own camera/mask and consume their
	// own ranges, so the saved range stays valid for the whole frame.
	const bool mainPass = !drawReflection && !drawRefraction;
	const bool reuseWanted = configHandler->GetBool("ProjectileDrawReuseWaterPasses");

	// the above-water main pass and the water refraction pass both view the
	// same particles from the player camera; both can re-submit the geometry
	// saved by the below-water pass with their own clip plane instead of
	// refilling. (Refraction previously filled only the underwater-flagged
	// subset; submitting the full range is visually equivalent because the
	// clip plane discards everything above the surface.) The reflection pass
	// uses the mirrored camera and must keep building its own billboards.
	const bool reuseAbove = mainPass && drawAboveWater && !drawBelowWater;
	const bool reuseRefraction = drawRefraction && !drawReflection;
	const bool reusePass = reuseWanted && (reuseAbove || reuseRefraction) && (alphaRangeDrawFrame == globalRendering->drawFrame);

	if (!reusePass) {
		const uint8_t thisPassMask =
			(1 - (drawReflection || drawRefraction)) * DrawFlags::SO_ALPHAF_FLAG +
			(drawReflection * DrawFlags::SO_REFLEC_FLAG) +
			(drawRefraction * DrawFlags::SO_REFRAC_FLAG);

		sortedParticles.clear();
		unsortedParticles.clear();

		const uint32_t sortCamType = camera->GetCamType();
		const bool useDrawOrder = wantDrawOrder;

		{
			ZoneScopedN("ProjectileDrawer::DrawAlpha(DP)");
			for (CProjectile* p : renderProjectiles) {
				if (!ShouldDrawProjectile(p, thisPassMask))
					continue;

				if (drawSorted && p->drawSorted)
					sortedParticles.emplace_back(SortableParticle{ ParticleSortKey(p, sortCamType, useDrawOrder), p });
				else
					unsortedParticles.emplace_back(p);
			}
		}

		{
			ZoneScopedN("ProjectileDrawer::DrawAlpha(SO)");
			RadixSortByKey(sortedParticles, sortScratch, [](const SortableParticle& sp) noexcept { return sp.sortKey; });
		}

		const bool threadedFill = configHandler->GetBool("ProjectileDrawThreadedFill") && ThreadPool::HasThreads();

		{
			ZoneScopedN("ProjectileDrawer::DrawAlpha(DS)");
			FillParticleGeometry(mtFillBuffers, sortedParticles.size(), threadedFill, [this](size_t j) { return sortedParticles[j].proj; });
		}
		{
			ZoneScopedN("ProjectileDrawer::DrawAlpha(DU)");
			FillParticleGeometry(mtFillBuffers, unsortedParticles.size(), threadedFill, [this](size_t j) { return unsortedParticles[j]; });
		}
	}

	{
		ZoneScopedN("ProjectileDrawer::DrawAlpha(RR)");

		using namespace GL::State;
		auto state = GL::SubState(
			Blending(GL_TRUE),
			BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA),
			DepthTest(GL_TRUE),
			DepthMask(GL_FALSE),
			ClipDistance<0>(GL_TRUE)
		);

		eventHandler.DrawWorldPreParticles(drawAboveWater, drawBelowWater, drawReflection, drawRefraction);

		auto& rb = CExpGenSpawnable::GetPrimaryRenderBuffer();

		if (reuseWanted && mainPass && drawBelowWater && !drawAboveWater) {
			// an above-water pass will follow this frame; remember what to re-draw
			const auto pendingRange = rb.GetPendingElemsRange();
			alphaRangeStart = pendingRange.first;
			alphaRangeCount = pendingRange.second;
			alphaRangeDrawFrame = globalRendering->drawFrame;
		}

		const bool haveRangeToRedraw = (reusePass && alphaRangeCount > 0);

		if (!haveRangeToRedraw && !rb.ShouldSubmit())
			return;

		const bool needSoften = (wantSoften > 0) && !drawReflection && !drawRefraction;

		glActiveTexture(GL_TEXTURE0); textureAtlas->BindTexture();

		if (needSoften) {
			glActiveTexture(GL_TEXTURE15); glBindTexture(GL_TEXTURE_2D, depthBufferCopy->GetDepthBufferTexture(false));
		}

		const auto camPlayer = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);
		const auto& sky = ISky::GetSky();

		fxShader->Enable();
		fxShader->SetFlag("SMOOTH_PARTICLES", needSoften);
		fxShader->SetFlag("USE_TEXTURE_ARRAY", (textureAtlas->GetNumPages() > 1));
		fxShader->SetUniform("clipPlane", clipPlane[0], clipPlane[1], clipPlane[2], clipPlane[3]);
		fxShader->SetUniform("alphaCtrl", 0.0f, 1.0f, 0.0f, 0.0f);
		fxShader->SetUniform("softenThreshold", CProjectileDrawer::softenThreshold[0]);

		fxShader->SetUniform("camPos", camPlayer->pos.x, camPlayer->pos.y, camPlayer->pos.z);
		fxShader->SetUniform("fogColor", sky->fogColor.x, sky->fogColor.y, sky->fogColor.z);
		fxShader->SetUniform("fogParams", sky->fogStart * camPlayer->GetFarPlaneDist(), sky->fogEnd * camPlayer->GetFarPlaneDist());

		if (reusePass) {
			rb.DrawElementsRange(GL_TRIANGLES, alphaRangeStart, alphaRangeCount);
			// consume anything appended since the range was saved (e.g. by the
			// DrawWorldPreParticles handlers above), so it cannot leak into a
			// later submit running a different shader
			if (rb.ShouldSubmit())
				rb.DrawElements(GL_TRIANGLES);
		} else {
			rb.DrawElements(GL_TRIANGLES);
		}

		fxShader->Disable();

		if (needSoften) {
			glBindTexture(GL_TEXTURE_2D, 0); //15th slot
			glActiveTexture(GL_TEXTURE0);
		}
		textureAtlas->UnbindTexture();
	}
}

void CProjectileDrawer::DrawShadowOpaque()
{
	ZoneScopedN("ProjectileDrawer::DrawShadowOpaque");
	Shader::IProgramObject* po = shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_PROJECTILE);

	po->Enable();

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; modelType++) {
		CModelDrawerHelper::PushModelRenderState(modelType);

		const auto& mdlRenderer = modelRenderers[modelType];

		for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
			if (mdlRenderer.GetObjectBin(i).empty())
				continue;

			CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

			for (CProjectile* p : mdlRenderer.GetObjectBin(i)) {
				if (!ShouldDrawProjectile(p, DrawFlags::SO_SHOPAQ_FLAG))
					continue;

				DrawProjectileModel(p);
			}

			CModelDrawerHelper::UnbindModelTypeTexture(modelType);
		}

		DrawFlyingPieces(modelType);

		CModelDrawerHelper::PopModelRenderState(modelType);
	}

	po->Disable();
}

void CProjectileDrawer::DrawShadowTransparent()
{
	ZoneScopedN("ProjectileDrawer::DrawShadowTransparent");
	// Method #1 here: https://wickedengine.net/2018/01/18/easy-transparent-shadow-maps/

	// 1) Render opaque objects into depth stencil texture from light's point of view - done elsewhere

	// draw the model-less projectiles; the multiplicative shadow blend is
	// order-independent, so the list needs no sorting. unsortedParticles is
	// only otherwise used inside DrawAlpha, which runs later in the frame and
	// clears it first.
	unsortedParticles.clear();
	for (CProjectile* p : renderProjectiles) {
		if (!ShouldDrawProjectile(p, DrawFlags::SO_SHTRAN_FLAG))
			continue;

		unsortedParticles.emplace_back(p);
	}

	{
		ZoneScopedN("ProjectileDrawer::DrawShadowTransparent(Fill)");
		const bool threadedFill = configHandler->GetBool("ProjectileDrawThreadedFill") && ThreadPool::HasThreads();
		FillParticleGeometry(mtFillBuffers, unsortedParticles.size(), threadedFill, [this](size_t j) { return unsortedParticles[j]; });
	}

	auto& rb = CExpGenSpawnable::GetPrimaryRenderBuffer();
	if (!rb.ShouldSubmit())
		return;

	// 2) Bind render target for shadow color filter: R11G11B10 works good
	shadowHandler.EnableColorOutput(true);

	// 3) Clear render target to 1,1,1,0 (RGBA) color - done elsewhere

	// 4) Apply depth stencil state with depth read, but no write
	//glEnable(GL_DEPTH_TEST);
	//glDepthMask(GL_FALSE);

	// 5) Apply multiplicative blend state eg:
	// SrcBlend = BLEND_ZERO
	//	DestBlend = BLEND_SRC_COLOR
	//	BlendOp = BLEND_OP_ADD
	//glBlendFunc(GL_ZERO, GL_SRC_COLOR);
	//glEnable(GL_BLEND);
	using namespace GL::State;
	auto state = GL::SubState(
		DepthTest(GL_TRUE),
		DepthMask(GL_FALSE),
		Blending(GL_TRUE),
		BlendFunc(GL_ZERO, GL_SRC_COLOR)
	);

	// 6) Render transparents in arbitrary order
	textureAtlas->BindTexture();

	fxShadowShader->Enable();
	fxShadowShader->SetFlag("USE_TEXTURE_ARRAY", (textureAtlas->GetNumPages() > 1));
	fxShadowShader->SetUniform("shadowColorMode", shadowHandler.shadowColorMode > 0 ? 1.0f : 0.0f);

	rb.DrawElements(GL_TRIANGLES);

	fxShadowShader->Disable();
	glBindTexture(GL_TEXTURE_2D, 0);

	//shadowHandler.EnableColorOutput(false);
}



void CProjectileDrawer::DrawProjectileModel(const CProjectile* p)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(p->model);

	switch ((p->weapon * 2) + (p->piece * 1)) {
		case 2: {
			// weapon-projectile
			const CWeaponProjectile* wp = static_cast<const CWeaponProjectile*>(p);

			CUnitDrawer::SetTeamColor(wp->GetTeamID());

			glPushMatrix();
				glMultMatrixf(wp->GetTransformMatrix(wp->GetProjectileType() == WEAPON_MISSILE_PROJECTILE));

				if (!p->luaDraw || !eventHandler.DrawProjectile(p))
					wp->model->DrawStatic();

			glPopMatrix();
			return;
		} break;

		case 1: {
			// piece-projectile
			const CPieceProjectile* pp = static_cast<const CPieceProjectile*>(p);

			CUnitDrawer::SetTeamColor(pp->GetTeamID());

			auto scopedPushPop = spring::ScopedNullResource(glPushMatrix, glPopMatrix);

			glTranslatef3(pp->drawPos);
			glRotatef(pp->GetDrawAngle(), pp->spinVec.x, pp->spinVec.y, pp->spinVec.z);

			if (p->luaDraw && eventHandler.DrawProjectile(p)) {
				return;
			}

			if ((pp->explFlags & PF_Recursive) != 0) {
				// DrawStaticLegacyRec applies bpose rotation+scale per-piece internally
				pp->omp->DrawStaticLegacyRec();
			}
			else {
				// Apply the rotation+scale from bposeTransform (but NOT translation, which is the
				// piece's model-space offset and is irrelevant for a standalone flying piece).
				{
					const auto& bpose = pp->omp->bposeTransform;
					const Transform bposeNoTrans{ bpose.r, ZeroVector, bpose.s };
					glMultMatrixf(bposeNoTrans.ToMatrix());
				}
				pp->omp->DrawStaticLegacy(true, false);
			}

			return;
		} break;

		default: {
		} break;
	}
}

void CProjectileDrawer::DrawGroundFlashes()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const GroundFlashContainer& gfc = projectileHandler.groundFlashes;

	if (gfc.empty())
		return;

	static constexpr GLfloat black[] = {0.0f, 0.0f, 0.0f, 0.0f};

	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glActiveTexture(GL_TEXTURE0);
	groundFXAtlas->BindTexture();
/*
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.01f);
*/
	glPolygonOffset(-20, -1000);
	glEnable(GL_POLYGON_OFFSET_FILL);
//	glFogfv(GL_FOG_COLOR, black);

	bool depthTest = true;
	bool depthMask = false;

	const bool needSoften = (wantSoften > 0);

	auto& rb = CExpGenSpawnable::GetPrimaryRenderBuffer();

	if (needSoften) {
		glActiveTexture(GL_TEXTURE15); glBindTexture(GL_TEXTURE_2D, depthBufferCopy->GetDepthBufferTexture(false));
	}

	const auto camPlayer = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);
	const auto& sky = ISky::GetSky();

	fxShader->Enable();
	fxShader->SetFlag("USE_TEXTURE_ARRAY", (groundFXAtlas->GetNumPages() > 1));
	fxShader->SetUniform("alphaCtrl", 0.01f, 1.0f, 0.0f, 0.0f);
	fxShader->SetUniform("softenThreshold", -CProjectileDrawer::softenThreshold[1]);
	fxShader->SetUniform("camPos", camPlayer->pos.x, camPlayer->pos.y, camPlayer->pos.z);
	fxShader->SetUniform("fogColor", sky->fogColor.x, sky->fogColor.y, sky->fogColor.z);
	fxShader->SetUniform("fogParams", sky->fogStart * camPlayer->GetFarPlaneDist(), sky->fogEnd * camPlayer->GetFarPlaneDist());

	for (CGroundFlash* gf: gfc) {
		const bool inLos = gf->alwaysVisible || gu->spectatingFullView || losHandler->InAirLos(gf, gu->myAllyTeam);
		if (!inLos)
			continue;

		if (!camera->InView(gf->pos, gf->size))
			continue;

		bool depthTestWanted = needSoften ? false : gf->depthTest;

		if (depthTest != depthTestWanted || depthMask != gf->depthMask) {
			rb.DrawElements(GL_TRIANGLES);

			if ((depthTest = depthTestWanted)) {
				glEnable(GL_DEPTH_TEST);
			} else {
				glDisable(GL_DEPTH_TEST);
			}

			if ((depthMask = gf->depthMask)) {
				glDepthMask(GL_TRUE);
			} else {
				glDepthMask(GL_FALSE);
			}
		}

		gf->Draw();
	}

	rb.DrawElements(GL_TRIANGLES);

	fxShader->Disable();

	if (needSoften) {
		glBindTexture(GL_TEXTURE_2D, 0); //15th slot
		glActiveTexture(GL_TEXTURE0);
	}

	groundFXAtlas->UnbindTexture();

//	glFogfv(GL_FOG_COLOR, sky->fogColor);
	glDisable(GL_POLYGON_OFFSET_FILL);
//	glDisable(GL_ALPHA_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
}



void CProjectileDrawer::UpdateTextures() {
	RECOIL_DETAILED_TRACY_ZONE;
	if (perlinTexObjects > 0 && drawPerlinTex)
		UpdatePerlin();
}

void CProjectileDrawer::UpdatePerlin() {
	RECOIL_DETAILED_TRACY_ZONE;
	perlinFB.Bind();
	glViewport(perlintex->xstart * (textureAtlas->GetSize()).x, perlintex->ystart * (textureAtlas->GetSize()).y, perlinTexSize, perlinTexSize);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glLoadMatrixf(CMatrix44f::ClipOrthoProj01());
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	unsigned char col[4];
	float time = globalRendering->lastFrameTime * gs->speedFactor * 0.003f;
	float speed = 1.0f;
	float size = 1.0f;

	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_TC>();
	rb.AssertSubmission();

	auto& sh = rb.GetShader();
	sh.Enable();
	sh.SetUniform("alphaCtrl", 0.0f, 0.0f, 0.0f, 1.0f); // no test
	sh.Disable();

	for (int a = 0; a < 4; ++a) {
		perlinBlend[a] += time * speed;
		if (perlinBlend[a] > 1) {
			uint32_t temp = perlinBlendTex[a * 2];
			perlinBlendTex[a * 2    ] = perlinBlendTex[a * 2 + 1];
			perlinBlendTex[a * 2 + 1] = temp;

			GenerateNoiseTex(perlinBlendTex[a * 2 + 1]);
			perlinBlend[a] -= 1;
		}

		float tsize = 8.0f / size;

		if (a == 0)
			glDisable(GL_BLEND);

		for (int b = 0; b < 4; ++b)
			col[b] = int((1.0f - perlinBlend[a]) * 16 * size);

		glBindTexture(GL_TEXTURE_2D, perlinBlendTex[a * 2]);

		rb.AddQuadTriangles(
			{ ZeroVector, 0,         0, col },
			{   UpVector, 0,     tsize, col },
			{   XYVector, tsize, tsize, col },
			{  RgtVector, tsize,     0, col }
		);
		sh.Enable();
		rb.DrawElements(GL_TRIANGLES);
		sh.Disable();

		if (a == 0)
			glEnable(GL_BLEND);

		for (int b = 0; b < 4; ++b)
			col[b] = int(perlinBlend[a] * 16 * size);

		glBindTexture(GL_TEXTURE_2D, perlinBlendTex[a * 2 + 1]);

		rb.AddQuadTriangles(
			{ ZeroVector,     0,     0, col },
			{ UpVector  ,     0, tsize, col },
			{ XYVector  , tsize, tsize, col },
			{ RgtVector , tsize,     0, col }
		);
		sh.Enable();
		rb.DrawElements(GL_TRIANGLES);
		sh.Disable();

		speed *= 0.6f;
		size *= 2;
	}

	perlinFB.Unbind();
	globalRendering->LoadViewport();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
}

void CProjectileDrawer::GenerateNoiseTex(uint32_t tex)
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::array<unsigned char, 4 * perlinBlendTexSize * perlinBlendTexSize> mem;

	for (int a = 0; a < perlinBlendTexSize * perlinBlendTexSize; ++a) {
		const unsigned char rnd = int(std::max(0.0f, guRNG.NextFloat() * 555.0f - 300.0f));

		mem[a * 4 + 0] = rnd;
		mem[a * 4 + 1] = rnd;
		mem[a * 4 + 2] = rnd;
		mem[a * 4 + 3] = rnd;
	}

	glBindTexture(GL_TEXTURE_2D, tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, perlinBlendTexSize, perlinBlendTexSize, GL_RGBA, GL_UNSIGNED_BYTE, &mem[0]);
}



void CProjectileDrawer::RenderProjectileCreated(const CProjectile* p)
{
	RECOIL_DETAILED_TRACY_ZONE;
	{
		const_cast<CProjectile*>(p)->SetRenderIndex(renderProjectiles.size());
		renderProjectiles.push_back(const_cast<CProjectile*>(p));
	}

	if (p->model != nullptr)
		modelRenderers[MDL_TYPE(p)].AddObject(p);
}

void CProjectileDrawer::RenderProjectileDestroyed(const CProjectile* p)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto ri = p->GetRenderIndex();
	if (ri >= renderProjectiles.size()) {
		assert(false);
		return;
	}

	renderProjectiles[ri] = renderProjectiles.back();
	renderProjectiles[ri]->SetRenderIndex(ri);
	renderProjectiles.pop_back();

	if (p->model != nullptr)
		modelRenderers[MDL_TYPE(p)].DelObject(p);
}

