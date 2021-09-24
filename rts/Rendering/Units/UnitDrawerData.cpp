#include "UnitDrawerData.h"
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
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/IconHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"

static FixedDynMemPool<sizeof(GhostSolidObject), MAX_UNITS / 1000, MAX_UNITS / 32> ghostMemPool;

///////////////////////////

CUnitDrawerData::CUnitDrawerData()
	: CUnitRenderDataBase("[CUnitDrawerData]", 271828)
{
	//LuaObjectDrawer::ReadLODScales(LUAOBJ_UNIT);

	SetUnitDrawDist(static_cast<float>(configHandler->GetInt("UnitLodDist")));
	SetUnitIconDist(static_cast<float>(configHandler->GetInt("UnitIconDist")));

	iconScale      = configHandler->GetFloat("UnitIconScaleUI");
	iconFadeStart  = configHandler->GetFloat("UnitIconFadeStart");
	iconFadeVanish = configHandler->GetFloat("UnitIconFadeVanish");
	useScreenIcons = configHandler->GetBool("UnitIconsAsUI");
	iconHideWithUI = configHandler->GetBool("UnitIconsHideWithUI");

	unitDefImages.clear();
	unitDefImages.resize(unitDefHandler->NumUnitDefs() + 1);

	deadGhostBuildings.resize(teamHandler.ActiveAllyTeams());
	liveGhostBuildings.resize(teamHandler.ActiveAllyTeams());
}

CUnitDrawerData::~CUnitDrawerData()
{
	for (CUnit* u : unsortedObjects) {
		groundDecals->ForceRemoveSolidObject(u);
	}

	for (UnitDefImage& img : unitDefImages) {
		img.Free();
	}

	for (int allyTeam = 0; allyTeam < deadGhostBuildings.size(); ++allyTeam) {
		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; modelType++) {
			auto& lgb = liveGhostBuildings[allyTeam][modelType];
			auto& dgb = deadGhostBuildings[allyTeam][modelType];

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
	iconSizeBase = std::max(12.0f, std::max(globalRendering->viewSizeX, globalRendering->viewSizeY) * iconSizeMult * iconScale);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; modelType++) {
		UpdateTempDrawUnits(tempOpaqueUnits[modelType]);
		UpdateTempDrawUnits(tempAlphaUnits[modelType]);
	}

	iconUnits.clear();

	const float3 camPos = (camHandler->GetCurrentController()).GetPos();
	const float3 camDir = (camHandler->GetCurrentController()).GetDir();
	float dist = CGround::LineGroundCol(camPos, camDir * 150000.0f, false);
	if (dist < 0)
		dist = std::max(0.0f, CGround::LinePlaneCol(camPos, camDir, 150000.0f, readMap->GetCurrAvgHeight()));
	iconZoomDist = dist;

	for (CUnit* unit : unsortedObjects) {
		if (useScreenIcons)
			UpdateUnitIconStateScreen(unit);
		else
			UpdateUnitIconState(unit);

		UpdateUnitDrawPos(unit);
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
	for (int allyTeam = 0; allyTeam < deadGhostBuildings.size(); ++allyTeam) {
		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; modelType++) {
			auto& dgb = deadGhostBuildings[allyTeam][modelType];

			for (int i = 0; i < dgb.size(); /*no-op*/) {
				GhostSolidObject* gso = dgb[i];

				if (!losHandler->InLos(gso->pos, allyTeam)) {
					++i;
					continue;
				}

				// obtained LOS on the ghost of a dead building
				if (!gso->DecRef()) {
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
	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];
	const unsigned short prevMask = (LOS_PREVLOS | LOS_CONTRADAR);

	const UnitDef* unitDef = unit->unitDef;
	const icon::CIconData* iconData = nullptr;

	// use the unit's custom icon if we can currently see it,
	// or have seen it before and did not lose contact since
	bool unitVisible = ((losStatus & (LOS_INLOS | LOS_INRADAR)) && ((losStatus & prevMask) == prevMask));
	unitVisible |= gameSetup->ghostedBuildings && unit->unitDef->IsBuildingUnit() && (losStatus & LOS_PREVLOS);
	const bool customIcon = (minimap->UseUnitIcons() && (unitVisible || gu->spectatingFullView));

	if (customIcon)
		return (unitDef->iconType.GetIconData());

	if ((losStatus & LOS_INRADAR) != 0)
		iconData = icon::iconHandler.GetDefaultIconData();

	return iconData;
}

void CUnitDrawerData::UpdateUnitDefMiniMapIcons(const UnitDef* ud)
{
	for (int teamNum = 0; teamNum < teamHandler.ActiveTeams(); teamNum++) {
		for (const CUnit* unit : unitHandler.GetUnitsByTeamAndDef(teamNum, ud->id)) {
			UpdateUnitMiniMapIcon(unit, true, false);
		}
	}
}

void CUnitDrawerData::UpdateUnitMiniMapIcon(const CUnit* unit, bool forced, bool killed)
{
	CUnit* u = const_cast<CUnit*>(unit);

	icon::CIconData* oldIcon = unit->myIcon;
	icon::CIconData* newIcon = const_cast<icon::CIconData*>(GetUnitIcon(unit));

	u->myIcon = nullptr;

	if (!killed) {
		if ((oldIcon != newIcon) || forced) {
			spring::VectorErase(unitsByIcon[oldIcon], unit);
			unitsByIcon[newIcon].push_back(unit);
		}

		u->myIcon = newIcon;
		return;
	}

	spring::VectorErase(unitsByIcon[oldIcon], unit);
}

void CUnitDrawerData::UpdateUnitIconState(CUnit* unit)
{
	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];

	// reset
	unit->isIcon = losStatus & LOS_INRADAR;

	if ((losStatus & LOS_INLOS) || gu->spectatingFullView)
		unit->isIcon = DrawAsIcon(unit, (unit->pos - camera->GetPos()).SqLength());

	if (!unit->isIcon)
		return;
	if (unit->noDraw)
		return;
	if (unit->IsInVoid())
		return;
	// drawing icons is cheap but not free, avoid a perf-hit when many are offscreen
	if (!camera->InView(unit->drawMidPos, unit->GetDrawRadius()))
		return;

	iconUnits.push_back(unit);
}

void CUnitDrawerData::UpdateUnitIconStateScreen(CUnit* unit)
{
	if (game->hideInterface && iconHideWithUI) // icons are hidden with UI
	{
		unit->isIcon = false; // draw unit model always
		return;
	}

	if (unit->health <= 0 || unit->beingBuilt)
	{
		unit->isIcon = false;
		return;
	}

	// If the icon is to be drawn as a radar blip, we want to get the default icon.
	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];
	const unsigned short plosBits = (losStatus & (LOS_PREVLOS | LOS_CONTRADAR));
	bool useDefaultIcon = !gu->spectatingFullView && !(losStatus & (LOS_INLOS)) && plosBits != (LOS_PREVLOS | LOS_CONTRADAR);

	const icon::CIconData* iconData = useDefaultIcon ? icon::iconHandler.GetDefaultIconData() : unit->unitDef->iconType.GetIconData();

	float iconSizeMult = iconData->GetSize();
	if (iconData->GetRadiusAdjust() && !useDefaultIcon)
		iconSizeMult *= (unit->radius / iconData->GetRadiusScale());
	iconSizeMult = (iconSizeMult - 1) * 0.75 + 1;

	float limit = iconSizeBase / 2 * iconSizeMult;

	// calculate unit's radius in screen space and compare with the size of the icon
	float3 pos = unit->pos;
	float3 radiusPos = pos + camera->right * unit->radius;

	pos = camera->CalcWindowCoordinates(pos);
	radiusPos = camera->CalcWindowCoordinates(radiusPos);

	unit->iconRadius = unit->radius * ((limit * 0.9) / std::abs(pos.x - radiusPos.x)); // used for clicking on iconified units (world space!!!)

	if (!(losStatus & LOS_INLOS) && !gu->spectatingFullView) // no LOS on unit
	{
		unit->isIcon = losStatus & LOS_INRADAR; // draw icon if unit is on radar
		return;
	}

	// don't render unit's model if it is smaller than icon by 10% in screen space
	// render it anyway in case icon isn't completely opaque (below FadeStart distance)
	unit->isIcon = iconZoomDist / iconSizeMult > iconFadeStart && std::abs(pos.x - radiusPos.x) < limit * 0.9;
}

void CUnitDrawerData::UpdateUnitDrawPos(CUnit* u)
{
	const CUnit* t = u->GetTransporter();

	if (t != nullptr) {
		u->drawPos = u->preFramePos + t->GetDrawDeltaPos(globalRendering->timeOffset);
	}
	else {
		u->drawPos = u->preFramePos + u->GetDrawDeltaPos(globalRendering->timeOffset);
	}

	u->drawMidPos = u->GetMdlDrawMidPos();
}

bool CUnitDrawerData::DrawAsIcon(const CUnit* unit, const float sqUnitCamDist) const
{
	const float sqIconDistMult = unit->unitDef->iconType->GetDistanceSqr();
	const float realIconLength = iconLength * sqIconDistMult;

	if (useDistToGroundForIcons)
		return (sqCamDistToGroundForIcons > realIconLength);
	else
		return (sqUnitCamDist > realIconLength);
}

static inline bool LoadBuildPic(const std::string& filename, CBitmap& bitmap)
{
	if (CFileHandler::FileExists(filename, SPRING_VFS_RAW_FIRST)) {
		bitmap.Load(filename);
		return true;
	}

	return false;
}

void CUnitDrawerData::SetUnitDefImage(const UnitDef* unitDef, const std::string& texName)
{
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

	unitImage->textureID = bitmap.CreateTexture();
	unitImage->imageSizeX = bitmap.xsize;
	unitImage->imageSizeY = bitmap.ysize;
}

void CUnitDrawerData::SetUnitDefImage(const UnitDef* unitDef, unsigned int texID, int xsize, int ysize)
{
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
	if (unitDef->buildPic == nullptr)
		SetUnitDefImage(unitDef, unitDef->buildPicName);

	return (unitDef->buildPic->textureID);
}

void CUnitDrawerData::AddTempDrawUnit(const TempDrawUnit& tdu)
{
	const UnitDef* unitDef = tdu.unitDef;
	const S3DModel* model = unitDef->LoadModel();

	if (tdu.drawAlpha) {
		tempAlphaUnits[model->type].push_back(tdu);
	}
	else {
		tempOpaqueUnits[model->type].push_back(tdu);
	}
}

void CUnitDrawerData::UpdateTempDrawUnits(std::vector<TempDrawUnit>& tempDrawUnits)
{
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

void CUnitDrawerData::RenderUnitCreated(const CUnit* unit, int cloaked)
{
	AddObject(unit);
	UpdateUnitMiniMapIcon(unit, false, false);
}

void CUnitDrawerData::RenderUnitDestroyed(const CUnit* unit)
{
	CUnit* u = const_cast<CUnit*>(unit);

	const UnitDef* unitDef = unit->unitDef;
	const UnitDef* decoyDef = unitDef->decoyDef;

	const bool addNewGhost = unitDef->IsBuildingUnit() && gameSetup->ghostedBuildings;

	// TODO - make ghosted buildings per allyTeam - so they are correctly dealt with
	// when spectating
	GhostSolidObject* gso = nullptr;
	// FIXME -- adjust decals for decoys? gets weird?
	S3DModel* gsoModel = (decoyDef == nullptr) ? u->model : decoyDef->LoadModel();

	for (int allyTeam = 0; allyTeam < deadGhostBuildings.size(); ++allyTeam) {
		const bool canSeeGhost = !(u->losStatus[allyTeam] & (LOS_INLOS | LOS_CONTRADAR)) && (u->losStatus[allyTeam] & (LOS_PREVLOS));

		if (addNewGhost && canSeeGhost) {
			if (gso == nullptr) {
				gso = ghostMemPool.alloc<GhostSolidObject>();

				gso->pos = u->pos;
				gso->model = gsoModel;
				gso->decal = nullptr;
				gso->facing = u->buildFacing;
				gso->dir = u->frontdir;
				gso->team = u->team;
				gso->refCount = 0;
				gso->lastDrawFrame = 0;

				groundDecals->GhostCreated(u, gso);
			}

			// <gso> can be inserted for multiple allyteams
			// (the ref-counter saves us come deletion time)
			deadGhostBuildings[allyTeam][gsoModel->type].push_back(gso);
			gso->IncRef();
		}

		spring::VectorErase(liveGhostBuildings[allyTeam][MDL_TYPE(u)], u);
	}

	DelObject(unit);
	UpdateUnitMiniMapIcon(unit, false, true);

	LuaObjectDrawer::SetObjectLOD(u, LUAOBJ_UNIT, 0);
}

void CUnitDrawerData::UnitEnteredRadar(const CUnit* unit, int allyTeam)
{
	if (allyTeam != gu->myAllyTeam)
		return;

	UpdateUnitMiniMapIcon(unit, false, false);
}

void CUnitDrawerData::UnitEnteredLos(const CUnit* unit, int allyTeam)
{
	CUnit* u = const_cast<CUnit*>(unit); //cleanup

	if (gameSetup->ghostedBuildings && unit->unitDef->IsBuildingUnit())
		spring::VectorErase(liveGhostBuildings[allyTeam][MDL_TYPE(unit)], u);

	if (allyTeam != gu->myAllyTeam)
		return;

	UpdateUnitMiniMapIcon(unit, false, false);
}

void CUnitDrawerData::UnitLeftLos(const CUnit* unit, int allyTeam)
{
	CUnit* u = const_cast<CUnit*>(unit); //cleanup

	if (gameSetup->ghostedBuildings && unit->unitDef->IsBuildingUnit())
		spring::VectorInsertUnique(liveGhostBuildings[allyTeam][MDL_TYPE(unit)], u, true);

	if (allyTeam != gu->myAllyTeam)
		return;

	UpdateUnitMiniMapIcon(unit, false, false);
}

void CUnitDrawerData::UnitCloaked(const CUnit* unit)
{
	CUnit* u = const_cast<CUnit*>(unit);

	if (u->model != nullptr) {
		alphaModelRenderers[MDL_TYPE(u)].AddObject(u);
		opaqueModelRenderers[MDL_TYPE(u)].DelObject(u);
	}
}

void CUnitDrawerData::UnitDecloaked(const CUnit* unit)
{
	CUnit* u = const_cast<CUnit*>(unit);

	if (u->model != nullptr) {
		opaqueModelRenderers[MDL_TYPE(u)].AddObject(u);
		alphaModelRenderers[MDL_TYPE(u)].DelObject(u);
	}
}

void CUnitDrawerData::PlayerChanged(int playerNum)
{
	if (playerNum != gu->myPlayerNum)
		return;

	for (auto iconIt = unitsByIcon.begin(); iconIt != unitsByIcon.end(); ++iconIt) {
		(iconIt->second).clear();
	}

	for (CUnit* unit : unsortedObjects) {
		// force an erase (no-op) followed by an insert
		UpdateUnitMiniMapIcon(unit, true, false);
	}
}

void CUnitDrawerData::SunChanged()
{
	CUnitDrawer::SunChangedStatic();
}
