/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include "PathManager.h"
#include "PathingState.h"
#include "PathConstants.h"
#include "PathFinder.h"
#include "PathEstimator.h"
#include "PathFlowMap.hpp"
#include "PathHeatMap.h"
#include "PathLog.h"
#include "PathMemPool.h"
#include "PathSearch.h"
#include "Registry.h"
#include "Map/MapInfo.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"

#include "PathGlobal.h"

#include "System/Misc/TracyDefs.h"

// #include "Game/GlobalUnsynced.h"
// #include "Game/SelectedUnitsHandler.h"
// #include "Rendering/IPathDrawer.h"
// #define DEBUG_DRAWING_ENABLED ((gs->cheatEnabled || gu->spectatingFullView) && pathDrawer->IsEnabled())

// MH Note: Init NumThreads * 3 Finders (IPAthFinder) rather than static init here.

/*
static CPathFinder    gMaxResPF;
static CPathEstimator gMedResPE;
static CPathEstimator gLowResPE;
*/

namespace HAPFS {

int debugLoggingActive = -1;

enum {
	PATH_LOW_RES = 0,
	PATH_MED_RES = 1,
	PATH_ESTIMATOR_LEVELS,

	PATH_MAX_RES = 2,
	PATH_ALL_LEVELS,
};

static PathingState pathingStates[PATH_ESTIMATOR_LEVELS];

const CPathFinder* CPathManager::GetMaxResPF() const { return &maxResPFs[0]; }
const CPathEstimator* CPathManager::GetMedResPE() const { return &medResPEs[0]; }
const CPathEstimator* CPathManager::GetLowResPE() const { return &lowResPEs[0]; }
const PathingState* CPathManager::GetMedResPS() const { return &pathingStates[PATH_MED_RES]; }
const PathingState* CPathManager::GetLowResPS() const { return &pathingStates[PATH_LOW_RES]; }

CPathManager::CPathManager()
// : maxResPF(nullptr)
// , medResPE(nullptr)
// , lowResPE(nullptr)
: pathFlowMap(nullptr)
, pathHeatMap(nullptr)
, nextPathID(0)
{
	IPathFinder::InitStatic();
	CPathFinder::InitStatic();

	InitStatic();

	pathFlowMap = PathFlowMap::GetInstance();
	gPathHeatMap.Init(PATH_HEATMAP_XSCALE, PATH_HEATMAP_ZSCALE);
	pathHeatMap = &gPathHeatMap;

	constexpr size_t initialPathMapSize = 1024;
	pathMap.reserve(initialPathMapSize);

	// PathNode::nodePos is an ushort2, PathNode::nodeNum is an int
	// therefore the maximum map size is limited to 64k*64k squares
	assert(mapDims.mapx <= 0xFFFFU && mapDims.mapy <= 0xFFFFU);
}

void CPathManager::InitStatic()
{
	RECOIL_DETAILED_TRACY_ZONE;
	pathFinderGroups = ThreadPool::GetNumThreads();

	LOG("TK CPathManager::InitStatic: %d threads available", pathFinderGroups);

	// pathFinders[i] = pfMemPool.alloc<CPathFinder>(true);

	const size_t pathFinderCount = PATH_ALL_LEVELS * pathFinderGroups;
	const size_t medLowResMem = sizeof(CPathEstimator) * pathFinderGroups;
	const size_t maxResMem = sizeof(CPathFinder) * pathFinderGroups;

	const size_t totalMem = medLowResMem*PATH_ESTIMATOR_LEVELS + maxResMem;

	const size_t lowResPEsOffset = 0;
	const size_t medResPEsOffset = lowResPEsOffset + medLowResMem;
	const size_t maxResPFsOffset = medResPEsOffset + medLowResMem;

	char* baseAddr = reinterpret_cast<char*>(::operator new(totalMem));
	lowResPEs = reinterpret_cast<CPathEstimator*>( baseAddr + lowResPEsOffset );
	medResPEs = reinterpret_cast<CPathEstimator*>( baseAddr + medResPEsOffset );
	maxResPFs = reinterpret_cast<CPathFinder*>   ( baseAddr + maxResPFsOffset );

	for (int i = 0; i<pathFinderGroups; ++i){
		new (&lowResPEs[i]) CPathEstimator();
		new (&medResPEs[i]) CPathEstimator();
		new (&maxResPFs[i]) CPathFinder();
	}

	std::vector<IPathFinder*> newPathFinders(pathFinderCount);
	for (int i = 0; i<pathFinderGroups; ++i){
		newPathFinders[i*PATH_ALL_LEVELS + PATH_LOW_RES] = &lowResPEs[i];
		newPathFinders[i*PATH_ALL_LEVELS + PATH_MED_RES] = &medResPEs[i];
		newPathFinders[i*PATH_ALL_LEVELS + PATH_MAX_RES] = &maxResPFs[i];
	}

	pathFinders = std::move(newPathFinders);

	constexpr size_t memoryPageSize = 4096;

	//finalized = true;
}

CPathManager::~CPathManager()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// Finalize is not called in case of forced exit
	// if (maxResPF != nullptr) {
	// 	lowResPE->Kill();
	// 	medResPE->Kill();
	// 	maxResPF->Kill();

	// 	maxResPF = nullptr;
	// 	medResPE = nullptr;
	// 	lowResPE = nullptr;
	// }

	for (int i = 0; i<pathFinders.size(); ++i){
		if (pathFinders[i] != nullptr){
			pathFinders[i]->Kill();
			(*pathFinders[i]).~IPathFinder();
		}
	}

	pathFinders.clear();

	if (lowResPEs != nullptr) {
		::operator delete(lowResPEs);
		lowResPEs = nullptr;
	}

	for (int i=0; i<PATH_ESTIMATOR_LEVELS; ++i)
		pathingStates[i].Terminate();

	PathHeatMap::FreeInstance(pathHeatMap);
	PathFlowMap::FreeInstance(pathFlowMap);
	IPathFinder::KillStatic();
	PathingState::KillStatic();

	registry.clear();
}


void CPathManager::RemoveCacheFiles()
{
	RECOIL_DETAILED_TRACY_ZONE;
	pathingStates[PATH_MED_RES].RemoveCacheFile("pe" , mapInfo->map.name);
	pathingStates[PATH_LOW_RES].RemoveCacheFile("pe2", mapInfo->map.name);
}


std::uint32_t CPathManager::GetPathCheckSum() const {
	assert(IsFinalized());

	// MH: At the moment, blockstate cannot be synced.
	//     VertexCost can but need next phase work to make that happen.
	return ( pathingStates[PATH_MED_RES].GetPathChecksum()
		   + pathingStates[PATH_LOW_RES].GetPathChecksum() );

	//return (medResPE->GetPathChecksum() + lowResPE->GetPathChecksum());
}

std::int64_t CPathManager::Finalize()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const spring_time t0 = spring_gettime();

	{
		std::vector<IPathFinder*> maxResList(pathFinderGroups);
		std::vector<IPathFinder*> medResList(pathFinderGroups);

		for (int i = 0; i<pathFinderGroups; ++i){
			maxResPFs[i].Init(true);
			medResPEs[i].Init(&maxResPFs[i], MEDRES_PE_BLOCKSIZE, &pathingStates[PATH_MED_RES]);
			lowResPEs[i].Init(&medResPEs[i], LOWRES_PE_BLOCKSIZE, &pathingStates[PATH_LOW_RES]);
			maxResList[i] = &maxResPFs[i];
			medResList[i] = &medResPEs[i];
			LOG("TK CPathManager::Finalize PathFinder 0x%p has BLOCKSIZE %d", &maxResPFs[i], maxResPFs[i].BLOCK_SIZE);
		}
		
		pathingStates[PATH_MED_RES].Init(std::move(maxResList), nullptr,                      MEDRES_PE_BLOCKSIZE, "pe" , mapInfo->map.name);
		pathingStates[PATH_LOW_RES].Init(std::move(medResList), &pathingStates[PATH_MED_RES], LOWRES_PE_BLOCKSIZE, "pe2", mapInfo->map.name);
	}

	finalized = true;

	const spring_time dt = spring_gettime() - t0;
	return (dt.toMilliSecsi());
}

std::int64_t CPathManager::PostFinalizeRefresh()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const spring_time t0 = spring_gettime();

	if (finalized)
	{
		pathingStates[PATH_MED_RES].UpdateVertexPathCosts(-1);
		pathingStates[PATH_LOW_RES].UpdateVertexPathCosts(-1);
	}

	const spring_time dt = spring_gettime() - t0;
	return (dt.toMilliSecsi());
}

void CPathManager::FinalizePath(MultiPath* path, const float3 startPos, const float3 goalPos, const bool cantGetCloser)
{
	ZoneScoped;
	IPath::Path* sp = &path->lowResPath;
	IPath::Path* ep = &path->maxResPath;

	if (!path->medResPath.path.empty())
		sp = &path->medResPath;
	if (!path->maxResPath.path.empty())
		sp = &path->maxResPath;

	if (!sp->path.empty()) {
		sp->path.back() = startPos;
		sp->path.back().y = CMoveMath::yLevel(*path->moveDef, sp->path.back());
	}

	if (!path->maxResPath.path.empty() && !path->medResPath.path.empty())
		path->medResPath.path.back() = path->maxResPath.path.front();

	if (!path->medResPath.path.empty() && !path->lowResPath.path.empty())
		path->lowResPath.path.back() = path->medResPath.path.front();

	if (cantGetCloser)
		return;


	if (!path->medResPath.path.empty())
		ep = &path->medResPath;
	if (!path->lowResPath.path.empty())
		ep = &path->lowResPath;

	if (!ep->path.empty()) {
		ep->path.front() = goalPos;
		ep->path.front().y = CMoveMath::yLevel(*path->moveDef, ep->path.front());
	}
}


IPath::SearchResult CPathManager::ArrangePath(
	MultiPath* newPath,
	const MoveDef* moveDef,
	const float3& startPos,
	const float3& goalPos,
	CSolidObject* caller
) const {
	RECOIL_DETAILED_TRACY_ZONE;
	CPathFinderDef* pfDef = &newPath->peDef;

	// choose the PF or the PE depending on the projected 2D goal-distance
	// NOTE: this distance can be far smaller than the actual path length!
	// NOTE: take height difference into consideration for "special" cases
	// (unit at top of cliff, goal at bottom or vv.)
	const float heurGoalDist2D = pfDef->Heuristic(startPos.x / SQUARE_SIZE, startPos.z / SQUARE_SIZE, 1) + math::fabs(goalPos.y - startPos.y) / SQUARE_SIZE;
	const float searchDistances[] = {std::numeric_limits<float>::max(), MEDRES_SEARCH_DISTANCE, MAXRES_SEARCH_DISTANCE};

	// MAX_SEARCHED_NODES_PF is 65536, MAXRES_SEARCH_DISTANCE is 50 squares
	// the circular-constraint area therefore is PI*50*50 squares (i.e. 7854
	// rounded up to nearest integer) which means MAX_SEARCHED_NODES_*>>3 is
	// only slightly larger (8192) so the constraint has no purpose even for
	// max-res queries (!)
	assert(MAX_SEARCHED_NODES_PF <= 65536u);
	assert(MAXRES_SEARCH_DISTANCE <= 50.0f);

	constexpr unsigned int nodeLimits[] = {MAX_SEARCHED_NODES_PE >> 3, MAX_SEARCHED_NODES_PE >> 3, MAX_SEARCHED_NODES_PF >> 3};

	constexpr bool useConstraints[] = {false, false, false};
	constexpr bool allowRawSearch[] = {false, false, false};

	const int currentThread = ThreadPool::GetThreadNum(); // thread ids start at 1.

	IPathFinder* ownPathFinders[] =	{ pathFinders[currentThread*PATH_ALL_LEVELS + PATH_LOW_RES]
									, pathFinders[currentThread*PATH_ALL_LEVELS + PATH_MED_RES]
									, pathFinders[currentThread*PATH_ALL_LEVELS + PATH_MAX_RES]
									};
	//{lowResPE, medResPE, maxResPF};
	IPath::Path* pathObjects[] = {&newPath->lowResPath, &newPath->medResPath, &newPath->maxResPath};

	IPath::SearchResult bestResult = IPath::Error;

	unsigned int bestSearch = -1u; // index

	pfDef->useVerifiedStartBlock = true; // ((caller != nullptr) && ThreadPool::inMultiThreadedSection);

	{
		RECOIL_DETAILED_TRACY_ZONE;
		if (heurGoalDist2D <= (MAXRES_SEARCH_DISTANCE * modInfo.pfRawDistMult)) {
			pfDef->AllowRawPathSearch( true);
			pfDef->AllowDefPathSearch(false); // block default search

			// only the max-res CPathFinder implements DoRawSearch
			bestResult = ownPathFinders[PATH_MAX_RES]->GetPath(*moveDef, *pfDef, caller, startPos, *pathObjects[PATH_MAX_RES], nodeLimits[PATH_MAX_RES]);
			bestSearch = PATH_MAX_RES;

			pfDef->AllowRawPathSearch(false);
			pfDef->AllowDefPathSearch( true);

			// if (debugLoggingActive == currentThread){
			// 	LOG("RAW PATH Search Result is: %d", bestResult);
			// }
		}

		if (bestResult != IPath::Ok) {
			// try each pathfinder in order from MAX to LOW limited by distance,
			// with constraints disabled for all three since these break search
			// completeness (CPU usage is still limited by MAX_SEARCHED_NODES_*)
			for (int n = PATH_MAX_RES; n >= PATH_LOW_RES; n--) {
				// distance-limits are in ascending order
				if (heurGoalDist2D > searchDistances[n])
					continue;

				pfDef->DisableConstraint(!useConstraints[n]);
				pfDef->AllowRawPathSearch(allowRawSearch[n]);

				const IPath::SearchResult currResult = ownPathFinders[n]->GetPath(*moveDef, *pfDef, caller, startPos, *pathObjects[n], nodeLimits[n]);

				// if (debugLoggingActive == currentThread){
				// 	LOG("PATH level %d Search Result is: %d",  n, currResult);
				// }

				// note: GEQ s.t. MED-OK will be preferred over LOW-OK, etc
				if (currResult >= bestResult)
					continue;

				bestResult = currResult;
				bestSearch = n;

				// if (debugLoggingActive == currentThread){
				// 	LOG("PATH Best level %d Search Result is: %d",  bestSearch, bestResult);
				// }

				if (currResult == IPath::Ok)
					break;
			}
		}
	}

	for (unsigned int n = PATH_LOW_RES; n <= PATH_MAX_RES; n++) {
		if (n != bestSearch) {
			pathObjects[n]->path.clear();
			pathObjects[n]->squares.clear();

			// if (debugLoggingActive == currentThread){
			// 	LOG("PATH level %d clearing paths",  n);
			// }
		}
	}

	if (bestResult == IPath::Ok)
		return bestResult;

	// if we did not get a complete path with distance/search
	// constraints enabled, run a final unconstrained fallback
	// MED search (unconstrained MAX search is not useful with
	// current node limits and could kill performance without)
	if (heurGoalDist2D > searchDistances[PATH_MED_RES]) {
		pfDef->DisableConstraint(true);

		// we can only have a low-res result at this point
		pathObjects[PATH_LOW_RES]->path.clear();
		pathObjects[PATH_LOW_RES]->squares.clear();

		bestResult = std::min(bestResult, ownPathFinders[PATH_MED_RES]->GetPath(*moveDef, *pfDef, caller, startPos, *pathObjects[PATH_MED_RES], nodeLimits[PATH_MED_RES]));
		
		// if (debugLoggingActive == currentThread){
		// 	LOG("Last ditched pathing attempt result is", bestResult);
		// }
	
	}

	return bestResult;
}

void CPathManager::DeletePath(unsigned int pathID, bool /* force ignored*/) {
	if (pathID == 0)
		return;
	{
		RECOIL_DETAILED_TRACY_ZONE;
		const std::lock_guard<std::mutex> lock(pathMapUpdate); // TODO: remove this? not called in MT sections anymore?
		const auto pi = pathMap.find(pathID);

		if (pi == pathMap.end())
			return;

		pathMap.erase(pi);

		PathSearch* existingSearch = nullptr;
		auto searchView = registry.view<PathSearch>();
		searchView.each([&searchView, pathID](entt::entity entity){
			auto& search = searchView.get<PathSearch>(entity);
			if (search.pathId == pathID)
				registry.destroy(entity);
		});
	}
}

/*
Request a new multipath, store the result and return a handle-id to it.
*/
unsigned int CPathManager::RequestPath(
	CSolidObject* caller,
	const MoveDef* moveDef,
	float3 startPos,
	float3 goalPos,
	float goalRadius,
	bool synced
) {
	unsigned int pathId = 0;

	if (!IsFinalized())
		return 0;

	if (synced) {
		assert(!ThreadPool::inMultiThreadedSection);

		PathSearch* existingSearch = nullptr;
		auto searchView = registry.view<PathSearch>();
		for ( entt::entity entity : searchView ) {
			auto& search = searchView.get<PathSearch>(entity);
			if (search.caller == caller) {
				existingSearch = &search;
				break;
			}
		}

		MultiPath newPath = MultiPath(moveDef, startPos, goalPos, goalRadius);
		newPath.searchResult = IPath::SearchResult::Unitialized;

		if (existingSearch == nullptr) {
			pathId = Store(newPath);
			entt::entity searchEntity = registry.create();
			registry.emplace<PathSearch>(searchEntity, caller, moveDef, startPos, goalPos, goalRadius, pathId);
		}
		else {
			pathId = existingSearch->pathId;
			MultiPath* curMultiPath = GetMultiPath(existingSearch->pathId);
			*curMultiPath = std::move(newPath);
			*existingSearch = std::move(PathSearch(caller, moveDef, startPos, goalPos, goalRadius, pathId));
		}
	}
	else {
		MultiPath newPath = IssuePathRequest(caller, moveDef, startPos, goalPos, goalRadius, synced);
		pathId = Store(newPath);
	}

	return pathId;
}

CPathManager::MultiPath CPathManager::IssuePathRequest(
	CSolidObject* caller,
	const MoveDef* moveDef,
	float3 startPos,
	float3 goalPos,
	float goalRadius,
	bool synced
) {
	ZoneScoped;

	startPos.ClampInBounds();
	goalPos.ClampInBounds();

	// Create an estimator definition.
	goalRadius = std::max<float>(goalRadius, PATH_NODE_SPACING * SQUARE_SIZE); //FIXME do on a per PE & PF level?
	assert(moveDef == moveDefHandler.GetMoveDefByPathType(moveDef->pathType));

	MultiPath newPath = MultiPath(moveDef, startPos, goalPos, goalRadius);
	newPath.finalGoal = goalPos;
	newPath.caller = caller;
	newPath.peDef.synced = synced;

	const IPath::SearchResult result = ArrangePath(&newPath, moveDef, startPos, goalPos, caller);
	if (result != IPath::Error) {
		if (newPath.maxResPath.path.empty()) {
			if (result != IPath::CantGetCloser) {
				LowRes2MaxRes(newPath, startPos, caller, synced);
			} else {
				// add one dummy waypoint so that the calling MoveType
				// does not consider this request a failure, which can
				// happen when startPos is very close to goalPos
				//
				// otherwise, code relying on MoveType::progressState
				// (eg. BuilderCAI::MoveInBuildRange) would misbehave
				// (eg. reject build orders)
				newPath.maxResPath.path.push_back(startPos);
				newPath.maxResPath.squares.push_back(int2(startPos.x / SQUARE_SIZE, startPos.z / SQUARE_SIZE));
			}
		}

		FinalizePath(&newPath, startPos, goalPos, result == IPath::CantGetCloser);
	}
	newPath.searchResult = result;

	return newPath;
}


// converts part of a med-res or low-res that has a path into a max-res path
// HAPFS used to got low -> med, and then med -> max, but there are times when low and med disagree on whether an area
// can be routed through and so could end up with units turning around mid path.
void CPathManager::LowRes2MaxRes(MultiPath& multiPath, const float3& startPos, const CSolidObject* owner, bool synced) const
{
	ZoneScoped;
	assert(IsFinalized());

	IPath::Path& maxResPath = multiPath.maxResPath;
	IPath::Path& lowResPath = (multiPath.medResPath.path.size() > 0) ? multiPath.medResPath : multiPath.lowResPath;

	if (lowResPath.path.empty())
		return;

	lowResPath.path.pop_back();

	// remove med-res waypoints until the next one is far enough
	// note: this should normally never consume the entire path!
	while (!lowResPath.path.empty() && startPos.SqDistance2D(lowResPath.path.back()) < Square(MAXRES_SEARCH_DISTANCE_EXT)) {
		lowResPath.path.pop_back();
	}

	// get the goal of the detailed search
	float3 goalPos = lowResPath.pathGoal;
	if (!lowResPath.path.empty())
		goalPos = lowResPath.path.back();

	// define the search
	CCircularSearchConstraint rangedGoalDef(startPos, goalPos, 0.0f, 2.0f, Square(MAXRES_SEARCH_DISTANCE));
	rangedGoalDef.synced = synced;
	// TODO
	// rangedGoalDef.allowRawPath = true;

	// Perform the search.
	// If this is the final improvement of the path, then use the original goal.
	const auto& pfd = (lowResPath.path.empty()) ? multiPath.peDef : rangedGoalDef;

	auto maxResPF = &maxResPFs[ThreadPool::GetThreadNum()];

	const IPath::SearchResult result = maxResPF->GetPath(*multiPath.moveDef, pfd, owner, startPos, maxResPath, MAX_SEARCHED_NODES_ON_REFINE);

	// If no refined path could be found, set goal as desired goal.
	if (result == IPath::CantGetCloser || result == IPath::Error) {
		maxResPath.pathGoal = goalPos;
	}
}

/*
Removes and return the next waypoint in the multipath corresponding to given id.
*/
float3 CPathManager::NextWayPoint(
	const CSolidObject* owner,
	unsigned int pathID,
	unsigned int numRetries,
	float3 callerPos,
	float radius,
	bool synced
) {
	ZoneScoped;

	const float3 noPathPoint = -XZVector;

	if (!IsFinalized())
		return noPathPoint;

	// 0 indicates the null-path ID
	if (pathID == 0)
		return noPathPoint;

	// find corresponding multipath entry
	MultiPath localMultiPath = GetMultiPathMT(pathID);

	auto createTempWaypoint = [](const float3& sourcePoint, const float3& targetPoint){
		const float3  targetDirec = (targetPoint - sourcePoint).SafeNormalize() * SQUARE_SIZE;
		return float3(sourcePoint.x + targetDirec.x, -1.0f, sourcePoint.z + targetDirec.z);
	};

	if (localMultiPath.searchResult == IPath::SearchResult::Unitialized) {
		return createTempWaypoint(callerPos, localMultiPath.peDef.wsGoalPos);
	}
	else if (localMultiPath.searchResult == IPath::SearchResult::Error) {
		return noPathPoint;
	}

	MultiPath* multiPath = localMultiPath.moveDef != nullptr ? &localMultiPath : nullptr;
	if (multiPath == nullptr)
		return noPathPoint;

	IPath::Path& maxResPath = multiPath->maxResPath;
	IPath::Path& lowResPath = (multiPath->medResPath.path.size() > 0) ? multiPath->medResPath : multiPath->lowResPath;

	if (callerPos == ZeroVector) {
		if (owner != nullptr)
			callerPos = owner->pos;
		else if (!maxResPath.path.empty())
			callerPos = maxResPath.path.back();
	}

	assert(multiPath->peDef.synced == synced);

	#define EXTEND_PATH_POINTS(curResPts, nxtResPts, dist) ((!curResPts.empty() && (curResPts.back()).SqDistance2D(callerPos) < Square((dist))) || nxtResPts.size() <= 2)
	const bool extendMaxResPath = EXTEND_PATH_POINTS(lowResPath.path, maxResPath.path, MAXRES_SEARCH_DISTANCE_EXT);
	#undef EXTEND_PATH_POINTS

	// This position is used to resolve maxres query start points.
	// In longer paths, if this isn't updated to the caller's current
	// position, then the query will produce incorrect results.
	multiPath->peDef.wsStartPos = callerPos;

	// check whether the max-res path needs extending through
	// recursive refinement of its lower-resolution segments
	// if so, check if the med-res path also needs extending
	if (extendMaxResPath && (!synced)) {
		assert(!ThreadPool::inMultiThreadedSection);
		LowRes2MaxRes(*multiPath, callerPos, owner, synced);
		FinalizePath(multiPath, callerPos, multiPath->finalGoal, multiPath->searchResult == IPath::CantGetCloser);
	}

	float3 waypoint = noPathPoint;

	do {
		// eat waypoints from the max-res path until we
		// find one that lies outside the search-radius
		// or equals the goal
		//
		// if this is not possible, then either we are
		// at the goal OR the path could not reach all
		// the way to it (ie. a GoalOutOfRange result)
		// OR we are stuck on an impassable square
		if (maxResPath.path.empty()) {
			if (lowResPath.path.empty()) {
				if (multiPath->searchResult == IPath::Ok)
					waypoint = multiPath->finalGoal;

				// [else]
				// reached in the CantGetCloser case for any max-res searches
				// that start within their goal radius (ie. have no waypoints)
				// RequestPath always puts startPos into maxResPath to handle
				// this so waypoint will have been set to it (during previous
				// iteration) if we end up here
			} else {
				if (!synced) {
					waypoint = NextWayPoint(owner, pathID, numRetries + 1, callerPos, radius, synced);
					break;
				}
				// direct to the nearest lower-res waypoint as a fallback until the path can be expanded.
				// this creates the next step, so it will be checked very regularly until the path is
				// extended.
				if (waypoint == noPathPoint) {
					auto createTempWaypointFromLowerRes = [&createTempWaypoint, &callerPos, radius](IPath::Path& path) {
						float3 waypoint = createTempWaypoint(callerPos, path.path.back());
						if ((callerPos.SqDistance2D(waypoint) < Square(radius)) && (waypoint != path.pathGoal))
							path.path.pop_back();
						return waypoint;
					};

					waypoint = createTempWaypointFromLowerRes(lowResPath);
				}
			}

			break;
		} else {
			// y=0 indicates this is not a temporary waypoint
			waypoint = maxResPath.path.back() * XZVector;
			maxResPath.path.pop_back();
		}
	} while ((callerPos.SqDistance2D(waypoint) < Square(radius)) && (waypoint != maxResPath.pathGoal));

	// check whether the max-res path needs extending through
	// recursive refinement of its lower-resolution segments
	// if so, check if the med-res path also needs extending
	if (extendMaxResPath && synced) {
		std::lock_guard<std::mutex> lock(pathMapUpdate);

		PathSearch* existingSearch = nullptr;
		auto searchView = registry.view<PathSearch>();
		for ( entt::entity entity : searchView ) {
			auto& search = searchView.get<PathSearch>(entity);
			if (search.caller == owner) {
				existingSearch = &search;
				break;
			}
		}

		// if found then a full path search is underway, so don't mess with it.
		// Or an extension has been requested, but that doesn't need to be done twice in a row.
		if (existingSearch == nullptr) {
			// try to look further ahead for the start point.
			float3 startPos = (!maxResPath.path.empty()) ? maxResPath.path.back() : callerPos;
			if (maxResPath.path.empty() && waypoint.y != (-1.f)) {
				startPos = (!waypoint.same(noPathPoint)) ? waypoint : startPos;
			}
			entt::entity pathExtendEntity = registry.create();
			registry.emplace<PathSearch>(pathExtendEntity
					, const_cast<CSolidObject*>(owner) // TODO: sort this const issue out
					, owner->moveDef, startPos, multiPath->peDef.wsGoalPos, radius, pathID);
			registry.emplace<PathExtension>(pathExtendEntity, ExtendPathResType::EXTEND_MAX_RES );
		}
	}

	UpdateMultiPathMT(pathID, localMultiPath);
	return waypoint;
}


CPathManager::MultiPath CPathManager::ExpandCurrentPath(
		const CSolidObject* owner,
		unsigned int pathID,
		unsigned int numRetries,
		float3 callerPos,
		float radius,
		bool extendMedResPath
) {
	ZoneScoped;

	// find corresponding multipath entry
	MultiPath localMultiPath = GetMultiPathMT(pathID);
	MultiPath* multiPath = &localMultiPath;

	if (!IsFinalized())
		return localMultiPath;

	// This position is used to resolve maxres query start points.
	// In longer paths, if this isn't updated to the caller's current
	// position, then the query will produce incorrect results.
	multiPath->peDef.wsStartPos = callerPos;

	LowRes2MaxRes(*multiPath, callerPos, owner, true);
	FinalizePath(multiPath, callerPos, multiPath->finalGoal, multiPath->searchResult == IPath::CantGetCloser);

	localMultiPath.updated = true;
	return localMultiPath;
}


// Tells estimators about changes in or on the map.
void CPathManager::TerrainChange(unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2, unsigned int /*type*/) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (!IsFinalized())
		return;
		
	auto medResPE = &pathingStates[PATH_MED_RES];
	auto lowResPE = &pathingStates[PATH_LOW_RES];

	medResPE->MapChanged(x1, z1, x2, z2);
	lowResPE->MapChanged(x1, z1, x2, z2);
}



void CPathManager::Update()
{
	{
		SCOPED_TIMER("Sim::PathUpdates");
		assert(IsFinalized());

		//pathFlowMap->Update();
		pathHeatMap->Update();

		auto medResPE = &pathingStates[PATH_MED_RES];
		auto lowResPE = &pathingStates[PATH_LOW_RES];

		if (gs->frameNum >= frameNumToRefreshPathStateWorkloadRatio) {
			const auto medResUpdatesCount = std::max(0.01f, float(medResPE->getCountOfUpdates()));
			const auto lowResUpdatesCount = std::max(0.01f, float(lowResPE->getCountOfUpdates()));
			const auto ratio = std::min(medResUpdatesCount / lowResUpdatesCount, float(std::numeric_limits<int>::max()));

			if (ratio < 1.f) {
				highPriorityResPS = lowResPE;
				lowPriorityResPS = medResPE;
				const auto invRatio = std::min(1.f / ratio, float(std::numeric_limits<int>::max()));
				pathStateWorkloadRatio = std::clamp(int(invRatio + .5f)+1, 1, GAME_SPEED);
			} else {
				highPriorityResPS = medResPE;
				lowPriorityResPS = lowResPE;
				pathStateWorkloadRatio = std::clamp(int(ratio + .5f)+1, 1, GAME_SPEED);
			}

			// LOG("PATH medResUpdatesCount=%f lowResUpdatesCount=%f ratio=%f pathStateWorkloadRatio=%d"
			// 		, medResUpdatesCount, lowResUpdatesCount, ratio, pathStateWorkloadRatio);

			frameNumToRefreshPathStateWorkloadRatio = gs->frameNum + GAME_SPEED;
		}

		if (gs->frameNum % pathStateWorkloadRatio)
			highPriorityResPS->Update();
		else
			lowPriorityResPS->Update();
	}
	{
		SCOPED_TIMER("Sim::PathRequests");

		auto pathSearchView = registry.view<PathSearch>();
		for_mt(0, pathSearchView.size(), [this, &pathSearchView](int idx){
			entt::entity searchEntity = pathSearchView.begin()[idx];

			PathSearch& pathSearch = pathSearchView.get<PathSearch>( searchEntity );
			PathExtension* pathExtend = registry.try_get<PathExtension>( searchEntity );

			MultiPath newPath = (pathExtend == nullptr) ?
					IssuePathRequest
						( pathSearch.caller
						, pathSearch.moveDef
						, pathSearch.startPos
						, pathSearch.goalPos
						, pathSearch.goalRadius
						, true)
					: ExpandCurrentPath
						( pathSearch.caller
						, pathSearch.pathId
						, 0
						, pathSearch.startPos
						, pathSearch.goalRadius
						, pathExtend->pathResToExtend == ExtendPathResType::EXTEND_MED_RES);

			UpdateMultiPathMT(pathSearch.pathId, newPath);

			// LOG("%s: ent = %x, pathId = %d, result = %d, steps = %d", __func__
			// 		, entt::to_integral(entities[idx]), pathSearch.pathId, newPath.searchResult
			// 		, (int)newPath.maxResPath.path.size());
		});

		// Clear out the search entities.
		pathSearchView.each([](entt::entity ent){ registry.destroy(ent); });
	}
}

// used to deposit heat on the heat-map as a unit moves along its path
void CPathManager::UpdatePath(const CSolidObject* owner, unsigned int pathID)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(IsFinalized());

	//pathFlowMap->AddFlow(owner);
	pathHeatMap->AddHeat(owner, this, pathID);
}


void CPathManager::SavePathCacheForPathId(int pathIdToSave)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const MultiPath& mpath = *GetMultiPathConst(pathIdToSave);
	if (mpath.moveDef == nullptr) { return; }

	if (!mpath.lowResPath.path.empty()) {
		pathingStates[PATH_LOW_RES].PromotePathForCurrentFrame
				( &mpath.lowResPath
				, mpath.searchResult
				, mpath.peDef.wsStartPos
				, mpath.peDef.wsGoalPos
				, mpath.peDef.sqGoalRadius
				, mpath.moveDef->pathType
				, mpath.peDef.synced
				);
	}
	if (!mpath.medResPath.path.empty())
	{
		pathingStates[PATH_MED_RES].PromotePathForCurrentFrame
				( &mpath.medResPath
				, mpath.searchResult
				, mpath.peDef.wsStartPos
				, mpath.peDef.wsGoalPos
				, mpath.peDef.sqGoalRadius
				, mpath.moveDef->pathType
				, mpath.peDef.synced
				);
	}
	// if (mpath.medResPath.path.empty() && mpath.lowResPath.path.empty())
	// 	LOG("Path resolved to max level ONLY");
}

// get the waypoints in world-coordinates
void CPathManager::GetDetailedPath(unsigned pathID, std::vector<float3>& points) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const MultiPath* multiPath = GetMultiPathConst(pathID);

	if (multiPath == nullptr) {
		points.clear();
		return;
	}

	const IPath::Path& path = multiPath->maxResPath;
	const IPath::path_list_type& maxResPoints = path.path;

	points.clear();
	points.reserve(maxResPoints.size());

	for (auto pvi = maxResPoints.rbegin(); pvi != maxResPoints.rend(); ++pvi) {
		points.push_back(*pvi);
	}
}

void CPathManager::GetDetailedPathSquares(unsigned pathID, std::vector<int2>& squares) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const MultiPath* multiPath = GetMultiPathConst(pathID);

	if (multiPath == nullptr) {
		squares.clear();
		return;
	}

	const IPath::Path& path = multiPath->maxResPath;
	const IPath::square_list_type& maxResSquares = path.squares;

	squares.clear();
	squares.reserve(maxResSquares.size());

	for (auto pvi = maxResSquares.rbegin(); pvi != maxResSquares.rend(); ++pvi) {
		squares.push_back(*pvi);
	}
}



void CPathManager::GetPathWayPoints(
	unsigned int pathID,
	std::vector<float3>& points,
	std::vector<int>& starts
) const {
	RECOIL_DETAILED_TRACY_ZONE;
	points.clear();
	starts.clear();

	const MultiPath* multiPath = GetMultiPathConst(pathID);

	if (multiPath == nullptr)
		return;

	const IPath::path_list_type& maxResPoints = multiPath->maxResPath.path;
	const IPath::path_list_type& medResPoints = multiPath->medResPath.path;
	const IPath::path_list_type& lowResPoints = multiPath->lowResPath.path;

	points.reserve(maxResPoints.size() + medResPoints.size() + lowResPoints.size());
	starts.reserve(3);
	starts.push_back(points.size());

	for (IPath::path_list_type::const_reverse_iterator pvi = maxResPoints.rbegin(); pvi != maxResPoints.rend(); ++pvi) {
		points.push_back(*pvi);
	}

	starts.push_back(points.size());

	for (IPath::path_list_type::const_reverse_iterator pvi = medResPoints.rbegin(); pvi != medResPoints.rend(); ++pvi) {
		points.push_back(*pvi);
	}

	starts.push_back(points.size());

	for (IPath::path_list_type::const_reverse_iterator pvi = lowResPoints.rbegin(); pvi != lowResPoints.rend(); ++pvi) {
		points.push_back(*pvi);
	}
}



bool CPathManager::SetNodeExtraCost(unsigned int x, unsigned int z, float cost, bool synced) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (!IsFinalized())
		return false;

	if (x >= mapDims.mapx) { return false; }
	if (z >= mapDims.mapy) { return false; }

	for (int i = 0; i<pathFinderGroups; ++i){
		auto maxResPF = &maxResPFs[i];
		PathNodeStateBuffer& maxResBuf = maxResPF->GetNodeStateBuffer();
		maxResBuf.SetNodeExtraCost(x, z, cost, synced);
	}

	auto medResPE = &pathingStates[PATH_MED_RES];
	auto lowResPE = &pathingStates[PATH_LOW_RES];

	PathNodeStateBuffer& medResBuf = medResPE->GetNodeStateBuffer();
	PathNodeStateBuffer& lowResBuf = lowResPE->GetNodeStateBuffer();
	
	medResBuf.SetNodeExtraCost(x, z, cost, synced);
	lowResBuf.SetNodeExtraCost(x, z, cost, synced);
	return true;
}

bool CPathManager::SetNodeExtraCosts(const float* costs, unsigned int sizex, unsigned int sizez, bool synced) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (!IsFinalized())
		return false;

	if (sizex < 1 || sizex > mapDims.mapx) { return false; }
	if (sizez < 1 || sizez > mapDims.mapy) { return false; }

	for (int i = 0; i<pathFinderGroups; ++i){
		auto maxResPF = &maxResPFs[i];
		PathNodeStateBuffer& maxResBuf = maxResPF->GetNodeStateBuffer();
		maxResBuf.SetNodeExtraCosts(costs, sizex, sizez, synced);
	}

	auto medResPE = &pathingStates[PATH_MED_RES];
	auto lowResPE = &pathingStates[PATH_LOW_RES];

	PathNodeStateBuffer& medResBuf = medResPE->GetNodeStateBuffer();
	PathNodeStateBuffer& lowResBuf = lowResPE->GetNodeStateBuffer();

	// make all buffers share the same cost-overlay
	medResBuf.SetNodeExtraCosts(costs, sizex, sizez, synced);
	lowResBuf.SetNodeExtraCosts(costs, sizex, sizez, synced);
	return true;
}

float CPathManager::GetNodeExtraCost(unsigned int x, unsigned int z, bool synced) const {
	RECOIL_DETAILED_TRACY_ZONE;
	if (!IsFinalized())
		return 0.0f;

	if (x >= mapDims.mapx) { return 0.0f; }
	if (z >= mapDims.mapy) { return 0.0f; }

	auto maxResPF = &maxResPFs[ThreadPool::GetThreadNum()];

	const PathNodeStateBuffer& maxResBuf = maxResPF->GetNodeStateBuffer();
	const float cost = maxResBuf.GetNodeExtraCost(x, z, synced);
	return cost;
}

const float* CPathManager::GetNodeExtraCosts(bool synced) const {
	RECOIL_DETAILED_TRACY_ZONE;
	if (!IsFinalized())
		return nullptr;

	auto maxResPF = &maxResPFs[ThreadPool::GetThreadNum()];

	const PathNodeStateBuffer& buf = maxResPF->GetNodeStateBuffer();
	const float* costs = buf.GetNodeExtraCosts(synced);
	return costs;
}

int2 CPathManager::GetNumQueuedUpdates() const {
	RECOIL_DETAILED_TRACY_ZONE;
	int2 data;

	if (IsFinalized()) {
		auto medResPE = &pathingStates[PATH_MED_RES];
		auto lowResPE = &pathingStates[PATH_LOW_RES];

		data.x = medResPE->updatedBlocks.size();
		data.y = lowResPE->updatedBlocks.size();
	}

	return data;
}

}