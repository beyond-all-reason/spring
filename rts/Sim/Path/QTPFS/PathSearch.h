/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATHSEARCH_HDR
#define QTPFS_PATHSEARCH_HDR

#include <queue>
#include <vector>

#include "PathDefines.h"
#include "PathThreads.h"

#include "System/float3.h"

struct CollisionVolume;

namespace QTPFS {
	struct IPath;
	struct NodeLayer;
	struct PathCache;
	struct SearchNode;

	namespace PathSearchTrace {
		struct Iteration {
			Iteration() { nodeIndices.push_back(-1u); }
			Iteration(const Iteration& i) { *this = i; }
			Iteration(Iteration&& i) { *this = std::move(i); }

			Iteration& operator = (const Iteration& i) { nodeIndices = i.nodeIndices; return *this; }
			Iteration& operator = (Iteration&& i) { nodeIndices = std::move(i.nodeIndices); return *this; }

			void Clear() {
				nodeIndices.clear();
				nodeIndices.push_back(-1u);
			}
			void SetPoppedNodeIdx(unsigned int i) { (nodeIndices.front()) = i; }
			void AddPushedNodeIdx(unsigned int i) { (nodeIndices.push_back(i)); }

			const std::vector<unsigned int>& GetNodeIndices() const { return nodeIndices; }

		private:
			// NOTE: indices are only valid so long as tree is not re-tesselated
			std::vector<unsigned int> nodeIndices;
		};

		struct Execution {
			Execution(unsigned int f): searchFrame(f) {}
			Execution(const Execution& e) = delete;
			Execution(Execution&& e) { *this = std::move(e); }

			Execution& operator = (const Execution& e) = delete;
			Execution& operator = (Execution&& e) {
				searchFrame = e.GetFrame();
				iterations = std::move(e.iterations);
				return *this;
			}

			void AddIteration(const Iteration& iter) { iterations.push_back(iter); }
			const std::vector<Iteration>& GetIterations() const { return iterations; }

			unsigned int GetFrame() const { return searchFrame; }

			size_t GetMemFootPrint() const {
				return iterations.size() * sizeof(decltype(iterations)::value_type);
			};

		private:
			std::vector<Iteration> iterations;

			// sim-frame at which the search was executed
			unsigned int searchFrame;
		};
	}

	struct PathSearch {
		void SetID(unsigned int n) { searchID = n; }
		void SetTeam(unsigned int n) { searchTeam = n; }
		unsigned int GetID() const { return searchID; }
		unsigned int GetTeam() const { return searchTeam; }

	protected:
		unsigned int searchID;     // links us to the temp-path that this search will finalize
		unsigned int searchTeam;   // which team queued this search

		unsigned int searchType;   // indicates if Dijkstra (h==0) or A* (h!=0) search is employed
		unsigned int searchState;  // offset that identifies nodes as part of current search

	public:
		static void InitStatic();

		PathSearch()
			: searchID(0)
			, searchTeam(0)
			, searchType(0)
			, searchState(0)
			, nodeLayer(nullptr)
			, searchExec(nullptr)
			, hCostMult(0.0f)
			, haveFullPath(false)
			, havePartPath(false)
			, pathOwner(nullptr)
			, tryPathRepair(false)
			{}
		PathSearch(unsigned int pathSearchType)
			: PathSearch()
			{ searchType = pathSearchType; }
		~PathSearch() { /* openNodes->reset(); */ }

		void Initialize(
			NodeLayer* layer,
			const float3& sourcePoint,
			const float3& targetPoint,
			const CSolidObject* owner
		);
		void InitializeThread(SearchThreadData* threadData);
		void PreLoadNode(uint32_t dir, uint32_t nodeId, uint32_t prevNodeId, const float2& netPoint, uint32_t stepIndex);
		void LoadPartialPath(IPath* path);
		void LoadRepairPath();
		bool Execute(unsigned int searchStateOffset = 0);
		void Finalize(IPath* path);
		bool SharedFinalize(const IPath* srcPath, IPath* dstPath);
		PathSearchTrace::Execution* GetExecutionTrace() { return searchExec; }

		const PathHashType GetHash() const { return pathSearchHash; };
		const PathHashType GetPartialSearchHash() const { return pathPartialSearchHash; };

		bool PathWasFound() const { return haveFullPath | havePartPath; }

		void SetPathType(int newPathType) { pathType = newPathType; }
		int GetPathType() const { return pathType; }

		void SetGoalDistance(float dist) { goalDistance = dist; }

		const CSolidObject* Getowner() const { return pathOwner; }

	private:
		struct DirectionalSearchData {
			DirectionalSearchData()
				: openNodes(nullptr)
				, srcSearchNode(nullptr)
				, tgtSearchNode(nullptr)
				, minSearchNode(nullptr)
			{}

			// global queue: allocated once, re-used by all searches without clear()'s
			// this relies on INode::operator< to sort the INode*'s by increasing f-cost
			SearchPriorityQueue* openNodes;

			SearchNode *srcSearchNode, *tgtSearchNode;
			float3 srcPoint, tgtPoint;
			SearchNode *minSearchNode;
			SearchNode *repairPathRealSrcSearchNode;
		};

		void ResetState(SearchNode* node, struct DirectionalSearchData& searchData, const float3& srcPoint);
		void UpdateNode(SearchNode* nextNode, SearchNode* prevNode, unsigned int netPointIdx);
		void LocalUpdateNode(SearchNode* nextNode, SearchNode* prevNode, float gCost, float hCost, const float2& netPoint);

		void InitSearchNodeData(QTPFS::SearchNode *curSearchNode, QTPFS::INode *curNode) const {
			curSearchNode->xmin = curNode->xmin();
			curSearchNode->xmax = curNode->xmax();
			curSearchNode->zmin = curNode->zmin();
			curSearchNode->zmax = curNode->zmax();
			curSearchNode->nodeNumber = curNode->GetNodeNumber();
		}


		void IterateNodes(unsigned int searchDir);
		void IterateNodeNeighbors(const INode* curNode, unsigned int searchDir);

		float3 FindNearestPointOnNodeToGoal(const QTPFS::SearchNode& node, const float3& goalPos) const;

		void TracePath(IPath* path);
		void SmoothPath(IPath* path);
		bool SmoothPathIter(IPath* path);
		void SmoothSharedPath(IPath* path);
		int SmoothPathPoints(const INode* nn0, const INode* nn1, const float3& p0, const float3& p1, const float3& p2, float3& result) const;

		void InitStartingSearchNodes();
		void UpdateHcostMult();
		void RemoveOutdatedOpenNodesFromQueue(int searchDir);
		bool IsNodeActive(const SearchNode& curSearchNode) const;

		bool ExecutePathSearch();
		bool ExecuteRawSearch();

		void SetForwardSearchLimit();

		void GetRectangleCollisionVolume(const SearchNode& snode, CollisionVolume& v, float3& rm) const;

		const PathHashType GenerateHash(const INode* srcNode, const INode* tgtNode) const;
		const PathHashType GenerateHash2(uint32_t p1, uint32_t p2) const;

		const PathHashType GenerateVirtualHash(const INode* srcNode, const INode* tgtNode) const;

		public:
		static const std::uint32_t GenerateVirtualNodeNumber(const QTPFS::NodeLayer& nodeLayer, const INode* startNode, int nodeMaxSize, int x, int z, uint32_t* depth = nullptr);

		private:
		QTPFS::SearchThreadData* searchThreadData;

		// Identifies the layer, target quad and source quad for a search query so that similar
		// searches can be combined.
		PathHashType pathSearchHash;

		// Similar to hash, but the target quad and source quad numbers may not relate to actual
		// leaf nodes in the quad tree. They represent the quad that would be there if the leaf node
		// was exactly the size of QTPFS_PARTIAL_SHARE_PATH_MAX_SIZE. This allows searches that
		// start and/or end in different, but close, quads. This is used to handle partially-
		// shared path searches.
		PathHashType pathPartialSearchHash;

		const CSolidObject* pathOwner;
		NodeLayer* nodeLayer;
		int pathType;

		// not used unless QTPFS_TRACE_PATH_SEARCHES is defined
		PathSearchTrace::Execution* searchExec;
		PathSearchTrace::Iteration searchIter;

		SearchNode *curSearchNode, *nextSearchNode;

		DirectionalSearchData directionalSearchData[2];

		float2 netPoints[QTPFS_MAX_NETPOINTS_PER_NODE_EDGE];
		float3 goalPos;
		float3 searchLimitMins;
		float3 searchLimitMaxs;

		float gDists[QTPFS_MAX_NETPOINTS_PER_NODE_EDGE];
		float hDists[QTPFS_MAX_NETPOINTS_PER_NODE_EDGE];
		float gCosts[QTPFS_MAX_NETPOINTS_PER_NODE_EDGE];
		float hCosts[QTPFS_MAX_NETPOINTS_PER_NODE_EDGE];

		float hCostMult;
		float goalDistance = 0.f;
		float adjustedGoalDistance;

		int fwdStepIndex = 0;
		int bwdStepIndex = 0;

		int fwdNodeSearchLimit = 0;

		size_t fwdNodesSearched = 0;
		size_t bwdNodesSearched = 0;

		bool haveFullPath;
		bool havePartPath;
		bool badGoal;
		bool disallowNodeRevisit = false;

public:
		bool rawPathCheck = false;
		bool synced = false;
		bool pathRequestWaiting = false;
		bool doPartialSearch = false;
		bool tryPathRepair = false;
		bool rejectPartialSearch = false;
		bool allowPartialSearch = false;
		bool expectIncompletePartialSearch = false;
		bool searchEarlyDrop = false;
		bool initialized = false;
		bool partialReverseTrace = false;
		bool doPathRepair = false;

		bool fwdPathConnected = false;
		bool bwdPathConnected = false;
		bool useFwdPathOnly = false;

		// int postLoadRepairPathIndexOverride = 0;

		static float MAP_RELATIVE_MAX_NODES_SEARCHED;
		static int MAP_MAX_NODES_SEARCHED;
	};
}

#endif

