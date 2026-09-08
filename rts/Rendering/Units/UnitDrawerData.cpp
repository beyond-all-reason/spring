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
#include "System/Matrix44f.h"
#include "System/MathConstants.h"
#include "System/Transform.hpp"
#include "Rendering/Models/ModelsMemStorage.h"

static FixedDynMemPoolT<MAX_UNITS / 1000, MAX_UNITS / 32, GhostSolidObject> ghostMemPool;

// world transform of a (static) ghost building, matching the legacy staticModelMatrix construction
static Transform MakeGhostWorldTransform(const float3& pos, int facing)
{
	CMatrix44f m;
	m.Translate(pos);
	m.RotateY(-facing * math::HALFPI);
	return Transform::FromMatrix(m);
}

///////////////////////////

CR_BIND_POOL(GhostSolidObject, ,ghostMemPool.allocMem, ghostMemPool.freeMem)
CR_REG_METADATA(GhostSolidObject, (
	CR_MEMBER(modelName),

	CR_MEMBER(pos),
	CR_MEMBER(midPos),
	CR_MEMBER(dir),
	CR_MEMBER(radius),
	CR_MEMBER(iconRadius),

	CR_MEMBER(refCount),

	CR_MEMBER(facing),
	CR_MEMBER(team),
	CR_MEMBER(paletteIndex),
	CR_IGNORED(currentIconIndex),

	CR_IGNORED(worldTransformAlloc),
	CR_IGNORED(model),

	CR_POSTLOAD(PostLoad)
))

CR_BIND(CUnitDrawerData::LiveGhostBuilding, )
CR_REG_METADATA(CUnitDrawerData::LiveGhostBuilding, (
	CR_MEMBER(unit),
	CR_MEMBER(paletteIndex),
	CR_MEMBER(team)
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

	// the GPU world transform slot is render-only state; re-create it from the saved pos/facing
	InitWorldTransform();
}

void GhostSolidObject::InitWorldTransform()
{
	worldTransformAlloc = ScopedTransformMemAlloc(1);
	worldTransformAlloc.UpdateForced(0, MakeGhostWorldTransform(pos, facing));
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
	sortUnitIconsByDepth = configHandler->GetBool("UnitIconsSortedByDepth");
	ghostIconDimming = configHandler->GetFloat("UnitGhostIconsDimming");

	configHandler->NotifyOnChange(this, {"UnitGhostIconsDimming", "UnitIconsSortedByDepth"});

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

			for (auto& gso : dgb) {
				auto* tmpGso = std::exchange(gso, nullptr);
				if (tmpGso->DecRef())
					continue;

				// worldTransformAlloc frees its slot in ~GhostSolidObject (ghostMemPool.free below)
				// <ghost> might be the gbOwner of a decal; groundDecals is deleted after us
				groundDecals->GhostDestroyed(tmpGso);
				ghostMemPool.free(tmpGso);
			}
			dgb.clear();
			lgb.clear();
		}
	}
	liveGhostTransforms.clear(); // each entry's ScopedTransformMemAlloc frees its slot on erase
	assert(ghostMemPool.allocs() == 0);
	ghostMemPool.clear();

	configHandler->RemoveObserver(this);
}

void CUnitDrawerData::ConfigNotify(const std::string& key, const std::string& value)
{
	ghostIconDimming = configHandler->GetFloat("UnitGhostIconsDimming");
	sortUnitIconsByDepth = configHandler->GetBool("UnitIconsSortedByDepth");
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

	UpdateLiveGhostTransforms();

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
				RemoveDeadGhost(gso, dgb, i); // swaps element with last so counter shouldn't be increased.
			}
		}
	}
}

void CUnitDrawerData::UpdateUnitIconsByUnitDef(const UnitDef* ud)
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (int teamNum = 0; teamNum < teamHandler.ActiveTeams(); teamNum++) {
		for (auto* unit : unitHandler.GetUnitsByTeamAndDef(teamNum, ud->id)) {
			UpdateCurrentUnitIcon(unit);
		}
	}
}

size_t CUnitDrawerData::GetUnitIconIndex(const CUnit* unit, int allyTeam, bool fullView) const
{
	const unsigned short losStatus = unit->losStatus[allyTeam];
	constexpr unsigned short prevMask = (LOS_PREVLOS | LOS_CONTRADAR);

	// use the unit's custom icon if the viewer can currently see it,
	// or has seen it before and did not lose contact since
	const bool isInLOS    = (losStatus & LOS_INLOS  ) == LOS_INLOS;
	const bool isInPrvLos = (losStatus & LOS_PREVLOS) == LOS_PREVLOS;
	const bool isInRadar  = (losStatus & LOS_INRADAR) == LOS_INRADAR;
	const bool seenUnit   = (losStatus & prevMask   ) == prevMask;
	const bool isGhostBuilding = gameSetup->ghostedBuildings && unit->leavesGhost && isInPrvLos;

	const bool unitVisible = isInLOS || (isInRadar && seenUnit) || isGhostBuilding;

	if (unitVisible || fullView)
		return (unit->customIconIndex != icon::INVALID_ICON_INDEX) ? unit->customIconIndex : icon::iconHandler.GetIconIdxOrDefault(unit->definedIconName);

	if ((losStatus & LOS_INRADAR) != 0)
		return icon::iconHandler.GetDefaultIconIdx();

	return icon::INVALID_ICON_INDEX;
}

void CUnitDrawerData::UpdateCurrentUnitIcon(const CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	unit->currentIconIndex = GetUnitIconIndex(unit, gu->myAllyTeam, gu->spectatingFullView);
}

void CUnitDrawerData::UpdateUnitIconState(CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;

	if unlikely(unit->currentIconIndex == icon::INVALID_ICON_INDEX) {
		unit->SetIsIcon(false);
		return;
	}

	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];

	unit->SetIsIcon((losStatus & LOS_INRADAR) != 0);

	//further refinement if visible
	if ((losStatus & LOS_INLOS) != 0 || gu->spectatingFullView) {
		bool asIcon = true;

		asIcon = asIcon && !unit->noDraw;
		asIcon = asIcon && !unit->IsInVoid();

		asIcon = asIcon && DrawAsIconByDistance(unit, (unit->pos - camera->GetPos()).SqLength());
		// drawing icons is cheap but not free, avoid a perf-hit when many are offscreen
		asIcon = asIcon && camera->InView(unit->drawMidPos, unit->GetDrawRadius());

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

	if (unit->currentIconIndex == icon::INVALID_ICON_INDEX || unit->health <= 0 || unit->beingBuilt || unit->noDraw || unit->IsInVoid())
	{
		unit->SetIsIcon(false);
		return;
	}

	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];

	const auto& iconData = icon::iconHandler.GetIconData(unit->currentIconIndex);

	float iconSizeMult = iconData.GetSize();
	if (iconData.GetRadiusAdjust())
		iconSizeMult *= (unit->radius / iconData.GetRadiusScale());
	iconSizeMult = iconSizeMult * 0.75f + 0.25f;

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

	if (const CUnit* t = u->GetTransporter(); t != nullptr) {
		u->drawPos = u->GetDrawPosOther(t->preFrameTra.t, t->pos, globalRendering->timeOffset);
	}
	else {
		u->drawPos = u->GetDrawPos(globalRendering->timeOffset);
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
	const auto& sqIconDistMult = icon::iconHandler.GetIconData(unit->currentIconIndex).GetDistanceSq();
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

	unitImage->textureID = bitmap.CreateTexture(GL::TextureCreationParams{
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
	UpdateCurrentUnitIcon(unit);
}

S3DModel* CUnitDrawerData::GetUnitModel(const CUnit* unit) const
{
	const UnitDef* unitDef = unit->unitDef;
	const UnitDef* decoyDef = unitDef->decoyDef;

	// FIXME -- adjust decals for decoys? gets weird?
	S3DModel* gsoModel = (decoyDef == nullptr) ? unit->model : decoyDef->LoadModel();
	return gsoModel;
}

bool CUnitDrawerData::UpdateUnitGhosts(const CUnit* unit, const bool addNewGhost)
{
	if (!gameSetup->ghostedBuildings)
		return false;

	bool addedOwnAllyTeam = false;
	CUnit* u = const_cast<CUnit*>(unit);

	// TODO - make ghosted buildings per allyTeam - so they are correctly dealt with
	// when spectating
	GhostSolidObject* gso = nullptr;
	S3DModel* gsoModel = GetUnitModel(unit);

	for (int allyTeam = 0; allyTeam < savedData.deadGhostBuildings.size(); ++allyTeam) {
		const bool canSeeGhost = !(u->losStatus[allyTeam] & (LOS_INLOS | LOS_CONTRADAR | LOS_INRADAR)) && (u->losStatus[allyTeam] & (LOS_PREVLOS));

		if (addNewGhost && canSeeGhost) {
			if (gso == nullptr) {
				gso = ghostMemPool.alloc<GhostSolidObject>();

				gso->pos = u->pos;
				gso->midPos = u->midPos;
				gso->modelName = gsoModel->name;
				gso->refCount = 0;
				gso->facing = u->buildFacing;
				gso->dir = u->frontdir;
				gso->team = u->team;
				gso->paletteIndex = u->paletteIndex;
				gso->radius = u->radius;
				gso->GetModel();

				// gso is a shared object, we can't rely on the u->currentIconIndex being representative in case the team changes
				gso->currentIconIndex = icon::iconHandler.GetIconIdxOrDefault(unit->definedIconName);

				gso->iconRadius = u->iconRadius;

				gso->InitWorldTransform();

				groundDecals->GhostCreated(u, gso);

			}

			// <gso> can be inserted for multiple allyteams
			// (the ref-counter saves us come deletion time)
			savedData.deadGhostBuildings[allyTeam][gsoModel->type].push_back(gso);
			gso->IncRef();

			u->losStatus[allyTeam] &= ~LOS_PREVLOS;
			if (allyTeam == gu->myAllyTeam)
				addedOwnAllyTeam = true;

		}

		spring::VectorEraseIf(savedData.liveGhostBuildings[allyTeam][MDL_TYPE(u)],
			[u](const LiveGhostBuilding& lgb) { return lgb.unit == u; });
	}
	return addedOwnAllyTeam;
}

void CUnitDrawerData::RenderUnitDestroyed(const CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CUnit* u = const_cast<CUnit*>(unit);

	UpdateUnitGhosts(unit, unit->leavesGhost);
	// must happen after UpdateUnitGhosts()
	u->currentIconIndex = icon::INVALID_ICON_INDEX;

	DelObject(unit, true);	

	LuaObjectDrawer::SetObjectLOD(u, LUAOBJ_UNIT, 0);
}

void CUnitDrawerData::UnitEnteredRadar(const CUnit* unit, int allyTeam)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (allyTeam != gu->myAllyTeam)
		return;

	UpdateCurrentUnitIcon(unit);
}

void CUnitDrawerData::UnitEnteredLos(const CUnit* unit, int allyTeam)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CUnit* u = const_cast<CUnit*>(unit); //cleanup

	if (unit->leavesGhost)
		spring::VectorEraseIf(savedData.liveGhostBuildings[allyTeam][MDL_TYPE(unit)],
			[u](const LiveGhostBuilding& lgb) { return lgb.unit == u; });

	if (allyTeam != gu->myAllyTeam)
		return;

	UpdateCurrentUnitIcon(unit);
}

void CUnitDrawerData::UnitLeftLos(const CUnit* unit, int allyTeam)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CUnit* u = const_cast<CUnit*>(unit); //cleanup

	if (unit->leavesGhost) {
		// snapshot the color the unit is last seen under so a later team change (while out of LOS)
		// does not recolor its ghost. keep the earliest snapshot if it re-fires without re-entering.
		auto& lgbs = savedData.liveGhostBuildings[allyTeam][MDL_TYPE(unit)];
		const bool alreadyGhosted = std::any_of(lgbs.begin(), lgbs.end(),
			[u](const LiveGhostBuilding& lgb) { return lgb.unit == u; });
		if (!alreadyGhosted)
			lgbs.push_back({ u, u->paletteIndex, static_cast<uint8_t>(u->team) });
	}

	if (allyTeam != gu->myAllyTeam)
		return;

	UpdateCurrentUnitIcon(unit);
}

void CUnitDrawerData::UpdateLiveGhostTransforms()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// Maintain one world-transform slot per live ghost building drawn for the local allyTeam.
	// Ghosts are static, so each slot is filled once on first sight; entries not seen this sweep
	// (units that regained LOS, died, or belong to a different allyTeam now) are freed.
	const int stamp = ++liveGhostSweepStamp;

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; modelType++) {
		for (const auto& lgb : savedData.liveGhostBuildings[gu->myAllyTeam][modelType]) {
			const CUnit* u = lgb.unit;
			const auto it = liveGhostTransforms.find(u);
			if (it == liveGhostTransforms.end()) {
				ScopedTransformMemAlloc alloc(1);
				alloc.UpdateForced(0, MakeGhostWorldTransform(u->pos, u->buildFacing));
				liveGhostTransforms.emplace(u, std::make_pair(std::move(alloc), stamp));
			}
			else {
				it->second.second = stamp;
			}
		}
	}

	for (auto it = liveGhostTransforms.begin(); it != liveGhostTransforms.end(); ) {
		if (it->second.second != stamp)
			it = liveGhostTransforms.erase(it); // ScopedTransformMemAlloc frees the slot on erase
		else
			++it;
	}
}

void CUnitDrawerData::UnitLeavesGhostChanged(const CUnit* unit, const bool leaveDeadGhost)
{
	if (unit->leavesGhost) {
		ReviewPrevLos(unit);
		return;
	}

	if (UpdateUnitGhosts(unit, leaveDeadGhost)) {
		// left decoy dead ghost for own team
		UpdateCurrentUnitIcon(unit);
	}
}

void CUnitDrawerData::ReviewPrevLos(const CUnit* unit)
{
	// When reinstating leavesGhost, we need to check whether the unit is still in los or
	// contradar, and otherwise disable PREVLOS, otherwise specs will see it after going in and
	// out of player mode.
	for (int allyTeam = 0; allyTeam < savedData.liveGhostBuildings.size(); ++allyTeam) {
		if (!(unit->losStatus[allyTeam] & (LOS_INLOS | LOS_CONTRADAR))) {
			CUnit* u = const_cast<CUnit*>(unit);
			u->losStatus[allyTeam] &= ~LOS_PREVLOS;
		}
	}
}

void CUnitDrawerData::PlayerChanged(int playerID)
{
	for (auto* unit : unsortedObjects) {
		UpdateCurrentUnitIcon(unit);
	}
}

void CUnitDrawerData::RemoveDeadGhost(GhostSolidObject* gso, std::vector<GhostSolidObject*>& dgb, int index)
{
	if (!gso->DecRef()) {
		// worldTransformAlloc frees its slot in ~GhostSolidObject (ghostMemPool.free)
		groundDecals->GhostDestroyed(gso);
		ghostMemPool.free(gso);
	}

	dgb[index] = dgb.back();
	dgb.pop_back();
}