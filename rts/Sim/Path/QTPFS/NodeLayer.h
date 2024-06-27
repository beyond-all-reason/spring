/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_NODELAYER_H_
#define QTPFS_NODELAYER_H_

// #undef NDEBUG

#include <limits>
#include <vector>
#include <deque>
#include <cinttypes>

#include "System/Rectangle.h"
#include "Node.h"
#include "PathDefines.h"
#include "PathThreads.h"

#include "System/Log/ILog.h"
#include "System/Rectangle.h"

#include "Registry.h"

struct MoveDef;

namespace QTPFS {
	struct INode;

	struct NodeLayer {
	public:
		static void InitStatic();
		static size_t MaxSpeedModTypeValue() { return (std::numeric_limits<SpeedModType>::max()); }
		static size_t MaxSpeedBinTypeValue() { return (std::numeric_limits<SpeedBinType>::max()); }

		~NodeLayer() { registry.clear(); }

		NodeLayer() = default;
		NodeLayer(NodeLayer&& nl) = default;

		NodeLayer& operator = (const NodeLayer& nl) = delete;
		NodeLayer& operator = (NodeLayer&& nl) = default;

		void Init(unsigned int layerNum);
		void Clear();

		bool Update(UpdateThreadData& threadData);

		void ExecNodeNeighborCacheUpdates(const SRectangle& ur, UpdateThreadData& threadData);
		float GetNodeRatio() const { return (numLeafNodes / std::max(1.0f, float(xsize * zsize))); }

		const INode* GetNode(unsigned int x, unsigned int z) const {
			ZoneScoped;
			int iz = (z / rootNodeSize) * xRootNodes;
			int ix = (x / rootNodeSize);
			int i = iz + ix;

			const INode* curNode = GetPoolNode(i);

			assert (curNode->xmin() <= x);
			assert (curNode->xmax() >= x);
			assert (curNode->zmin() <= z);
			assert (curNode->zmax() >= z);

			while (!curNode->IsLeaf()) {
				bool isRight = x >= curNode->xmid();
				bool isDown = z >= curNode->zmid();
				int offset = 1*(isRight) + 2*(isDown);
				int nextIndex = curNode->GetChildBaseIndex() + offset;
				curNode = GetPoolNode(nextIndex);
			}

			return curNode;
		}
		INode* GetNode(unsigned int x, unsigned int z) {
			const auto* cthis = this;
			return const_cast<INode*>(cthis->GetNode(x, z));
		}

		const INode* GetPoolNode(unsigned int i) const { return &poolNodes[i / POOL_CHUNK_SIZE][i % POOL_CHUNK_SIZE]; }
		      INode* GetPoolNode(unsigned int i)       { return &poolNodes[i / POOL_CHUNK_SIZE][i % POOL_CHUNK_SIZE]; }

		unsigned int AllocPoolNode(const INode* parent, unsigned int nn,  unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2) {
			unsigned int idx = -1u;

			if (nodeIndcs.empty())
				return idx;

			if (poolNodes[(idx = nodeIndcs.back()) / POOL_CHUNK_SIZE].empty())
				poolNodes[idx / POOL_CHUNK_SIZE].resize(POOL_CHUNK_SIZE);

			poolNodes[idx / POOL_CHUNK_SIZE][idx % POOL_CHUNK_SIZE].Init(parent, nn, x1, z1, x2, z2, idx);
			nodeIndcs.pop_back();

			if (idx >= maxNodesAlloced) { maxNodesAlloced = idx + 1; }

			// LOG("%s: [%p] alloc'ed id=%d", __func__, &poolNodes, idx);

			return idx;
		}

		int GetMaxNodesAlloced() const { return maxNodesAlloced; }

		void FreePoolNode(unsigned int nodeIndex) {
			//LOG("%s: [%p] free'ed id=%d", __func__, &poolNodes, nodeIndex);
			nodeIndcs.push_back(nodeIndex);
			auto* curNode = GetPoolNode(nodeIndex);
			curNode->DeactivateNode();
		}

		void DecreaseOpenNodeCounter() { assert(numOpenNodes > 0); numOpenNodes -= (numOpenNodes > 0); }
		void DecreaseClosedNodeCounter() { assert(numClosedNodes > 0); numClosedNodes -= (numClosedNodes > 0); }

		void IncreaseOpenNodeCounter() { numOpenNodes++; }
		void IncreaseClosedNodeCounter() { numClosedNodes++; }

		unsigned int GetNumOpenNodes() const { return numOpenNodes; }
		unsigned int GetNumClosedNodes() const { return numClosedNodes; }

		const std::vector<SpeedBinType>& GetCurSpeedBins() const { return curSpeedBins; }
		const std::vector<SpeedModType>& GetCurSpeedMods() const { return curSpeedMods; }

		void SetNumLeafNodes(unsigned int n) { numLeafNodes = n; }
		unsigned int GetNumLeafNodes() const { return numLeafNodes; }

		SpeedBinType GetSpeedModBin(float absSpeedMod, float relSpeedMod) const;

		std::uint64_t GetMemFootPrint() const {
			std::uint64_t memFootPrint = sizeof(NodeLayer);
			memFootPrint += (curSpeedMods.size() * sizeof(SpeedModType));
			memFootPrint += (curSpeedBins.size() * sizeof(SpeedBinType));

			memFootPrint += (selectedNodes.size() * sizeof(decltype(selectedNodes)::value_type));
			memFootPrint += (openNodes.size()     * sizeof(decltype(openNodes)::value_type));

			for (size_t i = 0, n = NUM_POOL_CHUNKS; i < n; i++) {
				memFootPrint += (poolNodes[i].size() * sizeof(QTNode));

				for (int32_t nodeIndex = poolNodes[i].size() - 1; nodeIndex >= 0; --nodeIndex) {
					const auto& neighbours = poolNodes[i][nodeIndex].GetNeighbours();
					memFootPrint += neighbours.size() * sizeof(std::remove_reference_t<decltype(neighbours)>::value_type);
				}
			}

			memFootPrint += (nodeIndcs.size() * sizeof(decltype(nodeIndcs)::value_type));
			return memFootPrint;
		}

		void SetRootNodeCountAndDimensions(int numRoots, int xsize, int zside, int maxNodeSize) {
			numRootNodes = numRoots;
			xRootNodes = xsize;
			zRootNodes = zside;
			rootNodeSize = maxNodeSize;
		} 

		int GetRootNodeCount() const {
			return numRootNodes;
		}

		int GetNodelayer() const {
			return layerNumber;
		}

		void GetNodesInArea(const SRectangle& areaToSearch, std::vector<INode*>& nodesFound);
		INode* GetNearestNodeInArea(const SRectangle& areaToSearch, int2 referencePoint, std::vector<INode*>& openNodes);
		INode* GetNodeThatEncasesPowerOfTwoArea(const SRectangle& areaToEncase);

		bool UseShortestPath() { return useShortestPath; }

	private:
		std::vector<QTNode> poolNodes[16];
		std::vector<unsigned int> nodeIndcs;

		std::vector<INode*> selectedNodes;
		std::vector<INode*> openNodes;

		std::vector<SpeedModType> curSpeedMods;
		std::vector<SpeedBinType> curSpeedBins;

public:
		static constexpr unsigned int NUM_POOL_CHUNKS = sizeof(poolNodes) / sizeof(poolNodes[0]);
		static constexpr unsigned int POOL_TOTAL_SIZE = (1024 * 1024) / 2;
		static constexpr unsigned int POOL_CHUNK_SIZE = POOL_TOTAL_SIZE / NUM_POOL_CHUNKS;

		void SetRootMask(uint32_t newMask) { rootMask = newMask; }
		uint32_t GetRootMask() const { return rootMask; }

		QTNode* GetRootNode(int x, int z) {
			int iz = (z / rootNodeSize) * xRootNodes;
			int ix = (x / rootNodeSize);
			int i = iz + ix;

			assert(i < numRootNodes);

			return GetPoolNode(i);
		}

		const QTNode* GetRootNode(int x, int z) const {
			// This is fine, the class doesn't get modified in the process of the call.
			// This call is to add const to the return value.
			return const_cast<QTPFS::NodeLayer *>(this)->GetRootNode(x, z);
		}

public:

		// NOTE:
		//   we need a fixed range that does not become wider / narrower
		//   during terrain deformations (otherwise the bins would change
		//   across ALL nodes)
		static unsigned int NUM_SPEEDMOD_BINS;
		static float        MIN_SPEEDMOD_VALUE;
		static float        MAX_SPEEDMOD_VALUE;

private:
		unsigned int layerNumber = 0;
		unsigned int numLeafNodes = 0;
		unsigned int updateCounter = 0;
		unsigned int numOpenNodes = 0;
		unsigned int numClosedNodes = 0;

		int32_t maxNodesAlloced = 0;
		int32_t numRootNodes = 0;
		int32_t xRootNodes = 0;
		int32_t zRootNodes = 0;
		int32_t rootNodeSize = 0;
		uint32_t rootMask = 0;

		unsigned int xsize = 0;
		unsigned int zsize = 0;

		float maxRelSpeedMod = 0.0f; // TODO: Remove these?
		float avgRelSpeedMod = 0.0f;
		bool useShortestPath = false;
	};
}

#endif

