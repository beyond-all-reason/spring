/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DebugColVolDrawer.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/ReadMap.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/SubState.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/Weapon.h"
#include "System/UnorderedSet.hpp"

static constexpr float4 DEFAULT_COLVOL_COLOR = float4(0.45f, 0.00f, 0.45f, 0.35f); // purple (light)
static constexpr float4 DEFAULT_SELVOL_COLOR = float4(0.00f, 0.45f, 0.00f, 0.20f); // dark green
static constexpr float4 DEFAULT_TARGET_COLOR = float4(1.00f, 0.80f, 0.00f, 0.40f); // yellowish
static constexpr float4 DEFAULT_MUZZLE_COLOR = float4(1.00f, 0.00f, 0.00f, 0.40f); // red
static constexpr float4 DEFAULT_AIMFRM_COLOR = float4(1.00f, 1.00f, 0.00f, 0.40f); // yellow
static constexpr float4 DEFAULT_AIMPOS_COLOR = float4(1.00f, 0.00f, 0.00f, 0.35f); // red
static constexpr float4 DEFAULT_MIDPOS_COLOR = float4(1.00f, 0.00f, 1.00f, 0.35f); // purple (dark)
static constexpr float4 DEFAULT_SHIELD_COLOR = float4(0.00f, 0.00f, 0.60f, 0.35f); // bluish
static constexpr float4 DEFAULT_CUSTCV_COLOR = float4(0.50f, 0.50f, 0.50f, 0.35f); // grey
static constexpr float4 DEFAULT_BUGOFF_COLOR = float4(0.00f, 1.00f, 1.00f, 0.35f); // cyan

static inline void DrawCollisionVolume(const CollisionVolume* vol, const CMatrix44f& mSrc, const float4& color)
{
	CMatrix44f m = mSrc;
	switch (vol->GetVolumeType()) {
		case CollisionVolume::COLVOL_TYPE_ELLIPSOID:
		case CollisionVolume::COLVOL_TYPE_SPHERE: {
			// scaled sphere is special case of ellipsoid: radius, slices, stacks
			m.Translate(vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2));
			m.Scale(vol->GetHScale(0), vol->GetHScale(1), vol->GetHScale(2));
			GL::shapes.DrawWireSphere(20, 20, m, color);
		} break;
		case CollisionVolume::COLVOL_TYPE_CYLINDER: {
			// scaled cylinder: base-radius, top-radius, height, slices, stacks
			//
			// (cylinder base is drawn at unit center by default so add offset
			// by half major axis to visually match the mathematical situation,
			// height of the cylinder equals the unit's full major axis)
			switch (vol->GetPrimaryAxis()) {
				case CollisionVolume::COLVOL_AXIS_X: {
					m.Translate(-(vol->GetHScale(0)), 0.0f, 0.0f);
					m.Translate(vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2));
					m.Scale(vol->GetScale(0), vol->GetHScale(1), vol->GetHScale(2));
					m.RotateY( 90.0f * math::DEG_TO_RAD);
				} break;
				case CollisionVolume::COLVOL_AXIS_Y: {
					m.Translate(0.0f, -(vol->GetHScale(1)), 0.0f);
					m.Translate(vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2));
					m.Scale(vol->GetHScale(0), vol->GetScale(1), vol->GetHScale(2));
					m.RotateX(-90.0f * math::DEG_TO_RAD);
				} break;
				case CollisionVolume::COLVOL_AXIS_Z: {
					m.Translate(0.0f, 0.0f, -(vol->GetHScale(2)));
					m.Translate(vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2));
					m.Scale(vol->GetHScale(0), vol->GetHScale(1), vol->GetScale(2));
				} break;
			}
			GL::shapes.DrawWireCylinder(20, m, color);
		} break;
		case CollisionVolume::COLVOL_TYPE_BOX: {
			// scaled cube: length, width, height
			m.Translate(vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2));
			m.Scale(vol->GetScale(0), vol->GetScale(1), vol->GetScale(2));
			GL::shapes.DrawWireBox(m, color);
		} break;
	}
}

static void DrawObjectDebugPieces(const CSolidObject* o, const float4& defColor)
{
	const CMatrix44f mo = o->GetTransformMatrix(false);

	const int hitDeltaTime = gs->frameNum - o->pieceHitFrames[true];
	const int setFadeColor = (o->pieceHitFrames[true] > 0 && hitDeltaTime < 150);

	for (uint32_t n = 0; n < o->localModel.pieces.size(); n++) {
		const LocalModelPiece* lmp = o->localModel.GetPiece(n);
		const CollisionVolume* lmpVol = lmp->GetCollisionVolume();

		if (!lmp->GetScriptVisible() || lmpVol->IgnoreHits())
			continue;

		float4 curColor = (setFadeColor && lmp == o->hitModelPieces[true]) ?
			float4{ (1.0f - (hitDeltaTime / 150.0f)), 0.0f, 0.0f, 1.0f } :
			defColor;

		const CMatrix44f mp = mo * lmp->GetModelSpaceMatrix();
		// factors in the volume offsets
		DrawCollisionVolume(lmpVol, mp, curColor);
	}
}



static inline void DrawObjectMidAndAimPos(const CSolidObject* o)
{
	using namespace GL::State;
	auto state = GL::SubState(
		DepthTest(GL_FALSE)
	);

	auto* shader = GL::shapes.GetShader();
	auto shToken = shader->EnableScoped();

	auto m = o->GetTransformMatrix(false);

	shader->SetUniformMatrix4x4("viewProjMat", false, camera->GetViewProjectionMatrix().m);

	if (o->aimPos != o->midPos) {
		// draw the aim-point
		m.Translate(o->relAimPos);
		m.Scale(2.0f);

		shader->SetUniformMatrix4x4("worldMat", false, m.m);
		shader->SetUniform4v("meshColor", &DEFAULT_AIMPOS_COLOR.x);

		GL::shapes.DrawSolidSphere(5, 5);

		//undo
		m.Scale(0.5f);
		m.Translate(-o->relAimPos);
	}
	{
		// draw the mid-point, keep this transform baked in the matrix
		m.Translate(o->relMidPos);
		shader->SetUniform4v("meshColor", &DEFAULT_MIDPOS_COLOR.x);
		shader->SetUniformMatrix4x4("worldMat", false, m.m);
		GL::shapes.DrawSolidSphere(5, 5);
		// last thing, no need to undo
	}
}



static inline void DrawFeatureColVol(const CFeature* f)
{
	const CollisionVolume* v = &f->collisionVolume;

	if (f->IsInVoid())
		return;
	if (!f->IsInLosForAllyTeam(gu->myAllyTeam) && !gu->spectatingFullView)
		return;
	if (!camera->InView(f->pos, f->GetDrawRadius()))
		return;

	CMatrix44f fm(f->midPos);
	fm.SetXYZ(f->GetTransformMatrixRef(false));

	DrawObjectMidAndAimPos(f);
	DrawCollisionVolume(&f->selectionVolume, fm, DEFAULT_SELVOL_COLOR);

	if (v->DefaultToPieceTree()) {
		DrawObjectDebugPieces(f, DEFAULT_COLVOL_COLOR);
	} else {
		if (!v->IgnoreHits()) {
			DrawCollisionVolume(v, fm, DEFAULT_COLVOL_COLOR);
		}
	}

	if (v->HasCustomType() || v->HasCustomProp(f->radius)) {
		// assume this is a custom volume; draw radius-sphere next to it
		CMatrix44f m = fm;
		m.Scale(f->radius);

		GL::shapes.DrawWireSphere(20, 20, m, DEFAULT_CUSTCV_COLOR);
	}
}

static inline void DrawUnitColVol(const CUnit* u)
{
	if (u->IsInVoid())
		return;
	if (!(u->losStatus[gu->myAllyTeam] & LOS_INLOS) && !gu->spectatingFullView)
		return;
	if (!camera->InView(u->drawMidPos, u->GetDrawRadius()))
		return;

	const CollisionVolume* v = &u->collisionVolume;

	auto* shader = GL::shapes.GetShader();
	{
		using namespace GL::State;
		auto state = GL::SubState(
			DepthTest(GL_FALSE)
		);

		auto token = shader->EnableScoped();
		shader->SetUniformMatrix4x4("viewProjMat", false, camera->GetViewProjectionMatrix().m);


		for (const CWeapon* w : u->weapons) {
			{
				CMatrix44f m{ w->aimFromPos };
				shader->SetUniform4v("meshColor", &DEFAULT_AIMFRM_COLOR.x);
				shader->SetUniformMatrix4x4("worldMat", false, m.m);
				GL::shapes.DrawSolidSphere(5, 5);
			}
			{
				CMatrix44f m{ w->weaponMuzzlePos };
				if (w->HaveTarget()) {
					shader->SetUniform4v("meshColor", &DEFAULT_TARGET_COLOR.x);
				}
				else {
					shader->SetUniform4v("meshColor", &DEFAULT_MUZZLE_COLOR.x);
				}
				shader->SetUniformMatrix4x4("worldMat", false, m.m);
				GL::shapes.DrawSolidSphere(5, 5);
			}

			if (w->HaveTarget()) {
				CMatrix44f m{ w->GetCurrentTargetPos() };

				shader->SetUniform4v("meshColor", &DEFAULT_TARGET_COLOR.x);
				shader->SetUniformMatrix4x4("worldMat", false, m.m);
				GL::shapes.DrawSolidSphere(5, 5);
			}
		}
	}

	{
		using namespace GL::State;
		auto state = GL::SubState(
			DepthTest(GL_TRUE)
		);

		DrawObjectMidAndAimPos(u);

		CMatrix44f um(u->midPos);
		um.SetXYZ(u->GetTransformMatrix(false));
		DrawCollisionVolume(&u->selectionVolume, um, DEFAULT_SELVOL_COLOR);

		if (v->DefaultToPieceTree()) {
			DrawObjectDebugPieces(u, DEFAULT_COLVOL_COLOR);
		} else {
			if (!v->IgnoreHits()) {
				const int hitDeltaTime = gs->frameNum - u->lastAttackFrame;
				const int setFadeColor = (u->lastAttackFrame > 0 && hitDeltaTime < 150);

				// make it fade red under attack
				const float4 curColor = setFadeColor ?
					float4{ 1.0f - (hitDeltaTime / 150.0f), 0.0f, 0.0f, 1.0f } :
					DEFAULT_COLVOL_COLOR;

				DrawCollisionVolume(v, um, curColor);
			}
		}
		if (u->shieldWeapon != nullptr) {
			const CPlasmaRepulser* shieldWeapon = static_cast<const CPlasmaRepulser*>(u->shieldWeapon);
			const CollisionVolume* shieldColVol = &shieldWeapon->collisionVolume;

			const CMatrix44f m{ shieldWeapon->weaponMuzzlePos };
			DrawCollisionVolume(shieldColVol, m, DEFAULT_SHIELD_COLOR);
		}

		if (v->HasCustomType() || v->HasCustomProp(u->radius)) {
			// assume this is a custom volume; draw radius-sphere next to it
			CMatrix44f m = um;
			m.Scale(u->radius);
			GL::shapes.DrawWireSphere(20, 20, m, DEFAULT_CUSTCV_COLOR);
		}

		if (const CFactory* f = dynamic_cast<const CFactory*>(u)) {
			if (f->boPerform) {
				float3 boRelDir = (f->boRelHeading == 0) ? FwdVector : GetVectorFromHeading(f->boRelHeading % SPRING_MAX_HEADING);
				float3 boPos = f->pos + boRelDir * f->boOffset;

				CMatrix44f boMat(boPos);

				if (f->boSherical) {
					boMat.Scale(f->boRadius);
					GL::shapes.DrawWireSphere(20, 20, boMat, DEFAULT_BUGOFF_COLOR);
				}
				else {
					boMat.RotateX(-90.0f * math::DEG_TO_RAD);
					boMat.Scale(f->boRadius, f->boRadius, 20.0f);
					GL::shapes.DrawWireCylinder(20, boMat, DEFAULT_BUGOFF_COLOR);
				}
			}
		}
	}
}


class CDebugColVolQuadDrawer : public CReadMap::IQuadDrawer {
public:
	void ResetState() { alreadyDrawnIds.clear(); }
	void DrawQuad(int x, int y)
	{
		const CQuadField::Quad& q = quadField.GetQuadAt(x, y);

		for (const CFeature* f: q.features) {
			if (alreadyDrawnIds.find(MAX_UNITS + f->id) == alreadyDrawnIds.end()) {
				alreadyDrawnIds.insert(MAX_UNITS + f->id);
				DrawFeatureColVol(f);
			}
		}

		for (const CUnit* u: q.units) {
			if (alreadyDrawnIds.find(u->id) == alreadyDrawnIds.end()) {
				alreadyDrawnIds.insert(u->id);
				DrawUnitColVol(u);
			}
		}
	}

	spring::unordered_set<int> alreadyDrawnIds;
};



namespace DebugColVolDrawer
{
	bool enable = false;

	void Draw()
	{
		if (!enable)
			return;

		using namespace GL::State;
		auto state = GL::SubState(
			Culling(GL_FALSE),
			AlphaTest(GL_FALSE),
			ClipDistance<0>(GL_FALSE), // ClipDistance<0> is same as ClipPlane<0> could have been
			ClipDistance<1>(GL_FALSE),
			Blending(GL_TRUE),
			BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA),
			DepthMask(GL_TRUE),
			LineWidth(2.0f)
		);

		static CDebugColVolQuadDrawer drawer;
		drawer.ResetState();
		readMap->GridVisibility(nullptr, &drawer, 1e9, CQuadField::BASE_QUAD_SIZE / SQUARE_SIZE);
	}
}