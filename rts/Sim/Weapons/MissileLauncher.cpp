/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MissileLauncher.h"

#include "WeaponDef.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/SpringMath.h"

#include "Rendering/GlobalRendering.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"

#include <tracy/Tracy.hpp>

CR_BIND_DERIVED(CMissileLauncher, CWeapon, )
CR_REG_METADATA(CMissileLauncher, )


void CMissileLauncher::UpdateWantedDir()
{
	//ZoneScoped;
	CWeapon::UpdateWantedDir();

	if (weaponDef->trajectoryHeight > 0.0f) {
		wantedDir.y += weaponDef->trajectoryHeight;
		wantedDir.Normalize();
	}
}

void CMissileLauncher::FireImpl(const bool scriptCall)
{
	//ZoneScoped;
	float3 targetVec = currentTargetPos - weaponMuzzlePos;
	const float targetDist = targetVec.LengthNormalize();

	if (onlyForward) {
		targetVec = owner->frontdir;
	} else if (weaponDef->fixedLauncher) {
		targetVec = weaponDir;
	} else if (weaponDef->trajectoryHeight > 0.0f) {
		targetVec = (targetVec + UpVector * weaponDef->trajectoryHeight).Normalize();
	}

	targetVec += (gsRNG.NextVector() * SprayAngleExperience() + SalvoErrorExperience());
	targetVec.Normalize();

	float3 startSpeed = targetVec * weaponDef->startvelocity;

	// NOTE: why only for SAMT units?
	if (onlyForward && owner->unitDef->IsStrafingAirUnit())
		startSpeed += owner->speed;

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.end = currentTargetPos;
	params.speed = startSpeed;
	params.ttl = weaponDef->flighttime == 0? math::ceil(std::max(targetDist, range) / projectileSpeed + 25 * weaponDef->selfExplode): weaponDef->flighttime;

	WeaponProjectileFactory::LoadProjectile(params);
}

bool CMissileLauncher::HaveFreeLineOfFire(const float3 srcPos, const float3 tgtPos, const SWeaponTarget& trg) const
{
	//ZoneScoped;
	// high-trajectory missiles use curved path rather than linear ground intersection
	if (weaponDef->trajectoryHeight <= 0.0f)
		return (CWeapon::HaveFreeLineOfFire(srcPos, tgtPos, trg));

	float3 targetVec = (tgtPos - srcPos) * XZVector;
	float3 launchDir = (tgtPos - srcPos).SafeNormalize();

	const float xzTargetDist = targetVec.LengthNormalize();

	if (xzTargetDist == 0.0f)
		return true;

	// trajectoryHeight missiles follow a pursuit curve
	// https://en.wikipedia.org/wiki/Pursuit_curve
	// however, while the basic case of a pursuit curve has an explicit solution
	// the case here with a potentially accelerating pursuer has no explicit solution
	// and the last linear portion of the trajectory needs to be accounted for
	// The curve can still be stated as a differential equation and approximately solved
	// I found using Heun's method (a midpoint method) to work best
	// https://en.wikipedia.org/wiki/Heun%27s_method
	//
	// Following (2D) solution assumes nonzero turnrate
	// because a zero turnrate will fail to hit the target
	// 
	// extraHeight = eH = (dist * trajectoryHeight)
	// extraHeightTime = eHT = dist / maxSpeed
	// dr/dt = r'(t,r,y) = (V0 + a * t) * (rt - r)/distance
	// dy/dt = y'(t,r,y) = (V0 + a * t) * (yt + (eH * (1-t/eHT)) - y)/distance
	// distance = sqrt( (rt - r)^2 + (yt + (eH * (1-t/eHT)) - y)^2)
	// velocity capped at maxSpeed
	// ~r_n+1 = r_n + h*r'(t,r,y)
	// ~y_n+1 = y_n + h*y'(t,r,y)
	// r_n+1 = r_n + (h/2)*( r'(t,r,y) + r'(t+h,~r_n+1,~y_n+1)
	// y_n+1 = y_n + (h/2)*( y'(t,r,y) + y'(t+h,~r_n+1,~y_n+1)
	// 
	// for Heun's method, we choose h so that we only need to calculate 7 points
	// on the curved trajectoryheight controlled portion
	// so the final curve can be approximated by 8 straight line segments

	std::array<float, 9> mdist = {}; //distance radially the missile has travelled
	std::array<float, 9> mheight = {}; //distance vertically the missile has travelled
	// put the startpoint and endpoints in the arrays
	mdist[0] = 0;
	mheight[0] = 0;
	mdist[8] = xzTargetDist;
	mheight[8] = (tgtPos.y - srcPos.y);

	// set up constants and temp variables
	const float maxSpeed = weaponDef->projectilespeed;
	const float pSpeed = weaponDef->startvelocity;
	const float pAcc = weaponDef->weaponacceleration;
	float curspeed = weaponDef->startvelocity;
	float dist = srcPos.distance(tgtPos);
	float rt = (tgtPos - srcPos).Length2D();
	float yt = (tgtPos.y - srcPos.y);
	float eH = (dist * weaponDef->trajectoryHeight);
	int eHT = int(dist / maxSpeed);
	float hstep = eHT / 8.0f;

	// For close targets, impact within 8 frames, just use a TestTrajectoryCone check
	if (hstep < 1.0f)
		return (CWeapon::HaveFreeLineOfFire(srcPos, tgtPos, trg));

	float drdt = 0.0f;
	float dydt = 0.0f;
	float rt_est = 0.0f;
	float yt_est = 0.0f;
	float drdt_est = 0.0f;
	float dydt_est = 0.0f;

	float t = 0.0f;
	// perform the Heun's method
	for (int i = 1; i < 8; i++) {
		// due to maxSpeed boundary, and parameters of the pursuit curve, dist cannot be zero
		// but if a divide by zero error does somehow occur here, a zero check can be added
		dist = math::sqrt(math::pow((rt - mdist[i-1]), 2) + math::pow((yt + eH * (1 - t / eHT) - mheight[i-1]), 2));
		curspeed = std::min((pSpeed + pAcc * t), maxSpeed);
		drdt = curspeed * (rt - mdist[i-1]) / dist;
		dydt = curspeed * (yt + eH * (1 - t / eHT) - mheight[i-1]) / dist;
		rt_est = mdist[i-1] + hstep * drdt;
		yt_est = mheight[i-1] + hstep * dydt;
		t = t + hstep;
		dist = math::sqrt(math::pow((rt - rt_est), 2) + math::pow((yt + eH * (1 - t / eHT) - yt_est), 2));
		curspeed = std::min((pSpeed + pAcc * t), maxSpeed);
		drdt_est = curspeed * (rt - rt_est) / dist;
		dydt_est = curspeed * (yt + eH * (1 - t / eHT) - yt_est) / dist;
		mdist[i] = mdist[i-1] + (hstep * 0.5f) * (drdt + drdt_est);
		mheight[i] = mheight[i-1] + (hstep * 0.5f) * (dydt + dydt_est);
	}

	// debug draw
	if (globalRendering->drawDebugTraceRay) {
		for (int i = 1; i < 9; i++) {
			geometricObjects->SetColor(geometricObjects->AddLine(srcPos + targetVec * mdist[i-1] + UpVector * mheight[i-1], srcPos + targetVec * mdist[i] + UpVector * mheight[i], 3, 0, GAME_SPEED), 1.0f, 0.0f, 0.0f, 1.0f);
		}
	}

	// check for ground collision
	// might be better to spin this off into TraceRay.cpp 
	// but the pursuit curve (and the 8 piecewise linear approximation used here) 
	// that trajectoryheight missiles follow is singularly unique
	// so no need to spin it off until something else needs this
	int ii = 1;
	float delta1 = mdist[ii] - mdist[ii - 1];
	float delta2 = 0.0f;
	float ratio = 0.0f;
	float hitheight = 0.0f;
	if ((avoidFlags & Collision::NOGROUND) == 0) {
		// do not check last bit of trajectory, sized by damageAreaOfEffect
		// to avoid false positive values at very end of trajectory
		// this mimics CGround::TrajectoryGroundCol called by parabolic cannon shots 
		// GetApproximateHeight should already do map boundary checks
		for (float dd = 0; dd < xzTargetDist - damages->damageAreaOfEffect; dd += SQUARE_SIZE) {
			// make sure we are using correct part of the trajectory
			while (dd > mdist[ii]) {
				ii = ii + 1;
				delta1 = mdist[ii] - mdist[ii - 1];
			}
			delta2 = dd - mdist[ii - 1];
			ratio = delta2 / delta1;
			hitheight = mheight[ii - 1] + ratio * (mheight[ii] - mheight[ii - 1]);
			if (CGround::GetApproximateHeight(srcPos + targetVec*dd) > (srcPos.y + hitheight)) {
				return false;
			}
		}
	}

	// check for object collision
	// might be better to spin this off into TraceRay.cpp 
	// but the pursuit curve (and the 8 piecewise linear approximation used here) 
	// that trajectoryheight missiles follow is singularly unique
	// so no need to spin it off until something else needs this
	QuadFieldQuery qfQuery;
	quadField.GetQuadsOnRay(qfQuery, srcPos, targetVec, xzTargetDist);

	if (qfQuery.quads->empty())
		return true;

	CollisionQuery cq;

	const bool scanForAllies = ((avoidFlags & Collision::NOFRIENDLIES) == 0);
	const bool scanForNeutrals = ((avoidFlags & Collision::NONEUTRALS) == 0);
	const bool scanForFeatures = ((avoidFlags & Collision::NOFEATURES) == 0);
	for (const int quadIdx : *qfQuery.quads) {
		const CQuadField::Quad& quad = quadField.GetQuad(quadIdx);

		// friendly units in this quad
		if (scanForAllies) {
			for (const CUnit* u : quad.teamUnits[owner->allyteam]) {
				if (u == owner)
					continue;
				if (!u->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS))
					continue;

				// chord check here
				const CollisionVolume* cv = &u->collisionVolume;
				const float3 cvRelVec = cv->GetWorldSpacePos(u) - srcPos;
				const float  cvRelDst = std::clamp(cvRelVec.dot(targetVec), 0.0f, xzTargetDist);
				const CMatrix44f objTransform = u->GetTransformMatrix(true);
				for (int i = 1; i < 9; i++) {
					if (cvRelDst < mdist[i]) {
						// find the relevant linear segment
						// interpolate the location
						delta1 = mdist[i] - mdist[i - 1];
						delta2 = cvRelDst - mdist[i - 1];
						ratio = delta2 / delta1;
						hitheight = mheight[i - 1] + ratio*(mheight[i] - mheight[i - 1]);
						const float3 hitPos = srcPos + targetVec * cvRelDst + UpVector * hitheight;
						if (mheight[i] > mheight[i - 1]) {
							// do chord check backwards
							if (CCollisionHandler::DetectHit(u, objTransform, srcPos, hitPos, &cq, true)) {
								return false;
							}
						} else {
							// do chord check forwards
							if (CCollisionHandler::DetectHit(u, objTransform, hitPos, tgtPos, &cq, true)) {
								return false;
							}
							
						}
						break;
					}
				}
			}
		}

		
		// neutral units in this quad
		if (scanForNeutrals) {
			for (const CUnit* u : quad.units) {
				if (!u->IsNeutral())
					continue;
				if (u == owner)
					continue;
				if (!u->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS))
					continue;

				// chord check here
				const CollisionVolume* cv = &u->collisionVolume;
				const float3 cvRelVec = cv->GetWorldSpacePos(u) - srcPos;
				const float  cvRelDst = std::clamp(cvRelVec.dot(targetVec), 0.0f, xzTargetDist);
				const CMatrix44f objTransform = u->GetTransformMatrix(true);
				for (int i = 1; i < 9; i++) {
					if (cvRelDst < mdist[i]) {
						// find the relevant linear segment
						// interpolate the location
						delta1 = mdist[i] - mdist[i - 1];
						delta2 = cvRelDst - mdist[i - 1];
						ratio = delta2 / delta1;
						hitheight = mheight[i - 1] + ratio * (mheight[i] - mheight[i - 1]);
						const float3 hitPos = srcPos + targetVec * cvRelDst + UpVector * hitheight;
						if (mheight[i] > mheight[i - 1]) {
							// do chord check backwards
							if (CCollisionHandler::DetectHit(u, objTransform, srcPos, hitPos, &cq, true)) {
								return false;
							}
						}
						else {
							// do chord check forwards
							if (CCollisionHandler::DetectHit(u, objTransform, hitPos, tgtPos, &cq, true)) {
								return false;
							}

						}
						break;
					}
				}
			}
		}

		// features in this quad
		if (scanForFeatures) {
			for (const CFeature* f : quad.features) {
				if (!f->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS))
					continue;

				// chord check here
				const CollisionVolume* cv = &f->collisionVolume;
				const float3 cvRelVec = cv->GetWorldSpacePos(f) - srcPos;
				const float  cvRelDst = std::clamp(cvRelVec.dot(targetVec), 0.0f, xzTargetDist);
				const CMatrix44f objTransform = f->GetTransformMatrix(true);
				for (int i = 1; i < 9; i++) {
					if (cvRelDst < mdist[i]) {
						// find the relevant linear segment
						// interpolate the location
						delta1 = mdist[i] - mdist[i - 1];
						delta2 = cvRelDst - mdist[i - 1];
						ratio = delta2 / delta1;
						hitheight = mheight[i - 1] + ratio * (mheight[i] - mheight[i - 1]);
						const float3 hitPos = srcPos + targetVec * cvRelDst + UpVector * hitheight;
						if (mheight[i] > mheight[i - 1]) {
							// do chord check backwards
							if (CCollisionHandler::DetectHit(f, objTransform, srcPos, hitPos, &cq, true)) {
								return false;
							}
						}
						else {
							// do chord check forwards
							if (CCollisionHandler::DetectHit(f, objTransform, hitPos, tgtPos, &cq, true)) {
								return false;
							}

						}
						break;
					}
				}
			}
		}
	}

	return true;
}

