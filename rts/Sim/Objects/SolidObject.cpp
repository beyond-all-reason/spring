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

	CR_MEMBER(drawFlag),
	CR_MEMBER(previousDrawFlag),

	CR_MEMBER(buildFacing),
	CR_MEMBER(modParams),

	CR_POSTLOAD(PostLoad)
))


void CSolidObject::PostLoad()
{
	if ((model = GetDef()->LoadModel()) == nullptr)
		return;

	localModel.SetModel(model, false);
}


void CSolidObject::UpdatePhysicalState(float eps)
{
	const float gh = CGround::GetHeightReal(pos.x, pos.z);
	const float wh = std::max(gh, 0.0f);

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
	ps |= (PSTATE_BIT_ONGROUND    * ((   pos.y -         gh) <=  eps));
	ps |= (PSTATE_BIT_INWATER     * ((   pos.y             ) <= 0.0f));
//	ps |= (PSTATE_BIT_UNDERWATER  * ((   pos.y +     height) <  0.0f));
//	ps |= (PSTATE_BIT_UNDERGROUND * ((   pos.y +     height) <    gh));
	ps |= (PSTATE_BIT_UNDERWATER  * ((midPos.y +     radius) <  0.0f));
	ps |= (PSTATE_BIT_UNDERGROUND * ((midPos.y +     radius) <    gh));
	ps |= (PSTATE_BIT_INAIR       * ((   pos.y -         wh) >   eps));
	ps |= (PSTATE_BIT_INAIR       * ((    ps   & MASK_NOAIR) ==    0));
	#undef MASK_NOAIR

	physicalState = static_cast<PhysicalState>(ps);

	// verify mutex relations (A != B); if one
	// fails then A and B *must* both be false
	//
	// problem case: pos.y < eps (but > 0) &&
	// gh < -eps causes ONGROUND and INAIR to
	// both be false but INWATER will fail too
	#if 0
	assert((IsInAir() != IsOnGround()) || IsInWater());
	assert((IsInAir() != IsInWater()) || IsOnGround());
	assert((IsInAir() != IsUnderWater()) || (IsOnGround() || IsInWater()));
	#endif
}


bool CSolidObject::SetVoidState()
{
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
	if (set) {
		SetVoidState();
	} else {
		ClearVoidState();
	}

	noSelect = (set || !GetDef()->selectable);
}


void CSolidObject::SetMass(float newMass)
{
	mass = std::clamp(newMass, MINIMUM_MASS, MAXIMUM_MASS);
}


void CSolidObject::UnBlock()
{
	if (!IsBlocking())
		return;

	groundBlockingObjectMap.RemoveGroundBlockingObject(this);
	assert(!IsBlocking());
}

void CSolidObject::Block()
{
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
			const auto heightAboveWaterHere = std::max(*hPtr, 0.0f);
			if ((pos.y - SQUARE_SIZE) <= heightAboveWaterHere)
				return true;
			hPtr++;
		}
	}

	return false;
}


YardMapStatus CSolidObject::GetGroundBlockingMaskAtPos(float3 gpos) const
{
	const YardMapStatus* blockMap = GetBlockMap();

	if (blockMap == nullptr)
		return YARDMAP_OPEN;

	const int hxsize = footprint.x >> 1;
	const int hzsize = footprint.y >> 1;

	float3 frontv;
	float3 rightv;

	#if 1
		// use continuous floating-point space
		gpos   -= pos;
		gpos.x += SQUARE_SIZE / 2; //??? needed to move to SQUARE-center? (possibly current input is wrong)
		gpos.z += SQUARE_SIZE / 2;

		frontv =  frontdir;
		rightv = -rightdir; // world-space is RH, unit-space is LH
	#else
		// use old fixed space (4 facing dirs & ints for unit positions)

		// form the rotated axis vectors
		static constexpr float3 fronts[] = {FwdVector,  RgtVector, -FwdVector, -RgtVector};
		static constexpr float3 rights[] = {RgtVector, -FwdVector, -RgtVector,  FwdVector};

		// get used axis vectors
		frontv = fronts[buildFacing];
		rightv = rights[buildFacing];

		gpos -= float3(mapPos.x * SQUARE_SIZE, 0.0f, mapPos.y * SQUARE_SIZE);

		// need to revert some of the transformations of CSolidObject::GetMapPos()
		gpos.x += SQUARE_SIZE / 2 - (this->xsize >> 1) * SQUARE_SIZE;
		gpos.z += SQUARE_SIZE / 2 - (this->zsize >> 1) * SQUARE_SIZE;
	#endif

	// transform worldspace pos to unit rotation dependent `centered blockmap space` [-hxsize .. +hxsize] x [-hzsize .. +hzsize]
	float by = frontv.dot(gpos) / SQUARE_SIZE;
	float bx = rightv.dot(gpos) / SQUARE_SIZE;

	// outside of `blockmap space`?
	if ((math::fabsf(bx) >= hxsize) || (math::fabsf(by) >= hzsize))
		return YARDMAP_OPEN;

	// transform: [(-hxsize + eps) .. (+hxsize - eps)] x [(-hzsize + eps) .. (+hzsize - eps)] -> [0 .. (xsize - 1)] x [0 .. (zsize - 1)]
	bx += hxsize;
	by += hzsize;

	assert(int(bx) >= 0 && int(bx) < footprint.x);
	assert(int(by) >= 0 && int(by) < footprint.y);

	// read from blockmap
	return blockMap[int(bx) + int(by) * footprint.x];
}

int2 CSolidObject::GetMapPosStatic(const float3& position, int xsize, int zsize)
{
	int2 mp;

	mp.x = (int(position.x /*+ SQUARE_SIZE / 2*/) / SQUARE_SIZE) - (xsize / 2);
	mp.y = (int(position.z /*+ SQUARE_SIZE / 2*/) / SQUARE_SIZE) - (zsize / 2);
	mp.x = std::clamp(mp.x, 0, mapDims.mapx - xsize);
	mp.y = std::clamp(mp.y, 0, mapDims.mapy - zsize);

	return mp;
}

float3 CSolidObject::GetDragAccelerationVec(const float4& params) const
{
	// KISS: use the cross-sectional area of a sphere, object shapes are complex
	// this is a massive over-estimation so pretend the radius is in centimeters
	// other units as normal: mass in kg, speed in elmos/frame, density in kg/m^3
	//
	// params.xyzw map: {{atmosphere, water}Density, {drag, friction}Coefficient}
	//
	const float3 speedSignVec = float3(Sign(speed.x), Sign(speed.y), Sign(speed.z));
	const float3 dragScaleVec = float3(
		IsInAir()    * dragScales.x * (0.5f * params.x * params.z * (math::PI * sqRadius * 0.01f * 0.01f)), // air
		IsInWater()  * dragScales.y * (0.5f * params.y * params.z * (math::PI * sqRadius * 0.01f * 0.01f)), // water
		IsOnGround() * dragScales.z * (                  params.w * (                               mass))  // ground
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
	const float3 groundUp = CGround::GetSmoothNormal(pos.x, pos.z);
	const float3 curUpDir = float3{updir};
	const float3 objectUp = mix(UpVector, curUpDir, useObjectNormal);
	const float3 targetUp = mix(objectUp, groundUp, useGroundNormal);
	const float3 wantedUp = mix(targetUp, curUpDir, dirSmoothing).Normalize();

	return wantedUp;
}



void CSolidObject::SetDirVectorsEuler(const float3 angles)
{
	CMatrix44f matrix;

	// our system is left-handed, so R(X)R(Y)R(Z) is really T(R(-Z)R(-Y)R(-X))
	// whenever these angles are retrieved, the handedness is converted again
	SetDirVectors(matrix.RotateEulerXYZ(angles));
	SetHeadingFromDirection();
	SetFacingFromHeading();
	UpdateMidAndAimPos();
}

void CSolidObject::SetHeadingFromDirection() { heading = GetHeadingFromVector(frontdir.x, frontdir.z); }
void CSolidObject::SetFacingFromHeading() { buildFacing = GetFacingFromHeading(heading); }

void CSolidObject::UpdateDirVectors(bool useGroundNormal, bool useObjectNormal, float dirSmoothing)
{
	const float3 uDir = GetWantedUpDir(useGroundNormal, useObjectNormal, dirSmoothing);
	UpdateDirVectors(uDir);
}

void CSolidObject::UpdateDirVectors(const float3& uDir)
{
	// set initial rotation of the object around updir=UpVector first
	const float3 fDir = GetVectorFromHeading(heading);

	if likely(1.0f - math::fabs(uDir.y) >= 1e-6f) {
		const float3 norm = float3{ uDir.z, 0.0f, -uDir.x }.Normalize(); //same as UpVector.cross(uDir) to obtain normal vector, which will serve as a rotation axis
		frontdir = fDir.rotateByUpVector(uDir, norm); //doesn't change vector magnitude
	}
	else {
		frontdir = fDir * Sign(uDir.y);
	}
	rightdir = (frontdir.cross(uDir)).Normalize();
	updir = uDir;
}

void CSolidObject::ForcedSpin(const float3& zdir)
{
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



void CSolidObject::Kill(CUnit* killer, const float3& impulse, bool crushed)
{
	UpdateVoidState(false);
	DoDamage(DamageArray(health + 1.0f), impulse, killer, crushed? -DAMAGE_EXTSOURCE_CRUSHED: -DAMAGE_EXTSOURCE_KILLED, -1);
}



float CSolidObject::CalcFootPrintMinExteriorRadius(float scale) const { return ((math::sqrt((xsize * xsize + zsize * zsize)) * 0.5f * SQUARE_SIZE) * scale); }
float CSolidObject::CalcFootPrintMaxInteriorRadius(float scale) const { return ((std::max(xsize, zsize) * 0.5f * SQUARE_SIZE) * scale); }
float CSolidObject::CalcFootPrintAxisStretchFactor() const
{
	return (std::abs(xsize - zsize) * 1.0f / (xsize + zsize));
}

