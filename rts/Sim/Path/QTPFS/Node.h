/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_NODE_HDR
#define QTPFS_NODE_HDR

#include <array>
#include <vector>
#include <fstream>
#include <cinttypes>

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

	struct INode {
			friend SearchNode;
	public:
		void SetNodeNumber(unsigned int n) { nodeNumber = n; }
		// void SetHeapIndex(unsigned int n) { heapIndex = n; }
		unsigned int GetNodeNumber() const { return nodeNumber; }
		// unsigned int GetHeapIndex() const { return heapIndex; }
		// float GetHeapPriority() const { return GetPathCost(NODE_PATH_COST_F); }

		// bool operator <  (const INode* n) const { return (fCost <  n->fCost); }
		// bool operator >  (const INode* n) const { return (fCost >  n->fCost); }
		// bool operator == (const INode* n) const { return (fCost == n->fCost); }
		// bool operator <= (const INode* n) const { return (fCost <= n->fCost); }
		// bool operator >= (const INode* n) const { return (fCost >= n->fCost); }

		#ifdef QTPFS_VIRTUAL_NODE_FUNCTIONS
		virtual void Serialize(std::fstream&, NodeLayer&, unsigned int*, unsigned int, bool) = 0;
		virtual unsigned int GetNeighbors(const std::vector<INode*>&, std::vector<INode*>&) = 0;
		virtual const std::vector<INode*>& GetNeighbors(const std::vector<INode*>& v) = 0;
		virtual bool UpdateNeighborCache(const std::vector<INode*>& nodes) = 0;

		virtual void SetNeighborEdgeTransitionPoint(unsigned int ngbIdx, const float2& point) = 0;
		virtual const float2& GetNeighborEdgeTransitionPoint(unsigned int ngbIdx) const = 0;
		#endif

		unsigned int GetNeighborRelation(const INode* ngb) const;
		unsigned int GetRectangleRelation(const SRectangle& r) const;
		float GetDistance(const INode* n, unsigned int type) const;
		float2 GetNeighborEdgeTransitionPoint(const INode* ngb, const float3& pos, float alpha) const;
		SRectangle ClipRectangle(const SRectangle& r) const;

		#ifdef QTPFS_VIRTUAL_NODE_FUNCTIONS
		virtual unsigned int xmin() const = 0;
		virtual unsigned int zmin() const = 0;
		virtual unsigned int xmax() const = 0;
		virtual unsigned int zmax() const = 0;
		virtual unsigned int xmid() const = 0;
		virtual unsigned int zmid() const = 0;
		virtual unsigned int xsize() const = 0;
		virtual unsigned int zsize() const = 0;
		virtual unsigned int area() const = 0;

		virtual bool AllSquaresAccessible() const = 0;
		virtual bool AllSquaresImpassable() const = 0;

		virtual float GetSpeedMod() const = 0;
		virtual float GetMoveCost() const = 0;

		virtual void SetMoveCost(float cost) = 0;
		virtual void SetSearchState(unsigned int) = 0;
		virtual void SetMagicNumber(unsigned int) = 0;

		virtual unsigned int GetSearchState() const = 0;
		virtual unsigned int GetMagicNumber() const = 0;
		#endif

		// void SetPathCosts(float g, float h) { fCost = g + h; gCost = g; hCost = h; }
		// void SetPathCost(unsigned int type, float cost);
		// const float* GetPathCosts() const { return &fCost; }
		// float GetPathCost(unsigned int type) const;

		// void SetPrevNode(INode* n) { prevNode = n; }
		// INode* GetPrevNode() { return prevNode; }

		unsigned int GetIndex() { return index; }

	protected:
		// NOTE:
		//     storing the heap-index is an *UGLY* break of abstraction,
		//     but the only way to keep the cost of resorting acceptable
		unsigned int nodeNumber = -1u;
		// unsigned int heapIndex = -1u;
		unsigned int index = 0;

		// float fCost = 0.0f;
		// float gCost = 0.0f;
		// float hCost = 0.0f;

		// points back to previous node in path
		// INode* prevNode = nullptr;

	#ifdef QTPFS_VIRTUAL_NODE_FUNCTIONS
	};
	#endif



	#ifdef QTPFS_VIRTUAL_NODE_FUNCTIONS
	struct QTNode: public INode {
	#endif
	public:
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
		unsigned int GetChildID(unsigned int i) const { return (nodeNumber << 2) + (i + 1); }
		unsigned int GetParentID() const { return ((nodeNumber - 1) >> 2); }

		std::uint64_t GetMemFootPrint(const NodeLayer& nl) const;
		std::uint64_t GetCheckSum(const NodeLayer& nl) const;

		void PreTesselate(NodeLayer& nl, const SRectangle& r, SRectangle& ur, unsigned int depth);
		void Tesselate(NodeLayer& nl, const SRectangle& r, unsigned int depth);
		void Serialize(std::fstream& fStream, NodeLayer& nodeLayer, unsigned int* streamSize, unsigned int depth, bool readMode);

		bool IsLeaf() const { return (childBaseIndex == -1u); }
		bool CanSplit(unsigned int depth, bool forced) const;

		bool Split(NodeLayer& nl, unsigned int depth, bool forced);
		bool Merge(NodeLayer& nl);

		unsigned int GetMaxNumNeighbors() const;
		unsigned int GetNeighbors(const std::vector<INode*>&, std::vector<INode*>&);
		const std::vector<INode*>& GetNeighbors(const std::vector<INode*>&);
		bool UpdateNeighborCache(const std::vector<INode*>& nodes);

		void SetNeighborEdgeTransitionPoint(unsigned int ngbIdx, const float2& point) { netpoints[ngbIdx] = point; }
		const float2& GetNeighborEdgeTransitionPoint(unsigned int ngbIdx) const { return netpoints[ngbIdx]; }

		unsigned int xmin() const { return (_xminxmax  & 0xFFFF); }
		unsigned int zmin() const { return (_zminzmax  & 0xFFFF); }
		unsigned int xmax() const { return (_xminxmax >>     16); }
		unsigned int zmax() const { return (_zminzmax >>     16); }
		unsigned int xmid() const { return ((xmin() + xmax()) >> 1); }
		unsigned int zmid() const { return ((zmin() + zmax()) >> 1); }
		unsigned int xsize() const { return (xmax() - xmin()); }
		unsigned int zsize() const { return (zmax() - zmin()); }
		unsigned int area() const { return (xsize() * zsize()); }

		// true iff this node is fully open (partially open nodes have larger but non-infinite cost)
		bool AllSquaresAccessible() const { return (moveCostAvg < (QTPFS_CLOSED_NODE_COST / float(area()))); }
		bool AllSquaresImpassable() const { return (moveCostAvg == QTPFS_POSITIVE_INFINITY); }

		void SetMoveCost(float cost) { moveCostAvg = cost; }
		//void SetSearchState(unsigned int state) { searchState = state; }
		void SetMagicNumber(unsigned int number) { currMagicNum = number; }

		float GetSpeedMod() const { return speedModAvg; }
		float GetMoveCost() const { return moveCostAvg; }
		// unsigned int GetSearchState() const { return searchState; }
		unsigned int GetMagicNumber() const { return currMagicNum; }
		unsigned int GetChildBaseIndex() const { return childBaseIndex; }

		static unsigned int MinSizeX() { return MIN_SIZE_X; }
		static unsigned int MinSizeZ() { return MIN_SIZE_Z; }

	private:
		bool UpdateMoveCost(
			const NodeLayer& nl,
			const SRectangle& r,
			unsigned int& numNewBinSquares,
			unsigned int& numDifBinSquares,
			unsigned int& numClosedSquares,
			bool& wantSplit,
			bool& needSplit
		);

		static unsigned int MIN_SIZE_X;
		static unsigned int MIN_SIZE_Z;
		static unsigned int MAX_DEPTH;

		unsigned int _xminxmax = 0;
		unsigned int _zminzmax = 0;

		float speedModSum =  0.0f;
		float speedModAvg =  0.0f;
		float moveCostAvg = -1.0f;

		// unsigned int searchState = 0;
		unsigned int currMagicNum = 0;
		unsigned int prevMagicNum = -1u;

		unsigned int childBaseIndex = -1u;

		std::vector<INode*> neighbors;
		std::vector<float2> netpoints;
	};


	struct SearchNode {

		SearchNode() {}

		SearchNode(INode& srcNode)
			: nodeNumber(srcNode.nodeNumber)
			, index(srcNode.index)
			, prevNode(nullptr)
			// , netpoints(srcNode.netpoints)
			{}

		SearchNode(INode* srcNode)
			: nodeNumber(srcNode->nodeNumber)
			, index(srcNode->index)
			, prevNode(nullptr)
			// , netpoints(srcNode->netpoints)
			{}

		SearchNode(const SearchNode& other) { operator=(other); }

		SearchNode& operator=(const SearchNode& other) {
			nodeNumber = other.nodeNumber;
			fCost = QTPFS_POSITIVE_INFINITY;
			gCost = QTPFS_POSITIVE_INFINITY;
			hCost = QTPFS_POSITIVE_INFINITY;
			index = other.index;
			prevNode = other.prevNode;
			// netpoints = other.netpoints;
			// heapIndex = other.heapIndex;
			searchState = other.searchState;
			return *this;
		}

		void SetNodeNumber(unsigned int n) { nodeNumber = n; }
		// void SetHeapIndex(unsigned int n) { heapIndex = n; }
		unsigned int GetNodeNumber() const { return nodeNumber; }
		// unsigned int GetHeapIndex() const { return heapIndex; }
		float GetHeapPriority() const { return GetPathCost(NODE_PATH_COST_F); }
		unsigned int GetSearchState() const { return searchState; }

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
		void SetSearchState(unsigned int state) { searchState = state; }

		void SetNeighborEdgeTransitionPoint(const float2& point) { selectedNetpoint = point; }
		const float2& GetNeighborEdgeTransitionPoint() const { return selectedNetpoint; }

		unsigned int nodeNumber = -1u;
		// unsigned int heapIndex = -1u;
		unsigned int index = 0;
		unsigned int searchState = 0;

		float fCost = QTPFS_POSITIVE_INFINITY;
		float gCost = QTPFS_POSITIVE_INFINITY;
		float hCost = QTPFS_POSITIVE_INFINITY;

		// points back to previous node in path
		SearchNode* prevNode = nullptr;

		float2 selectedNetpoint;
	};
}

#endif

