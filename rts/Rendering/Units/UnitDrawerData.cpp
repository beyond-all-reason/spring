#include "UnitDrawerData.h"

#include "System/MemPoolTypes.h"
#include "System/ContainerUtil.h"
#include "System/EventHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Config/ConfigHandler.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/UI/MiniMap.h"
#include "Rendering/Common/ModelDrawerHelpers.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/Models/IModelParser.h"
#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/IconHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/Env/IWater.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"

#include "System/Misc/TracyDefs.h"

static FixedDynMemPoolT<MAX_UNITS / 1000, MAX_UNITS / 32, GhostSolidObject> ghostMemPool;

///////////////////////////

CR_BIND_POOL(GhostSolidObject, ,ghostMemPool.allocMem, ghostMemPool.freeMem)
CR_REG_METADATA(GhostSolidObject, (
	CR_MEMBER(modelName),

	CR_MEMBER(pos),
	CR_MEMBER(dir),

	CR_MEMBER(facing),
	CR_MEMBER(team),
	CR_MEMBER(refCount),

	CR_IGNORED(model),

	CR_POSTLOAD(PostLoad)
))

CR_BIND(CUnitDrawerData::TempDrawUnit, )
CR_REG_METADATA(CUnitDrawerData::TempDrawUnit, (
	CR_MEMBER(unitDefId),

	CR_MEMBER(team),
	CR_MEMBER(facing),
	CR_MEMBER(timeout),

	CR_MEMBER(pos),
	CR_MEMBER(rotation),

	CR_MEMBER(drawAlpha),
	CR_MEMBER(drawBorder),

	CR_IGNORED(unitDef)
))

CR_BIND(CUnitDrawerData::SavedData, )
CR_REG_METADATA(CUnitDrawerData::SavedData, (
	CR_MEMBER(tempOpaqueUnits),
	CR_MEMBER(tempAlphaUnits),
	CR_MEMBER(deadGhostBuildings),
	CR_MEMBER(liveGhostBuildings)
))

///////////////////////////


void GhostSolidObject::PostLoad()
{
	RECOIL_DETAILED_TRACY_ZONE;
	model = nullptr;
	GetModel();
}

const S3DModel* GhostSolidObject::GetModel() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!model)
		model = modelLoader.LoadModel(modelName);

	return model;
}

const UnitDef* CUnitDrawerData::TempDrawUnit::GetUnitDef() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!unitDef)
		unitDef = unitDefHandler->GetUnitDefByID(unitDefId);

	return unitDef;
}

///////////////////////////

CUnitDrawerData::CUnitDrawerData(bool& mtModelDrawer_)
	: CUnitDrawerDataBase("[CUnitDrawerData]", 271828, mtModelDrawer_)
{
	RECOIL_DETAILED_TRACY_ZONE;
	//LuaObjectDrawer::ReadLODScales(LUAOBJ_UNIT);

	eventHandler.AddClient(this); //cannot be done in CModelRenderDataConcept, because object is not fully constructed

	SetUnitIconDist(static_cast<float>(configHandler->GetInt("UnitIconDist")));

	iconScale      = configHandler->GetFloat("UnitIconScaleUI");
	iconFadeStart  = configHandler->GetFloat("UnitIconFadeStart");
	iconFadeVanish = configHandler->GetFloat("UnitIconFadeVanish");
	useScreenIcons = configHandler->GetBool("UnitIconsAsUI");
	iconHideWithUI = configHandler->GetBool("UnitIconsHideWithUI");

	unitDefImages.clear();
	unitDefImages.resize(unitDefHandler->NumUnitDefs() + 1);

	savedData.deadGhostBuildings.resize(teamHandler.ActiveAllyTeams());
	savedData.liveGhostBuildings.resize(teamHandler.ActiveAllyTeams());
}

CUnitDrawerData::~CUnitDrawerData()
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (CUnit* u : unsortedObjects) {
		groundDecals->ForceRemoveSolidObject(u);
	}

	for (UnitDefImage& img : unitDefImages) {
		img.Free();
	}

	for (int allyTeam = 0; allyTeam < savedData.deadGhostBuildings.size(); ++allyTeam) {
		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; modelType++) {
			auto& lgb = savedData.liveGhostBuildings[allyTeam][modelType];
			auto& dgb = savedData.deadGhostBuildings[allyTeam][modelType];

			for (auto it = dgb.begin(); it != dgb.end(); ++it) {
				GhostSolidObject* gso = *it;

				if (gso->DecRef())
					continue;

				// <ghost> might be the gbOwner of a decal; groundDecals is deleted after us
				groundDecals->GhostDestroyed(gso);
				ghostMemPool.free(gso);
			}

			dgb.clear();
			lgb.clear();
		}
	}

	unitsByIcon.clear();
}

void CUnitDrawerData::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	iconSizeBase = std::max(1.0f, std::max(globalRendering->viewSizeX, globalRendering->viewSizeY) * iconSizeMult * iconScale);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; modelType++) {
		UpdateTempDrawUnits(savedData.tempOpaqueUnits[modelType]);
		UpdateTempDrawUnits(savedData.tempAlphaUnits[modelType]);
	}

	const float3 camPos = (camHandler->GetCurrentController()).GetPos();
	const float3 camDir = (camHandler->GetCurrentController()).GetDir();
	float dist = CGround::LineGroundCol(camPos, camDir * 150000.0f, false);
	if (dist < 0)
		dist = std::max(0.0f, CGround::LinePlaneCol(camPos, camDir, 150000.0f, readMap->GetCurrAvgHeight()));

	iconZoomDist = dist;

	const auto updateBody = [this](CUnit* u) {
		UpdateDrawPos(u);

		if (useScreenIcons)
			UpdateUnitIconStateScreen(u);
		else
			UpdateUnitIconState(u);

		UpdateCommon(u);
	};

	if (mtModelDrawer) {
		for_mt_chunk(0, unsortedObjects.size(), [this, &updateBody](const int k) {
			CUnit* unit = unsortedObjects[k];
			updateBody(unit);
		}, CModelDrawerDataConcept::MT_CHUNK_OR_MIN_CHUNK_SIZE_UPDT);
	}
	else {
		for (CUnit* unit : unsortedObjects)
			updateBody(unit);
	}

	if ((useDistToGroundForIcons = (camHandler->GetCurrentController()).GetUseDistToGroundForIcons())) {
		const float3& camPos = camera->GetPos();
		// use the height at the current camera position
		//const float groundHeight = CGround::GetHeightAboveWater(camPos.x, camPos.z, false);
		// use the middle between the highest and lowest position on the map as average
		const float groundHeight = readMap->GetCurrAvgHeight();
		const float overGround = camPos.y - groundHeight;

		sqCamDistToGroundForIcons = overGround * overGround;
	}
}

void CUnitDrawerData::UpdateGhostedBuildings()
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (int allyTeam = 0; allyTeam < savedData.deadGhostBuildings.size(); ++allyTeam) {
		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; modelType++) {
			auto& dgb = savedData.deadGhostBuildings[allyTeam][modelType];

			for (int i = 0; i < dgb.size(); /*no-op*/) {
				GhostSolidObject* gso = dgb[i];

				if (!losHandler->InLos(gso->pos, allyTeam)) {
					++i;
					continue;
				}

				// obtained LOS on the ghost of a dead building
				if (!gso->DecRef()) {
					spring::VectorErase(unitsByIcon[gso->myIcon].second, const_cast<const GhostSolidObject*>(gso));
					groundDecals->GhostDestroyed(gso);
					ghostMemPool.free(gso);
				}

				dgb[i] = dgb.back();
				dgb.pop_back();
			}
		}
	}
}

const icon::CIconData* CUnitDrawerData::GetUnitIcon(const CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];
	const unsigned short prevMask = (LOS_PREVLOS | LOS_CONTRADAR);

	const UnitDef* unitDef = unit->unitDef;
	const icon::CIconData* iconData = nullptr;

	// use the unit's custom icon if we can currently see it,
	// or have seen it before and did not lose contact since
	bool unitVisible = ((losStatus & (LOS_INLOS | LOS_INRADAR)) && ((losStatus & prevMask) == prevMask));
	unitVisible |= gameSetup->ghostedBuildings && unit->unitDef->IsImmobileUnit() && (losStatus & LOS_PREVLOS);
	const bool customIcon = (unitVisible || gu->spectatingFullView);

	if (customIcon)
		return (unitDef->iconType.GetIconData());

	if ((losStatus & LOS_INRADAR) != 0)
		iconData = icon::iconHandler.GetDefaultIconData();

	return iconData;
}

void CUnitDrawerData::UpdateUnitDefMiniMapIcons(const UnitDef* ud)
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (int teamNum = 0; teamNum < teamHandler.ActiveTeams(); teamNum++) {
		for (const CUnit* unit : unitHandler.GetUnitsByTeamAndDef(teamNum, ud->id)) {
			UpdateUnitIcon(unit, true, false);
		}
	}
}

void CUnitDrawerData::UpdateUnitIcon(const CUnit* unit, bool forced, bool killed)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CUnit* u = const_cast<CUnit*>(unit);

	icon::CIconData* oldIcon = unit->myIcon;
	icon::CIconData* newIcon = const_cast<icon::CIconData*>(GetUnitIcon(unit));

	u->myIcon = nullptr;

	if (!killed) {
		if ((oldIcon != newIcon) || forced) {
			spring::VectorErase(unitsByIcon[oldIcon].first, unit);
			unitsByIcon[newIcon].first.push_back(unit);
		}

		u->myIcon = newIcon;
		return;
	}

	spring::VectorErase(unitsByIcon[oldIcon].first, unit);
}

void CUnitDrawerData::UpdateUnitIconState(CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];

	unit->SetIsIcon((losStatus & LOS_INRADAR) != 0);

	//further refinement if visible
	if ((losStatus & LOS_INLOS) != 0 || gu->spectatingFullView) {
		bool asIcon = true;

		asIcon &= (!unit->noDraw);
		asIcon &= (!unit->IsInVoid());

		asIcon &= DrawAsIconByDistance(unit, (unit->pos - camera->GetPos()).SqLength());
		// drawing icons is cheap but not free, avoid a perf-hit when many are offscreen
		asIcon &= (camera->InView(unit->drawMidPos, unit->GetDrawRadius()));
		unit->SetIsIcon(asIcon);
	}
}

void CUnitDrawerData::UpdateUnitIconStateScreen(CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (game->hideInterface && iconHideWithUI) // icons are hidden with UI
	{
		unit->SetIsIcon(false); // draw unit model always
		return;
	}

	if (unit->health <= 0 || unit->beingBuilt || unit->noDraw || unit->IsInVoid())
	{
		unit->SetIsIcon(false);
		return;
	}

	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];
	bool useDefaultIcon = (unit->myIcon == icon::iconHandler.GetDefaultIconData());

	const icon::CIconData* iconData = useDefaultIcon ? icon::iconHandler.GetDefaultIconData() : unit->unitDef->iconType.GetIconData();

	float iconSizeMult = iconData->GetSize();
	if (iconData->GetRadiusAdjust() && !useDefaultIcon)
		iconSizeMult *= (unit->radius / iconData->GetRadiusScale());
	iconSizeMult = (iconSizeMult - 1) * 0.75 + 1;

	float limit = iconSizeBase / 2 * iconSizeMult;

	// calculate unit's radius in screen space and compare with the size of the icon
	float3 pos = unit->pos;
	float3 radiusPos = pos + camera->right * unit->radius;

	pos = camera->CalcViewPortCoordinates(pos);
	radiusPos = camera->CalcViewPortCoordinates(radiusPos);

	unit->iconRadius = unit->radius * ((limit * 0.9) / std::abs(pos.x - radiusPos.x)); // used for clicking on iconified units (world space!!!)

	if (!(losStatus & LOS_INLOS) && !gu->spectatingFullView) // no LOS on unit
	{
		unit->SetIsIcon(losStatus & LOS_INRADAR); // draw icon if unit is on radar
		return;
	}

	// don't render unit's model if it is smaller than icon by 10% in screen space
	// render it anyway in case icon isn't completely opaque (below FadeStart distance)
	unit->SetIsIcon(iconZoomDist / iconSizeMult > iconFadeStart && std::abs(pos.x - radiusPos.x) < limit * 0.9);
}

void CUnitDrawerData::UpdateDrawPos(CUnit* u)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const CUnit* t = u->GetTransporter();

	if (t != nullptr) {
		u->drawPos = u->preFramePos + t->GetDrawDeltaPos(globalRendering->timeOffset);
	}
	else {
		u->drawPos = u->preFramePos + u->GetDrawDeltaPos(globalRendering->timeOffset);
	}

	u->drawMidPos = u->GetMdlDrawMidPos();
}

void CUnitDrawerData::UpdateObjectDrawFlags(CSolidObject* o) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	CUnit* u = static_cast<CUnit*>(o);

	{
		//icons flag is set before UpdateObjectDrawFlags() is called
		const bool isIcon = u->HasDrawFlag(DrawFlags::SO_DRICON_FLAG);
		u->ResetDrawFlag();
		u->SetIsIcon(isIcon);
	}

	for (uint32_t camType = CCamera::CAMTYPE_PLAYER; camType < CCamera::CAMTYPE_ENVMAP; ++camType) {
		if (camType == CCamera::CAMTYPE_UWREFL && !IWater::GetWater()->CanDrawReflectionPass())
			continue;

		if (camType == CCamera::CAMTYPE_SHADOW && ((shadowHandler.shadowGenBits & CShadowHandler::SHADOWGEN_BIT_MODEL) == 0))
			continue;

		const CCamera* cam = CCameraHandler::GetCamera(camType);

		if (u->noDraw)
			continue;

		// unit will be drawn as icon instead
		if (u->GetIsIcon())
			continue;

		if (u->IsInVoid())
			continue;

		if (!(u->losStatus[gu->myAllyTeam] & LOS_INLOS) && !gu->spectatingFullView)
			continue;

		if (!cam->InView(u->drawMidPos, u->GetDrawRadius()))
			continue;

		switch (camType)
		{
			case CCamera::CAMTYPE_PLAYER: {
				const float sqrCamDist = (u->drawPos - cam->GetPos()).SqLength();

				if (!IsAlpha(u)) {
					u->SetDrawFlag(DrawFlags::SO_OPAQUE_FLAG);
				}
				else {
					u->SetDrawFlag(DrawFlags::SO_ALPHAF_FLAG);
				}

				if (u->IsInWater())
					u->AddDrawFlag(DrawFlags::SO_REFRAC_FLAG);
			} break;

			case CCamera::CAMTYPE_UWREFL: {
				if (CModelDrawerHelper::ObjectVisibleReflection(u->drawMidPos, cam->GetPos(), u->GetDrawRadius()))
					u->AddDrawFlag(DrawFlags::SO_REFLEC_FLAG);
			} break;

			case CCamera::CAMTYPE_SHADOW: {
				if unlikely(IsAlpha(u))
					u->AddDrawFlag(DrawFlags::SO_SHTRAN_FLAG);
				else
					u->AddDrawFlag(DrawFlags::SO_SHOPAQ_FLAG);
			} break;

			default: { assert(false); } break;

		}

	}
}

bool CUnitDrawerData::DrawAsIconByDistance(const CUnit* unit, const float sqUnitCamDist) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float sqIconDistMult = unit->unitDef->iconType->GetDistanceSqr();
	const float realIconLength = iconLength * sqIconDistMult;

	if (useDistToGroundForIcons)
		return (sqCamDistToGroundForIcons > realIconLength);
	else
		return (sqUnitCamDist > realIconLength);
}

static inline bool LoadBuildPic(const std::string& filename, CBitmap& bitmap)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (CFileHandler::FileExists(filename, SPRING_VFS_RAW_FIRST)) {
		bitmap.Load(filename);
		return true;
	}

	return false;
}

void CUnitDrawerData::SetUnitDefImage(const UnitDef* unitDef, const std::string& texName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	UnitDefImage*& unitImage = unitDef->buildPic;

	if (unitImage == nullptr) {
		unitImage = &unitDefImages[unitDef->id];
	}
	else {
		unitImage->Free();
	}

	CBitmap bitmap;

	if (!texName.empty()) {
		bitmap.Load("unitpics/" + texName);
	}
	else {
		if (!LoadBuildPic("unitpics/" + unitDef->name + ".dds", bitmap) &&
			!LoadBuildPic("unitpics/" + unitDef->name + ".png", bitmap) &&
			!LoadBuildPic("unitpics/" + unitDef->name + ".pcx", bitmap) &&
			!LoadBuildPic("unitpics/" + unitDef->name + ".bmp", bitmap)) {
			bitmap.AllocDummy(SColor(255, 0, 0, 255));
		}
	}

	unitImage->textureID = bitmap.CreateTexture(TextureCreationParams{
		.aniso = 0.0f,
		.lodBias = 0.0f,
		.texID = 0,
		.reqNumLevels = 1,
		.linearMipMapFilter = false,
		.linearTextureFilter = true
	});

	unitImage->imageSizeX = bitmap.xsize;
	unitImage->imageSizeY = bitmap.ysize;
}

void CUnitDrawerData::SetUnitDefImage(const UnitDef* unitDef, unsigned int texID, int xsize, int ysize)
{
	RECOIL_DETAILED_TRACY_ZONE;
	UnitDefImage*& unitImage = unitDef->buildPic;

	if (unitImage == nullptr) {
		unitImage = &unitDefImages[unitDef->id];
	}
	else {
		unitImage->Free();
	}

	unitImage->textureID = texID;
	unitImage->imageSizeX = xsize;
	unitImage->imageSizeY = ysize;
}

uint32_t CUnitDrawerData::GetUnitDefImage(const UnitDef* unitDef)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (unitDef->buildPic == nullptr)
		SetUnitDefImage(unitDef, unitDef->buildPicName);

	return (unitDef->buildPic->textureID);
}

void CUnitDrawerData::AddTempDrawUnit(const TempDrawUnit& tdu)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const UnitDef* unitDef = tdu.GetUnitDef();
	const S3DModel* model = unitDef->LoadModel();

	if (tdu.drawAlpha) {
		savedData.tempAlphaUnits[model->type].push_back(tdu);
	}
	else {
		savedData.tempOpaqueUnits[model->type].push_back(tdu);
	}
}

void CUnitDrawerData::UpdateTempDrawUnits(std::vector<TempDrawUnit>& tempDrawUnits)
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (unsigned int n = 0; n < tempDrawUnits.size(); /*no-op*/) {
		if (tempDrawUnits[n].timeout <= gs->frameNum) {
			// do not use spring::VectorErase; we already know the index
			tempDrawUnits[n] = tempDrawUnits.back();
			tempDrawUnits.pop_back();
			continue;
		}

		++n;
	}
}

void CUnitDrawerData::RenderUnitPreCreated(const CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	UpdateObject(unit, true);
}

void CUnitDrawerData::RenderUnitCreated(const CUnit* unit, int cloaked)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(std::find(unsortedObjects.begin(), unsortedObjects.end(), unit) != unsortedObjects.end());
	UpdateUnitIcon(unit, false, false);
}

void CUnitDrawerData::RenderUnitDestroyed(const CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CUnit* u = const_cast<CUnit*>(unit);

	const UnitDef* unitDef = unit->unitDef;
	const UnitDef* decoyDef = unitDef->decoyDef;

	const bool addNewGhost = unitDef->IsImmobileUnit() && gameSetup->ghostedBuildings;

	// TODO - make ghosted buildings per allyTeam - so they are correctly dealt with
	// when spectating
	GhostSolidObject* gso = nullptr;
	// FIXME -- adjust decals for decoys? gets weird?
	S3DModel* gsoModel = (decoyDef == nullptr) ? u->model : decoyDef->LoadModel();

	for (int allyTeam = 0; allyTeam < savedData.deadGhostBuildings.size(); ++allyTeam) {
		const bool canSeeGhost = !(u->losStatus[allyTeam] & (LOS_INLOS | LOS_CONTRADAR)) && (u->losStatus[allyTeam] & (LOS_PREVLOS));

		if (addNewGhost && canSeeGhost) {
			if (gso == nullptr) {
				gso = ghostMemPool.alloc<GhostSolidObject>();

				gso->pos = u->pos;
				gso->midPos = u->midPos;
				gso->modelName = gsoModel->name;
				gso->facing = u->buildFacing;
				gso->dir = u->frontdir;
				gso->team = u->team;
				gso->radius = u->radius;
				gso->refCount = 0;
				gso->GetModel();

				gso->myIcon = u->myIcon;
				gso->iconRadius = u->iconRadius;

				groundDecals->GhostCreated(u, gso);

			}

			// <gso> can be inserted for multiple allyteams
			// (the ref-counter saves us come deletion time)
			savedData.deadGhostBuildings[allyTeam][gsoModel->type].push_back(gso);
			gso->IncRef();

			if (allyTeam == gu->myAllyTeam) {
				unitsByIcon[u->myIcon].second.push_back(gso);
			}
		}

		spring::VectorErase(savedData.liveGhostBuildings[allyTeam][MDL_TYPE(u)], u);
	}

	DelObject(unit, true);
	UpdateUnitIcon(unit, false, true);

	LuaObjectDrawer::SetObjectLOD(u, LUAOBJ_UNIT, 0);
}

void CUnitDrawerData::UnitEnteredRadar(const CUnit* unit, int allyTeam)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (allyTeam != gu->myAllyTeam)
		return;

	UpdateUnitIcon(unit, false, false);
}

void CUnitDrawerData::UnitEnteredLos(const CUnit* unit, int allyTeam)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CUnit* u = const_cast<CUnit*>(unit); //cleanup

	if (gameSetup->ghostedBuildings && unit->unitDef->IsImmobileUnit())
		spring::VectorErase(savedData.liveGhostBuildings[allyTeam][MDL_TYPE(unit)], u);

	if (allyTeam != gu->myAllyTeam)
		return;

	UpdateUnitIcon(unit, false, false);
}

void CUnitDrawerData::UnitLeftLos(const CUnit* unit, int allyTeam)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CUnit* u = const_cast<CUnit*>(unit); //cleanup

	if (gameSetup->ghostedBuildings && unit->unitDef->IsImmobileUnit())
		spring::VectorInsertUnique(savedData.liveGhostBuildings[allyTeam][MDL_TYPE(unit)], u, true);

	if (allyTeam != gu->myAllyTeam)
		return;

	UpdateUnitIcon(unit, false, false);
}

void CUnitDrawerData::PlayerChanged(int playerNum)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (playerNum != gu->myPlayerNum)
		return;

	for (auto& [icon, data] : unitsByIcon) {
		data.first.clear();
		data.second.clear();
	}

	for (CUnit* unit : unsortedObjects) {
		// force an erase (no-op) followed by an insert
		UpdateUnitIcon(unit, true, false);
	}

	for (auto& ghosts : savedData.deadGhostBuildings[gu->myAllyTeam]) {
		for (auto ghost : ghosts) {
			unitsByIcon[ghost->myIcon].second.push_back(ghost);
		}
	}

}
