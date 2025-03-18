/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "SolidObject.h"
#include "SolidObjectDef.h"
#include "Map/ReadMap.h"
#include "Map/Ground.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Game/GameHelper.h"
#include "System/SpringMath.h"
#include "System/Quaternion.h"

#include "System/Misc/TracyDefs.h"

int CSolidObject::deletingRefID = -1;


CR_BIND_DERIVED_INTERFACE(CSolidObject, CWorldObject)
CR_REG_METADATA(CSolidObject,
(
	CR_MEMBER(health),
	CR_MEMBER(maxHealth),
	CR_MEMBER(entityReference),

	CR_MEMBER(mass),
	CR_MEMBER(crushResistance),

	CR_MEMBER(crushable),
	CR_MEMBER(immobile),
	CR_MEMBER(yardOpen),

	CR_MEMBER(blockEnemyPushing),
	CR_MEMBER(blockHeightChanges),

	CR_MEMBER_UN(noDraw),
	CR_MEMBER_UN(luaDraw),
	CR_MEMBER_UN(noSelect),
	CR_MEMBER_UN(alwaysUpdateMat), //don't save?
	CR_MEMBER_UN(engineDrawMask),

	CR_MEMBER(xsize),
	CR_MEMBER(zsize),
 	CR_MEMBER(footprint),
	CR_MEMBER(heading),

	CR_MEMBER(physicalState),
	CR_MEMBER(collidableState),

	CR_MEMBER(team),
	CR_MEMBER(allyteam),

	CR_MEMBER(pieceHitFrames),

	CR_MEMBER(moveDef),

	CR_MEMBER(localModel),
	CR_MEMBER(collisionVolume),
	CR_MEMBER(selectionVolume), // unsynced, could also be ignored
	CR_MEMBER(hitModelPieces),

	CR_MEMBER(frontdir),
	CR_MEMBER(rightdir),
	CR_MEMBER(updir),

	CR_MEMBER(relMidPos),
 	CR_MEMBER(relAimPos),
	CR_MEMBER(midPos),
	CR_MEMBER(aimPos),
	CR_MEMBER(mapPos),
	CR_MEMBER(groundBlockPos),

	CR_MEMBER(dragScales),

	CR_MEMBER(drawPos),
	CR_MEMBER(drawMidPos),

	CR_MEMBER(buildFacing),
	CR_MEMBER(modParams),

	CR_POSTLOAD(PostLoad)
))


void CSolidObject::PostLoad()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if ((model = GetDef()->LoadModel()) == nullptr)
		return;

	localModel.SetModel(model, false);
}


void CSolidObject::UpdatePhysicalState(float eps)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float waterLevel = CGround::GetWaterLevel(pos.x, pos.z);
	const float groundHeight = CGround::GetHeightReal(pos.x, pos.z);
	// Get height of whichever surface is higher between ground and water
	const float topSurfaceHeight = std::max(groundHeight, waterLevel);

	unsigned int ps = physicalState;

	// clear all non-void non-special bits
	ps &= (~PSTATE_BIT_ONGROUND   );
	ps &= (~PSTATE_BIT_INWATER    );
	ps &= (~PSTATE_BIT_UNDERWATER );
	ps &= (~PSTATE_BIT_UNDERGROUND);
	ps &= (~PSTATE_BIT_INAIR      );

	// NOTE:
	//   height is not in general equivalent to radius * 2.0
	//   the height property is used for much fewer purposes
	//   than radius, so less reliable for determining state
	#define MASK_NOAIR (PSTATE_BIT_ONGROUND | PSTATE_BIT_INWATER | PSTATE_BIT_UNDERWATER | PSTATE_BIT_UNDERGROUND)
	ps |= (PSTATE_BIT_ONGROUND    * ((   pos.y -         groundHeight) <=  eps));
	ps |= (PSTATE_BIT_INWATER     * ((   pos.y             ) <= waterLevel));
//	ps |= (PSTATE_BIT_UNDERWATER  * ((   pos.y +     height) <  0.0f));
//	ps |= (PSTATE_BIT_UNDERGROUND * ((   pos.y +     height) <    groundHeight));
	ps |= (PSTATE_BIT_UNDERWATER  * ((midPos.y +     radius) <  waterLevel));
	ps |= (PSTATE_BIT_UNDERGROUND * ((midPos.y +     radius) <    groundHeight));
	ps |= (PSTATE_BIT_INAIR       * ((   pos.y -         topSurfaceHeight) >   eps));
	ps |= (PSTATE_BIT_INAIR       * ((    ps   & MASK_NOAIR) ==    0));
	#undef MASK_NOAIR

	physicalState = static_cast<PhysicalState>(ps);

	// verify mutex relations (A != B); if one
	// fails then A and B *must* both be false
	//
	// problem case: pos.y < eps (but > 0) &&
	// groundHeight < -eps causes ONGROUND and INAIR to
	// both be false but INWATER will fail too
	#if 0
	assert((IsInAir() != IsOnGround()) || IsInWater());
	assert((IsInAir() != IsInWater()) || IsOnGround());
	assert((IsInAir() != IsUnderWater()) || (IsOnGround() || IsInWater()));
	#endif
}


bool CSolidObject::SetVoidState()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (IsInVoid())
		return false;

	// make us transparent to raycasts, quadfield queries, etc.
	// need to push and pop state bits in case Lua changes them
	// (otherwise gadgets must listen to all Unit*Loaded events)
	PushCollidableStateBit(CSTATE_BIT_SOLIDOBJECTS);
	PushCollidableStateBit(CSTATE_BIT_PROJECTILES);
	PushCollidableStateBit(CSTATE_BIT_QUADMAPRAYS);
	ClearCollidableStateBit(CSTATE_BIT_SOLIDOBJECTS | CSTATE_BIT_PROJECTILES | CSTATE_BIT_QUADMAPRAYS);
	SetPhysicalStateBit(PSTATE_BIT_INVOID);

	UnBlock();
	collisionVolume.SetIgnoreHits(true);
	return true;
}

bool CSolidObject::ClearVoidState()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!IsInVoid())
		return false;

	PopCollidableStateBit(CSTATE_BIT_SOLIDOBJECTS);
	PopCollidableStateBit(CSTATE_BIT_PROJECTILES);
	PopCollidableStateBit(CSTATE_BIT_QUADMAPRAYS);
	ClearPhysicalStateBit(PSTATE_BIT_INVOID);

	Block();
	collisionVolume.SetIgnoreHits(false);
	return true;
}

void CSolidObject::UpdateVoidState(bool set)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (set) {
		SetVoidState();
	} else {
		ClearVoidState();
	}

	noSelect = (set || !GetDef()->selectable);
}


void CSolidObject::SetMass(float newMass)
{
	RECOIL_DETAILED_TRACY_ZONE;
	mass = std::clamp(newMass, MINIMUM_MASS, MAXIMUM_MASS);
}


void CSolidObject::UnBlock()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!IsBlocking())
		return;

	groundBlockingObjectMap.RemoveGroundBlockingObject(this);
	assert(!IsBlocking());
}

void CSolidObject::Block()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// no point calling this if object is not
	// collidable in principle, but simplifies
	// external code to allow it
	if (!HasCollidableStateBit(CSTATE_BIT_SOLIDOBJECTS))
		return;

	if (IsBlocking() && !BlockMapPosChanged())
		return;

	UnBlock();

	// only block when `touching` the ground
	if (FootPrintOnGround()) {
		groundBlockingObjectMap.AddGroundBlockingObject(this);
		assert(IsBlocking());
	}
}

bool CSolidObject::FootPrintOnGround() const {
	const float sdist = std::max(radius, CalcFootPrintMinExteriorRadius());

	if ((pos.y - sdist) <= CGround::GetHeightAboveWater(pos.x, pos.z))
		return true;

	const auto fpr = CGameHelper::BuildPosToRect(pos, buildFacing, xsize, zsize);
	SRectangle hmFpr = {
		std::clamp(int(fpr.x) / SQUARE_SIZE, 0, mapDims.mapxm1),
		std::clamp(int(fpr.y) / SQUARE_SIZE, 0, mapDims.mapxm1),
		std::clamp(int(fpr.z) / SQUARE_SIZE, 0, mapDims.mapym1),
		std::clamp(int(fpr.w) / SQUARE_SIZE, 0, mapDims.mapym1)
	};

	for (int z = hmFpr.z1; z <= hmFpr.z2; ++z) {
		const float* hPtr = CGround::GetApproximateHeightUnsafePtr(hmFpr.x1, z, true);
		for (int x = hmFpr.x1; x <= hmFpr.x2; ++x) {
			const auto heightAboveWaterHere = std::max(*hPtr, CGround::GetWaterLevel(x, z));
			if ((pos.y - SQUARE_SIZE) <= heightAboveWaterHere)
				return true;
			hPtr++;
		}
	}

	return false;
}


YardMapStatus CSolidObject::GetGroundBlockingMaskAtPos(float3 gpos) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const YardMapStatus* blockMap = GetBlockMap();
	if (blockMap == nullptr)
		return YARDMAP_OPEN;

	const int2 hFootprint{footprint.x >> 1, footprint.y >> 1};
	const int2 hSize{ xsize >> 1, zsize >> 1};

	const int2 gPos2
			{ int(gpos.x / SQUARE_SIZE)
			, int(gpos.z / SQUARE_SIZE)};
	const int2 diff = gPos2 - (mapPos + hSize);

	constexpr int2 rotationDirs[] = { {0,1}, {1,0}, {0,-1}, {-1,0}, {0,1} };
	const int2 front = rotationDirs[buildFacing];
	const int2 right = rotationDirs[buildFacing+1];

	// corrections needed because the rotation is off centre.
	constexpr int2 rotationCorrections[] = { {0,0}, {-1,0}, {-1,-1}, {0,-1} };
	const int2 adjust = rotationCorrections[buildFacing];

	// Translate from map-space to yardmap-space
	// negative result overflows to super high number
	const uint32_t by = (front.x*diff.x) + (front.y*diff.y) + hFootprint.y + adjust.y;
	const uint32_t bx = (right.x*diff.x) + (right.y*diff.y) + hFootprint.x + adjust.x;

	if ((bx >= footprint.x) || (by >= footprint.y))
		return YARDMAP_OPEN;

	// read from blockmap
	return blockMap[bx + by*footprint.x];
}

int2 CSolidObject::GetMapPosStatic(const float3& position, int xsize, int zsize)
{
	RECOIL_DETAILED_TRACY_ZONE;
	int2 mp;

	mp.x = (int(position.x /*+ SQUARE_SIZE / 2*/) / SQUARE_SIZE) - (xsize / 2);
	mp.y = (int(position.z /*+ SQUARE_SIZE / 2*/) / SQUARE_SIZE) - (zsize / 2);
	mp.x = std::clamp(mp.x, 0, mapDims.mapx - xsize);
	mp.y = std::clamp(mp.y, 0, mapDims.mapy - zsize);

	return mp;
}

float3 CSolidObject::GetDragAccelerationVec(float atmosphericDensity, float waterDensity, float dragCoeff, float frictionCoeff) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	// KISS: use the cross-sectional area of a sphere, object shapes are complex
	// this is a massive over-estimation so pretend the radius is in centimeters
	// other units as normal: mass in kg, speed in elmos/frame, density in kg/m^3
	//
	// params.xyzw map: {{atmosphere, water}Density, {drag, friction}Coefficient}
	//
	const float3 speedSignVec = float3(Sign(speed.x), Sign(speed.y), Sign(speed.z));
	const float3 dragScaleVec = float3(
		(IsInAir() || IsOnGround()) * dragScales.x * (0.5f * atmosphericDensity * dragCoeff * (math::PI * sqRadius * 0.01f * 0.01f)), // air
		IsInWater()                 * dragScales.y * (0.5f * waterDensity * dragCoeff * (math::PI * sqRadius * 0.01f * 0.01f)), // water
		IsOnGround()                * dragScales.z * (frictionCoeff * mass)  // ground
	);

	float3 dragAccelVec;

	dragAccelVec.x += (speed.x * speed.x * dragScaleVec.x * -speedSignVec.x);
	dragAccelVec.y += (speed.y * speed.y * dragScaleVec.x * -speedSignVec.y);
	dragAccelVec.z += (speed.z * speed.z * dragScaleVec.x * -speedSignVec.z);

	dragAccelVec.x += (speed.x * speed.x * dragScaleVec.y * -speedSignVec.x);
	dragAccelVec.y += (speed.y * speed.y * dragScaleVec.y * -speedSignVec.y);
	dragAccelVec.z += (speed.z * speed.z * dragScaleVec.y * -speedSignVec.z);

	// FIXME?
	//   magnitude of dynamic friction may or may not depend on speed
	//   coefficient must be multiplied by mass or it will be useless
	//   (due to division by mass since the coefficient is normalized)
	dragAccelVec.x += (math::fabs(speed.x) * dragScaleVec.z * -speedSignVec.x);
	dragAccelVec.y += (math::fabs(speed.y) * dragScaleVec.z * -speedSignVec.y);
	dragAccelVec.z += (math::fabs(speed.z) * dragScaleVec.z * -speedSignVec.z);

	// convert from force
	dragAccelVec /= mass;

	// limit the acceleration
	dragAccelVec.x = std::clamp(dragAccelVec.x, -math::fabs(speed.x), math::fabs(speed.x));
	dragAccelVec.y = std::clamp(dragAccelVec.y, -math::fabs(speed.y), math::fabs(speed.y));
	dragAccelVec.z = std::clamp(dragAccelVec.z, -math::fabs(speed.z), math::fabs(speed.z));

	return dragAccelVec;
}

float3 CSolidObject::GetWantedUpDir(bool useGroundNormal, bool useObjectNormal, float dirSmoothing) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float3 groundUp = CGround::GetSmoothNormal(pos.x, pos.z);
	const float3 curUpDir = float3{updir};
	const float3 objectUp = mix(UpVector, curUpDir, useObjectNormal);
	const float3 targetUp = mix(objectUp, groundUp, useGroundNormal);
	const float3 wantedUp = mix(targetUp, curUpDir, dirSmoothing).Normalize();

	return wantedUp;
}



void CSolidObject::SetDirVectorsEuler(const float3 angles)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CMatrix44f matrix;

	// our system is left-handed, so R(X)R(Y)R(Z) is really T(R(-Z)R(-Y)R(-X))
	// whenever these angles are retrieved, the handedness is converted again
	SetDirVectors(matrix.RotateEulerXYZ(angles));
	SetHeadingFromDirection();
	SetFacingFromHeading();
	UpdateMidAndAimPos();
}

void CSolidObject::SetHeadingFromDirection() {
	// undo UpdateDirVectors transformation

	// construct quaternion to describe rotation from uDir to UpVector
	CQuaternion quat(-updir.z, 0.0f, updir.x, 1.0f + updir.y); // same angle as in UpdateDirVectors, but inverted axis
	quat.ANormalize();

	const float3 fDir = quat * frontdir;
	assert(epscmp(fDir.y, 0.0f, float3::cmp_eps()));

	heading = GetHeadingFromVector(fDir.x, fDir.z);
}
void CSolidObject::SetFacingFromHeading() { buildFacing = GetFacingFromHeading(heading); }

void CSolidObject::UpdateDirVectors(bool useGroundNormal, bool useObjectNormal, float dirSmoothing)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float3 uDir = GetWantedUpDir(useGroundNormal, useObjectNormal, dirSmoothing);
	UpdateDirVectors(uDir);
}

void CSolidObject::UpdateDirVectors(const float3& uDir)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// set initial rotation of the object around updir=UpVector first
	const float3 fDir = GetVectorFromHeading(heading);
	const float3 rDir = float3{ -fDir.z, 0.0f, fDir.x };

	// construct quaternion to describe rotation from UpVector to uDir
	// can use CQuaternion::MakeFrom(const float3& v1, const float3& v2);
	// but simplified given UpVector is trivial
	CQuaternion quat(uDir.z, 0.0f, -uDir.x, 1.0f + uDir.y);
	quat.ANormalize();

	frontdir = quat * fDir;
	rightdir = quat * rDir;
	updir = uDir;
}

void CSolidObject::ForcedSpin(const float3& zdir)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// new front-direction should be normalized
	assert(math::fabsf(zdir.SqLength() - 1.0f) <= float3::cmp_eps());

	// if zdir is parallel to world-y, use heading-vector
	// (or its inverse) as auxiliary to avoid degeneracies

	const float zdotup = zdir.dot(UpVector);
	const float3 udir = mix(UpVector, -FwdVector, math::fabs(zdotup) >= 0.999f);
	const float3 xdir = (zdir.cross(udir)).Normalize();
	const float3 ydir = (xdir.cross(zdir)).Normalize();

	frontdir = zdir;
	rightdir = xdir;
	   updir = ydir;

	SetHeadingFromDirection();
	UpdateMidAndAimPos();
}

void CSolidObject::ForcedSpin(const float3& newFrontDir, const float3& newRightDir)
{
	// new front & right directions should be normalized
	assert(math::fabsf(newFrontDir.SqLength() - 1.0f) <= float3::cmp_eps());
	assert(math::fabsf(newRightDir.SqLength() - 1.0f) <= float3::cmp_eps());

	frontdir = newFrontDir;
	rightdir = newRightDir;
	   updir = (newRightDir.cross(newFrontDir)).Normalize();

	SetHeadingFromDirection();
	UpdateMidAndAimPos();
}



void CSolidObject::Kill(CUnit* killer, const float3& impulse, bool crushed)
{
	RECOIL_DETAILED_TRACY_ZONE;
	UpdateVoidState(false);
	DoDamage(DamageArray(health + 1.0f), impulse, killer, crushed? -DAMAGE_EXTSOURCE_CRUSHED: -DAMAGE_EXTSOURCE_KILLED, -1);
}



float CSolidObject::CalcFootPrintMinExteriorRadius(float scale) const { return ((math::sqrt((xsize * xsize + zsize * zsize)) * 0.5f * SQUARE_SIZE) * scale); }
float CSolidObject::CalcFootPrintMaxInteriorRadius(float scale) const { return ((std::max(xsize, zsize) * 0.5f * SQUARE_SIZE) * scale); }
float CSolidObject::CalcFootPrintAxisStretchFactor() const
{
	return (std::abs(xsize - zsize) * 1.0f / (xsize + zsize));
}

