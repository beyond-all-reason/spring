/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Projectile.h"
#include "Map/MapInfo.h"
#include "Rendering/Colors.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Sim/Projectiles/ExpGenSpawnableMemberInfo.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/Matrix44f.h"

#include "System/Misc/TracyDefs.h"

CR_BIND_DERIVED_INTERFACE(CProjectile, CExpGenSpawnable)

CR_REG_METADATA(CProjectile,
(
	CR_MEMBER(synced),
	CR_MEMBER(weapon),
	CR_MEMBER(piece),
	CR_MEMBER(hitscan),

	CR_IGNORED(luaDraw),
	CR_MEMBER(luaMoveCtrl),
	CR_MEMBER(checkCol),
	CR_MEMBER(ignoreWater),

	CR_IGNORED(createMe),
	CR_MEMBER(deleteMe),

	CR_MEMBER(drawSorted),

	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(castShadow),
		CR_MEMBER(dir),
		CR_MEMBER(drawOrder),
	CR_MEMBER_ENDFLAG(CM_Config),

	CR_MEMBER(drawPos),

	CR_MEMBER(myrange),
	CR_MEMBER(mygravity),
	CR_IGNORED(sortDist),
	CR_MEMBER(sortDistOffset),

	CR_MEMBER(validTextures),

	CR_MEMBER(ownerID),
	CR_MEMBER(teamID),
	CR_MEMBER(allyteamID),
	CR_MEMBER(cegID),

	CR_MEMBER(projectileType),
	CR_MEMBER(collisionFlags),
	CR_IGNORED(renderIndex),

	CR_MEMBER(quads)
))

TypedRenderBuffer<VA_TYPE_C> CProjectile::mmLnsRB = { 1 << 12, 0 };
TypedRenderBuffer<VA_TYPE_C> CProjectile::mmPtsRB = { 1 << 14, 0 };

namespace Impl {
	float3 GetNormalizedOrDefaultDir(const float3& dir) {
		float sqLen = dir.SqLength();
		if unlikely(sqLen < float3::cmp_eps())
			return FwdVector;

		return dir * math::isqrt(sqLen);
	}
}

CProjectile::CProjectile()
	: myrange(0.0f)
	, mygravity((mapInfo != nullptr)? mapInfo->map.gravity: 0.0f)
{
}

CProjectile::CProjectile(
	const float3& pos,
	const float3& spd,
	const CUnit* owner,
	bool isSynced,
	bool isWeapon,
	bool isPiece,
	bool isHitScan
): CExpGenSpawnable(pos, spd)

	, synced(isSynced)
	, weapon(isWeapon)
	, piece(isPiece)
	, hitscan(isHitScan)

	, myrange(/*params.weaponDef->range*/0.0f)
	, mygravity((mapInfo != nullptr)? mapInfo->map.gravity: 0.0f)
{
	preFrameTra = Transform{ CQuaternion::MakeFrom(Impl::GetNormalizedOrDefaultDir(dir)), pos };
	SetRadiusAndHeight(1.7f, 0.0f);
	Init(owner, ZeroVector);
}


CProjectile::~CProjectile()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!synced)
		return;

	quadField.RemoveProjectile(this);
}

void CProjectile::Init(const CUnit* owner, const float3& offset)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (owner != nullptr) {
		// must be set before the AddProjectile call
		ownerID = owner->id;
		teamID = owner->team;
		allyteamID =  teamHandler.IsValidTeam(teamID)? teamHandler.AllyTeam(teamID): -1;
	}
	if (!hitscan) {
		SetPosition(pos + offset);
		SetVelocityAndSpeed(speed);
	}

	CExpGenSpawnable::Init(owner, offset);

	// NOTE:
	//   new CWeapon- and CPieceProjectile*'s add themselves
	//   to CProjectileHandler (other code needs to be able
	//   to dyna-cast CProjectile*'s to those derived types,
	//   and adding them here would throw away too much RTTI)
	if (!weapon && !piece)
		projectileHandler.AddProjectile(this);

	if (synced && !weapon)
		quadField.AddProjectile(this);


	preFrameTra = Transform{ CQuaternion::MakeFrom(Impl::GetNormalizedOrDefaultDir(dir)), pos };
}


void CProjectile::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (luaMoveCtrl)
		return;

	SetVelocityAndSpeed(speed + (UpVector * mygravity));
	SetPosition(pos + speed);
}


void CProjectile::Delete()
{
	RECOIL_DETAILED_TRACY_ZONE;
	deleteMe = true;
	checkCol = false;
}

void CProjectile::PreUpdate()
{
	preFrameTra = Transform{ CQuaternion::MakeFrom(Impl::GetNormalizedOrDefaultDir(dir)), pos };
}


void CProjectile::DrawOnMinimap() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	AddMiniMapVertices({ pos        , color4::whiteA }, { pos + speed, color4::whiteA });
}


CUnit* CProjectile::owner() const {
	RECOIL_DETAILED_TRACY_ZONE;
	// NOTE:
	//   this death dependency optimization using "ownerID" is logically flawed:
	//   because ID's are reused it could return a unit that is not the original
	//   owner (unlikely however unless ID's get recycled very rapidly)
	return (unitHandler.GetUnit(ownerID));
}


CMatrix44f CProjectile::GetTransformMatrix(bool offsetPos) const {
	float3 xdir;
	float3 ydir;

	if (math::fabs(dir.y) < 0.95f) {
		xdir = dir.cross(UpVector);
		xdir.SafeANormalize();
	} else {
		xdir.x = 1.0f;
	}

	ydir = xdir.cross(dir);

	return (CMatrix44f(drawPos + (dir * radius * 0.9f * offsetPos), -xdir, ydir, dir));
}

bool CProjectile::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (CExpGenSpawnable::GetMemberInfo(memberInfo))
		return true;

	CHECK_MEMBER_INFO_BOOL(CProjectile, castShadow)
	CHECK_MEMBER_INFO_FLOAT3(CProjectile, dir)
	CHECK_MEMBER_INFO_INT(CProjectile, drawOrder)

	return false;
}

bool CProjectile::IsValidTexture(const AtlasedTexture* tex)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return tex && (*tex != AtlasedTexture::DefaultAtlasTexture);
}

void CProjectile::AddMiniMapVertices(VA_TYPE_C&& v1, VA_TYPE_C&& v2)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (v1.pos.equals(v2.pos)) {
		mmPtsRB.AddVertex(std::move(v1));
	}
	else {
		mmLnsRB.AddVertex(std::move(v1));
		mmLnsRB.AddVertex(std::move(v2));
	}
}
