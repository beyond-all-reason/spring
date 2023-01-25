#include <limits>
#include <tuple>

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
#include "System/Sync/HsiehHash.h"
#include "System/TemplateUtils.hpp"
#include "Sim/Misc/GlobalSynced.h"


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

CExpGenSpawnable::CExpGenSpawnable(const float3& pos, const float3& spd)
	: CWorldObject(pos, spd)
	, createFrame{0}
	, rotVal{0}
	, rotVel{0}
{
	assert(projMemPool.alloced(this));
}

CExpGenSpawnable::CExpGenSpawnable()
	: CWorldObject()
	, createFrame{ 0 }
	, rotVal{ 0 }
	, rotVel{ 0 }
{
	assert(projMemPool.alloced(this));
}

CExpGenSpawnable::~CExpGenSpawnable()
{
	assert(projMemPool.mapped(this));
}

void CExpGenSpawnable::Init(const CUnit* owner, const float3& offset)
{
	createFrame = gs->frameNum;
	rotParams *= float3(math::DEG_TO_RAD / GAME_SPEED, math::DEG_TO_RAD / (GAME_SPEED * GAME_SPEED), math::DEG_TO_RAD);

	UpdateRotation();
}

void CExpGenSpawnable::UpdateRotation()
{
	const float t = (gs->frameNum - createFrame + globalRendering->timeOffset);
	// rotParams.y is acceleration in angle per frame^2
	rotVel = rotParams.x + rotParams.y * t;
	rotVal = rotParams.z + rotVel * t;
}

void CExpGenSpawnable::UpdateAnimParams()
{
	if (static_cast<int>(animParams.x) <= 1 && static_cast<int>(animParams.y) <= 1) {
		animProgress = 0.0f;
		return;
	}

	const float t = (gs->frameNum - createFrame + globalRendering->timeOffset);
	animProgress = math::fmod(t, animParams.z) / animParams.z;
}

bool CExpGenSpawnable::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	static const unsigned int memberHashes[] = {
		HsiehHash(          "pos",  sizeof(          "pos") - 1, 0),
		HsiehHash(        "speed",  sizeof(        "speed") - 1, 0),
		HsiehHash(    "useairlos",  sizeof(    "useairlos") - 1, 0),
		HsiehHash("alwaysvisible",  sizeof("alwaysvisible") - 1, 0),
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
	return RenderBuffer::GetTypedRenderBuffer<VA_TYPE_PROJ>();
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

template<typename Spawnable>
bool GetSpawnableMemberInfoImpl(const std::string& spawnableName, SExpGenSpawnableMemberInfo& memberInfo)
{
	static const std::string typeName = Spawnable::StaticClass()->name;

	if (spawnableName == typeName)
		return Spawnable::GetMemberInfo(memberInfo);

	return false;
}
bool CExpGenSpawnable::GetSpawnableMemberInfo(const std::string& spawnableName, SExpGenSpawnableMemberInfo& memberInfo)
{
	static auto funcTuple = MAKE_FUNCTIONS_TUPLE(GetSpawnableMemberInfoImpl);
	bool ret = false;
	std::apply([&spawnableName, &memberInfo, &ret](const auto&... func)
	{
		ret = ( func(spawnableName, memberInfo) || ... );
	}, funcTuple);

	return ret;
}

template<typename Spawnable>
bool GetSpawnableIDImpl(const std::string& spawnableName, int& idx)
{
	static const std::string typeName = Spawnable::StaticClass()->name;

	if (spawnableName == typeName)
		return true;

	++idx;
	return false;
}
int CExpGenSpawnable::GetSpawnableID(const std::string& spawnableName)
{
	static auto funcTuple = MAKE_FUNCTIONS_TUPLE(GetSpawnableIDImpl);

	int idx = 0;
	bool ret = false;
	std::apply([&spawnableName, &idx, &ret](const auto&... func)
		{
			ret = (func(spawnableName, idx) || ...);
		}, funcTuple);

	return ret ? idx : -1;
}



template<typename Spawnable>
CExpGenSpawnable* CreateSpawnableImpl()
{
	return projMemPool.alloc<Spawnable>();
}
CExpGenSpawnable* CExpGenSpawnable::CreateSpawnable(int spawnableID)
{
	static auto funcTuple = MAKE_FUNCTIONS_TUPLE(CreateSpawnableImpl);

	if (spawnableID < 0 || spawnableID > std::tuple_size<decltype(funcTuple)>::value - 1)
		return nullptr;

	CExpGenSpawnable* spawnable = nullptr;
	const auto Functor = [&spawnable](auto&& func) { spawnable = func(); };
	spring::tuple_exec_at(spawnableID, funcTuple, Functor);

	return spawnable;
}

#undef MAKE_FUNCTIONS_TUPLE

void CExpGenSpawnable::AddEffectsQuad(const VA_TYPE_TC& tl, const VA_TYPE_TC& tr, const VA_TYPE_TC& br, const VA_TYPE_TC& bl) const
{
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