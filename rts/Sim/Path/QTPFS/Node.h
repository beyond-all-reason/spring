/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_NODE_HDR
#define QTPFS_NODE_HDR

#include <array>
#include <cinttypes>
#include <fstream>
#include <limits>
#include <variant>
#include <vector>


#include "PathEnums.h"
#include "PathDefines.h"

#include "System/float3.h"
#include "System/Rectangle.h"

#ifndef QTPFS_VIRTUAL_NODE_FUNCTIONS
#define QTNode INode
#endif

#define QTNODE_CHILD_COUNT 4

namespace QTPFS {
	struct NodeLayer;
	struct SearchNode;
	struct UpdateThreadData;

	struct INode {
			friend SearchNode;
	public:
		struct NeighbourPoints {
			int nodeId;
			std::array<float2, QTPFS_MAX_NETPOINTS_PER_NODE_EDGE> netpoints;
		};

		void SetNodeNumber(unsigned int n) { nodeNumber = n; }
		unsigned int GetNodeNumber() const { return nodeNumber; }

		unsigned int GetNeighborRelation(const INode* ngb) const;
		unsigned int GetRectangleRelation(const SRectangle& r) const;
		float GetDistance(const INode* n, unsigned int type) const;
		float2 GetNeighborEdgeTransitionPoint(const INode* ngb, const float3& pos, float alpha) const;
		SRectangle ClipRectangle(const SRectangle& r) const;

		unsigned int GetIndex() const { return index & NODE_INDEX_MASK; }
		bool IsExitOnly() const { return !!(index & EXIT_ONLY_MASK); }

		unsigned int GetDepth() const { return (index & DEPTH_MASK) >> DEPTH_BIT_OFFSET; }
		unsigned int GetRawIndex() const { return index; }

		~QTNode() = default;
		QTNode() = default;
		QTNode(const QTNode& n) = delete;
		QTNode(QTNode&& n) = default;

		QTNode& operator = (const QTNode& n) = delete;
		QTNode& operator = (QTNode&& n) = default;

		static void InitStatic();

		void Init(
			const QTNode* parent,
			unsigned int nn,
			unsigned int x1, unsigned int z1,
			unsigned int x2, unsigned int z2,
			unsigned int idx
		);

		// NOTE:
		//     root-node identifier is always 0
		//     <i> is a NODE_IDX index in [0, 3]
		unsigned int GetChildID(unsigned int i, uint32_t rootMask) const {
			// uint32_t rootId = rootMask & nodeNumber;
			// uint32_t nodeId = ((~rootMask) & nodeNumber);
			uint32_t shift = (MAX_DEPTH - (GetDepth() + 1)) * QTPFS_NODE_NUMBER_SHIFT_STEP;
			return nodeNumber + ((i + 1) << shift);
		}

		std::uint64_t GetCheckSum(const NodeLayer& nl) const;

		void PreTesselate(NodeLayer& nl, const SRectangle& r, SRectangle& ur, unsigned int depth, const UpdateThreadData* threadData);
		void Tesselate(NodeLayer& nl, const SRectangle& r, unsigned int depth, const UpdateThreadData* threadData);
		void Serialize(std::fstream& fStream, NodeLayer& nodeLayer, unsigned int* streamSize, unsigned int depth, bool readMode);

		bool IsLeaf() const { return (childBaseIndex == -1u); }
		bool CanSplit(unsigned int depth, bool forced) const;

		bool Split(NodeLayer& nl, unsigned int depth, bool forced);
		bool Merge(NodeLayer& nl);

		unsigned int GetMaxNumNeighbors() const;
		bool UpdateNeighborCache(NodeLayer& nodeLayer, UpdateThreadData& threadData);

		int xmin() const { return _xmin; }
		int zmin() const { return _zmin; }
		int xmax() const { return _xmax; }
		int zmax() const { return _zmax; }
		unsigned int xmid() const { return ((xmin() + xmax()) >> 1); }
		unsigned int zmid() const { return ((zmin() + zmax()) >> 1); }
		unsigned int xsize() const { return (xmax() - xmin()); }
		unsigned int zsize() const { return (zmax() - zmin()); }
		unsigned int area() const { return (xsize() * zsize()); }
		unsigned int point(int i) const { return points[i]; }


		bool RectIsInside(const SRectangle& rect) const {
			return
				xmin() <= rect.x1 && zmin() <= rect.y1 &&
				xmax() >= rect.x2 && zmax() >= rect.y2;
		}

		bool RectIntersects(const SRectangle& rect) const {
			return  !( xmin() >= rect.x2
					|| xmax() <= rect.x1
					|| zmin() >= rect.z2
					|| zmax() <= rect.z1);
		}

		// true if this node is fully open (partially open nodes have larger but non-infinite cost)
		bool AllSquaresAccessible() const { return (moveCostAvg < (QTPFS_CLOSED_NODE_COST / float(area()))); }
		bool AllSquaresImpassable() const { return (moveCostAvg == QTPFS_POSITIVE_INFINITY); }

		void SetMoveCost(float cost) { moveCostAvg = cost; }
		float GetSpeedMod() const { return 1.0f / moveCostAvg; }
		float GetMoveCost() const { return moveCostAvg; }

		unsigned int GetChildBaseIndex() const { return childBaseIndex; }

		static unsigned int MinSizeX() { return MIN_SIZE_X; }
		static unsigned int MinSizeZ() { return MIN_SIZE_Z; }

		const std::vector<NeighbourPoints>& GetNeighbours() const {
			return neighbours;
		}

		void DeactivateNode() { _xmin = std::numeric_limits<decltype(_xmin)>::max(); }
		bool NodeDeactivated() const { return (_xmin == std::numeric_limits<decltype(_xmin)>::max()); }

	private:
		bool UpdateMoveCost(
			const UpdateThreadData* threadData,
			NodeLayer& nl,
			const SRectangle& r,
			unsigned int& numNewBinSquares,
			unsigned int& numDifBinSquares,
			unsigned int& numClosedSquares,
			bool& wantSplit,
			bool& needSplit
		);

		bool UpdateExitOnly(
			NodeLayer& nl,
			bool& needSplit
		);

		static unsigned int MIN_SIZE_X;
		static unsigned int MIN_SIZE_Z;

	public:
		static unsigned int MAX_DEPTH;

	private:
		// Mask off the bits for node index, we can use higher bits for other purposes. We wouldn't normally, worry
		// about so much about this, but when you have hundreds of thousands of nodes per movetype, the memory used
		// adds up rather quickly so it is important to keep these objects as small as possible. 
		static constexpr unsigned int NODE_INDEX_MASK = 0x000fffff;

		static constexpr unsigned int EXIT_ONLY_BIT_OFFSET = 31;
		static constexpr unsigned int EXIT_ONLY_MASK = (0x1 << EXIT_ONLY_BIT_OFFSET);

		static constexpr unsigned int DEPTH_BIT_OFFSET = 20;
		static constexpr unsigned int DEPTH_MASK = (0xf << DEPTH_BIT_OFFSET);

		unsigned int nodeNumber = -1u;
		unsigned int index = 0;

		union {
			struct {
				unsigned short _xmin;
				unsigned short _xmax;
				unsigned short _zmin;
				unsigned short _zmax;
			};
			std::array<unsigned short, 4> points = {};
		};

		float moveCostAvg = -1.0f;

		unsigned int childBaseIndex = -1u;
		std::vector<NeighbourPoints> neighbours;
	};

	struct NodeSearched {};

	struct SearchNode {

		SearchNode() {}

		SearchNode(INode& srcNode)
			: index(srcNode.index)
			, nodeNumber(srcNode.nodeNumber)
			{}

		SearchNode(INode* srcNode)
			: index(srcNode->index)
			, nodeNumber(srcNode->nodeNumber)
			{}

		SearchNode(int nodeId)
			: index(nodeId)
			{}

		SearchNode(const SearchNode& other) { operator=(other); }

		SearchNode& operator=(const SearchNode& other) {
			fCost = QTPFS_POSITIVE_INFINITY;
			gCost = QTPFS_POSITIVE_INFINITY;
			hCost = QTPFS_POSITIVE_INFINITY;
			index = other.index;
			prevNode = other.prevNode;
			xmin = other.xmin;
			zmin = other.zmin;
			xmax = other.xmax;
			zmax = other.zmax;
			nodeNumber = other.nodeNumber;
			badNode = other.badNode;
			// searchState = other.searchState;
			return *this;
		}

		void CopyGeneralNodeData(const SearchNode& other) {
			index = other.index;
			xmin = other.xmin;
			zmin = other.zmin;
			xmax = other.xmax;
			zmax = other.zmax;
			nodeNumber = other.nodeNumber;
		}

		float GetHeapPriority() const { return GetPathCost(NODE_PATH_COST_F); }
		// unsigned int GetSearchState() const { return searchState; }

		bool operator <  (const SearchNode* n) const { return (fCost <  n->fCost); }
		bool operator >  (const SearchNode* n) const { return (fCost >  n->fCost); }
		bool operator == (const SearchNode* n) const { return (fCost == n->fCost); }
		bool operator <= (const SearchNode* n) const { return (fCost <= n->fCost); }
		bool operator >= (const SearchNode* n) const { return (fCost >= n->fCost); }

		void SetPathCosts(float g, float h) { fCost = g + h; gCost = g; hCost = h; }
		const float* GetPathCosts() const { return &fCost; }
		float GetPathCost(unsigned int type) const;

		void SetPrevNode(SearchNode* n) { prevNode = n; }
		SearchNode* GetPrevNode() const { return prevNode; }

		unsigned int GetIndex() const { return index; }
		// void SetSearchState(unsigned int state) { searchState = state; }

		void SetNeighborEdgeTransitionPoint(const float2& point) { selectedNetpoint = point; }
		const float2& GetNeighborEdgeTransitionPoint() const { return selectedNetpoint; }

		uint32_t GetStepIndex() const { return stepIndex; }
		void SetStepIndex(uint32_t idx) { stepIndex = idx; }

		bool isNodeBad() const { return badNode; /*selectedNetpoint.x == -1.f;*/ }
		void SetNodeBad(bool state) { badNode = state; }

		unsigned int index = 0;
		// unsigned int searchState = 0;

		float fCost = QTPFS_POSITIVE_INFINITY; // TODO: drop this as it is only a derived field?
		float gCost = QTPFS_POSITIVE_INFINITY;
		float hCost = QTPFS_POSITIVE_INFINITY;

		// points back to previous node in path
		SearchNode* prevNode = nullptr;

		float2 selectedNetpoint;

		uint32_t stepIndex = 0;

		int xmin = 0;
		int zmin = 0;
		int xmax = 0;
		int zmax = 0;

		unsigned int nodeNumber = -1;
		bool badNode = false;
	};
}

#endif

