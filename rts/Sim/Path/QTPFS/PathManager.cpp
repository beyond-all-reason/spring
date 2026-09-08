/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include <assert.h>

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <deque>
#include <functional>

#include "System/Threading/ThreadPool.h"
#include "System/Threading/SpringThreading.h"

#include "PathDefines.h"
#include "PathManager.h"

#include "Utils/PathSpeedModInfoSystemUtils.h"

#include "Game/GameSetup.h"
#include "Game/LoadScreen.h"
#include "Map/MapInfo.h"

#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Objects/SolidObject.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/Platform/Threading.h"
#include "System/Rectangle.h"
#include "System/TimeProfiler.h"
#include "System/StringUtil.h"

#include "Components/Path.h"
#include "Components/PathSpeedModInfo.h"
#include "Components/RemoveDeadPaths.h"
#include "Components/SyncUpdatedPaths.h"
#include "Systems/PathSpeedModInfoSystem.h"
#include "Systems/RemoveDeadPathsSystem.h"
#include "Systems/RequeuePathsSystem.h"
#include "Systems/SyncUpdatedPathsSystem.h"
#include "Utils/DestroyEntityUtils.h"
#include "Utils/SyncUpdatedPathsSystemUtils.h"
#include "Registry.h"

#include <assert.h>
#include "System/Misc/TracyDefs.h"

#ifdef GetTempPath
#undef GetTempPath
#undef GetTempPathA
#endif

#define NUL_RECTANGLE SRectangle(0, 0,             0,            0)
#define MAP_RECTANGLE SRectangle(0, 0,  mapDims.mapx, mapDims.mapy)

CONFIG(int, PathingThreadCount).defaultValue(0).safemodeValue(1).minimumValue(0);

namespace QTPFS {
	struct PMLoadScreen {
	public:
		PMLoadScreen() { loadMessages.reserve(8); }
		~PMLoadScreen() { assert(loadMessages.empty()); }

		void Kill() { loading = false; }
		void Show(const std::function<void(QTPFS::PathManager*)>& lf, QTPFS::PathManager* pm) {
			Init(lf, pm);
			Loop();
			Join();
		}

		void AddMessage(std::string&& msg) {
			std::lock_guard<spring::mutex> loadMessageLock(loadMessageMutex);
			loadMessages.emplace_back(std::move(msg));
		}

	private:
		void Init(const std::function<void(QTPFS::PathManager*)>& lf, QTPFS::PathManager* pm) {
			// must be set here to handle reloading
			loading = true;
			loadThread = spring::thread(std::bind(lf, pm));
		}
		void Loop() {
			while (loading) {
				spring::this_thread::sleep_for(std::chrono::milliseconds(50));

				// need this to be always executed after waking up
				SetMessages();
			}

			// handle any leftovers
			SetMessages();
		}
		void Join() {
			loadThread.join();
		}

		void SetMessages() {
			std::lock_guard<spring::mutex> loadMessageLock(loadMessageMutex);

			for (std::string& msg: loadMessages) {
				#ifdef QTPFS_NO_LOADSCREEN
				LOG("%s", msg.c_str());
				#else
				loadscreen->SetLoadMessage(std::move(msg));
				#endif
			}

			loadMessages.clear();
		}

	private:
		std::vector<std::string> loadMessages;
		spring::mutex loadMessageMutex;
		spring::thread loadThread;

		std::atomic<bool> loading = {false};
	};

	static PMLoadScreen pmLoadScreen;

	static size_t GetNumThreads() {
		const size_t numThreads = std::max(0, configHandler->GetInt("PathingThreadCount"));
		const size_t numCores = Threading::GetLogicalCpuCores();
		return ((numThreads == 0)? numCores: numThreads);
	}

	unsigned int PathManager::LAYERS_PER_UPDATE;
	unsigned int PathManager::MAX_TEAM_SEARCHES;

	IPath* GetPath(QTPFS::entity entityId) {
		if (!registry.valid(entityId)) return nullptr;

		IPath* path = registry.try_get<IPath>(entityId);
		if (path != nullptr) return path;

		path = registry.try_get<ExternallyManagedSyncedIPath>(entityId);
		if (path != nullptr) return path;

		return registry.try_get<UnsyncedIPath>(entityId);
	};

	PathSearch* GetSearch(QTPFS::entity entityId) {
		if (!registry.valid(entityId)) return nullptr;

		PathSearch* path = registry.try_get<PathSearch>(entityId);
		if (path != nullptr) return path;

		path = registry.try_get<ExternallyManagedPathSearch>(entityId);
		if (path != nullptr) return path;

		return registry.try_get<UnsyncedPathSearch>(entityId);
	};

	IPath* GetSearchPath(QTPFS::entity entityId) {
		if (!registry.valid(entityId)) return nullptr;

		IPath* path = registry.try_get<SearchModeIPath>(entityId);
		if (path != nullptr) return path;

		return registry.try_get<UnsyncedIPath>(entityId);
	};
}

QTPFS::PathManager::PathManager() {
	RECOIL_DETAILED_TRACY_ZONE;
	QTNode::InitStatic();
	NodeLayer::InitStatic();
	PathManager::InitStatic();
	PathSearch::InitStatic();
	UnsyncedPathSearch::InitStatic();
	ExternallyManagedPathSearch::InitStatic();

	assert(registry.alive() == 0);

	// reserve entity 0 so it can't be used picked up by a path by accident.
	systemEntity = registry.create();

	CTimeProfiler::RegisterTimer("Sim::Path::Requests");

	assert(entt::to_entity(systemEntity) == 0);
}

QTPFS::PathManager::~PathManager() {
	RECOIL_DETAILED_TRACY_ZONE;
	isFinalized = false;

	RequeuePathsSystem::Shutdown();
	PathSpeedModInfoSystem::Shutdown();
	RemoveDeadPathsSystem::Shutdown();
	SyncUpdatedPathsSystem::Shutdown();

	// print out and clear anything still left in the registry
	// due to delayed path deletion there may be some entities still around.
	registry.each([this](auto entity) {
		bool isPath = registry.all_of<IPath>(entity);
		bool isUnsyncedPath = registry.all_of<UnsyncedIPath>(entity);
		bool isExternallyManagedSyncedPath = registry.all_of<ExternallyManagedSyncedIPath>(entity);

		bool isSearch = registry.all_of<PathSearch>(entity);
		bool isUnsyncedSearch = registry.all_of<UnsyncedPathSearch>(entity);
		bool isExternallyManagedSearch = registry.all_of<ExternallyManagedPathSearch>(entity);

		if (isPath) {
			LOG("%s: IPath %x still active!", __func__, entt::to_integral(entity));
			DestroyPathEntity(entity);
		}
		if (isUnsyncedPath) {
			LOG("%s: UnsyncedIPath %x still active!", __func__, entt::to_integral(entity));
			DestroyPathEntity(entity);
		}
		if (isExternallyManagedSyncedPath) {
			LOG("%s: ExternallyManagedSyncedIPath %x still active!", __func__, entt::to_integral(entity));
			DestroyPathEntity(entity);
		}
		if (isSearch) {
			LOG("%s: PathSearch %x still active!", __func__, entt::to_integral(entity));
			DestroyPathSearchEntity(entity);
		}
		if (isUnsyncedSearch) {
			LOG("%s: UnsyncedPathSearch %x still active!", __func__, entt::to_integral(entity));
			DestroyPathSearchEntity(entity);
		}
		if (isExternallyManagedSearch) {
			LOG("%s: ExternallyManagedPathSearch %x still active!", __func__, entt::to_integral(entity));
			DestroyPathSearchEntity(entity);
		}
	});

	nodeLayerUpdatePriorityOrder.clear();
	for (unsigned int layerNum = 0; layerNum < nodeLayers.size(); layerNum++) {
		auto& nodeLayer = nodeLayers[layerNum];
		for (int i = 0; i < nodeLayer.GetRootNodeCount(); ++i){
			auto curRootNode = nodeLayer.GetPoolNode(i);
			curRootNode->Merge(nodeLayers[layerNum]);
		}

		nodeLayers[layerNum].Clear();
	}
	std::for_each(pathTraces.begin(), pathTraces.end(), [](std::pair<unsigned int, QTPFS::PathSearchTrace::Execution*>& t){delete t.second;} );

	auto clearTrackers = [](auto& track){
		track.damageMap.clear();
		track.damageQueue.clear();
	};

	pathTraces.clear();
	std::for_each(nodeLayersMapDamageTrack.mapChangeTrackers.begin(), nodeLayersMapDamageTrack.mapChangeTrackers.end(), clearTrackers);
	nodeLayersMapDamageTrack.mapChangeTrackers.clear();
	sharedPaths.clear();
	partialSharedPaths.clear();

	// numCurrExecutedSearches.clear();
	// numPrevExecutedSearches.clear();

	searchThreadData.clear();
	updateThreadData.clear();

	systemGlobals.ClearComponents();

	// make sure this is destroyed last to ensure entity 0 will be first picked up next time.
	registry.destroy(systemEntity);

	LOG("%s: %d entities still active!", __func__, int(registry.alive()));

	assert(registry.alive() == 0);

	registry.clear();
}

std::int64_t QTPFS::PathManager::Finalize() {
	RECOIL_DETAILED_TRACY_ZONE;
	const spring_time t0 = spring_gettime();

	{
		pmLoadScreen.Show(&PathManager::Load, this);
	}

	const spring_time t1 = spring_gettime();
	const spring_time dt = t1 - t0;

	return (dt.toMilliSecsi());
}

std::int64_t QTPFS::PathManager::PostFinalizeRefresh() {
	RECOIL_DETAILED_TRACY_ZONE;
	const spring_time t0 = spring_gettime();
	
	bool updateNeeded = nodeLayersMapDamageTrack.mapChangeTrackers.end() !=
		std::ranges::find_if(nodeLayersMapDamageTrack.mapChangeTrackers,
			[](const QTPFS::PathManager::MapChangeTrack &ct) -> bool { return ct.damageQueue.size() > 0; });

	if (updateNeeded) {
		// Rescan the map otherwise random maps won't work correctly.

		SRectangle rect(0,0,0,0);
		for_mt(0, nodeLayers.size(), [this, &rect](const int index) {
			int curThread = ThreadPool::GetThreadNum();
			int layerNum = nodeLayerUpdatePriorityOrder[index];
			int blocksToUpdate = nodeLayersMapDamageTrack.mapChangeTrackers[layerNum].damageQueue.size();
			for (int i = 0; i < blocksToUpdate; ++i) { UpdateNodeLayer(layerNum, rect, curThread); }
		});

		PathSpeedModInfoSystem::Init();
	}

	const spring_time dt = spring_gettime() - t0;
	return (dt.toMilliSecsi());
}

void QTPFS::PathManager::InitStatic() {
	RECOIL_DETAILED_TRACY_ZONE;
	LAYERS_PER_UPDATE = std::max(1u, mapInfo->pfs.qtpfs_constants.layersPerUpdate);
	MAX_TEAM_SEARCHES = std::max(1u, mapInfo->pfs.qtpfs_constants.maxTeamSearches);

	// Ensure SharedPathChain is assigned a Pool by EnTT to avoid it happening in an MT section,
	// which would cause a potential race condition. Failure to do this can cause seemingly random
	// memory-related errors to occur.
	{ auto view = registry.view<SharedPathChain>();
	  if (view.size() > 0) { LOG("%s: SharedPathChain is unexpectedly greater than 0.", __func__); }
	}
	{ auto view = registry.view<PartialSharedPathChain>();
	  if (view.size() > 0) { LOG("%s: PartialSharedPathChain is unexpectedly greater than 0.", __func__); }
	}
	{ auto view = registry.view<IPath>();
	  if (view.size() > 0) { LOG("%s: IPath is unexpectedly greater than 0.", __func__); }
	}
	{ auto view = registry.view<UnsyncedIPath>();
	  if (view.size() > 0) { LOG("%s: UnsyncedIPath is unexpectedly greater than 0.", __func__); }
	}
	{ auto view = registry.view<ExternallyManagedSyncedIPath>();
	  if (view.size() > 0) { LOG("%s: ExternallyManagedSyncedIPath is unexpectedly greater than 0.", __func__); }
	}
	{ auto view = registry.view<PathSearch>();
	  if (view.size() > 0) { LOG("%s: PathSearch is unexpectedly greater than 0.", __func__); }
	}
	{ auto view = registry.view<UnsyncedPathSearch>();
	  if (view.size() > 0) { LOG("%s: UnsyncedPathSearch is unexpectedly greater than 0.", __func__); }
	}
	{ auto view = registry.view<ExternallyManagedPathSearch>();
	  if (view.size() > 0) { LOG("%s: ExternallyManagedPathSearch is unexpectedly greater than 0.", __func__); }
	}
	// Views are created in multi-threaded sections, but they are referenced and I haven't determined
	// yet whether that is safe in EnTT so creating views here to ensure everything is initialized
	// prior to being used.
	{ auto view = registry.view<PathIsTemp>();
	  if (view.size() > 0) { LOG("%s: PathIsTemp is unexpectedly greater than 0.", __func__); }
	}
	{ auto view = registry.view<PathIsDirty>();
	  if (view.size() > 0) { LOG("%s: PathIsDirty is unexpectedly greater than 0.", __func__); }
	}
	{ auto view = registry.view<PathSpeedModInfoSystemComponent>();
	  if (view.size() > 0) { LOG("%s: PathSpeedModInfoSystemComponent is unexpectedly greater than 0.", __func__); }
	}
	{ auto view = registry.view<PathSearchRef>();
	  if (view.size() > 0) { LOG("%s: PathSearchRef is unexpectedly greater than 0.", __func__); }
	}
}

void QTPFS::PathManager::Load() {
	RECOIL_DETAILED_TRACY_ZONE;
	// NOTE: offset *must* start at a non-zero value
	searchStateOffset = NODE_STATE_OFFSET;
	numPathRequests   = 0;
	int maxAllocedNodes   = 0;

	deadPathsToUpdatePerFrame = 1;
	recalcDeadPathUpdateRateOnFrame = 0;

	const int numMoveDefs = moveDefHandler.GetNumMoveDefs();

	pathCache.Init(numMoveDefs);
	nodeLayers.resize(numMoveDefs);

	InitRootSize(MAP_RECTANGLE);

	nodeLayerUpdatePriorityOrder.resize(numMoveDefs);

	nodeLayersMapDamageTrack.width = mapDims.mapx / DAMAGE_MAP_BLOCK_SIZE;
	nodeLayersMapDamageTrack.height = mapDims.mapy / DAMAGE_MAP_BLOCK_SIZE;
	nodeLayersMapDamageTrack.cellSize = DAMAGE_MAP_BLOCK_SIZE;

	nodeLayersMapDamageTrack.mapChangeTrackers.clear();
	nodeLayersMapDamageTrack.mapChangeTrackers.reserve(numMoveDefs);
	for (int i = 0; i < numMoveDefs; ++i) {
		{
			MapChangeTrack newChangeTrack;
			newChangeTrack.damageMap.resize(nodeLayersMapDamageTrack.width*nodeLayersMapDamageTrack.height);
			nodeLayersMapDamageTrack.mapChangeTrackers.emplace_back(newChangeTrack);
		}
		nodeLayerUpdatePriorityOrder[i] = i;
	}

	// This will be used to determine the order in which the threads process the layers. Start if
	// the layers with larger footprints because they require more processing to complete.
	std::stable_sort(nodeLayerUpdatePriorityOrder.begin(), nodeLayerUpdatePriorityOrder.end(), [](int a, int b){
		return (moveDefHandler.GetMoveDefByPathType(a)->xsize > moveDefHandler.GetMoveDefByPathType(b)->xsize);
	});

	// for (int i=0; i < nodeLayerUpdatePriorityOrder.size(); ++i) {
	// 	LOG("%s: %d [priority %d] xsize = %d", __func__, nodeLayerUpdatePriorityOrder[i], i
	// 			, moveDefHandler.GetMoveDefByPathType(nodeLayerUpdatePriorityOrder[i])->xsize);
	// }

	isFinalized = true;
	{
		int threads = ThreadPool::GetNumThreads();
		updateThreadData.reserve(threads);
		while (threads-- > 0) {
			updateThreadData.emplace_back(UpdateThreadData());
		}
	}
	// add one extra element for object-less requests
	// numCurrExecutedSearches.resize(teamHandler.ActiveTeams() + 1, 0);
	// numPrevExecutedSearches.resize(teamHandler.ActiveTeams() + 1, 0);

	{
		const sha512::raw_digest mapCheckSum = archiveScanner->GetArchiveCompleteChecksumBytes(gameSetup->mapName);
		const sha512::raw_digest modCheckSum = archiveScanner->GetArchiveCompleteChecksumBytes(gameSetup->modName);

		sha512::hex_digest mapCheckSumHex;
		sha512::hex_digest modCheckSumHex;
		sha512::dump_digest(mapCheckSum, mapCheckSumHex);
		sha512::dump_digest(modCheckSum, modCheckSumHex);

		InitNodeLayersThreaded(MAP_RECTANGLE);

		// This system syncs the background pathing requests.
		SyncUpdatedPathsSystem::Init();
	
		// Systems following here can make changes that would otherwise break active searches. It is safe from this
		// point on.
		RemoveDeadPathsSystem::Init();
		PathSpeedModInfoSystem::Init();
		RequeuePathsSystem::Init();

		// NOTE:
		//   should be sufficient in theory, because if either
		//   the map or the mod changes then the checksum does
		//   (should!) as well and we get a cache-miss
		//   this value is also combined with the tree-sums to
		//   make it depend on the tesselation code specifics
		// FIXME:
		//   assumption is invalid now (Lua inits before we do)
		pfsCheckSum = 0;
		// temporary measure until the false-positives around map files is solved.
			// ((mapCheckSum[0] << 24) | (mapCheckSum[1] << 16) | (mapCheckSum[2] << 8) | (mapCheckSum[3] << 0)) ^
			// ((modCheckSum[0] << 24) | (modCheckSum[1] << 16) | (modCheckSum[2] << 8) | (modCheckSum[3] << 0));

		for (unsigned int layerNum = 0; layerNum < nodeLayers.size(); layerNum++) {
			auto& nodeLayer = nodeLayers[layerNum];
			for (int i = 0; i < nodeLayer.GetRootNodeCount(); ++i){
				auto curRootNode = nodeLayer.GetPoolNode(i);
				pfsCheckSum ^= curRootNode->GetCheckSum(nodeLayers[layerNum]);
			}
			maxAllocedNodes = std::max(nodeLayers[layerNum].GetMaxNodesAlloced(), maxAllocedNodes);
		}

		{ SyncedUint tmp(pfsCheckSum); }

		int threads = ThreadPool::GetNumThreads();
		searchThreadData.reserve(threads);
		while (threads-- > 0) {
			searchThreadData.emplace_back(SearchThreadData(maxAllocedNodes, threads));
		}
	}

	{
		const int memFootPrintMb = GetMemFootPrint();
		const std::string sumStr = "pfs-checksum: " + IntToString(pfsCheckSum, "%08x") + ", ";
		const std::string memStr = "mem-footprint: " + IntToString(memFootPrintMb) + "MB";

		LOG("[QTPFS] pfs-checksum: %08x", pfsCheckSum);
		LOG("[QTPFS] mem-footprint: %dMB", memFootPrintMb);

		char loadMsg[512] = {'\0'};
		const char* fmtString = "[PathManager::%s] Complete. Used %u threads for %u node-layers";
		snprintf(loadMsg, sizeof(loadMsg), fmtString, __func__, ThreadPool::GetNumThreads(), nodeLayers.size());

		pmLoadScreen.AddMessage(loadMsg);
		pmLoadScreen.Kill();
	}
}

std::uint64_t QTPFS::PathManager::GetMemFootPrint() const {
	std::uint64_t memFootPrint = sizeof(PathManager);

	memFootPrint += nodeLayers.size() * sizeof(decltype(nodeLayers)::value_type);
	memFootPrint += pathCache.dirtyPaths.size() * sizeof(decltype(pathCache.dirtyPaths)::value_type);

	memFootPrint += searchThreadData.size() * sizeof(decltype(searchThreadData)::value_type);
	memFootPrint += updateThreadData.size() * sizeof(decltype(updateThreadData)::value_type);
	memFootPrint += nodeLayerUpdatePriorityOrder.size() * sizeof(decltype(nodeLayerUpdatePriorityOrder)::value_type);

	memFootPrint += pathTraces.size() * sizeof(decltype(pathTraces)::value_type);
	memFootPrint += sharedPaths.size() * sizeof(decltype(sharedPaths)::value_type);
	memFootPrint += partialSharedPaths.size() * sizeof(decltype(partialSharedPaths)::value_type);

	memFootPrint += sizeof(nodeLayersMapDamageTrack);
	memFootPrint += nodeLayersMapDamageTrack.mapChangeTrackers.size()
					* sizeof(decltype(nodeLayersMapDamageTrack.mapChangeTrackers)::value_type);


	for (auto& threadData : searchThreadData) {
		memFootPrint += threadData.GetMemFootPrint();
	}
	for (auto& threadData : updateThreadData) {
		memFootPrint += threadData.GetMemFootPrint();
	}
	for (unsigned int i = 0; i < nodeLayers.size(); i++) {
		memFootPrint += nodeLayers[i].GetMemFootPrint();
	}
	for (auto& trace : pathTraces) {
		memFootPrint += sizeof(decltype(*trace.second));
		memFootPrint += trace.second->GetMemFootPrint();
	}

	// convert to megabytes
	return (memFootPrint / (1024 * 1024));
}



void QTPFS::PathManager::InitNodeLayersThreaded(const SRectangle& rect) {
	RECOIL_DETAILED_TRACY_ZONE;
	streflop::streflop_init<streflop::Simple>();

	char loadMsg[512] = {'\0'};
	const char* fmtString = "[PathManager::%s] using %u threads for %u node-layers";
	snprintf(loadMsg, sizeof(loadMsg), fmtString, __func__, ThreadPool::GetNumThreads(), nodeLayers.size());
	pmLoadScreen.AddMessage(loadMsg);

	// #ifndef NDEBUG
	// const char* preFmtStr = "  initializing node-layer %u";
	// const char* pstFmtStr = "  initialized node-layer %u (%u MB, %u leafs, ratio %f)";
	// #endif

	for_mt(0, nodeLayers.size(), [this,&loadMsg, &rect](const int layerNum){
		int currentThread = ThreadPool::GetThreadNum();
		// #ifndef NDEBUG
		// snprintf(loadMsg, sizeof(loadMsg), preFmtStr, layerNum);
		// pmLoadScreen.AddMessage(loadMsg);
		// #endif

		NodeLayer& layer = nodeLayers[layerNum];

		InitNodeLayer(layerNum, rect);

		INode* rootNode = layer.GetPoolNode(0);

		std::vector<SRectangle> rootRects;
		rootRects.reserve(layer.GetRootNodeCount());

		int rootXMax = rootNode->xmax();
		int rootZMax = rootNode->zmax();
		for (int hmz = rect.z1; hmz < rect.z2; hmz += rootZMax) {
			assert(hmz + rootZMax <= rect.z2);
			for (int hmx = rect.x1; hmx < rect.x2; hmx += rootXMax) {
				assert(hmx + rootXMax <= rect.x2);
				rootRects.emplace_back(hmx, hmz, hmx + rootXMax, hmz + rootZMax);
			}
		}
		
		std::for_each(rootRects.begin(), rootRects.end(), [this, layerNum, currentThread](auto &rect){
			UpdateNodeLayer(layerNum, rect, currentThread);
		});
	});

	// Full map-wide allocations have been made, we shouldn't need that much memory in future.
	for (int i = 0; i <ThreadPool::GetNumThreads(); ++i) {
		updateThreadData[i].Reset();
	}

	streflop::streflop_init<streflop::Simple>();
}

void QTPFS::PathManager::InitRootSize(const SRectangle& r) {
	RECOIL_DETAILED_TRACY_ZONE;
	// setup the root node system
	int width = r.x2 - r.x1;
	int height = r.z2 - r.z1;
	LOG("%s: map root size is (%d, %d)", __func__, width, height);

	// Optimal function of QTPFS relies on power of 2 squares. Find the largest 2^x squares that
	// fit the map. 64 is the smallest as understood by map makers. So use 32 here to detect a map
	// that falls below that threshold.
	rootSize = QTPFS_BAD_ROOT_NODE_SIZE;
	int limit = std::min(width, height);
	for (int factor = rootSize<<1; factor <= limit; factor <<= 1) {
		if (width % factor == 0 && height % factor == 0)
			rootSize = factor;
	}
	// Don't allow the root size to get too big to limit memory usage. (sizes given with 60 movetypes and 6 threads)
	// Nine Metal Islands could have gone to 2048x2048 root node (2880MB)
	// Nine Metal Islands has 512x512 nodes in each corner (180MB)
	// 256x256 (45MB)
	// Quick Silver has 128x128 nodes in corners (11.25 MB)
	int maxRootSize = QTPFS_MAX_NODE_SIZE;
	rootSize = rootSize > maxRootSize ? maxRootSize : rootSize;
	LOG("%s: root node size is set to: %d", __func__, rootSize);

	assert(rootSize != QTPFS_BAD_ROOT_NODE_SIZE);
	if (rootSize == QTPFS_BAD_ROOT_NODE_SIZE)
		LOG("%s: Warning! Map width and height highest common factor is smaller than QTPFS is designed to handle.", __func__);

	// Prevent too big a size being picked due to 15 levels of node Indexing possible: 2^(steps -1) (steps=(bits-2)/2)
	constexpr float maxNodeLevels = ((sizeof(uint32_t)*4)-2);
	uint32_t maxNodeSize = math::pow(2.f, maxNodeLevels);
	rootSize = rootSize > maxNodeSize ? maxNodeSize : rootSize;
}

void QTPFS::PathManager::InitNodeLayer(unsigned int layerNum, const SRectangle& r) {
	RECOIL_DETAILED_TRACY_ZONE;
	NodeLayer& nl = nodeLayers[layerNum];

	nl.Init(layerNum);

	// TODO: partial zones just in case %64 != 0? need to check tessalation off map is okay.
	//       This should not happen.
	int numRootCount = 0;
	int zRootNodes = 0;
	for (int z = r.z1; z < r.z2; z += rootSize) {
		for (int x = r.x1; x < r.x2; x += rootSize) {
			int idx = nl.AllocPoolNode(nullptr, -1, x, z, x + rootSize, z + rootSize);

			// Keep the counters balanced.
			nl.IncreaseOpenNodeCounter();

			// LOG("%s: %d root node [%d,%d:%d,%d] allocated.", __func__
			// 		, idx, x, z, x + rootSize, z + rootSize);

			assert(idx == numRootCount);
			numRootCount++;
		}
		zRootNodes++;
	}

	nl.SetNumLeafNodes(numRootCount);
	
	// Root Mask is the part of the node number reserved for root nodes.
	// This limits the maximum number of levels of nodes we can create unique, position ids for.
	// (MAX_DEPTH)
	uint32_t rootShift = 30;
	for (int factor = 4; factor < numRootCount; factor <<= 2) {
		rootShift -= 2;
	}
	QTNode::MAX_DEPTH = (rootShift)/QTPFS_NODE_NUMBER_SHIFT_STEP;
	uint32_t rootMask = (~0) << rootShift;
	nl.SetRootMask(rootMask);

	// LOG("rootShift = %d, maxDepth = %d", rootShift, rootMask);
	// LOG("%s: %d root nodes allocated (%d x %d) mask: 0x%08x.", __func__
	// 		, numRootCount, (numRootCount/zRootNodes), zRootNodes, rootMask);

	for (int i=0; i<numRootCount; ++i) {
		nl.GetPoolNode(i)->SetNodeNumber(i << rootShift);
		// LOG("%s: check %x (%x) == %x (%x)", __func__, i, i << rootShift
		// 	, (nl.GetPoolNode(i)->GetNodeNumber() & rootMask) >> rootShift
		// 	, nl.GetPoolNode(i)->GetNodeNumber()
		// 	);
		assert(i == (nl.GetPoolNode(i)->GetNodeNumber() & rootMask) >> rootShift);
	}
	nl.SetRootNodeCountAndDimensions(numRootCount, (numRootCount/zRootNodes), zRootNodes, rootSize);
	assert((numRootCount/zRootNodes)*zRootNodes == numRootCount);
}



// __FORCE_ALIGN_STACK__

// called in the non-staggered (#ifndef QTPFS_STAGGERED_LAYER_UPDATES)
// layer update scheme and during initialization; see ::TerrainChange
void QTPFS::PathManager::UpdateNodeLayer(unsigned int layerNum, const SRectangle& rect, int currentThread) {
	const MoveDef* md = moveDefHandler.GetMoveDefByPathType(layerNum);

	if (!IsFinalized())
		return;

	// adjust the borders so we are not left with "rims" of
	// impassable squares when eg. a structure is reclaimed

	const bool isIncrementalUpdate = (rect.x1 == 0 && rect.x2 == 0);
	SRectangle r(rect);

	if (isIncrementalUpdate) {
		auto& nlMapDmgTracker = nodeLayersMapDamageTrack.mapChangeTrackers[layerNum];

		// No more damaged areas. Finish up.
		if (nlMapDmgTracker.damageQueue.size() == 0) { return; }

		const int sectorId = nodeLayersMapDamageTrack.mapChangeTrackers[layerNum].damageQueue.front();
		const int blockIdxX = (sectorId % nodeLayersMapDamageTrack.width) * nodeLayersMapDamageTrack.cellSize;
		const int blockIdxY = (sectorId / nodeLayersMapDamageTrack.width) * nodeLayersMapDamageTrack.cellSize;

		assert(sectorId < nlMapDmgTracker.damageMap.size());
		nlMapDmgTracker.damageMap[sectorId] = false;
		nlMapDmgTracker.damageQueue.pop_front();

		r = SRectangle
			( blockIdxX
			, blockIdxY
			, blockIdxX + DAMAGE_MAP_BLOCK_SIZE
			, blockIdxY + DAMAGE_MAP_BLOCK_SIZE
			);
	}

	INode* containingNode = nodeLayers[layerNum].GetNodeThatEncasesPowerOfTwoArea(r);
	SRectangle re(containingNode->xmin(), containingNode->zmin(), containingNode->xmax(), containingNode->zmax());

	assert(re.x1 <= r.x1);
	assert(re.z1 <= r.z1);
	assert(re.x2 >= r.x2);
	assert(re.z2 >= r.z2);

	// { bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1);
	// 	if (printMoveInfo) {
	// 		for (const int unitID: selectedUnitsHandler.selectedUnits) {
	// 			printMoveInfo = unitHandler.GetUnit(unitID)->moveDef->pathType == layerNum;
	// 		}
	// 		if (printMoveInfo){
	// 		LOG("%s re (%d) [%d,%d][%d,%d]", __func__
	// 				, layerNum, re.x1, re.z1, re.x2, re.z2);
	// 		}}}

	updateThreadData[currentThread].InitUpdate(r, *containingNode, *md, currentThread);
	const bool needTesselation = [=]() {
		if (isIncrementalUpdate)
			return this->nodeLayers[layerNum].IncrementalUpdate(this->updateThreadData[currentThread]);
		else
			return this->nodeLayers[layerNum].InitialUpdate(this->updateThreadData[currentThread]);
	}();

	// process the affected root nodes.

	// LOG("%s: [%d] needTesselation=%d, wantTesselation=%d", __func__, layerNum, (int)needTesselation, (int)wantTesselation);

	if (needTesselation) {
		SRectangle ur(re.x1, re.z1, re.x2, re.z2);
		auto& nodeLayer = nodeLayers[layerNum];

		containingNode->PreTesselate(nodeLayers[layerNum], re, ur, 0, &updateThreadData[currentThread]);
		#ifndef NDEBUG
		{
			auto& nl = nodeLayers[layerNum];
			assert(nl.GetNumOpenNodes() + nl.GetNumClosedNodes() == nl.GetNumLeafNodes());
		}
		#endif

		pathCache.SetLayerPathCount(layerNum, INITIAL_PATH_RESERVE);
		pathCache.MarkDeadPaths(re, nodeLayer);

		#ifndef QTPFS_CONSERVATIVE_NEIGHBOR_CACHE_UPDATES
		nodeLayers[layerNum].ExecNodeNeighborCacheUpdates(ur, updateThreadData[currentThread]);
		#endif
	}
}

// note that this is called twice per object:
// height-map changes, then blocking-map does
void QTPFS::PathManager::TerrainChange(unsigned int x1, unsigned int z1,  unsigned int x2, unsigned int z2, unsigned int type) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (!IsFinalized())
		return;

	MapChanged(x1, z1, x2, z2);
}

void QTPFS::PathManager::MapChanged(int x1, int y1, int x2, int y2) {
	RECOIL_DETAILED_TRACY_ZONE;
	const int res = DAMAGE_MAP_BLOCK_SIZE;

	const auto layers = nodeLayers.size();
	for (int i = 0; i < layers; ++i) {
		auto& nlChangeTracker = nodeLayersMapDamageTrack.mapChangeTrackers[i];
		const int w = nodeLayersMapDamageTrack.width;
		const int h = nodeLayersMapDamageTrack.height;

		auto* moveDef = moveDefHandler.GetMoveDefByPathType(i);
		int xsizeh = moveDef->xsizeh;
		int zsizeh = moveDef->zsizeh;
		const int2 min  { std::max((x1-xsizeh) / res, 0)
						, std::max((y1-zsizeh) / res, 0)};
		const int2 max  { std::min((x2+xsizeh) / res, (w-1))
						, std::min((y2+zsizeh) / res, (h-1))};

		for (int y = min.y; y <= max.y; ++y) {
			int quad = min.x + y*w;
			for (int x = min.x; x <= max.x; ++x, ++quad) {
				if (!nlChangeTracker.damageMap[quad]) {
					nlChangeTracker.damageMap[quad] = true;
					nlChangeTracker.damageQueue.emplace_back(quad);
				}
			}	
		}
	}
}

void QTPFS::PathManager::Update() {
	SCOPED_TIMER("Sim::Path");
	{
		systemUtils.NotifyUpdate();
	}
	{
		SCOPED_TIMER("Sim::Path::MapUpdates");

		RequestMaxSpeedModRefreshForLayer(0);

		auto numBlocksToUpdate = [this](int layerNum) {
			int blocksToUpdate = 0;
			int updatedBlocks = nodeLayersMapDamageTrack.mapChangeTrackers[layerNum].damageQueue.size();
			{
				constexpr int BLOCKS_TO_UPDATE = 16;
				const int progressiveUpdates = std::ceil(updatedBlocks * (1.f / (BLOCKS_TO_UPDATE<<3)) * modInfo.pfUpdateRateScale);
				constexpr int MIN_BLOCKS_TO_UPDATE = 0;
				constexpr int MAX_BLOCKS_TO_UPDATE = std::max<int>(BLOCKS_TO_UPDATE, MIN_BLOCKS_TO_UPDATE);

				blocksToUpdate = std::clamp(progressiveUpdates, MIN_BLOCKS_TO_UPDATE, MAX_BLOCKS_TO_UPDATE);
			
				// LOG("[%d] blocksToUpdate=%d updatedBlocks=%d [%f]"
				// 		, layerNum, blocksToUpdate, updatedBlocks, modInfo.pfUpdateRateScale);
			}
			return blocksToUpdate;
		};

		SRectangle rect(0,0,0,0);
		for_mt(0, nodeLayers.size(), [this, &rect, &numBlocksToUpdate](const int index) {
			int curThread = ThreadPool::GetThreadNum();
			int layerNum = nodeLayerUpdatePriorityOrder[index];
			int blocksToUpdate = numBlocksToUpdate(layerNum);
			for (int i = 0; i < blocksToUpdate; ++i) { UpdateNodeLayer(layerNum, rect, curThread); }
		});

		// Mark all dirty paths so that they can be recalculated
		int pathsMarkedDirty = 0;
		for (auto& layerDirtyPaths : pathCache.dirtyPaths) {
			// LOG("%s: start: %d", __func__, (int)layerDirtyPaths.size());
			for (auto dirtyPathDetail : layerDirtyPaths) {
				QTPFS::entity pathEntity = dirtyPathDetail.pathEntity;

				// May have already been deleted.
				if (!registry.valid(pathEntity)) { continue; }

				// assert(!registry.all_of<PathIsDirty>(pathEntity));
				// LOG("%s: alreadyDirty=%d, pathEntity=%x", __func__, (int)registry.all_of<PathIsDirty>(pathEntity)
				// 		, (int)pathEntity);

				// If the path was going to be deleted anyway, then remove it instead of marking for rebuild.
				if (registry.all_of<PathDelayedDelete>(pathEntity)) {
					DeletePathEntity(pathEntity);
					continue;
				}

				// TODO: perhaps not mark paths multiple times if multiple blocks are updated in same frame?
				// if (registry.all_of<PathIsDirty>(pathEntity)) { continue; }

				if ( !registry.all_of<PathIsDirty>(pathEntity) ) {
					if (dirtyPathDetail.clearPath) {
						registry.emplace<PathIsDirty>(pathEntity);
						pathsMarkedDirty++;
					}
				// currently always true
				//if (dirtyPathDetail.clearSharing) {
					RemovePathFromShared(pathEntity);
					RemovePathFromPartialShared(pathEntity);
				}

					// The path may still be fine for owner, even if it can't be shared any more.
					auto& path = registry.get<IPath>(pathEntity);
					assert(path.GetOwner() != nullptr);
					assert(path.IsSynced());

					if (dirtyPathDetail.autoRepathTrigger > 0) {
						// Rather than repath immediately we can defer the repath until the unit
						// gets closer to the damaged area.
						const unsigned int currRepathTrigger = path.GetRepathTriggerIndex();
						if (currRepathTrigger == 0 || currRepathTrigger > dirtyPathDetail.autoRepathTrigger) {
							path.SetRepathTriggerIndex(dirtyPathDetail.autoRepathTrigger);
							path.SetBoundingBox();
						}
					}
					// LOG("%s: clean path pos %d -> %d", __func__
					// 	, path.GetFirstNodeIdOfCleanPath(), dirtyPathDetail.nodesAreCleanFromNodeId);

					// Ensure the last clean node is always taken in case multiple nodes were processed this frame.
					const int curCleanNodeId = path.GetFirstNodeIdOfCleanPath();
					const int nextCleanNodeId = dirtyPathDetail.nodesAreCleanFromNodeId;
					path.SetFirstNodeIdOfCleanPath(std::max(curCleanNodeId, nextCleanNodeId));
					// if (path.IsBoundingBoxOverriden())
						path.SetBoundingBox();
				//}
			}
			layerDirtyPaths.clear();
			// LOG("%s: end: %d", __func__, (int)layerDirtyPaths.size());
		}
		if (refreshDirtyPathRateFrame == QTPFS_LAST_FRAME && pathsMarkedDirty > 0)
			refreshDirtyPathRateFrame = gs->frameNum + GAME_SPEED;
	}
	{
		ThreadUpdate();
	}
}

__FORCE_ALIGN_STACK__
void QTPFS::PathManager::ThreadUpdate() {
	QueueDeadPathSearches();
	ExecuteQueuedSearches();
}


bool QTPFS::PathManager::InitializeSearch(QTPFS::entity searchEntity) {
	ZoneScoped;
	PathSearch* search = GetSearch(searchEntity);

	if (search->initialized)
		return true;

	// if (search->Getowner() != nullptr && 2102 == search->Getowner()->id)
	// 	LOG("%s: search prep (%d)", __func__, search->GetID());

	int pathType = search->GetPathType();

	assert(pathType < nodeLayers.size());
	NodeLayer& nodeLayer = nodeLayers[pathType];

	QTPFS::entity pathEntity = (QTPFS::entity)search->GetID();
	if (registry.valid(pathEntity) && !registry.all_of<PathDelayedDelete>(pathEntity)) {
		assert((registry.any_of<IPath, UnsyncedIPath, ExternallyManagedSyncedIPath>(pathEntity)));
		IPath* path = GetPath(pathEntity);
		assert(path->GetPathType() == pathType);

		// Somehow units can get wiped without triggering a delete. This is a catch for that until the
		// cause can be found and resolved.
		const CSolidObject* owner = path->GetOwner();
		if (owner != nullptr) {
			if (owner->objectUsable == false)
				return false;
		}

		search->Initialize(&nodeLayer, path->GetSourcePoint(), path->GetGoalPosition(), path->GetOwner());
		path->SetHash(search->GetHash());
		path->SetVirtualHash(search->GetPartialSearchHash());

		// LOG("%s: search vhash %x%x", __func__, int(search->GetPartialSearchHash() >> 32), int(search->GetPartialSearchHash() & 32));
		// LOG("%s: path vhash %x%x", __func__, int(path->GetVirtualHash() >> 32), int(path->GetVirtualHash() & 32));
		// assert(search->GetPartialSearchHash() == path->GetVirtualHash());

		if (path->GetOwner() != nullptr) {
			if (search->GetHash() != QTPFS::BAD_HASH) {
				assert(!registry.all_of<SharedPathChain>(pathEntity));
				SharedPathMap::iterator sharedPathsIt = sharedPaths.find(path->GetHash());
				if (sharedPathsIt == sharedPaths.end()) {
					registry.emplace<SharedPathChain>(pathEntity, pathEntity, pathEntity);
					sharedPaths[path->GetHash()] = pathEntity;
					search->fullSharedPathHead = pathEntity;
				} else {
					search->fullSharedPathHead = sharedPathsIt->second;
					linkedListHelper.InsertChain<SharedPathChain>(sharedPaths[path->GetHash()], pathEntity);
				}
			}
			if (search->GetPartialSearchHash() != QTPFS::BAD_HASH) {
				assert(path->GetVirtualHash() != QTPFS::BAD_HASH);
				assert(!registry.all_of<PartialSharedPathChain>(pathEntity));
				PartialSharedPathMap::iterator partialSharedPathsIt = partialSharedPaths.find(path->GetVirtualHash());
				if (partialSharedPathsIt == partialSharedPaths.end()) {
					registry.emplace<PartialSharedPathChain>(pathEntity, pathEntity, pathEntity);
					partialSharedPaths[path->GetVirtualHash()] = pathEntity;
					search->partSharedPathHead = pathEntity;
				} else {
					search->partSharedPathHead = partialSharedPathsIt->second;
					linkedListHelper.InsertChain<PartialSharedPathChain>(partialSharedPaths[path->GetVirtualHash()], pathEntity);
				}
			}
		}

		// We don't want to work on the live path in the background tasks because it can be changed and that would
		// potentially cause a desync.
		IPath* searchPath = GetSearchPath(pathEntity);
		(*searchPath) = (*path);

		search->initialized = true;
	} else // If the underlying path is missing for some reason, then this search is invalid.
		return false;

	return search->initialized;
}

void QTPFS::PathManager::ReadyQueuedSearches() {
	RECOIL_DETAILED_TRACY_ZONE;
	{
		// Only synced searches get queued for batch processing.
		auto pathView = registry.view<PathSearch>();

		// Go through in reverse order to minimize reshuffling EnTT will do with the grouping.
		std::for_each(pathView.rbegin(), pathView.rend(), [this](QTPFS::entity entity){
			if (!registry.all_of<ProcessPath>(entity)) {
				if (InitializeSearch(entity))
					registry.emplace<ProcessPath>(entity);
			}
		});
	}
	{
		auto pathView = registry.view<PathSearch>();

		// Any requests that cannot be processed should be removed. We can't do that with the r*
		// iterators because that will break them.
		std::for_each(pathView.begin(), pathView.end(), [this, &pathView](QTPFS::entity entity){
			if (!registry.all_of<ProcessPath>(entity)){
				// Get the search PathSearch and then the path it is connected to a remove the search.
				// find the path that is connected to this search
				auto& pathSearch = pathView.get<PathSearch>(entity);
				auto pathEntity = (QTPFS::entity)pathSearch.GetID();
				RemovePathSearch(pathEntity);

				// Just in case there isn't a back reference on the path then clear remove this search.
				if (registry.valid(entity))
					DestroyPathSearchEntity(entity);
			}
		});
	}
}

// Common process path search entries during MT Sections
void QTPFS::PathManager::ProcessPathSearch(int i, bool shouldBeRaw){
	auto pathView = registry.group<PathSearch, ProcessPath>();
	QTPFS::entity pathSearchEntity = pathView.begin()[i];

	assert(registry.valid(pathSearchEntity));
	assert(registry.all_of<PathSearch>(pathSearchEntity));

	PathSearch* search = &pathView.get<PathSearch>(pathSearchEntity);

	if (search->rawPathCheck == shouldBeRaw) {
		int pathType = search->GetPathType();
		NodeLayer& nodeLayer = nodeLayers[pathType];

		ExecuteSearch(search, nodeLayer, pathType, false);
	}
};

void QTPFS::PathManager::ExecuteQueuedSearches() {
	ZoneScoped;

	ReadyQueuedSearches();

	// Only synced searches get queued for batch processing.
	auto& comp = systemGlobals.GetSystemComponent<SyncUpdatedPathsComponent>();

	{
		auto pathSearchView = registry.group<PathSearch, ProcessPath>();
		bool rawPathsProcessed = false;

		// Process the path searches that have been marked as raw search. These searches are dependent on map data and as
		// such cannot be safely processed in the background without risking desyncs.
		for_mt(0, pathSearchView.size(), std::function<void(int)>{[this](int i){
			SCOPED_MT_TIMER("Sim::Path::RawSearches");
			ProcessPathSearch(i, true); // shouldBeRaw = true
		}});

		// Clean up raw path searches and queue any new regular path searches needed, which can be processed in the
		// background.
		for (auto pathSearchEntity : pathSearchView) {
			assert(registry.valid(pathSearchEntity));
			assert(registry.all_of<PathSearch>(pathSearchEntity));

			PathSearch* search = &pathSearchView.get<PathSearch>(pathSearchEntity);
			if (search->rawPathCheck) {
				FinishPathSearch(this, search);

				// LOG("%s: delete search %x", __func__, entt::to_integral(pathSearchEntity));
				if (registry.valid(pathSearchEntity))
					DestroyPathSearchEntity(pathSearchEntity);

				rawPathsProcessed = true;
			}
		}

		// Raw path searches may have failed and now we need to use a regular path search. We do this now to avoid any
		// additional frame delays in resolving the path requests.
		if (rawPathsProcessed)
			ReadyQueuedSearches();
	}
	{
		// Remember: Do NOT impact this group while the background tasks are running!
		auto pathSearchView = registry.group<PathSearch, ProcessPath>();

		// Execute pending searches collected via RequestPath and QueueDeadPathSearches in the background. This allows
		// other systems to run while the path searches are being processed, which can be a significant time saving if
		// there are many path searches to process.
		comp.backgroundTask = for_mt_background(0, pathSearchView.size(), std::function<void(int)>{[this](int i){
			SCOPED_MT_TIMER("Sim::Path::Requests");
			ProcessPathSearch(i, false); // shouldBeRaw = false
		}});
	}
}

// #pragma GCC push_options
// #pragma GCC optimize ("O0")

bool QTPFS::PathManager::ExecuteSearch(
	PathSearch* search,
	NodeLayer& nodeLayer,
	unsigned int pathType,
	bool immediateSearch
) {
	ZoneScoped;

	BasicTimer searchTimer(0);

	QTPFS::entity pathEntity = (QTPFS::entity)search->GetID();
	if (!registry.valid(pathEntity))
		return false;

	auto GetSearchModePath = [=]() {
		if (immediateSearch)
			return GetPath(pathEntity);

		return GetSearchPath(pathEntity);
	};

	// Initialize independent path data to avoid impacting the rest of simulation. This will be moved back at a
	// suitable sync-safe time later. Immediate path requests work directly on the live path because there is no
	// delay.
	IPath* path = GetSearchModePath();

	int currentThread = ThreadPool::GetThreadNum();

	assert(search != nullptr);
	assert(search->initialized);

	// temp-path might have been removed already via
	// DeletePath before we got a chance to process it
	if (path == nullptr)
		return false;

	assert(path->GetID() == search->GetID());

	bool forceFullPath = false;
	QTPFS::entity chainHeadEntity = entt::null;
	QTPFS::entity partialChainHeadEntity = entt::null;

	// TODO: make a function?
	if (path->GetOwner() != nullptr)
	{
		// Always clear incase the situation has changed since the last frame, if a partial search
		// was intended, but not carried out. For example, a full-path share wait.
		search->doPartialSearch = false;

		if (search->allowPartialSearch)
		{
			if (search->partSharedPathHead != entt::null && QTPFS::registry.valid(search->partSharedPathHead)) {
				assert(path->GetVirtualHash() != QTPFS::BAD_HASH);
				partialChainHeadEntity = search->partSharedPathHead;
				if (partialChainHeadEntity != pathEntity) {
					bool pathIsCopyable = !registry.all_of<PathSearchRef>(partialChainHeadEntity);
					if (!pathIsCopyable) {

						// if (search->Getowner() != nullptr && 2102 == search->Getowner()->id)
						// 	LOG("%s: partial-share search waiting (%d)", __func__, search->GetID());

						search->pathRequestWaiting = true;
						return false;
					}

					#ifndef NDEBUG
					// the head of a partial share chain must be a non-trivial path that went through TracePath and
					// had a real bounding box computed from node boundaries. If this assert triggers then it means
					// that the headPath is a straight line between two points and has nothing to share.
					IPath* headPath = registry.try_get<IPath>(partialChainHeadEntity);
					assert(headPath != nullptr);
					assert(headPath->IsBoundingBoxOverriden());
					#endif
					
					// proceed with the search.
					search->pathRequestWaiting = false;
					search->doPartialSearch = true;

					// if (search->Getowner() != nullptr && 2102 == search->Getowner()->id)
					// 	LOG("%s: partial search start (%d)", __func__, search->GetID());

				}
			}
		}
		{
			if (search->fullSharedPathHead != entt::null && QTPFS::registry.valid(search->fullSharedPathHead)) {
				chainHeadEntity = search->fullSharedPathHead;
				// LOG("%s: chainHeadEntity %x != pathEntity %x", __func__
				// 		, entt::to_integral(chainHeadEntity), entt::to_integral(pathEntity));
				if (chainHeadEntity != pathEntity){
					bool pathIsCopyable = !registry.all_of<PathSearchRef>(chainHeadEntity);
					if (pathIsCopyable) {
						// LOG("%s: pathEntity %x pathIsCopyable = %d", __func__
						// 		, entt::to_integral(pathEntity), int(pathIsCopyable));
						auto& headChainPath = registry.get<IPath>(chainHeadEntity);
						search->SharedFinalize(&headChainPath, path);
						search->pathRequestWaiting = false;

						// if (search->Getowner() != nullptr && 2102 == search->Getowner()->id)
						// 	LOG("%s: full shared (%d)", __func__, search->GetID());
					}
					else {
						if (search->partSharedPathHead != entt::null && QTPFS::registry.valid(search->partSharedPathHead)) {
							assert(path->GetVirtualHash() != QTPFS::BAD_HASH);
							partialChainHeadEntity = search->partSharedPathHead;

							// If this path is the head of a partial path, we need to make sure it isn't blocking the head
							// of the full path copy (which would cause a deadlock.)
							if (partialChainHeadEntity == pathEntity) {
								auto& fullCopyHeadChainPath = registry.get<IPath>(chainHeadEntity);
								if (fullCopyHeadChainPath.GetVirtualHash() == path->GetVirtualHash()) {
									// we have deadlock, so force this path to be processed now.
									forceFullPath = true;
									search->pathRequestWaiting = false;
								}
							}
						}

						if (!forceFullPath) {
							search->pathRequestWaiting = true;

							// if (search->Getowner() != nullptr && 2102 == search->Getowner()->id)
							// 	LOG("%s: fully-shared search waiting (%d)", __func__, search->GetID());
						}
					}
					if (!forceFullPath)
						return false;
				}
			}
		}
	}

	// Only the head of a partial path share is allowed to attempt a path repair. It doesn't make sense for a
	// subordinate sharing path to attempt a repair, because the repair should be done already.
	bool isHeadOfPathSharing = !search->doPartialSearch;
	search->tryPathRepair &= isHeadOfPathSharing;

	search->InitializeThread(&searchThreadData[currentThread], path);

	if (search->doPartialSearch) {
		auto* sharedPath = &registry.get<IPath>(partialChainHeadEntity);
		search->LoadPartialPath(sharedPath);
	} else if (search->doPathRepair) {
		search->LoadRepairPath(path);
	}

	if (search->Execute(searchStateOffset)) {
		search->Finalize(path);

		#ifdef QTPFS_TRACE_PATH_SEARCHES
		pathTraces[path->GetID()] = search->GetExecutionTrace();
		#endif
	}

	path->SetSearchTime(searchTimer.GetDuration());

	return true;
}

void QTPFS::PathManager::QueueDeadPathSearches() {
	ZoneScoped;

	// Only owned can be marked as dead.
	auto pathUpdatesView = registry.view<IPath, PathIsToBeUpdated>();
	if (pathUpdatesView.size_hint() == 0 && gs->frameNum >= refreshDirtyPathRateFrame) {
		// LOG("%s: pathUpdatesView=%d,frame=%d>%d", __func__
		// 		, (int)pathUpdatesView.size_hint(), gs->frameNum, refreshDirtyPathRateFrame
		// 		);
		auto dirtyView = registry.view<PathIsDirty>();
		auto pathsToUpdate = dirtyView.size();
		// LOG("%s: dirtyView=%d", __func__, (int)pathsToUpdate);
		if (pathsToUpdate > 0) {
			for (auto path : dirtyView) {
				assert(!registry.any_of<PathIsToBeUpdated>(path));
				registry.emplace<PathIsToBeUpdated>(path);
			}
			updateDirtyPathRate = pathsToUpdate / GAME_SPEED;
			updateDirtyPathRemainder = pathsToUpdate % GAME_SPEED;
			// LOG("%s: updateDirtyPathRate=%d,updateDirtyPathRemainder=%d", __func__
			// 		, updateDirtyPathRate, updateDirtyPathRemainder
			// 		);
		}
		refreshDirtyPathRateFrame = QTPFS_LAST_FRAME;
	}
	
	if (pathUpdatesView.size_hint() > 0) {
		auto rate = std::min(updateDirtyPathRate + (updateDirtyPathRemainder-- > 0), (int)pathUpdatesView.size_hint());
		updateDirtyPathRemainder += (updateDirtyPathRemainder < 0);

		std::for_each_n(pathUpdatesView.begin(), rate, [this, &pathUpdatesView](auto entity) {
			assert(registry.valid(entity));
			IPath* path = &pathUpdatesView.get<IPath>(entity);

			assert(path->GetPathType() < moveDefHandler.GetNumMoveDefs());
			const MoveDef* moveDef = moveDefHandler.GetMoveDefByPathType(path->GetPathType());

			assert(registry.all_of<PathIsToBeUpdated>(entity));
			registry.remove<PathIsToBeUpdated>(entity);
			registry.emplace_or_replace<PathUpdatedCounterIncrease>(entity);

			RequeueSearch(path, true, false, true);
		});
	}
}

// #pragma GCC push_options
// #pragma GCC optimize ("O0")

unsigned int QTPFS::PathManager::QueueSearch(
	const CSolidObject* object,
	const MoveDef* moveDef,
	const float3& sourcePoint,
	const float3& targetPoint,
	const float radius,
	const bool synced,
	const bool externalRequest,
	const bool allowRawSearch
) {
	RECOIL_DETAILED_TRACY_ZONE;
	assert(!ThreadPool::IsInMultiThreadedSection());

	// NOTE:
	//     all paths get deleted by the cache they are in;
	//     all searches get deleted by subsequent Update's
	// NOTE:
	//     the path-owner object handed to us can never become
	//     dangling (even with delayed execution) because ~GMT
	//     calls DeletePath, which ensures any path is removed
	//     from its cache before we get to ExecuteSearch

	QTPFS::entity pathEntity = registry.create();
	assert((!registry.any_of<IPath, UnsyncedIPath, ExternallyManagedSyncedIPath>(pathEntity)));

	auto createNewPath = [](QTPFS::entity entityId, bool synced, bool externalRequest) -> IPath* {
		if (!synced)
			return &(registry.emplace<UnsyncedIPath>(entityId));
		else if (externalRequest)
			return &(registry.emplace<ExternallyManagedSyncedIPath>(entityId));
		else
			return &(registry.emplace<IPath>(entityId));

	};

	auto createNewSearch = [](QTPFS::entity entityId, bool synced, bool externalRequest) -> PathSearch* {
		if (!synced)
			return &(registry.emplace<UnsyncedPathSearch>(entityId, PATH_SEARCH_ASTAR));
		else if (externalRequest)
			return &(registry.emplace<ExternallyManagedPathSearch>(entityId, PATH_SEARCH_ASTAR));
		else
			return &(registry.emplace<PathSearch>(entityId, PATH_SEARCH_ASTAR));
	};

	IPath* newPath = createNewPath(pathEntity, synced, externalRequest);

	// Requeue demands get changed in a multi-threaded section, so we can't add them on demand.
	// Unsynced paths don't requeue their searches (also, unsynced paths cannot have owning units.)
	// Also, externally managed synced paths don't requeue their searches.
	if (synced && !externalRequest) {
		registry.emplace<PathRequeueSearch>(pathEntity, false);
		registry.emplace<SearchModeIPath>(pathEntity);
	} else
		object = nullptr;

	QTPFS::entity searchEntity = registry.create();
	PathSearch* newSearch = createNewSearch(searchEntity, synced, externalRequest);

	assert(targetPoint.x >= 0.f);
	assert(targetPoint.z >= 0.f);
	assert(targetPoint.x / SQUARE_SIZE <= mapDims.mapx);
	assert(targetPoint.z / SQUARE_SIZE <= mapDims.mapy);

	assert(newPath != nullptr);
	assert(newSearch != nullptr);

	// 0 is considered a null path. Entity id 0 should have been taken by the pathing system itself.
	assert(pathEntity != (QTPFS::entity)0);

	// NOTE:
	//     the unclamped end-points are temporary
	//     zero is a reserved ID, so pre-increment
	newPath->SetID((int)pathEntity);
	newPath->SetRadius(radius);
	newPath->SetSynced(synced);
	newPath->AllocPoints(2);
	newPath->AllocNodes(0);
	newPath->SetOwner(object);
	newPath->SetSourcePoint(sourcePoint.cClampInBounds());
	newPath->SetTargetPoint(targetPoint.cClampInBounds());
	newPath->SetGoalPosition(newPath->GetTargetPoint());
	newPath->SetPathType(moveDef->pathType);

	registry.emplace<PathIsTemp>(pathEntity);
	registry.emplace<PathSearchRef>(pathEntity, searchEntity);

	newSearch->SetID(newPath->GetID());
	newSearch->SetTeam((object != nullptr)? object->team: teamHandler.ActiveTeams());
	newSearch->SetPathType(newPath->GetPathType());
	newSearch->SetGoalDistance(newPath->GetRadius());
	newSearch->rawPathCheck = allowRawSearch;
	newSearch->allowPartialSearch = !allowRawSearch;
	newSearch->initialized = false;
	newSearch->synced = synced;

	// if (object != nullptr && object->id == 25278) {
	// 	CUnit *unit = object != nullptr ? dynamic_cast<CUnit*>(const_cast<CSolidObject*>(object)) : nullptr;
	// 	LOG("%s: NEW %s (%x) %d ", __func__
	// 			, unit != nullptr ? unit->unitDef->name.c_str() : "non-unit"
	// 			, newPath->GetID()
	// 			, moveDef->pathType
	// 			);

	// 	LOG("%s: NEW [%d] (%f,%f) -> (%f,%f)", __func__, newPath->GetPathType()
	// 			, sourcePoint.x, sourcePoint.z, targetPoint.x, targetPoint.z);
	// }

	return (newPath->GetID());
}

// #pragma GCC pop_options

unsigned int QTPFS::PathManager::RequeueSearch(
	IPath* oldPath, const bool allowRawSearch, const bool allowPartialSearch, const bool allowRepair
) {
	RECOIL_DETAILED_TRACY_ZONE;
	assert(!ThreadPool::IsInMultiThreadedSection());
	QTPFS::entity pathEntity = QTPFS::entity(oldPath->GetID());

	bool pathIsBeingProcessed = registry.any_of<PathIsDirty, PathSearchRef>(pathEntity);

	if (registry.any_of<PathDelayedDelete>(pathEntity)){
		RemovePathFromShared(pathEntity);
		RemovePathFromPartialShared(pathEntity);
		return (oldPath->GetID());
	}

	// If a path request is already in progress then don't create another one.
	if (registry.any_of<PathSearchRef>(pathEntity))
		return (oldPath->GetID());

	const CSolidObject* object = oldPath->GetOwner();
	if (object != nullptr && object->objectUsable == false) {
		DeletePathEntity(pathEntity);
		return 0;
	}

	// Always create the search object first to ensure pathEntity can never be 0 (which is
	// considered a non-path)
	QTPFS::entity searchEntity = registry.create();
	PathSearch* newSearch = &registry.emplace<PathSearch>(searchEntity, PATH_SEARCH_ASTAR);
	assert(oldPath != nullptr);
	assert(newSearch != nullptr);
	assert(oldPath->GetID() != 0);
	assert(pathEntity != entt::null);

	const float3& pos = (object != nullptr)? object->pos: oldPath->GetSourcePoint();

	RemovePathFromShared(pathEntity);
	RemovePathFromPartialShared(pathEntity);

	oldPath->SetHash(QTPFS::BAD_HASH);
	// oldPath->SetNextPointIndex(0); - don't clear, will mess up active units.
	// oldPath->SetNumPathUpdates(oldPath->GetNumPathUpdates() + 1);

	// start re-request from the current point
	// along the path, not the original source
	// oldPath->AllocPoints(2); - don't clear, will mess up active units.
	//oldPath->AllocNodes(0);
	oldPath->SetSourcePoint(pos);

	newSearch->SetID(oldPath->GetID());
	newSearch->SetTeam((object != nullptr)? object->team: teamHandler.ActiveTeams());
	newSearch->SetPathType(oldPath->GetPathType());
	newSearch->SetGoalDistance(oldPath->GetRadius());
	newSearch->rawPathCheck = allowRawSearch;
	newSearch->initialized = false;
	newSearch->allowPartialSearch = allowPartialSearch;
	newSearch->synced = oldPath->IsSynced();

	assert(newSearch->synced == true);

	newSearch->tryPathRepair = allowRepair;

	registry.emplace_or_replace<PathSearchRef>(pathEntity, searchEntity);

	assert(	oldPath->GetSourcePoint().x != 0.f || oldPath->GetSourcePoint().z != 0.f );

	// LOG("%s: [p%x:s%x] (%f,%f) -> (%f,%f)", __func__, oldPath->GetID(), entt::to_integral(searchEntity)
	// 		, pos.x, pos.z, targetPoint.x, targetPoint.z);

	// if (object != nullptr && object->id == 25278) {
	// 	CUnit *unit = object != nullptr ? dynamic_cast<CUnit*>(const_cast<CSolidObject*>(object)) : nullptr;
	// 	LOG("%s: REQUEUE %s (%x) %d -> %d ", __func__
	// 			, unit != nullptr ? unit->unitDef->name.c_str() : "non-unit"
	// 			, oldPath->GetID()
	// 			, (oldPath != nullptr) ? oldPath->GetPathType() : -1
	// 			, oldPath->GetPathType()
	// 			);

	// 	LOG("%s: REQUEUE [%d] (%f,%f) -> x,z", __func__, oldPath->GetPathType()
	// 			, pos.x, pos.z);
	// }

	return (oldPath->GetID());
}

// #pragma GCC pop_options

void QTPFS::PathManager::UpdatePath(const CSolidObject* owner, unsigned int pathID) {
}

void QTPFS::PathManager::DeletePath(unsigned int pathID, bool force) {
	RECOIL_DETAILED_TRACY_ZONE;
	assert(!ThreadPool::IsInMultiThreadedSection());

	QTPFS::entity pathEntity = QTPFS::entity(pathID);

	if (!registry.valid(pathEntity)) return;

	bool pathMarkedForSharing = registry.all_of<SharedPathChain>(pathEntity);
	bool pathIsBeingProcessed = registry.any_of<PathIsDirty, PathSearchRef>(pathEntity);

	if (!registry.all_of<PathDelayedDelete>(pathEntity)) {
		// We either hold a potentially useful valid path for a short while so that it can be shared with other path
		// requests, or it is a path we can throw away at the first safe opportunity: this function could be called
		// while background path requests are underway, which could run the risk of a desync.
		int delayFrames = (pathMarkedForSharing && !pathIsBeingProcessed && !force) ? GAME_SPEED : 0;
		registry.emplace<PathDelayedDelete>(pathEntity, gs->frameNum + delayFrames);
	}
}

void QTPFS::PathManager::DeletePathEntity(QTPFS::entity pathEntity) {
	RECOIL_DETAILED_TRACY_ZONE;
	const PathTraceMapIt pathTraceIt = pathTraces.find(entt::to_integral(pathEntity));

	RemovePathFromShared(pathEntity);
	RemovePathFromPartialShared(pathEntity);

	// if (registry.valid(pathEntity)) - check is already done.
	RemovePathSearch(pathEntity);

	DestroyPathEntity(pathEntity);

	if (pathTraceIt != pathTraces.end()) {
		delete (pathTraceIt->second);
		pathTraces.erase(pathTraceIt);
	}
}

void QTPFS::PathManager::RemovePathFromShared(QTPFS::entity entity) {
	RECOIL_DETAILED_TRACY_ZONE;
	// if (!registry.valid(entity)) return;
	if (!registry.all_of<SharedPathChain>(entity)) return;

	IPath* path = &registry.get<IPath>(entity);
	auto iter = sharedPaths.find(path->GetHash());

	// case: when entity is at the head of the chain.
	if (iter != sharedPaths.end() && iter->second == entity) {
		auto& chain = registry.get<SharedPathChain>(entity);
		if (chain.next == entity) {
			sharedPaths.erase(path->GetHash());
		} else {
			sharedPaths[path->GetHash()] = chain.next;
		}
	}

	linkedListHelper.RemoveChain<SharedPathChain>(entity);
}

void QTPFS::PathManager::RemovePathFromPartialShared(QTPFS::entity entity) {
	RECOIL_DETAILED_TRACY_ZONE;
	// if (!registry.valid(entity)) return;
	if (!registry.all_of<PartialSharedPathChain>(entity)) return;

	IPath* path = &registry.get<IPath>(entity);
	auto iter = partialSharedPaths.find(path->GetVirtualHash());

	// case: when entity is at the head of the chain.
	if (iter != partialSharedPaths.end() && iter->second == entity) {
		assert(path->GetVirtualHash() != QTPFS::BAD_HASH);
		auto& chain = registry.get<PartialSharedPathChain>(entity);
		if (chain.next == entity) {
			partialSharedPaths.erase(path->GetVirtualHash());
		} else {
			partialSharedPaths[path->GetVirtualHash()] = chain.next;
		}
	}

	linkedListHelper.RemoveChain<PartialSharedPathChain>(entity);
}

void QTPFS::PathManager::RemovePathSearch(QTPFS::entity pathEntity) {
	RECOIL_DETAILED_TRACY_ZONE;

	auto search = registry.try_get<PathSearchRef>(pathEntity);

	// if (pathEntity == QTPFS::entity(257949903))
	// 		LOG("%s: id: %d search %p", __func__
	// 			, entt::to_integral(pathEntity), search);

	if (search != nullptr) {
		QTPFS::entity searchId = search->value;
		if (registry.valid(searchId))
			DestroyPathSearchEntity(searchId);

		registry.remove<PathSearchRef>(pathEntity);
	}
}

unsigned int QTPFS::PathManager::RequestPath(
	CSolidObject* object,
	const MoveDef* moveDef,
	float3 sourcePoint,
	float3 targetPoint,
	float radius,
	bool synced,
	bool immediateResult
) {
	RECOIL_DETAILED_TRACY_ZONE;
	unsigned int returnPathId = 0;

	if (!IsFinalized())
		return returnPathId;

	assert(	sourcePoint.x != 0.f || sourcePoint.z != 0.f );

	returnPathId = QueueSearch(object, moveDef, sourcePoint, targetPoint, radius, synced, immediateResult, (synced && object != nullptr));

	// if (object != nullptr && 30809 == object->id)
	// 	LOG("%s: RequestPath (%d).", __func__, returnPathId);

	if (immediateResult && returnPathId != 0) {
		// Immediate searches are occurring at the same time as background searches. It is absolutely critical that they
		// do not have an owner otherwise InitializeSearch will attempt to share paths with the background searches,
		// which can cause desyncs.
		assert(object == nullptr);
		returnPathId = ExecuteImmediateSearch(returnPathId);
	// 	auto path = GetPath(QTPFS::entity(returnPathId));
	// 	LOG("%s: IMMEDIATE non-owner (synced=%d) pathType=%d (srcPoint=%f,%f) (dstPoint=%f,%f) radius=%f hash=%x"
	// 			, __func__
	// 			// , returnPathId	
	// 			, int(synced)
	// 			, moveDef->pathType
	// 			, sourcePoint.x, sourcePoint.z
	// 			, targetPoint.x, targetPoint.z
	// 			, radius
	// 			, path != nullptr ? path->CalculateHash() : -1
	// 			);
	// } else {
	// 	LOG("%s: QUEUED owner id %d (synced=%d) pathType=%d (srcPoint=%f,%f) (dstPoint=%f,%f) radius=%f"
	// 			, __func__
	// 			, object != nullptr ? object->id : -1
	// 			// , returnPathId
	// 			, int(synced)
	// 			, moveDef->pathType
	// 			, sourcePoint.x, sourcePoint.z
	// 			, targetPoint.x, targetPoint.z
	// 			, radius
	// 			);
	}

	return returnPathId;
}

unsigned int QTPFS::PathManager::ExecuteImmediateSearch(unsigned int pathId){
	RECOIL_DETAILED_TRACY_ZONE;
	QTPFS::entity pathEntity = QTPFS::entity(pathId);
	assert(registry.valid(pathEntity));
	QTPFS::entity pathSearchEntity = registry.get<PathSearchRef>(pathEntity).value;
	assert(registry.valid(pathSearchEntity));
	InitializeSearch(pathSearchEntity);

	PathSearch& pathSearch = *GetSearch(pathSearchEntity);
	int pathType = pathSearch.GetPathType();
	NodeLayer& nodeLayer = nodeLayers[pathType];
	ExecuteSearch(&pathSearch, nodeLayer, pathType, true);

	if (registry.valid(pathEntity)) {
		IPath* path = GetPath(pathEntity);
		if (path != nullptr) {
			if (pathSearch.PathWasFound()) {
				registry.remove<PathIsTemp>(pathEntity);
				registry.remove<PathIsDirty>(pathEntity);
			} else {
				pathId = 0;
			}
		}
	}

	// If successful, just remove the path search, otherwise delete the path entity and its search.
	if (pathId > 0)
		RemovePathSearch(pathEntity);
	else
		DeletePathEntity(pathEntity);

	return pathId;
}

bool QTPFS::PathManager::PathUpdated(unsigned int pathID) {
	RECOIL_DETAILED_TRACY_ZONE;
	QTPFS::entity pathEntity = (QTPFS::entity)pathID;
	if (!registry.valid(pathEntity)) { return false; }
	IPath* livePath = registry.try_get<IPath>(pathEntity);

	if (livePath == nullptr)
		return false;

	return (livePath->GetNumPathUpdates() > 0);
}

void QTPFS::PathManager::ClearPathUpdated(unsigned int pathID) {
	RECOIL_DETAILED_TRACY_ZONE;
	QTPFS::entity pathEntity = (QTPFS::entity)pathID;
	if (!registry.valid(pathEntity)) { return; }
	IPath* livePath = registry.try_get<IPath>(pathEntity);

	if (livePath == nullptr)
		return;

	livePath->SetNumPathUpdates(0);
}


float3 QTPFS::PathManager::NextWayPoint(
	const CSolidObject* owner,
	unsigned int pathID,
	unsigned int, // numRetries
	float3 point,
	float, // radius,
	bool synced
) {
	ZoneScoped;
	const float3 noPathPoint = -XZVector;

	if (!IsFinalized())
		return noPathPoint;

	QTPFS::entity pathEntity = QTPFS::entity(pathID);
	IPath* livePath = GetPath(pathEntity);
	if (livePath == nullptr)
		return noPathPoint;

	// Do not permit unsynced code/data to potentially impact synced code/data.
	if (livePath->IsSynced() != synced)
		return noPathPoint;

	if (registry.all_of<PathIsTemp>(pathEntity)) {
		// path-request has not yet been processed (so ID still maps to
		// a temporary path); just set the unit off toward its target to
		// hide latency
		//
		// <curPoint> is initially the position of the unit requesting a
		// path, but later changes to the subsequent values returned here
		//
		// NOTE:
		//     if the returned point P is too far away, then a unit U will
		//     never switch to its live-path even after it becomes available
		//     (because NextWayPoint is not called again until U gets close
		//     to P), so always keep it a fixed small distance in front
		//
		//     make the y-coordinate -1 to indicate these are temporary
		//     waypoints to GMT and should not be followed religiously
		const float3& sourcePoint = point;
		const float3& targetPoint = livePath->GetTargetPoint();
		const float3  targetDirec = (targetPoint - sourcePoint).SafeNormalize() * SQUARE_SIZE;
		return float3(sourcePoint.x + targetDirec.x, -1.0f, sourcePoint.z + targetDirec.z);
	}

	unsigned int nextPointIndex = livePath->GetNextPointIndex() + 1;
	unsigned int lastPointIndex = livePath->NumPoints() - 1;

	// If this is the first call then we may need to jump a bit further in the path if the unit
	// managed to travel past the first point in the time it took to make the route. 
	if (nextPointIndex == 1) {
		constexpr float invSin45deg = 1.42f; // to account for a square's diagonal being longer.
		constexpr float squareRadius = SQUARE_SIZE*SQUARE_SIZE*invSin45deg;
		for (unsigned int i = (livePath->GetNextPointIndex()); i < lastPointIndex; i++) {
			// find waypoints <p0> and <p1> such that <point> is
			// "in front" of p0 and "behind" p1 (ie. in between)
			//
			// we do this rather than the radius-based search
			// since depending on the value of <radius> we may
			// or may not find a "next" node (even though one
			// always exists)
			const float3& p0 = livePath->GetPoint(i    ), v0 = float3(p0.x - point.x, 0.0f, p0.z - point.z);
			const float3& p1 = livePath->GetPoint(i + 1), v1 = float3(p1.x - point.x, 0.0f, p1.z - point.z);

			// NOTE:
			//     either v0 or v1 can be a zero-vector (p0 == point or p1 == point)
			//     in those two cases the dot-product is meaningless so we skip them
			//     vectors are NOT normalized, so it can happen that NO case matches
			//     and we must fall back to assuming waypoint 1 is best.
			if (v0.SqLength() < squareRadius) { nextPointIndex = i + 1; break; }
			if (v1.SqLength() < squareRadius) { nextPointIndex = i + 2; break; }
			if (v0.dot(v1) <= -0.f)           { nextPointIndex = i + 1; break; }
		}
	}

	if (nextPointIndex > lastPointIndex) {
		nextPointIndex = lastPointIndex;
	} else {
		livePath->SetNextPointIndex(nextPointIndex);
	}

	// if (owner != nullptr && 30809 == owner->id)
	// 	LOG("%s: repath target waypoint (%d) current waypoint (%d) of (%d) pathId=%d", __func__
	// 			, livePath->GetRepathTriggerIndex(), nextPointIndex, lastPointIndex, pathID);

	if (livePath->GetRepathTriggerIndex() > 0
			&& nextPointIndex >= livePath->GetRepathTriggerIndex()
			&& !livePath->IsRepathTriggered()) {
		// Request an update to the path.
		assert(livePath->GetOwner() != nullptr);
		assert(registry.all_of<PathRequeueSearch>(pathEntity));
		registry.get<PathRequeueSearch>(pathEntity).value = true;
		// livePath->ClearGetRepathTriggerIndex();
		livePath->SetRepathTriggered(true);
	}

	return livePath->GetPoint(nextPointIndex);
}


bool QTPFS::PathManager::CurrentWaypointIsUnreachable(unsigned int pathID) {
	RECOIL_DETAILED_TRACY_ZONE;
	QTPFS::entity pathEntity = QTPFS::entity(pathID);
	if (!registry.valid(pathEntity))
		return true;

	IPath* livePath = registry.try_get<IPath>(pathEntity);
	if (livePath == nullptr)
		return true;

	// LOG("%s: lastwaypoint=%d, isFullPath=%d", __func__
	// 		, int(livePath->GetNextPointIndex() == livePath->NumPoints() - 1)
	// 		, int(livePath->IsFullPath()));

	return ( livePath->GetNextPointIndex() == livePath->NumPoints() - 1 ) && ( !livePath->IsFullPath() );
}


bool QTPFS::PathManager::NextWayPointIsUnreachable(unsigned int pathID) {
	RECOIL_DETAILED_TRACY_ZONE;
	QTPFS::entity pathEntity = QTPFS::entity(pathID);
	if (!registry.valid(pathEntity))
		return true;

	IPath* livePath = registry.try_get<IPath>(pathEntity);
	if (livePath == nullptr)
		return true;

	unsigned int lastWaypoint = livePath->NumPoints() - 1;
	unsigned int nextWaypoint = livePath->GetNextPointIndex() + 1;

	return ( nextWaypoint >= lastWaypoint ) && ( !livePath->IsFullPath() );
}


void QTPFS::PathManager::GetPathWayPoints(
	unsigned int pathID,
	std::vector<float3>& points,
	std::vector<int>& starts
) const {
	RECOIL_DETAILED_TRACY_ZONE;
	if (!IsFinalized())
		return;

	QTPFS::entity pathEntity = (QTPFS::entity)pathID;
	if (!registry.valid(pathEntity))
		return;

	const IPath* path = GetPath(pathEntity);
	if (path == nullptr)
		return;

	// maintain compatibility with the tri-layer legacy PFS
	points.resize(path->NumPoints());
	starts.resize(3, 0);

	for (unsigned int n = 0; n < path->NumPoints(); n++) {
		points[n] = path->GetPoint(n);
	}
}

int2 QTPFS::PathManager::GetNumQueuedUpdates() const {
	RECOIL_DETAILED_TRACY_ZONE;
	int2 data;

	data.x = updateDirtyPathRate;//mapChangeTrack.damageQueue.size();// registry.size();
	data.y = updateDirtyPathRemainder;

	return data;
}
