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

#include "Utils/PathMaxSpeedModSystemUtils.h"

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
#include "Components/PathMaxSpeedMod.h"
#include "Components/RemoveDeadPaths.h"
#include "Systems/PathMaxSpeedModSystem.h"
#include "Systems/RemoveDeadPathsSystem.h"
#include "Registry.h"

#include <assert.h>
#include <tracy/Tracy.hpp>

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

	std::vector<NodeLayer> PathManager::nodeLayers;
}

QTPFS::PathManager::PathManager() {
	QTNode::InitStatic();
	NodeLayer::InitStatic();
	PathManager::InitStatic();

	assert(registry.alive() == 0);

	// reserve entity 0 so it can't be used picked up by a path by accident.
	systemEntity = registry.create();

	assert(entt::to_entity(systemEntity) == 0);
}

QTPFS::PathManager::~PathManager() {
	isFinalized = false;

	PathMaxSpeedModSystem::Shutdown();
	RemoveDeadPathsSystem::Shutdown();

	// print out anything still left in the registry - there should be nothing
	registry.each([this](auto entity) {
		bool isPath = registry.all_of<IPath>(entity);
		bool isSearch = registry.all_of<PathSearch>(entity);
		if (isPath) {
			const IPath& path = registry.get<IPath>(entity);
			LOG("path [%x] type=%d, owner=%p", entt::to_integral(entity)
					, path.GetPathType()
					, path.GetOwner()
					);
			registry.destroy(entity);
		}
		if (isSearch) {
			const PathSearch& search = registry.get<PathSearch>(entity);
			LOG("search [%x] type=%d, id=%x", entt::to_integral(entity)
					, search.GetPathType()
					, search.GetID()
					);
			registry.destroy(entity);
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

	// numCurrExecutedSearches.clear();
	// numPrevExecutedSearches.clear();

	searchThreadData.clear();
	updateThreadData.clear();

	systemGlobals.ClearComponents();

	// make this is destroyed last to ensure entity 0 will be first picked up next time.
	registry.destroy(systemEntity);

	LOG("%s: %d entities still active!", __func__, int(registry.alive()));

	assert(registry.alive() == 0);

	registry.clear();
}

std::int64_t QTPFS::PathManager::Finalize() {
	const spring_time t0 = spring_gettime();

	{
		pmLoadScreen.Show(&PathManager::Load, this);
	}

	const spring_time t1 = spring_gettime();
	const spring_time dt = t1 - t0;

	return (dt.toMilliSecsi());
}

void QTPFS::PathManager::InitStatic() {
	LAYERS_PER_UPDATE = std::max(1u, mapInfo->pfs.qtpfs_constants.layersPerUpdate);
	MAX_TEAM_SEARCHES = std::max(1u, mapInfo->pfs.qtpfs_constants.maxTeamSearches);

	// Ensure SharedPathChain is assigned a Pool by EnTT to avoid it happening in an MT section
	{ auto view = registry.view<SharedPathChain>();
	  if (view.size() > 0) { LOG("%s: SharedPathChain is unexpectedly greater than 0.", __func__); }
	}
}

void QTPFS::PathManager::Load() {
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
		const sha512::raw_digest& mapCheckSum = archiveScanner->GetArchiveCompleteChecksumBytes(gameSetup->mapName);
		const sha512::raw_digest& modCheckSum = archiveScanner->GetArchiveCompleteChecksumBytes(gameSetup->modName);

		sha512::hex_digest mapCheckSumHex;
		sha512::hex_digest modCheckSumHex;
		sha512::dump_digest(mapCheckSum, mapCheckSumHex);
		sha512::dump_digest(modCheckSum, modCheckSumHex);

		InitNodeLayersThreaded(MAP_RECTANGLE);
		PathMaxSpeedModSystem::Init();
		RemoveDeadPathsSystem::Init();

		// NOTE:
		//   should be sufficient in theory, because if either
		//   the map or the mod changes then the checksum does
		//   (should!) as well and we get a cache-miss
		//   this value is also combined with the tree-sums to
		//   make it depend on the tesselation code specifics
		// FIXME:
		//   assumption is invalid now (Lua inits before we do)
		pfsCheckSum =
			((mapCheckSum[0] << 24) | (mapCheckSum[1] << 16) | (mapCheckSum[2] << 8) | (mapCheckSum[3] << 0)) ^
			((modCheckSum[0] << 24) | (modCheckSum[1] << 16) | (modCheckSum[2] << 8) | (modCheckSum[3] << 0));

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

		pmLoadScreen.Kill();
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

		loadscreen->SetLoadMessage(loadMsg);
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

	memFootPrint += sizeof(nodeLayersMapDamageTrack);
	memFootPrint += nodeLayersMapDamageTrack.mapChangeTrackers.size()
					* sizeof(decltype(nodeLayersMapDamageTrack.mapChangeTrackers)::value_type);


	for (auto threadData : searchThreadData) {
		memFootPrint += threadData.GetMemFootPrint();
	}
	for (auto threadData : updateThreadData) {
		memFootPrint += threadData.GetMemFootPrint();
	}
	for (unsigned int i = 0; i < nodeLayers.size(); i++) {
		memFootPrint += nodeLayers[i].GetMemFootPrint();
	}
	for (auto trace : pathTraces) {
		memFootPrint += sizeof(decltype(*trace.second));
		memFootPrint += trace.second->GetMemFootPrint();
	}

	// convert to megabytes
	return (memFootPrint / (1024 * 1024));
}



void QTPFS::PathManager::InitNodeLayersThreaded(const SRectangle& rect) {
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
		LOG("%s: Warning! Map width and height are supposed to be multiples of 1024 elmos.", __func__);

	// Prevent too big a size being picked due to 15 levels of node Indexing possible: 2^(steps -1) (steps=(bits-2)/2)
	constexpr float maxNodeLevels = ((sizeof(uint32_t)*4)-2);
	uint32_t maxNodeSize = math::pow(2.f, maxNodeLevels);
	rootSize = rootSize > maxNodeSize ? maxNodeSize : rootSize;
}

void QTPFS::PathManager::InitNodeLayer(unsigned int layerNum, const SRectangle& r) {
	NodeLayer& nl = nodeLayers[layerNum];

	nl.Init(layerNum);

	// TODO: partial zones just in case %64 != 0? need to check tessalation off map is okay.
	//       This should not happen.
	int numRootCount = 0;
	int zRootNodes = 0;
	for (int z = r.z1; z < r.z2; z += rootSize) {
		for (int x = r.x1; x < r.x2; x += rootSize) {
			int idx = nl.AllocPoolNode(nullptr, -1, x, z, x + rootSize, z + rootSize);

			// LOG("%s: %d root node [%d,%d:%d,%d] allocated.", __func__
			// 		, idx, x, z, x + rootSize, z + rootSize);

			assert(idx == numRootCount);
			numRootCount++;
		}
		zRootNodes++;
	}
	
	// Root Mask is the part of the node number reserved for root nodes.
	// This limits the maximum number of levels of nodes we can create unique, position ids for.
	// (MAX_DEPTH)
	uint32_t rootShift = 30;
	for (int factor = 4; factor < numRootCount; factor <<= 2) {
		rootShift -= 2;
	}
	QTNode::MAX_DEPTH = rootShift>>1; // std::min(rootShift>>1, QTNode::MAX_DEPTH);
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

	SRectangle r(rect);
	if (rect.x1 == 0 && rect.x2 == 0) {
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
	assert(re.z2 >= r.z2); // TODO: re can be dropped?

	updateThreadData[currentThread].InitUpdate(r, *containingNode, *md, currentThread);
	const bool needTesselation = nodeLayers[layerNum].Update(updateThreadData[currentThread]);

	// process the affected root nodes.

	// LOG("%s: [%d] needTesselation=%d, wantTesselation=%d", __func__, layerNum, (int)needTesselation, (int)wantTesselation);

	if (needTesselation) {
		SRectangle ur(re.x1, re.z1, re.x2, re.z2);
		auto& nodeLayer = nodeLayers[layerNum];

		containingNode->PreTesselate(nodeLayers[layerNum], re, ur, 0, &updateThreadData[currentThread]);

		pathCache.SetLayerPathCount(layerNum, INITIAL_PATH_RESERVE);
		pathCache.MarkDeadPaths(re, layerNum);

		#ifndef QTPFS_CONSERVATIVE_NEIGHBOR_CACHE_UPDATES
		nodeLayers[layerNum].ExecNodeNeighborCacheUpdates(ur, updateThreadData[currentThread]);
		#endif
	}
}

// note that this is called twice per object:
// height-map changes, then blocking-map does
void QTPFS::PathManager::TerrainChange(unsigned int x1, unsigned int z1,  unsigned int x2, unsigned int z2, unsigned int type) {
	if (!IsFinalized())
		return;

	MapChanged(x1, z1, x2, z2);
}

void QTPFS::PathManager::MapChanged(int x1, int y1, int x2, int y2) {
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
		SCOPED_TIMER("Sim::Path::Requests");
		ThreadUpdate();
	}
	{
		// start off with simple update

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
			for (auto pathEntity : layerDirtyPaths) {
				assert(registry.valid(pathEntity));
				// assert(!registry.all_of<PathIsDirty>(pathEntity));
				// LOG("%s: alreadyDirty=%d, pathEntity=%x", __func__, (int)registry.all_of<PathIsDirty>(pathEntity)
				// 		, (int)pathEntity);

				// TODO: perhaps not mark paths multiple times if multiple blocks are updated in same frame?
				if (registry.all_of<PathIsDirty>(pathEntity)) { continue; }

				registry.emplace<PathIsDirty>(pathEntity);
				RemovePathFromShared(pathEntity);
				pathsMarkedDirty++;
			}
			layerDirtyPaths.clear();
			// LOG("%s: end: %d", __func__, (int)layerDirtyPaths.size());
		}
		if (refreshDirtyPathRateFrame == QTPFS_LAST_FRAME && pathsMarkedDirty > 0)
			refreshDirtyPathRateFrame = gs->frameNum + GAME_SPEED;
	}
}

__FORCE_ALIGN_STACK__
void QTPFS::PathManager::ThreadUpdate() {
	QueueDeadPathSearches();
	ExecuteQueuedSearches();
}


void QTPFS::PathManager::InitializeSearch(entt::entity searchEntity) {
	ZoneScoped;
	assert(registry.all_of<PathSearch>(searchEntity));
	PathSearch* search = &registry.get<PathSearch>(searchEntity);
	int pathType = search->GetPathType();

	assert(pathType < nodeLayers.size());
	NodeLayer& nodeLayer = nodeLayers[pathType];

	entt::entity pathEntity = (entt::entity)search->GetID();
	if (registry.valid(pathEntity)) {
		assert(registry.all_of<IPath>(pathEntity));
		IPath* path = &registry.get<IPath>(pathEntity);
		search->Initialize(&nodeLayer, path->GetSourcePoint(), path->GetTargetPoint(), path->GetOwner());
		path->SetHash(search->GetHash());

		if (path->IsSynced() && search->GetHash() != PathSearch::BAD_HASH) {
			assert(!registry.all_of<SharedPathChain>(pathEntity));
			SharedPathMap::iterator sharedPathsIt = sharedPaths.find(path->GetHash());
			if (sharedPathsIt == sharedPaths.end()) {
				registry.emplace<SharedPathChain>(pathEntity, pathEntity, pathEntity);
				sharedPaths[path->GetHash()] = pathEntity;
			} else {
				linkedListHelper.InsertChain<SharedPathChain>(sharedPaths[path->GetHash()], pathEntity);
			}
		}
	}
}

void QTPFS::PathManager::ReadyQueuedSearches() {
	auto pathView = registry.view<PathSearch>();

	// Go through in reverse order to minimize reshuffling EnTT will do with the grouping.
	std::for_each(pathView.rbegin(), pathView.rend(), [this](entt::entity entity){
		InitializeSearch(entity);
		registry.emplace_or_replace<ProcessPath>(entity);
	});
}

void QTPFS::PathManager::ExecuteQueuedSearches() {
	ZoneScoped;

	auto pathView = registry.group<PathSearch, ProcessPath>();

	ReadyQueuedSearches();

	// execute pending searches collected via
	// RequestPath and QueueDeadPathSearches
	for_mt(0, pathView.size(), [this, &pathView](int i){
		entt::entity pathSearchEntity = pathView.begin()[i];
        // entt::entity pathSearchEntity = pathView.storage<PathSearch>()[i];

		assert(registry.valid(pathSearchEntity));
		assert(registry.all_of<PathSearch>(pathSearchEntity));

		PathSearch* search = &pathView.get<PathSearch>(pathSearchEntity);
		int pathType = search->GetPathType();
		NodeLayer& nodeLayer = nodeLayers[pathType];
		ExecuteSearch(search, nodeLayer, pathType);
	});

	for (auto pathSearchEntity : pathView) {
		assert(registry.valid(pathSearchEntity));
		assert(registry.all_of<PathSearch>(pathSearchEntity));

		PathSearch* search = &pathView.get<PathSearch>(pathSearchEntity);
		entt::entity pathEntity = (entt::entity)search->GetID();
		if (registry.valid(pathEntity)) {
			IPath* path = registry.try_get<IPath>(pathEntity);
			if (path != nullptr) {
				if (search->PathWasFound()) {
					registry.remove<PathIsTemp>(pathEntity);
					registry.remove<PathIsDirty>(pathEntity);
					registry.remove<PathSearchRef>(pathEntity);
				} else {
					if (search->rawPathCheck) {
						registry.remove<PathSearchRef>(pathEntity);
						registry.remove<PathIsDirty>(pathEntity);

						// adding a new search doesn't break this loop because new paths do not
						// have the tag ProcessPath and so don't impact this group view.
						RequeueSearch(path, false);
					} else {
						DeletePath(path->GetID());
					}
				}
			}
		}

		// LOG("%s: delete search %x", __func__, entt::to_integral(pathSearchEntity));
		registry.destroy(pathSearchEntity);
	}
}

bool QTPFS::PathManager::ExecuteSearch(
	PathSearch* search,
	NodeLayer& nodeLayer,
	unsigned int pathType
) {
	ZoneScoped;

	entt::entity pathEntity = (entt::entity)search->GetID();
	if (!registry.valid(pathEntity))
		return false;

	IPath* path = registry.try_get<IPath>(pathEntity);

	int currentThread = ThreadPool::GetThreadNum();

	assert(search != nullptr);

	// temp-path might have been removed already via
	// DeletePath before we got a chance to process it
	if (path == nullptr)
		return false;

	assert(path->GetID() == search->GetID());

	bool synced = path->IsSynced();

	entt::entity chainHeadEntity = entt::null;
	if (synced)
	{
		SharedPathMap::const_iterator sharedPathsIt = sharedPaths.find(path->GetHash());
		if (sharedPathsIt != sharedPaths.end()) {
			chainHeadEntity = sharedPathsIt->second;
			// LOG("%s: chainHeadEntity %x != pathEntity %x", __func__
			// 		, entt::to_integral(chainHeadEntity), entt::to_integral(pathEntity));
			if (chainHeadEntity != pathEntity){
				bool pathIsCopyable = !registry.all_of<PathSearchRef>(chainHeadEntity);
				if (pathIsCopyable) {
					// LOG("%s: pathEntity %x pathIsCopyable = %d", __func__
					// 		, entt::to_integral(pathEntity), int(pathIsCopyable));
					auto& headChainPath = registry.get<IPath>(chainHeadEntity);
					search->SharedFinalize(&headChainPath, path);
				}
				return false;
			}
		}
	}

	search->InitializeThread(&searchThreadData[currentThread]);

	if (search->Execute(searchStateOffset)) {
		search->Finalize(path);

		if (chainHeadEntity == pathEntity){
			ZoneScopedN("Sim::QTPFS::post-check shared path");

			// Copy results to all applicable paths. Walk chain backwards and stop early.
			linkedListHelper.BackWalkWithEarlyExit<SharedPathChain>(chainHeadEntity
					, [this, path, chainHeadEntity](entt::entity next) {
				if (next == chainHeadEntity) { return true; }

				assert(registry.valid(next));

				auto* linkedPath = &registry.get<IPath>(next);
				assert(linkedPath != nullptr);
				
				auto* searchRef = registry.try_get<PathSearchRef>(next);
				if (searchRef == nullptr) { return false; }

				assert(registry.all_of<PathSearch>(searchRef->value));
				auto& chainSearch = registry.get<PathSearch>(searchRef->value);

				chainSearch.SharedFinalize(path, linkedPath);

				return true;
			});
		}

		#ifdef QTPFS_TRACE_PATH_SEARCHES
		pathTraces[path->GetID()] = search->GetExecutionTrace();
		#endif
	}

	return true;
}

void QTPFS::PathManager::QueueDeadPathSearches() {
	ZoneScoped;
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
			RequeueSearch(path, true);

			assert(registry.all_of<PathIsToBeUpdated>(entity));
			registry.remove<PathIsToBeUpdated>(entity);
		});
	}
}

unsigned int QTPFS::PathManager::QueueSearch(
	const CSolidObject* object,
	const MoveDef* moveDef,
	const float3& sourcePoint,
	const float3& targetPoint,
	const float radius,
	const bool synced,
	const bool allowRawSearch
) {
	assert(!ThreadPool::inMultiThreadedSection);

	// NOTE:
	//     all paths get deleted by the cache they are in;
	//     all searches get deleted by subsequent Update's
	// NOTE:
	//     the path-owner object handed to us can never become
	//     dangling (even with delayed execution) because ~GMT
	//     calls DeletePath, which ensures any path is removed
	//     from its cache before we get to ExecuteSearch

	entt::entity pathEntity = registry.create();
	assert(!registry.all_of<IPath>(pathEntity));
	IPath* newPath = &(registry.emplace<IPath>(pathEntity));

	entt::entity searchEntity = registry.create();
	PathSearch* newSearch = &registry.emplace<PathSearch>(searchEntity, PATH_SEARCH_ASTAR);

	if (synced) {
		assert(object->pos.x == sourcePoint.x);
		assert(object->pos.z == sourcePoint.z);
	}
	assert(targetPoint.x >= 0.f);
	assert(targetPoint.z >= 0.f);
	assert(targetPoint.x / SQUARE_SIZE < mapDims.mapx);
	assert(targetPoint.z / SQUARE_SIZE < mapDims.mapy);

	assert(newPath != nullptr);
	assert(newSearch != nullptr);

	// 0 is considered a null path. Entity id 0 should have been taken by the pathing system itself.
	assert(pathEntity != (entt::entity)0);

	// NOTE:
	//     the unclamped end-points are temporary
	//     zero is a reserved ID, so pre-increment
	newPath->SetID((int)pathEntity);
	newPath->SetRadius(radius);
	newPath->SetSynced(synced);
	newPath->AllocPoints(2);
	newPath->SetOwner(object);
	newPath->SetSourcePoint(sourcePoint);
	newPath->SetTargetPoint(targetPoint);
	newPath->SetPathType(moveDef->pathType);

	registry.emplace<PathIsTemp>(pathEntity);
	registry.emplace<PathSearchRef>(pathEntity, searchEntity);

	newSearch->SetID(newPath->GetID());
	newSearch->SetTeam((object != nullptr)? object->team: teamHandler.ActiveTeams());
	newSearch->SetPathType(newPath->GetPathType());
	newSearch->rawPathCheck = allowRawSearch;

	// LOG("%s: %s (%x) %d -> %d ", __func__
	// 		, unit != nullptr ? unit->unitDef->name.c_str() : "non-unit"
	// 		, newPath->GetID()
	// 		, (oldPath != nullptr) ? oldPath->GetPathType() : -1
	// 		, moveDef->pathType
	// 		);

	// LOG("%s: [%d] (%f,%f) -> (%f,%f)", __func__, newPath->GetPathType()
	// 		, sourcePoint.x, sourcePoint.z, targetPoint.x, targetPoint.z);

	return (newPath->GetID());
}

unsigned int QTPFS::PathManager::RequeueSearch(
	IPath* oldPath, const bool allowRawSearch
) {
	assert(!ThreadPool::inMultiThreadedSection);
	entt::entity pathEntity = entt::entity(oldPath->GetID());

	// If a path request is already in progress then don't create another one.
	if (registry.all_of<PathSearchRef>(pathEntity))
		return (oldPath->GetID());

	// Always create the search object first to ensure pathEntity can never be 0 (which is
	// considered a non-path)
	entt::entity searchEntity = registry.create();
	PathSearch* newSearch = &registry.emplace<PathSearch>(searchEntity, PATH_SEARCH_ASTAR);
	assert(oldPath != nullptr);
	assert(newSearch != nullptr);
	assert(oldPath->GetID() != 0);
	assert(pathEntity != entt::null);

	const CSolidObject* obj = oldPath->GetOwner();
	const float3& pos = (obj != nullptr)? obj->pos: oldPath->GetSourcePoint();

	const auto targetPoint = oldPath->GetTargetPoint();

	RemovePathFromShared(pathEntity);

	oldPath->SetHash(PathSearch::BAD_HASH);
	oldPath->SetNextPointIndex(-1);
	oldPath->SetNumPathUpdates(oldPath->GetNumPathUpdates() + 1);

	// start re-request from the current point
	// along the path, not the original source
	oldPath->AllocPoints(2);
	oldPath->SetSourcePoint(pos);
	oldPath->SetTargetPoint(targetPoint);

	newSearch->SetID(oldPath->GetID());

	auto object = oldPath->GetOwner();
	newSearch->SetTeam((object != nullptr)? object->team: teamHandler.ActiveTeams());
	newSearch->SetPathType(oldPath->GetPathType());
	newSearch->rawPathCheck = allowRawSearch;

	registry.emplace_or_replace<PathIsTemp>(pathEntity);
	registry.emplace<PathSearchRef>(pathEntity, searchEntity);

	// LOG("%s: [%d] (%f,%f) -> (%f,%f)", __func__, oldPath->GetPathType()
	// 		, pos.x, pos.z, targetPoint.x, targetPoint.z);

	return (oldPath->GetID());
}

void QTPFS::PathManager::UpdatePath(const CSolidObject* owner, unsigned int pathID) {
}

void QTPFS::PathManager::DeletePath(unsigned int pathID) {
	assert(!ThreadPool::inMultiThreadedSection);

	if (registry.all_of<SharedPathChain>(entt::entity(pathID))) {
		if (!registry.all_of<PathDelayedDelete>(entt::entity(pathID))) {
			registry.emplace<PathDelayedDelete>(entt::entity(pathID), gs->frameNum + GAME_SPEED);
		}
	} else {
		DeletePathEntity(entt::entity(pathID));
	}
}

void QTPFS::PathManager::DeletePathEntity(entt::entity pathEntity) {
	const PathTraceMapIt pathTraceIt = pathTraces.find(entt::to_integral(pathEntity));

	RemovePathFromShared(pathEntity);

	if (registry.valid(pathEntity))
		registry.destroy(pathEntity);

	if (pathTraceIt != pathTraces.end()) {
		delete (pathTraceIt->second);
		pathTraces.erase(pathTraceIt);
	}
}

void QTPFS::PathManager::RemovePathFromShared(entt::entity entity) {
	if (!registry.valid(entity)) return;
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

unsigned int QTPFS::PathManager::RequestPath(
	CSolidObject* object,
	const MoveDef* moveDef,
	float3 sourcePoint,
	float3 targetPoint,
	float radius,
	bool synced
) {
	unsigned int returnPathId = 0;

	if (!IsFinalized())
		return returnPathId;

	returnPathId = QueueSearch(object, moveDef, sourcePoint, targetPoint, radius, synced, synced);

	if (!synced && returnPathId != 0) {
		// Unsynced calls are expected to resolve immediately.
		returnPathId = ExecuteUnsyncedSearch(returnPathId);
	}

	return returnPathId;
}

unsigned int QTPFS::PathManager::ExecuteUnsyncedSearch(unsigned int pathId){
	entt::entity pathEntity = entt::entity(pathId);
	assert(registry.valid(pathEntity));
	entt::entity pathSearchEntity = registry.get<PathSearchRef>(pathEntity).value;
	assert(registry.valid(pathSearchEntity));
	InitializeSearch(pathSearchEntity);

	PathSearch& pathSearch = registry.get<PathSearch>(pathSearchEntity);
	int pathType = pathSearch.GetPathType();
	NodeLayer& nodeLayer = nodeLayers[pathType];
	ExecuteSearch(&pathSearch, nodeLayer, pathType);

	if (registry.valid(pathEntity)) {
		IPath* path = registry.try_get<IPath>(pathEntity);
		if (path != nullptr) {
			if (pathSearch.PathWasFound()) {
				registry.remove<PathIsTemp>(pathEntity);
				registry.remove<PathIsDirty>(pathEntity);
				registry.remove<PathSearchRef>(pathEntity);
			} else {
				DeletePath(path->GetID());
				pathId = 0;
			}
		}
	}
	registry.destroy(pathSearchEntity);

	return pathId;
}

bool QTPFS::PathManager::PathUpdated(unsigned int pathID) {
	entt::entity pathEntity = (entt::entity)pathID;
	if (!registry.valid(pathEntity)) { return false; }
	IPath* livePath = registry.try_get<IPath>(pathEntity);

	if (livePath == nullptr)
		return false;

	if (livePath->GetNumPathUpdates() == 0)
		return false;

	livePath->SetNumPathUpdates(0);
	return true;
}



float3 QTPFS::PathManager::NextWayPoint(
	const CSolidObject*, // owner
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

	entt::entity pathEntity = entt::entity(pathID);
	if (!registry.valid(pathEntity))
		return noPathPoint;

	IPath* livePath = registry.try_get<IPath>(pathEntity);
	if (livePath == nullptr)
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
	livePath->SetNextPointIndex(nextPointIndex);
	return livePath->GetPoint(nextPointIndex);
}


bool QTPFS::PathManager::CurrentWaypointIsUnreachable(unsigned int pathID) {
	entt::entity pathEntity = entt::entity(pathID);
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


void QTPFS::PathManager::GetPathWayPoints(
	unsigned int pathID,
	std::vector<float3>& points,
	std::vector<int>& starts
) const {
	if (!IsFinalized())
		return;

	entt::entity pathEntity = (entt::entity)pathID;
	if (!registry.valid(pathEntity))
		return;

	const IPath* path = registry.try_get<IPath>(pathEntity);
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
	int2 data;

	data.x = updateDirtyPathRate;//mapChangeTrack.damageQueue.size();// registry.size();
	data.y = updateDirtyPathRemainder;

	return data;
}
