#include <limits>

#include "ExpGenSpawnableMemberInfo.h"
#include "ExpGenSpawner.h"
#include "ProjectileMemPool.h"
#include "Rendering/GroundFlash.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/Classes/BitmapMuzzleFlame.h"
#include "Rendering/Env/Particles/Classes/BubbleProjectile.h"
#include "Rendering/Env/Particles/Classes/DirtProjectile.h"
#include "Rendering/Env/Particles/Classes/ExploSpikeProjectile.h"
#include "Rendering/Env/Particles/Classes/HeatCloudProjectile.h"
#include "Rendering/Env/Particles/Classes/NanoProjectile.h"
#include "Rendering/Env/Particles/Classes/SimpleParticleSystem.h"
#include "Rendering/Env/Particles/Classes/SmokeProjectile.h"
#include "Rendering/Env/Particles/Classes/SmokeProjectile2.h"
#include "Rendering/Env/Particles/Classes/SpherePartProjectile.h"
#include "Rendering/Env/Particles/Classes/TracerProjectile.h"
#include "Rendering/GL/RenderBuffers.h"
#include "System/SpringHash.h"
#include "System/TemplateUtils.hpp"
#include "Sim/Misc/GlobalSynced.h"

#include <tracy/Tracy.hpp>


CR_BIND_DERIVED_INTERFACE_POOL(CExpGenSpawnable, CWorldObject, projMemPool.allocMem, projMemPool.freeMem)
CR_REG_METADATA(CExpGenSpawnable, (
	CR_MEMBER(rotVal),
	CR_MEMBER(rotVel),
	CR_MEMBER(createFrame),
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(rotParams),
		CR_MEMBER(animParams),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_IGNORED(animProgress)
))

std::array<CExpGenSpawnable::SpawnableTuple, 14> CExpGenSpawnable::spawnables = {};

CExpGenSpawnable::CExpGenSpawnable(const float3& pos, const float3& spd)
	: CWorldObject(pos, spd)
	, rotVal{ 0 }
	, rotVel{ 0 }
	, createFrame{ 0 }
{
	//ZoneScoped;
	assert(projMemPool.alloced(this));
}

CExpGenSpawnable::CExpGenSpawnable()
	: CWorldObject()
	, rotVal{ 0 }
	, rotVel{ 0 }
	, createFrame{ 0 }
{
	//ZoneScoped;
	assert(projMemPool.alloced(this));
}

CExpGenSpawnable::~CExpGenSpawnable()
{
	//ZoneScoped;
	assert(projMemPool.mapped(this));
}

void CExpGenSpawnable::Init(const CUnit* owner, const float3& offset)
{
	//ZoneScoped;
	createFrame = gs->frameNum;
	rotParams *= float3(math::DEG_TO_RAD / GAME_SPEED, math::DEG_TO_RAD / (GAME_SPEED * GAME_SPEED), math::DEG_TO_RAD);

	UpdateRotation();
}

void CExpGenSpawnable::UpdateRotation()
{
	//ZoneScoped;
	const float t = (gs->frameNum - createFrame + globalRendering->timeOffset);
	// rotParams.y is acceleration in angle per frame^2
	rotVel = rotParams.x + rotParams.y * t;
	rotVal = rotParams.z + rotVel      * t;
}

void CExpGenSpawnable::UpdateAnimParams()
{
	//ZoneScoped;
	if (static_cast<int>(animParams.x) <= 1 && static_cast<int>(animParams.y) <= 1) {
		animProgress = 0.0f;
		return;
	}

	const float t = (gs->frameNum - createFrame + globalRendering->timeOffset);
	const float animSpeed = math::fabs(animParams.z);
	if (animParams.z < 0.0f) {
		#if 0
			animProgress = math::fmod(t, 2.0f * animSpeed) / animSpeed;
			if (animProgress > 1.0)
				animProgress = 2.0f - animProgress;
		#else
			animProgress = 1.0f - math::fabs(math::fmod(t, 2.0f * animSpeed) / animSpeed - 1.0f);
		#endif
	}
	else {
		animProgress = math::fmod(t, animSpeed) / animSpeed;
	}
}

bool CExpGenSpawnable::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	//ZoneScoped;
	static const unsigned int memberHashes[] = {
		spring::LiteHash(          "pos",  sizeof(          "pos") - 1, 0),
		spring::LiteHash(        "speed",  sizeof(        "speed") - 1, 0),
		spring::LiteHash(    "useairlos",  sizeof(    "useairlos") - 1, 0),
		spring::LiteHash("alwaysvisible",  sizeof("alwaysvisible") - 1, 0),
	};

	CHECK_MEMBER_INFO_FLOAT3_HASH(CExpGenSpawnable, pos          , memberHashes[0])
	CHECK_MEMBER_INFO_FLOAT4_HASH(CExpGenSpawnable, speed        , memberHashes[1])
	CHECK_MEMBER_INFO_BOOL_HASH  (CExpGenSpawnable, useAirLos    , memberHashes[2])
	CHECK_MEMBER_INFO_BOOL_HASH  (CExpGenSpawnable, alwaysVisible, memberHashes[3])

	CHECK_MEMBER_INFO_FLOAT3(CExpGenSpawnable, rotParams)
	CHECK_MEMBER_INFO_FLOAT3(CExpGenSpawnable, animParams)

	return false;
}

TypedRenderBuffer<VA_TYPE_PROJ>& CExpGenSpawnable::GetPrimaryRenderBuffer()
{
	//ZoneScoped;
	return RenderBuffer::GetTypedRenderBuffer<VA_TYPE_PROJ>();
}

template<typename Spawnable>
CExpGenSpawnable::SpawnableTuple GetSpawnableEntryImpl()
{
	//ZoneScoped;
	CExpGenSpawnable::SpawnableTuple entry{};

	return std::make_tuple(
		std::string{ Spawnable::StaticClass()->name },
		[](SExpGenSpawnableMemberInfo& memberInfo) { return Spawnable::GetMemberInfo(memberInfo); },
		[]() { return static_cast<CExpGenSpawnable*>(projMemPool.alloc<Spawnable>()); }
	);
}

#define MAKE_FUNCTIONS_TUPLE(Func) \
std::make_tuple( \
	Func<CExpGenSpawner        >, \
	Func<CStandardGroundFlash  >, \
	Func<CSimpleGroundFlash    >, \
	Func<CBitmapMuzzleFlame    >, \
	Func<CDirtProjectile       >, \
	Func<CExploSpikeProjectile >, \
	Func<CHeatCloudProjectile  >, \
	Func<CNanoProjectile       >, \
	Func<CSimpleParticleSystem >, \
	Func<CSphereParticleSpawner>, \
	Func<CSmokeProjectile      >, \
	Func<CSmokeProjectile2     >, \
	Func<CSpherePartSpawner    >, \
	Func<CTracerProjectile     >  \
)

void CExpGenSpawnable::InitSpawnables()
{
	//ZoneScoped;
	auto funcTuple = MAKE_FUNCTIONS_TUPLE(GetSpawnableEntryImpl);
	static_assert(std::tuple_size<decltype(funcTuple)>::value == spawnables.size());

	for (size_t i = 0; i < spawnables.size(); ++i) {
		CExpGenSpawnable::SpawnableTuple entry;
		const auto Functor = [&entry](auto&& func) { entry = func(); };
		spring::tuple_exec_at(i, funcTuple, Functor);
		spawnables[i] = entry;
	}
}

#undef MAKE_FUNCTIONS_TUPLE

bool CExpGenSpawnable::GetSpawnableMemberInfo(const std::string& spawnableName, SExpGenSpawnableMemberInfo& memberInfo)
{
	//ZoneScoped;
	auto it = std::find_if(spawnables.begin(), spawnables.end(), [&spawnableName](const auto& entry) {
		return std::get<0>(entry) == spawnableName;
	});

	if (it == spawnables.end())
		return false;

	return std::get<1>(*it)(memberInfo);
}

int CExpGenSpawnable::GetSpawnableID(const std::string& spawnableName)
{
	//ZoneScoped;
	auto it = std::find_if(spawnables.begin(), spawnables.end(), [&spawnableName](const auto& entry) {
		return std::get<0>(entry) == spawnableName;
	});

	if (it == spawnables.end())
		return -1;

	return static_cast<int>(std::distance(spawnables.begin(), it));
}

CExpGenSpawnable* CExpGenSpawnable::CreateSpawnable(int spawnableID)
{
	//ZoneScoped;
	if (spawnableID < 0 || spawnableID > spawnables.size() - 1)
		return nullptr;

	return std::get<2>(spawnables[spawnableID])();
}

void CExpGenSpawnable::AddEffectsQuad(const VA_TYPE_TC& tl, const VA_TYPE_TC& tr, const VA_TYPE_TC& br, const VA_TYPE_TC& bl) const
{
	//ZoneScoped;
	float minS = std::numeric_limits<float>::max()   ; float minT = std::numeric_limits<float>::max()   ;
	float maxS = std::numeric_limits<float>::lowest(); float maxT = std::numeric_limits<float>::lowest();
	std::invoke([&](auto&&... arg) {
		((minS = std::min(minS, arg.s)), ...);
		((minT = std::min(minT, arg.t)), ...);
		((maxS = std::max(maxS, arg.s)), ...);
		((maxT = std::max(maxT, arg.t)), ...);
	}, tl, tr, br, bl);

	auto& rb = GetPrimaryRenderBuffer();

	const auto uvInfo = float4{ minS, minT, maxS - minS, maxT - minT };
	const auto animInfo = float3{ animParams.x, animParams.y, animProgress };
	constexpr float layer = 0.0f; //for future texture arrays

	//pos, uvw, uvmm, col
	rb.AddQuadTriangles(
		{ tl.pos, float3{ tl.s, tl.t, layer }, uvInfo, animInfo, tl.c },
		{ tr.pos, float3{ tr.s, tr.t, layer }, uvInfo, animInfo, tr.c },
		{ br.pos, float3{ br.s, br.t, layer }, uvInfo, animInfo, br.c },
		{ bl.pos, float3{ bl.s, bl.t, layer }, uvInfo, animInfo, bl.c }
	);
}