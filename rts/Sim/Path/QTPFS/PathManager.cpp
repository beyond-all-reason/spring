/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#undef NDEBUG

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <functional>

#include "System/Threading/ThreadPool.h"
#include "System/Threading/SpringThreading.h"

#include "PathDefines.h"
#include "PathManager.h"

#include "Game/GameSetup.h"
#include "Game/LoadScreen.h"
#include "Map/MapInfo.h"

#include "Sim/Misc/GlobalSynced.h"
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
#include "Components/PathSearch.h"
#include "Registry.h"

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
	std::vector<QTNode*> PathManager::nodeTrees;
	// std::vector<PathCache> PathManager::pathCaches;
	std::vector<PathSearch*> PathManager::pathSearches;
}



QTPFS::PathManager::PathManager() {
	QTNode::InitStatic();
	NodeLayer::InitStatic();
	PathManager::InitStatic();
}

QTPFS::PathManager::~PathManager() {
	for (unsigned int layerNum = 0; layerNum < nodeLayers.size(); layerNum++) {
		nodeTrees[layerNum]->Merge(nodeLayers[layerNum]);
		nodeLayers[layerNum].Clear();
	}
	std::for_each(pathTraces.begin(), pathTraces.end(), [](std::pair<unsigned int, QTPFS::PathSearchTrace::Execution*>& t){delete t.second;} );
	std::for_each(pathSearches.begin(), pathSearches.end(), [](PathSearch* p){delete p;});

	// for (auto tracesIt = pathTraces.begin(); tracesIt != pathTraces.end(); ++tracesIt) {
	// 	delete (tracesIt->second);
	// }

	nodeTrees.clear();
	// reuse layer pools when reloading
	// nodeLayers.clear();
	// pathCaches.clear();
	pathSearches.clear();
	// pathTypes.clear();
	pathTraces.clear();
	mapChangeTrack.damageMap.clear();
	mapChangeTrack.damageQueue.clear();

	numCurrExecutedSearches.clear();
	numPrevExecutedSearches.clear();

	searchThreadData.clear();
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
}

void QTPFS::PathManager::Load() {
	// NOTE: offset *must* start at a non-zero value
	searchStateOffset = NODE_STATE_OFFSET;
	numTerrainChanges = 0;
	numPathRequests   = 0;
	maxNumLeafNodes   = 0;

	deadPathsToUpdatePerFrame = 1;
	recalcDeadPathUpdateRateOnFrame = 0;

	pathCache.Init(moveDefHandler.GetNumMoveDefs());
	nodeTrees.resize(moveDefHandler.GetNumMoveDefs(), nullptr);
	nodeLayers.resize(moveDefHandler.GetNumMoveDefs());
	pathSearches.reserve(200);

	mapChangeTrack.width = mapDims.mapx / DAMAGE_MAP_BLOCK_SIZE + (mapDims.mapx % DAMAGE_MAP_BLOCK_SIZE > 0);
	mapChangeTrack.height = mapDims.mapy / DAMAGE_MAP_BLOCK_SIZE + (mapDims.mapy % DAMAGE_MAP_BLOCK_SIZE > 0);
	mapChangeTrack.damageMap.resize(mapChangeTrack.width*mapChangeTrack.height);

	// add one extra element for object-less requests
	numCurrExecutedSearches.resize(teamHandler.ActiveTeams() + 1, 0);
	numPrevExecutedSearches.resize(teamHandler.ActiveTeams() + 1, 0);

	{
		const sha512::raw_digest& mapCheckSum = archiveScanner->GetArchiveCompleteChecksumBytes(gameSetup->mapName);
		const sha512::raw_digest& modCheckSum = archiveScanner->GetArchiveCompleteChecksumBytes(gameSetup->modName);

		sha512::hex_digest mapCheckSumHex;
		sha512::hex_digest modCheckSumHex;
		sha512::dump_digest(mapCheckSum, mapCheckSumHex);
		sha512::dump_digest(modCheckSum, modCheckSumHex);

		const std::string& cacheDirName = GetCacheDirName({mapCheckSumHex.data()}, {modCheckSumHex.data()});

		{
			layersInited = false;
			haveCacheDir = FileSystem::DirExists(cacheDirName);

			InitNodeLayersThreaded(MAP_RECTANGLE);
			Serialize(cacheDirName);

			layersInited = true;
		}

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
			#ifndef QTPFS_CONSERVATIVE_NEIGHBOR_CACHE_UPDATES
			if (haveCacheDir) {
				// if cache-dir exists, must set node relations after de-serializing its trees
				nodeLayers[layerNum].ExecNodeNeighborCacheUpdates(MAP_RECTANGLE, numTerrainChanges);
			}
			#endif

			pfsCheckSum ^= nodeTrees[layerNum]->GetCheckSum(nodeLayers[layerNum]);
			maxNumLeafNodes = std::max(nodeLayers[layerNum].GetNumLeafNodes(), maxNumLeafNodes);
		}

		{ SyncedUint tmp(pfsCheckSum); }

		// PathSearch::InitGlobalQueue(maxNumLeafNodes);
		int threads = ThreadPool::GetNumThreads();
		searchThreadData.reserve(threads);
		while (threads-- > 0)
			searchThreadData.emplace_back(SearchThreadData(maxNumLeafNodes /*NodeLayer::POOL_TOTAL_SIZE*/));
	}

	{
		const int memFootPrintMb = GetMemFootPrint();
		const std::string sumStr = "pfs-checksum: " + IntToString(pfsCheckSum, "%08x") + ", ";
		const std::string memStr = "mem-footprint: " + IntToString(memFootPrintMb) + "MB";

		LOG("[QTPFS] pfs-checksum: %08x", pfsCheckSum);
		LOG("[QTPFS] pfs-checksum: %dMB", memFootPrintMb);

		pmLoadScreen.AddMessage("[" + std::string(__func__) + "] " + sumStr + memStr);
		pmLoadScreen.Kill();
	}
}

std::uint64_t QTPFS::PathManager::GetMemFootPrint() const {
	std::uint64_t memFootPrint = sizeof(PathManager);

	for (unsigned int i = 0; i < nodeLayers.size(); i++) {
		memFootPrint += nodeLayers[i].GetMemFootPrint();
		memFootPrint += nodeTrees[i]->GetMemFootPrint(nodeLayers[i]);
	}

	// convert to megabytes
	return (memFootPrint / (1024 * 1024));
}



// void QTPFS::PathManager::SpawnSpringThreads(MemberFunc f, const SRectangle& r) {
// 	static std::vector<spring::thread*> threads(std::min(GetNumThreads(), nodeLayers.size()), nullptr);

// 	for (unsigned int threadNum = 0; threadNum < threads.size(); threadNum++) {
// 		threads[threadNum] = new spring::thread(std::bind(f, this, threadNum, threads.size(), r));
// 	}

// 	for (unsigned int threadNum = 0; threadNum < threads.size(); threadNum++) {
// 		threads[threadNum]->join(); delete threads[threadNum];
// 	}
// }



void QTPFS::PathManager::InitNodeLayersThreaded(const SRectangle& rect) {
	streflop::streflop_init<streflop::Simple>();

	char loadMsg[512] = {'\0'};
	const char* fmtString = "[PathManager::%s] using %u threads for %u node-layers (%s)";

	{
		sprintf(loadMsg, fmtString, __func__, ThreadPool::GetNumThreads(), nodeLayers.size(), (haveCacheDir? "cached": "uncached"));
		pmLoadScreen.AddMessage(loadMsg);

		#ifndef NDEBUG
		const char* preFmtStr = "  initializing node-layer %u (thread %u)";
		const char* pstFmtStr = "  initialized node-layer %u (%u MB, %u leafs, ratio %f)";
		#endif

		for_mt(0, nodeLayers.size(), [=,&loadMsg, &rect](const int layerNum){
			int currentThread = ThreadPool::GetThreadNum();
			#ifndef NDEBUG
			if (currentThread == 0) {
				sprintf(loadMsg, preFmtStr, layerNum, ThreadPool::GetThreadNum());
				pmLoadScreen.AddMessage(loadMsg);
			}
			#endif

			// construct each tree from scratch IFF no cache-dir exists
			// (if it does, we only need to initialize speed{Mods, Bins}
			// since Serialize will fill in the branches)
			// NOTE:
			//     silently assumes trees either ALL exist or ALL do not
			//     (if >= 1 are missing for some player in MP, we desync)
			InitNodeLayer(layerNum, rect);
			UpdateNodeLayer(layerNum, rect);

			const QTNode* tree = nodeTrees[layerNum];
			const NodeLayer& layer = nodeLayers[layerNum];
			const unsigned int mem = (tree->GetMemFootPrint(layer) + layer.GetMemFootPrint()) / (1024 * 1024);

			// #ifndef NDEBUG
			// sprintf(loadMsg, pstFmtStr, layerNum, mem, layer.GetNumLeafNodes(), layer.GetNodeRatio());
			// pmLoadScreen.AddMessage(loadMsg);
			// #endif
		});
	}

	streflop::streflop_init<streflop::Simple>();
}

void QTPFS::PathManager::InitNodeLayer(unsigned int layerNum, const SRectangle& r) {
	NodeLayer& nl = nodeLayers[layerNum];

	nl.Init(layerNum);
	nl.RegisterNode(nodeTrees[layerNum] = nl.AllocRootNode(nullptr, 0,  r.x1, r.z1,  r.x2, r.z2));
}



// void QTPFS::PathManager::UpdateNodeLayersThreaded(const SRectangle& rect) {
// 	streflop::streflop_init<streflop::Simple>();

// 	// #ifdef QTPFS_OPENMP_ENABLED
// 	{
// 		for_mt(0, nodeLayers.size(), [&,rect](const int layerNum) {
// 			UpdateNodeLayer(layerNum, rect);
// 		});
// 	}
// 	// #else
// 	// {
// 	// 	SpawnSpringThreads(&PathManager::UpdateNodeLayersThread, rect);
// 	// }
// 	// #endif

// 	streflop::streflop_init<streflop::Simple>();
// }

// __FORCE_ALIGN_STACK__
// void QTPFS::PathManager::UpdateNodeLayersThread(
// 	unsigned int threadNum,
// 	unsigned int numThreads,
// 	const SRectangle& rect
// ) {
// 	const unsigned int layersPerThread = (nodeLayers.size() / numThreads);
// 	const unsigned int numExcessLayers = (threadNum == (numThreads - 1))?
// 		(nodeLayers.size() % numThreads): 0;

// 	const unsigned int minLayer = threadNum * layersPerThread;
// 	const unsigned int maxLayer = minLayer + layersPerThread + numExcessLayers;

// 	for (unsigned int layerNum = minLayer; layerNum < maxLayer; layerNum++) {
// 		UpdateNodeLayer(layerNum, rect);
// 	}
// }

// called in the non-staggered (#ifndef QTPFS_STAGGERED_LAYER_UPDATES)
// layer update scheme and during initialization; see ::TerrainChange
void QTPFS::PathManager::UpdateNodeLayer(unsigned int layerNum, const SRectangle& r) {
	const MoveDef* md = moveDefHandler.GetMoveDefByPathType(layerNum);

	if (!IsFinalized())
		return;

	// NOTE:
	//     this is needed for IsBlocked* --> SquareIsBlocked --> IsNonBlocking
	//     but no point doing it in ExecuteSearch because the IsBlocked* calls
	//     are only made from NodeLayer::Update and also no point doing it here
	//     since we are independent of a specific path --> requires redesign
	//
	// md->tempOwner = const_cast<CSolidObject*>(path->GetOwner());

	// adjust the borders so we are not left with "rims" of
	// impassable squares when eg. a structure is reclaimed
	// SRectangle mr;
	SRectangle ur(r);

	// mr.x1 = std::max((r.x1 - md->xsizeh) - int(QTNode::MinSizeX() >> 1),            0);
	// mr.z1 = std::max((r.z1 - md->zsizeh) - int(QTNode::MinSizeZ() >> 1),            0);
	// mr.x2 = std::min((r.x2 + md->xsizeh) + int(QTNode::MinSizeX() >> 1), mapDims.mapx);
	// mr.z2 = std::min((r.z2 + md->zsizeh) + int(QTNode::MinSizeZ() >> 1), mapDims.mapy);
	// ur.x1 = mr.x1;
	// ur.z1 = mr.z1;
	// ur.x2 = mr.x2;
	// ur.z2 = mr.z2;

	const bool wantTesselation = (layersInited || !haveCacheDir);
	const bool needTesselation = nodeLayers[layerNum].Update(r, md);

	if (needTesselation && wantTesselation) {
		nodeTrees[layerNum]->PreTesselate(nodeLayers[layerNum], r, ur, 0);

		pathCache.SetLayerPathCount(layerNum, 200); // TODO sort out placeholder.
		pathCache.MarkDeadPaths(r, layerNum);

		#ifndef QTPFS_CONSERVATIVE_NEIGHBOR_CACHE_UPDATES
		nodeLayers[layerNum].ExecNodeNeighborCacheUpdates(ur, numTerrainChanges);
		#endif
	}
}


std::string QTPFS::PathManager::GetCacheDirName(const std::string& mapCheckSumHexStr, const std::string& modCheckSumHexStr) const {
	const std::string ver = IntToString(QTPFS_CACHE_VERSION, "%04x");
	const std::string dir = FileSystem::GetCacheDir() + "/QTPFS/" + ver + "/" +
		mapCheckSumHexStr.substr(0, 16) + "-" +
		modCheckSumHexStr.substr(0, 16) + "/";

	char loadMsg[1024] = {'\0'};
	const char* fmtString = "[PathManager::%s] using cache-dir \"%s\" (map-checksum %s, mod-checksum %s)";

	snprintf(loadMsg, sizeof(loadMsg), fmtString, __func__, dir.c_str(), mapCheckSumHexStr.c_str(), modCheckSumHexStr.c_str());
	pmLoadScreen.AddMessage(loadMsg);

	return dir;
}

void QTPFS::PathManager::Serialize(const std::string& cacheFileDir) {
	std::vector<std::string> fileNames(nodeTrees.size(), "");
	std::vector<std::fstream*> fileStreams(nodeTrees.size(), nullptr);
	std::vector<unsigned int> fileSizes(nodeTrees.size(), 0);

	if (!haveCacheDir) {
		FileSystem::CreateDirectory(cacheFileDir);
		assert(FileSystem::DirExists(cacheFileDir));
	}

	#ifndef NDEBUG
	char loadMsg[512] = {'\0'};
	const char* fmtString = "[PathManager::%s] serializing node-tree %u (%s)";
	#endif

	// TODO: compress the tree cache-files?
	for (unsigned int i = 0; i < nodeTrees.size(); i++) {
		const MoveDef* md = moveDefHandler.GetMoveDefByPathType(i);

		fileNames[i] = cacheFileDir + "tree" + IntToString(i, "%02x") + "-" + md->name;
		fileStreams[i] = new std::fstream();

		if (haveCacheDir) {
			#ifdef QTPFS_CACHE_XACCESS
			{
				// FIXME: lock fileNames[i] instead of doing this
				// fstreams can not be easily locked however, see
				// http://stackoverflow.com/questions/839856/
				while (!FileSystem::FileExists(fileNames[i] + "-tmp")) {
					spring::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
				while (FileSystem::GetFileSize(fileNames[i] + "-tmp") != sizeof(unsigned int)) {
					spring::this_thread::sleep_for(std::chrono::milliseconds(100));
				}

				fileStreams[i]->open((fileNames[i] + "-tmp").c_str(), std::ios::in | std::ios::binary);
				fileStreams[i]->read(reinterpret_cast<char*>(&fileSizes[i]), sizeof(unsigned int));
				fileStreams[i]->close();

				while (!FileSystem::FileExists(fileNames[i])) {
					spring::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
				while (FileSystem::GetFileSize(fileNames[i]) != fileSizes[i]) {
					spring::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
			}

			#else
			assert(FileSystem::FileExists(fileNames[i]));
			#endif

			// read fileNames[i] into nodeTrees[i]
			fileStreams[i]->open(fileNames[i].c_str(), std::ios::in | std::ios::binary);
			assert(fileStreams[i]->good());
			assert(nodeTrees[i]->IsLeaf());
		} else {
			// write nodeTrees[i] into fileNames[i]
			fileStreams[i]->open(fileNames[i].c_str(), std::ios::out | std::ios::binary);
		}

		#ifndef NDEBUG
		sprintf(loadMsg, fmtString, __func__, i, md->name.c_str());
		pmLoadScreen.AddMessage(loadMsg);
		#endif

		nodeTrees[i]->Serialize(*fileStreams[i], nodeLayers[i], &fileSizes[i], 0, haveCacheDir);

		fileStreams[i]->flush();
		fileStreams[i]->close();

		#ifdef QTPFS_CACHE_XACCESS
		if (!haveCacheDir) {
			// signal any other (concurrently loading) Spring processes; needed for validation-tests
			fileStreams[i]->open((fileNames[i] + "-tmp").c_str(), std::ios::out | std::ios::binary);
			fileStreams[i]->write(reinterpret_cast<const char*>(&fileSizes[i]), sizeof(unsigned int));
			fileStreams[i]->flush();
			fileStreams[i]->close();
		}
		#endif

		delete fileStreams[i];
	}
}

// note that this is called twice per object:
// height-map changes, then blocking-map does
void QTPFS::PathManager::TerrainChange(unsigned int x1, unsigned int z1,  unsigned int x2, unsigned int z2, unsigned int type) {
	if (!IsFinalized())
		return;

	// if type is TERRAINCHANGE_OBJECT_INSERTED or TERRAINCHANGE_OBJECT_INSERTED_YM,
	// this rectangle covers the yardmap of a CSolidObject* and will be tesselated to
	// maximum depth automatically
	numTerrainChanges += 1;

	MapChanged(x1, z1, x2, z2);
}

void QTPFS::PathManager::MapChanged(int x1, int y1, int x2, int y2) {
	const int res = DAMAGE_MAP_BLOCK_SIZE;
	const int w = mapChangeTrack.width;
	const int h = mapChangeTrack.height;

	// TODO: add dynamic boundary to calculations
	const int2 min  { std::max((x1-4) / res, 0)
					, std::max((y1-4) / res, 0)};
	const int2 max  { std::min((x2+4) / res, (w-1))
					, std::min((y2+4) / res, (h-1))};

	for (int y = min.y; y <= max.y; ++y) {
		int i = min.x + y*w;
		for (int x = min.x; x <= max.x; ++x, ++i) {
			if (!mapChangeTrack.damageMap[i]) {
				mapChangeTrack.damageMap[i] = true;
				mapChangeTrack.damageQueue.emplace_back(i);
			}
		}	
	}
}

void QTPFS::PathManager::Update() {
	SCOPED_TIMER("Sim::Path");
	{
		SCOPED_TIMER("Sim::Path::Requests");
		ThreadUpdate();
	}
	{
		// start off with simple update
		if (mapChangeTrack.damageQueue.empty()) return;

		SCOPED_TIMER("Sim::Path::MapUpdates");

		const int sectorId = mapChangeTrack.damageQueue.front();
		const int blockIdxX = (sectorId % mapChangeTrack.width) * DAMAGE_MAP_BLOCK_SIZE;
		const int blockIdxY = (sectorId / mapChangeTrack.width) * DAMAGE_MAP_BLOCK_SIZE;
		SRectangle rect
			( blockIdxX
			, blockIdxY
			, blockIdxX + DAMAGE_MAP_BLOCK_SIZE
			, blockIdxY + DAMAGE_MAP_BLOCK_SIZE
			);

		for_mt(0, nodeLayers.size(), [this, &rect](const int layerNum) {
			UpdateNodeLayer(layerNum, rect);
		});

		// Mark all dirty paths so that they can be recalculated
		for (auto layerDirtyPaths : pathCache.dirtyPaths) {
			for (auto pathEntity : layerDirtyPaths) {
				assert(registry.valid(pathEntity));
				assert(!registry.all_of<PathIsDirty>(pathEntity));
				registry.emplace_or_replace<PathIsDirty>(pathEntity);
			}
			layerDirtyPaths.clear();
		}

		assert(sectorId < mapChangeTrack.damageMap.size());
		mapChangeTrack.damageMap[sectorId] = false;
		mapChangeTrack.damageQueue.pop_front();
	}
}

__FORCE_ALIGN_STACK__
void QTPFS::PathManager::ThreadUpdate() {
	// NOTE:
	//     for a mod with N move-types, any unit will be waiting
	//     (N / LAYERS_PER_UPDATE) sim-frames before its request
	//     executes at a minimum
	// const unsigned int layersPerUpdateTmp = LAYERS_PER_UPDATE;
	// const unsigned int numPathTypeUpdates = nodeLayers.size();

	// NOTE: thread-safe (only ONE thread ever accesses these)
	// static unsigned int minPathTypeUpdate = 0;
	// static unsigned int maxPathTypeUpdate = numPathTypeUpdates;

	// if (gs->frameNum >= recalcDeadPathUpdateRateOnFrame){
	// 	recalcDeadPathUpdateRateOnFrame = gs->frameNum + GAME_SPEED;
	// 	size_t totalDeadPaths = 0;
	// 	for (auto& pathCache : pathCaches)
	// 		totalDeadPaths += pathCache.GetDeadPaths().size();

	// 	deadPathsToUpdatePerFrame = std::max(int(totalDeadPaths / GAME_SPEED), 1);
	// }

	sharedPaths.clear();

	// for (unsigned int pathTypeUpdate = minPathTypeUpdate; pathTypeUpdate < maxPathTypeUpdate; pathTypeUpdate++) {
	// 	#ifndef QTPFS_IGNORE_DEAD_PATHS
	// 	QueueDeadPathSearches(pathTypeUpdate);
	// 	#endif
	// }

	QueueDeadPathSearches();
	ExecuteQueuedSearches();

	// std::copy(numCurrExecutedSearches.begin(), numCurrExecutedSearches.end(), numPrevExecutedSearches.begin());

	// minPathTypeUpdate = (minPathTypeUpdate + numPathTypeUpdates);
	// maxPathTypeUpdate = (minPathTypeUpdate + numPathTypeUpdates);

	// if (minPathTypeUpdate >= nodeLayers.size()) {
	// 	minPathTypeUpdate = 0;
	// 	maxPathTypeUpdate = numPathTypeUpdates;
	// }
	// if (maxPathTypeUpdate >= nodeLayers.size()) {
	// 	maxPathTypeUpdate = nodeLayers.size();
	// }
}

void QTPFS::PathManager::ExecuteQueuedSearches() {

	//if (pathSearches.empty()) return;

	auto pathView = registry.view<PathSearch>();

	// execute pending searches collected via
	// RequestPath and QueueDeadPathSearches
	// for_mt(0, pathSearches.size(), [this](int i){
		// PathSearch* search = pathSearches[i];

	// std::vector<PathSearch*> pathSearches;
	// pathSearches.reserve(pathView.size());

	entt::entity pathSearches[pathView.size()];
	{
		auto curIt = pathView.begin();
		for (int i = 0; i < pathView.size(); ++i, ++curIt){
			assert(curIt != pathView.end());
			if (curIt == pathView.end())
				pathSearches[i] = entt::null;
			else
				pathSearches[i] = *curIt;
		}
	}

	// for_each_mt(pathView.begin(), pathView.end(), [this, &pathView](entt::entity pathSearchEntity){
	// for_mt(0, pathView.size(), [this, &pathView](int i){
	for_mt(0, pathView.size(), [this, &pathView, &pathSearches](int i){
		// entt::entity pathSearchEntity = pathView[i];
		entt::entity pathSearchEntity = pathSearches[i];
		if (pathSearchEntity == entt::null) { return; }

		assert(registry.valid(pathSearchEntity));
		PathSearch* search = &pathView.get<PathSearch>(pathSearchEntity);
		int pathType = search->GetPathType();
		assert(pathType < nodeLayers.size());
		NodeLayer& nodeLayer = nodeLayers[pathType];
		PathCache& pathCache = pathCache;
		ExecuteSearch(search, nodeLayer, pathCache, pathType);
	});

	// const auto CleanUpSearch = [this](QTPFS::PathSearch *search){
	for (auto pathSearchEntity : pathView) {
		// int pathType = search->GetPathType();
		// PathCache& pathCache = pathCaches;
		// IPath* path = pathCache.GetTempPath(search->GetID());
		assert(registry.valid(pathSearchEntity));
		PathSearch* search = &pathView.get<PathSearch>(pathSearchEntity);
		entt::entity pathEntity = (entt::entity)search->GetID();
		IPath* path = registry.try_get<IPath>(pathEntity);

		if (search->PathWasFound()) {
			registry.remove<PathIsTemp>(pathEntity);
			registry.remove<PathIsDirty>(pathEntity);
			// pathCache.AddLivePath(path);
		} else {
			DeletePath(path->GetID());
		}
		// delete search;
		// LOG("%s: %x", __func__, (int)pathSearchEntity);
		registry.destroy(pathSearchEntity);
	}
	// };

	// std::for_each(pathSearches.begin(), pathSearches.end(), CleanUpSearch);
	// pathSearches.clear();
}

bool QTPFS::PathManager::ExecuteSearch(
	PathSearch* search,
	NodeLayer& nodeLayer,
	PathCache& pathCache,
	unsigned int pathType
) {
	// PathSearch* search = *searchesIt;
	// IPath* path = pathCache.GetTempPath(search->GetID());
	entt::entity pathEntity = (entt::entity)search->GetID();
	if (!registry.valid(pathEntity))
		return false;

	IPath* path = registry.try_get<IPath>(pathEntity);

	int currentThread = ThreadPool::GetThreadNum();

	assert(search != nullptr);
	// assert(path != nullptr);

	// temp-path might have been removed already via
	// DeletePath before we got a chance to process it
	// if (path->GetID() == 0)
	if (path == nullptr)
		return false;

	// assert(search->GetID() != 0);
	assert(path->GetID() == search->GetID());

	search->Initialize(&nodeLayer, &pathCache
			, path->GetSourcePoint(), path->GetTargetPoint()
			, MAP_RECTANGLE, &searchThreadData[currentThread]);
	path->SetHash(search->GetHash(mapDims.mapx * mapDims.mapy, pathType));

	{
		#ifdef QTPFS_SEARCH_SHARED_PATHS
		SharedPathMap::const_iterator sharedPathsIt = sharedPaths.find(path->GetHash());

		if (sharedPathsIt != sharedPaths.end()) {
			if (search->SharedFinalize(sharedPathsIt->second, path)) {
				// DeleteSearch(search, searches, searchesIt);
				return false;
			}
		}
		#endif
	}

	// removes path from temp-paths, adds it to live-paths
	if (search->Execute(searchStateOffset, numTerrainChanges)) {
		search->Finalize(path);

		#ifdef QTPFS_SEARCH_SHARED_PATHS
		sharedPaths[path->GetHash()] = path;
		#endif

		#ifdef QTPFS_TRACE_PATH_SEARCHES
		pathTraces[path->GetID()] = search->GetExecutionTrace();
		#endif
	}

	return true;
}

void QTPFS::PathManager::QueueDeadPathSearches() {
	// PathCache& pathCache = pathCaches[pathType];
	// PathCache::PathMap::const_iterator deadPathsIt;

	// PathCache::PathMap& deadPaths = const_cast<PathCache::PathMap&>(pathCache.GetDeadPaths());
	// PathCache::PathMap& livePaths = const_cast<PathCache::PathMap&>(pathCache.GetLivePaths());
	
	// const MoveDef* moveDef = moveDefHandler.GetMoveDefByPathType(pathType);

	// if (deadPaths.empty()) return;

	auto dirtyPaths = registry.view<IPath, PathIsDirty>();
	for (auto entity : dirtyPaths) {
		IPath* path = &dirtyPaths.get<IPath>(entity);
		assert(path->GetPathType() < moveDefHandler.GetNumMoveDefs());
		const MoveDef* moveDef = moveDefHandler.GetMoveDefByPathType(path->GetPathType());
		QueueSearch(path, nullptr, moveDef, ZeroVector, ZeroVector, -1.0f, true);
	}


	// auto& deadPaths = pathCaches.dirtyPaths;
	// if (deadPaths.empty()) return;

	// std::stable_sort(deadPaths.begin(), deadPaths.end(), [](const PathCache::DirtyPath& lv, const PathCache::DirtyPath& rv){
	// 	return lv.pathType < rv.pathType;
	// });

	// std::for_each(deadPaths.begin(), deadPaths.end(), [this](PathCache::DirtyPath& dirtyPath){
	// 	IPath* path = &pathCaches.allPaths[dirtyPath.pathId];
	// 	const MoveDef* moveDef = moveDefHandler.GetMoveDefByPathType(dirtyPath.pathType);
	// 	QueueSearch(path, nullptr, moveDef, ZeroVector, ZeroVector, -1.0f, true);
	// });

	// deadPaths.clear();

	// std::vector<PathCache::PathMapIt> deadPathsToRemove;
	// deadPathsToRemove.reserve(deadPathsToUpdatePerFrame);

	// re-request LIVE paths that were marked as DEAD by a TerrainChange
	// for each of these now-dead paths, reset the active point-idx to 0
	//std::for_each_n(deadPaths.begin(), deadPathsToUpdatePerFrame, [=, &moveDef, &pathCache, &deadPathsToRemove](PathCache::PathMapIt deadPathsIt) mutable {
	// for (PathCache::PathMapIt deadPathsIt = deadPaths.begin(); deadPathsIt != deadPaths.end(); ++deadPathsIt) {
	// 	pathCache.ReleaseLivePath(deadPathsIt->first);
	// 	QueueSearch(deadPathsIt->second, nullptr, moveDef, ZeroVector, ZeroVector, -1.0f, true);
	// 	deadPathsToRemove.push_back(deadPathsIt);
	// }
	//	});

	// for (auto it = deadPathsToRemove.begin(); it != deadPathsToRemove.end(); ++it) {
	// 	delete ((*it)->second);
	// 	deadPaths.erase(*it);
	// }
}

#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"

unsigned int QTPFS::PathManager::QueueSearch(
	const IPath* oldPath,
	const CSolidObject* object,
	const MoveDef* moveDef,
	const float3& sourcePoint,
	const float3& targetPoint,
	const float radius,
	const bool synced
) {
	// TODO:
	//     introduce synced and unsynced path-caches;
	//     somehow support extra-cost overlays again
	if (!synced)
		return 0;

	// NOTE:
	//     all paths get deleted by the cache they are in;
	//     all searches get deleted by subsequent Update's
	// NOTE:
	//     the path-owner object handed to us can never become
	//     dangling (even with delayed execution) because ~GMT
	//     calls DeletePath, which ensures any path is removed
	//     from its cache before we get to ExecuteSearch
	// IPath* newPath = new IPath();
	// PathSearch* newSearch = new PathSearch(PATH_SEARCH_ASTAR);

	// Always create the search object first to ensure pathEntity can never be 0 (which is
	// considered a non-path)
	entt::entity searchEntity = registry.create();
	PathSearch* newSearch = &registry.emplace<PathSearch>(searchEntity, PATH_SEARCH_ASTAR);

	entt::entity pathEntity = (oldPath != nullptr) ? (entt::entity)oldPath->GetID() : registry.create();
	IPath* newPath = &(registry.get_or_emplace<IPath>(pathEntity));
	// LOG("%s: newPath %p", __func__, newPath);

	assert(newPath != nullptr);
	assert(newSearch != nullptr);

	const CUnit* unit = dynamic_cast<const CUnit*>(object);

	// LOG("%s: %s _ %d -> %d ", __func__
	// 		, unit != nullptr ? unit->unitDef->name.c_str() : "non-unit"
	// 		, (oldPath != nullptr) ? oldPath->GetPathType() : moveDef->pathType
	// 		, moveDef->pathType
	// 		);

	if (oldPath != nullptr) {
		assert(oldPath->GetID() != 0);
		assert((entt::entity)oldPath->GetID() != entt::null);
		// argument values are unused in this case
		assert(object == nullptr);
		assert(sourcePoint == ZeroVector);
		assert(targetPoint == ZeroVector);
		assert(radius == -1.0f);

		const CSolidObject* obj = oldPath->GetOwner();
		const float3& pos = (obj != nullptr)? obj->pos: oldPath->GetSourcePoint();

		newPath->SetHash(-1);
		// newPath->SetID(oldPath->GetID());
		newPath->SetNextPointIndex(0);
		newPath->SetNumPathUpdates(oldPath->GetNumPathUpdates() + 1);
		// newPath->SetRadius(oldPath->GetRadius());
		// newPath->SetSynced(oldPath->GetSynced());

		// start re-request from the current point
		// along the path, not the original source
		// (oldPath->GetSourcePoint())
		newPath->AllocPoints(2);
		// newPath->SetOwner(oldPath->GetOwner());
		newPath->SetSourcePoint(pos);
		// newPath->SetTargetPoint(oldPath->GetTargetPoint());
		// newSearch->SetID(oldPath->GetID());
		newSearch->SetTeam(teamHandler.ActiveTeams());
	} else {
		// NOTE:
		//     the unclamped end-points are temporary
		//     zero is a reserved ID, so pre-increment
		// newPath->SetID(++numPathRequests);
		newPath->SetID((int)pathEntity);
		newPath->SetRadius(radius);
		newPath->SetSynced(synced);
		newPath->AllocPoints(2);
		newPath->SetOwner(object);
		newPath->SetSourcePoint(sourcePoint);
		newPath->SetTargetPoint(targetPoint);
		newSearch->SetID(newPath->GetID());
		newSearch->SetTeam((object != nullptr)? object->team: teamHandler.ActiveTeams());

		registry.emplace<PathIsTemp>(pathEntity);
	}

	// assert((pathCaches[moveDef->pathType].GetTempPath(newPath->GetID()))->GetID() == 0);

	newPath->SetPathType(moveDef->pathType);
	newSearch->SetPathType(moveDef->pathType);

	if (moveDef->pathType == 2) {
		// LOG("%s: com path", __func__);
		newPath->SetPathType(moveDef->pathType);
	}

	// map the path-ID to the index of the cache that stores it
	// pathTypes[newPath->GetID()] = moveDef->pathType;
	// pathSearches.push_back(newSearch);
	// pathCaches[moveDef->pathType].AddTempPath(newPath);

	// LOG("%s: %s (%x) %d -> %d ", __func__
	// 		, unit != nullptr ? unit->unitDef->name.c_str() : "non-unit"
	// 		, newPath->GetID()
	// 		, (oldPath != nullptr) ? oldPath->GetPathType() : -1
	// 		, moveDef->pathType
	// 		);

	return (newPath->GetID());
}

void QTPFS::PathManager::UpdatePath(const CSolidObject* owner, unsigned int pathID) {
	// const PathTypeMapIt pathTypeIt = pathTypes.find(pathID);

	// if (pathTypeIt != pathTypes.end()) {
		// PathCache& pathCache = pathCaches[pathTypeIt->second];
		// IPath* livePath = pathCache.GetLivePath(pathID);

		// entt::entity pathEntity = (entt::entity)pathID;
		// IPath* livePath = &registry.get<IPath>(pathEntity);

		// if (livePath->GetID() != 0) {
		// 	assert(owner == livePath->GetOwner());
		// }
	// }
}

void QTPFS::PathManager::DeletePath(unsigned int pathID) {
	// const PathTypeMapIt pathTypeIt = pathTypes.find(pathID);
	const PathTraceMapIt pathTraceIt = pathTraces.find(pathID);

	// if (pathTypeIt != pathTypes.end()) {
	// 	PathCache& pathCache = pathCaches;
	// 	pathCache.DelPath(pathID);

	// 	pathTypes.erase(pathTypeIt);
	// }

	// LOG("%s: %x", __func__, pathID);
	pathCache.DelPath(pathID);

	if (pathTraceIt != pathTraces.end()) {
		delete (pathTraceIt->second);
		pathTraces.erase(pathTraceIt);
	}
}

unsigned int QTPFS::PathManager::RequestPath(
	CSolidObject* object,
	const MoveDef* moveDef,
	float3 sourcePoint,
	float3 targetPoint,
	float radius,
	bool synced
) {
	if (!IsFinalized())
		return 0;

	return (QueueSearch(nullptr, object, moveDef, sourcePoint, targetPoint, radius, synced));
}



bool QTPFS::PathManager::PathUpdated(unsigned int pathID) {
	// const PathTypeMapIt pathTypeIt = pathTypes.find(pathID);

	// if (pathTypeIt == pathTypes.end())
	// 	return false;

	// PathCache& pathCache = pathCaches[pathTypeIt->second];
	// IPath* livePath = pathCache.GetLivePath(pathID);

	entt::entity pathEntity = (entt::entity)pathID;
	if (!registry.valid(pathEntity)) { return false; }
	IPath* livePath = registry.try_get<IPath>(pathEntity);

	// if (livePath->GetID() == 0)
	if (livePath == nullptr)
		return false;

	if (livePath->GetNumPathUpdates() == 0)
		return false;

	livePath->SetNumPathUpdates(livePath->GetNumPathUpdates() - 1);
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
	// in misc since it is called from many points
	SCOPED_TIMER("Misc::Path::NextWayPoint");

	// const PathTypeMap::const_iterator pathTypeIt = pathTypes.find(pathID);
	const float3 noPathPoint = -XZVector;

	if (!IsFinalized())
		return noPathPoint;
	if (!synced)
		return noPathPoint;

	// dangling ID after a re-request failure or regular deletion
	// return an error-vector so GMT knows it should stop the unit
	// if (pathTypeIt == pathTypes.end())
	// 	return noPathPoint;

	// IPath* tempPath = pathCaches[pathTypeIt->second].GetTempPath(pathID);
	// IPath* livePath = pathCaches[pathTypeIt->second].GetLivePath(pathID);

	entt::entity pathEntity = (entt::entity)pathID;
	if (!registry.valid(pathEntity)) { return noPathPoint; }
	IPath* livePath = registry.try_get<IPath>(pathEntity);
	if (livePath == nullptr) { return noPathPoint; }

	// if (tempPath->GetID() != 0) {
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
		// const float3& targetPoint = tempPath->GetTargetPoint();
		const float3& targetPoint = livePath->GetTargetPoint();
		const float3  targetDirec = (targetPoint - sourcePoint).SafeNormalize() * SQUARE_SIZE;
		return float3(sourcePoint.x + targetDirec.x, -1.0f, sourcePoint.z + targetDirec.z);
	}
	// if (livePath->GetID() == 0) {
	if (registry.all_of<PathIsDirty>(pathEntity)) {
		// the request WAS processed but then immediately undone by a
		// TerrainChange --> MarkDeadPaths event in the same frame as
		// NextWayPoint (so pathID is only in deadPaths)
		return point;
	}

	float minRadiusSq = QTPFS_POSITIVE_INFINITY;

	unsigned int minPointIdx = livePath->GetNextPointIndex();
	unsigned int nxtPointIdx = 1;

	for (unsigned int i = (livePath->GetNextPointIndex()); i < (livePath->NumPoints() - 1); i++) {
		const float radiusSq = (point - livePath->GetPoint(i)).SqLength2D();

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
		//     and we must fall back to the radius-based closest point
		if (v0.SqLength() < 0.1f) { nxtPointIdx = i + 1; break; }
		if (v1.SqLength() < 0.1f) { nxtPointIdx = i + 2; break; }
		if (v0.dot(v1) <= -0.01f) { nxtPointIdx = i + 1;        }

		if (radiusSq < minRadiusSq) {
			minRadiusSq = radiusSq;
			minPointIdx = i + 0;
		}
	}

	// handle a corner-case in which a unit is at the start of its path
	// and the goal is in front of it, but on the other side of a cliff
	if ((livePath->GetNextPointIndex() == 0) && (nxtPointIdx == (livePath->NumPoints() - 1)))
		nxtPointIdx = 1;

	if (minPointIdx < nxtPointIdx) {
		// if close enough to at least one waypoint <i>,
		// switch to the point immediately following it
		livePath->SetNextPointIndex(nxtPointIdx);
	} else {
		// otherwise just pick the closest point
		livePath->SetNextPointIndex(minPointIdx);
	}

	return (livePath->GetPoint(livePath->GetNextPointIndex()));
}



void QTPFS::PathManager::GetPathWayPoints(
	unsigned int pathID,
	std::vector<float3>& points,
	std::vector<int>& starts
) const {
	// const PathTypeMap::const_iterator pathTypeIt = pathTypes.find(pathID);

	if (!IsFinalized())
		return;
	// if (pathTypeIt == pathTypes.end())
	// 	return;

	// PathCache& pathCache = pathCaches[pathTypeIt->second];
	// const IPath* path = cache.GetLivePath(pathID);
	entt::entity pathEntity = (entt::entity)pathID;
	if (!registry.valid(pathEntity))
		return;

	const IPath* path = registry.try_get<IPath>(pathEntity);

	// if (path->GetID() == 0)
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

	data.x = registry.size();
	data.y = mapChangeTrack.damageQueue.size();

	return data;
}

