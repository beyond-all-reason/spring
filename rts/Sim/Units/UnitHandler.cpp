/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "UnitHandler.h"
#include "Unit.h"
#include "UnitDefHandler.h"
#include "UnitMemPool.h"
#include "UnitTypes/Builder.h"
#include "UnitTypes/ExtractorBuilding.h"
#include "UnitTypes/Factory.h"

#include "CommandAI/BuilderCAI.h"
#include "Sim/Ecs/Registry.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/MoveTypes/Systems/GeneralMoveSystem.h"
#include "Sim/MoveTypes/Systems/GroundMoveSystem.h"
#include "Sim/MoveTypes/Systems/UnitTrapCheckSystem.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Weapons/Weapon.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "System/Threading/ThreadPool.h"
#include "System/TimeProfiler.h"
#include "System/creg/STL_Deque.h"
#include "System/creg/STL_Set.h"
#include "System/Threading/ThreadPool.h"
#include "Sim/Path/HAPFS/PathGlobal.h"

#include "System/Misc/TracyDefs.h"

#include "System/Config/ConfigHandler.h"
CONFIG(bool, UpdateWeaponVectorsMT).deprecated(true);
CONFIG(bool, UpdateBoundingVolumeMT).deprecated(true);


CR_BIND(CUnitHandler, )
CR_REG_METADATA(CUnitHandler, (
	CR_MEMBER(idPool),

	CR_MEMBER(units),
	CR_MEMBER(unitsByDefs),
	CR_MEMBER(activeUnits),
	CR_MEMBER(unitsToBeRemoved),

	CR_MEMBER(builderCAIs),

	CR_MEMBER(activeSlowUpdateUnit),
	CR_MEMBER(activeUpdateUnit),

	CR_MEMBER(maxUnits),
	CR_MEMBER(maxUnitRadius),

	CR_MEMBER(inUpdateCall)
))



UnitMemPool unitMemPool;

CUnitHandler unitHandler;


CUnit* CUnitHandler::NewUnit(const UnitDef* ud)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// special static builder structures that can always be given
	// move orders (which are passed on to all mobile buildees)
	if (ud->IsFactoryUnit())
		return (unitMemPool.alloc<CFactory>());

	// all other types of non-structure "builders", including hubs and
	// nano-towers (the latter should not have any build-options at all,
	// whereas the former should be unable to build any mobile units)
	if (ud->IsMobileBuilderUnit() || ud->IsStaticBuilderUnit())
		return (unitMemPool.alloc<CBuilder>());

	// static non-builder structures
	if (ud->IsBuildingUnit()) {
		if (ud->IsExtractorUnit())
			return (unitMemPool.alloc<CExtractorBuilding>());

		return (unitMemPool.alloc<CBuilding>());
	}

	// regular mobile unit
	return (unitMemPool.alloc<CUnit>());
}



void CUnitHandler::Init() {
	RECOIL_DETAILED_TRACY_ZONE;
	GroundMoveSystem::Init();
	GeneralMoveSystem::Init();
	UnitTrapCheckSystem::Init();

	{
		// set the global (runtime-constant) unit-limit as the sum
		// of  all team unit-limits, which is *always* <= MAX_UNITS
		// (note that this also counts the Gaia team)
		//
		// teams can not be created at runtime, but they can die and
		// in that case the per-team limit is recalculated for every
		// other team in the respective allyteam
		maxUnits = CalcMaxUnits();
		maxUnitRadius = 0.0f;
	}
	{
		activeSlowUpdateUnit = 0;
		activeUpdateUnit = 0;
	}
	{
		units.resize(maxUnits, nullptr);
		activeUnits.reserve(maxUnits);

		unitMemPool.reserve(128);

		// id's are used as indices, so they must lie in [0, units.size() - 1]
		// (furthermore all id's are treated equally, none have special status)
		idPool.Clear();
		idPool.Expand(0, MAX_UNITS);

		for (int teamNum = 0; teamNum < teamHandler.ActiveTeams(); teamNum++) {
			unitsByDefs[teamNum].resize(unitDefHandler->NumUnitDefs() + 1);
		}
	}
}


void CUnitHandler::Kill()
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (CUnit* u: activeUnits) {
		// ~CUnit dereferences featureHandler which is destroyed already
		u->KilledScriptFinished(-1);
		unitMemPool.free(u);
	}
	{
		// do not clear in ctor because creg-loaded objects would be wiped out
		unitMemPool.clear();

		units.clear();

		for (int teamNum = 0; teamNum < MAX_TEAMS; teamNum++) {
			// reuse inner vectors when reloading
			// unitsByDefs[teamNum].clear();

			for (size_t defID = 0; defID < unitsByDefs[teamNum].size(); defID++) {
				unitsByDefs[teamNum][defID].clear();
			}
		}

		activeUnits.clear();
		unitsToBeRemoved.clear();

		// only iterated by unsynced code, GetBuilderCAIs has no synced callers
		builderCAIs.clear();
	}
	{
		maxUnits = 0;
		maxUnitRadius = 0.0f;
	}
}


void CUnitHandler::DeleteScripts()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// predelete scripts since they sometimes reference (pieces
	// of) models, which are already gone before KillSimulation
	for (CUnit* u: activeUnits) {
		u->DeleteScript();
	}
}


void CUnitHandler::InsertActiveUnit(CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	idPool.AssignID(unit);

	assert(unit->id < units.size());
	assert(units[unit->id] == nullptr);

	#if 0
	// randomized insertion is supposed to break up peak loads
	// during the (staggered) SlowUpdate step, but also causes
	// more jumping around in memory for regular Updates
	// in larger games (where it would matter most) the order
	// of insertion is essentially guaranteed to be random by
	// interleaved player actions anyway, and if needed could
	// also be achieved by periodic shuffling
	const unsigned int insertionPos = gsRNG.NextInt(activeUnits.size());

	assert(insertionPos < activeUnits.size());
	activeUnits.insert(activeUnits.begin() + insertionPos, unit);

	// do not (slow)update the same unit twice if the new one
	// gets inserted behind our current iterator position and
	// right-shifts the rest
	activeSlowUpdateUnit += (insertionPos <= activeSlowUpdateUnit);
	activeUpdateUnit += (insertionPos <= activeUpdateUnit);

	#else
	activeUnits.push_back(unit);
	#endif

	units[unit->id] = unit;
}


bool CUnitHandler::AddUnit(CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// LoadUnit should make sure this is true
	assert(CanAddUnit(unit->id));

	InsertActiveUnit(unit);

	teamHandler.Team(unit->team)->AddUnit(unit, CTeam::AddBuilt);

	// 0 is not a valid UnitDef id, so just use unitsByDefs[team][0]
	// as an unsorted bin to store all units belonging to unit->team
	spring::VectorInsertUnique(GetUnitsByTeamAndDef(unit->team,                 0), unit, false);
	spring::VectorInsertUnique(GetUnitsByTeamAndDef(unit->team, unit->unitDef->id), unit, false);

	maxUnitRadius = std::max(unit->radius, maxUnitRadius);
	return true;
}


bool CUnitHandler::GarbageCollectUnit(unsigned int id)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (inUpdateCall)
		return false;

	assert(unitsToBeRemoved.empty());

	if (!QueueDeleteUnit(units[id]))
		return false;

	// only processes units[id]
	DeleteUnits();

	return (idPool.RecycleID(id));
}


void CUnitHandler::QueueDeleteUnits()
{
	ZoneScoped;
	// gather up dead units
	for (activeUpdateUnit = 0; activeUpdateUnit < activeUnits.size(); ++activeUpdateUnit) {
		QueueDeleteUnit(activeUnits[activeUpdateUnit]);
	}
}

bool CUnitHandler::QueueDeleteUnit(CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!unit->deathScriptFinished)
		return false;

	// there are many ways to fiddle with "deathScriptFinished", so a unit may
	// arrive here not having been properly killed while isDead is still false
	// make sure we always call Killed; no-op if isDead was already set to true
	unit->ForcedKillUnit(nullptr, false, true);
	unitsToBeRemoved.push_back(unit);
	return true;
}


void CUnitHandler::DeleteUnits()
{
	ZoneScopedC(tracy::Color::Goldenrod);
	while (!unitsToBeRemoved.empty()) {
		DeleteUnit(unitsToBeRemoved.back());
		unitsToBeRemoved.pop_back();
	}
}

void CUnitHandler::DeleteUnit(CUnit* delUnit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(delUnit->isDead);
	// we want to call RenderUnitDestroyed while the unit is still valid
	eventHandler.RenderUnitDestroyed(delUnit);

	const auto it = std::find(activeUnits.begin(), activeUnits.end(), delUnit);

	if (it == activeUnits.end()) {
		assert(false);
		return;
	}

	const int delUnitTeam = delUnit->team;
	const int delUnitType = delUnit->unitDef->id;

	teamHandler.Team(delUnitTeam)->RemoveUnit(delUnit, CTeam::RemoveDied);

	if (activeSlowUpdateUnit > std::distance(activeUnits.begin(), it))
		--activeSlowUpdateUnit;

	activeUnits.erase(it);

	spring::VectorErase(GetUnitsByTeamAndDef(delUnitTeam,           0), delUnit);
	spring::VectorErase(GetUnitsByTeamAndDef(delUnitTeam, delUnitType), delUnit);

	idPool.FreeID(delUnit->id, true);

	units[delUnit->id] = nullptr;

	entt::entity delUnitEntity = delUnit->entityReference;

	CSolidObject::SetDeletingRefID(delUnit->id);
	unitMemPool.free(delUnit);
	CSolidObject::SetDeletingRefID(-1);

	assert( Sim::registry.valid(delUnitEntity) );
	Sim::registry.destroy(delUnitEntity);
}

void CUnitHandler::UpdateUnitMoveTypes()
{
	SCOPED_TIMER("Sim::Unit::MoveType");

	GroundMoveSystem::Update();
	GeneralMoveSystem::Update();
	UnitTrapCheckSystem::Update();
}

void CUnitHandler::UpdateUnitLosStates()
{
    SCOPED_TIMER("Sim::Unit::LosUpdate"); // I don't know why it's wasn't profiled earlier

    // Structure to hold data for deferred event processing
    struct LosUpdateData {
        CUnit* unit;
        int allyTeam;
        unsigned short diffBits; // Store the changed bits
    };

    // Use a temporary vector of vectors (one per thread) to store deferred updates.
    // This avoids locking during the parallel phase.
    std::vector<std::vector<LosUpdateData>> deferredLosUpdates(ThreadPool::GetMaxThreads());
    // Ensure inner vectors are cleared (important if reusing across frames, though here it's stack-local effectively)
for (auto& innerVec : deferredLosUpdates) {
        innerVec.clear();
    }


    // Parallel loop over active units
    {
        ZoneScopedN("Parallel LOS Calculation"); // Tracy zone for the parallel part
        for_mt(0, activeUnits.size(), [&](const int i) {
            CUnit* unit = activeUnits[i];
            const int tid = ThreadPool::GetThreadNum(); // Get thread ID for thread-local storage
            auto& threadDeferredUpdates = deferredLosUpdates[tid];

            // Optional: Reserve space if many ally teams are expected, reduces reallocations
            // threadDeferredUpdates.reserve(teamHandler.ActiveAllyTeams());

            for (int at = 0; at < teamHandler.ActiveAllyTeams(); ++at) {
                const unsigned short currStatus = unit->losStatus[at];

                // Optimization: Skip if all changes are masked by Lua or cheats
                if ((currStatus & LOS_ALL_MASK_BITS) == LOS_ALL_MASK_BITS) {
                    continue;
                }

                // Calculate the new LOS status based on current game state
                const unsigned short newStatus = unit->CalcLosStatus(at);
                // Determine which bits actually changed
                const unsigned short diffBits = (currStatus ^ newStatus);

                // Update the unit's LOS status directly.
                // This *should* be safe because for_mt partitions the work based on unit index,
                // meaning no two threads write to the same unit *instance* concurrently.
                // Reads from shared losHandler maps are safe.
                unit->losStatus[at] = newStatus;

                if (diffBits != 0) {
                    // Store data needed for deferred event handling in the thread's vector
                    threadDeferredUpdates.push_back({unit, at, diffBits});
                }
            }
        });
    } // End of parallel section


    // Serial processing of deferred events (outside the parallel loop, in the main sim thread)
    {
        ZoneScopedN("Deferred Event Processing"); // Tracy zone for the serial part
        for (const auto& threadUpdates : deferredLosUpdates) {
            for (const auto& updateData : threadUpdates) {
                CUnit* unit = updateData.unit;
                const int at = updateData.allyTeam;
                const unsigned short diffBits = updateData.diffBits;
                // Read the final state written by the worker thread
                const unsigned short newStatus = unit->losStatus[at];

                // Check specific bits and fire corresponding events
                // These calls happen serially, avoiding Lua state conflicts.
                if (diffBits & LOS_INLOS) {
                    if (newStatus & LOS_INLOS) {
                        eventHandler.UnitEnteredLos(unit, at);
                        // eoh->UnitEnteredLos(*unit, at); // If eoh needs events, defer/handle similarly
                    } else {
                        eventHandler.UnitLeftLos(unit, at);
                        // eoh->UnitLeftLos(*unit, at);
                    }
                }

                if (diffBits & LOS_INRADAR) {
                    if (newStatus & LOS_INRADAR) {
                        eventHandler.UnitEnteredRadar(unit, at);
                        // eoh->UnitEnteredRadar(*unit, at);
                    } else {
                        eventHandler.UnitLeftRadar(unit, at);
                        // eoh->UnitLeftRadar(*unit, at);
                    }
                }

                // Note: PREVLOS and CONTRADAR bits are implicitly updated by CalcLosStatus
                // based on the changes in INLOS and INRADAR. The original SetLosStatus
                // logic handled this update implicitly after the event calls. Since we
                // update losStatus[at] directly in the parallel section now, these bits
                // are already correct when we process events here.
            }
        }
    } // End of serial event processing
}


void CUnitHandler::SlowUpdateUnits()
{
	SCOPED_TIMER("Sim::Unit::SlowUpdate");

	assert(activeSlowUpdateUnit >= 0);

	// reset the iterator every <UNIT_SLOWUPDATE_RATE> frames
	if ((gs->frameNum % UNIT_SLOWUPDATE_RATE) == 0)
		activeSlowUpdateUnit = 0;

	const size_t idxBeg = activeSlowUpdateUnit;
	const size_t maximumCnt = activeUnits.size() - idxBeg;
	const size_t logicalCnt = (activeUnits.size() / UNIT_SLOWUPDATE_RATE) + 1;
	const size_t indCnt = logicalCnt > maximumCnt ? maximumCnt : logicalCnt;
	const size_t idxEnd = idxBeg + indCnt;

	activeSlowUpdateUnit = idxEnd;
	// stagger the SlowUpdate's

	static std::vector<CUnit*> updateBoundingVolumeList;
	updateBoundingVolumeList.clear();
	{
		ZoneScopedN("Sim::Unit::SlowUpdateST");
		for (size_t i = idxBeg; i < idxEnd; ++i) {
			CUnit* unit = activeUnits[i];

			unit->SanityCheck();
			unit->SlowUpdate();
			unit->SlowUpdateWeapons();
			unit->SanityCheck();

			if (!unit->isDead && unit->localModel.GetBoundariesNeedsRecalc())
				updateBoundingVolumeList.emplace_back(unit);
		}
	}
	// Since the bounding volumes are calculated from the maximum piecematrix-offset piece vertices
	// They dont have much of an effect if updated late-ish.
	{
		ZoneScopedN("Sim::Unit::SlowUpdateMT");
		for_mt(0, updateBoundingVolumeList.size(), [](int i) {
			updateBoundingVolumeList[i]->localModel.UpdateBoundingVolume();
		});
	}
}

void CUnitHandler::UpdateUnits()
{
	SCOPED_TIMER("Sim::Unit::Update");

	size_t activeUnitCount = activeUnits.size();
	for (size_t i = 0; i < activeUnitCount; ++i) {
		CUnit* unit = activeUnits[i];

		unit->SanityCheck();
		unit->Update();
		unit->moveType->UpdateCollisionMap();
		// unsynced; done on-demand when drawing unit
		// unit->UpdateLocalModel();
		unit->SanityCheck();

		assert(activeUnits[i] == unit);
	}
}

void CUnitHandler::UpdateUnitWeapons()
{
	{
		SCOPED_TIMER("Sim::Unit::UpdateWeaponVectors");

		for_mt_chunk(0, activeUnits.size(), [&](const int idx) {
			auto unit = activeUnits[idx];
			unit->UpdateWeaponVectors();
		});
	}
	{
		SCOPED_TIMER("Sim::Unit::Weapon");
		for (activeUpdateUnit = 0; activeUpdateUnit < activeUnits.size(); ++activeUpdateUnit) {
			activeUnits[activeUpdateUnit]->UpdateWeapons();
		}
	}
}


void CUnitHandler::Update()
{
	inUpdateCall = true;

	DeleteUnits();
	UpdateUnitMoveTypes();
	QueueDeleteUnits();
	UpdateUnitLosStates();
	SlowUpdateUnits();
	UpdateUnits();
	UpdateUnitWeapons();

	inUpdateCall = false;
}



void CUnitHandler::AddBuilderCAI(CBuilderCAI* b)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// called from CBuilderCAI --> owner is already valid
	builderCAIs[b->owner->id] = b;
}

void CUnitHandler::RemoveBuilderCAI(CBuilderCAI* b)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// called from ~CUnit --> owner is still valid
	assert(b->owner != nullptr);
	builderCAIs.erase(b->owner->id);
}


void CUnitHandler::ChangeUnitTeam(CUnit* unit, int oldTeamNum, int newTeamNum)
{
	RECOIL_DETAILED_TRACY_ZONE;
	spring::VectorErase       (GetUnitsByTeamAndDef(oldTeamNum,                 0), unit       );
	spring::VectorErase       (GetUnitsByTeamAndDef(oldTeamNum, unit->unitDef->id), unit       );
	spring::VectorInsertUnique(GetUnitsByTeamAndDef(newTeamNum,                 0), unit, false);
	spring::VectorInsertUnique(GetUnitsByTeamAndDef(newTeamNum, unit->unitDef->id), unit, false);
}


bool CUnitHandler::CanBuildUnit(const UnitDef* unitdef, int team) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (teamHandler.Team(team)->AtUnitLimit())
		return false;

	return (NumUnitsByTeamAndDef(team, unitdef->id) < unitdef->maxThisUnit);
}

unsigned int CUnitHandler::CalcMaxUnits() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	unsigned int n = 0;

	for (unsigned int i = 0; i < teamHandler.ActiveTeams(); i++) {
		n += teamHandler.Team(i)->GetMaxUnits();
	}

	return n;
}

