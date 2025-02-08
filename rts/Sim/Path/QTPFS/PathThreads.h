#ifndef PATH_THREADS_H__
#define PATH_THREADS_H__

#include <cstddef>
#include <functional>
#include <queue>
#include <vector>

#include "Node.h"

#include "Map/ReadMap.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "System/Rectangle.h"

namespace QTPFS {
    typedef unsigned char SpeedModType;
    typedef unsigned char SpeedBinType;

    // per thread
    template<typename T>
    struct SparseData {
        std::vector<int> sparseIndex;
        std::vector<T> denseData;

        void Reset(size_t sparseSize) {
            {
                ZoneScopedN("sparseIndex.assign");
                sparseIndex.assign(sparseSize, 0);
            }
            denseData.clear();
            denseData.emplace_back(T()); // 0-th element represents a dummy record.
        }

        void Reserve(size_t denseSize) {
            denseData.reserve(denseSize);
        }

        // iterators walk backwards
        // begin == last element
        // end  == first element

        void InsertAtIndex(T&& data, int index) {
            assert(size_t(index) < sparseIndex.size());
            if (sparseIndex[index] == 0) {
                denseData.emplace_back(data);
                sparseIndex[index] = denseData.size() - 1;
            } else {
                denseData[sparseIndex[index]] = data;
            }
        }

        T& InsertINode(int nodeId) {
            assert(size_t(nodeId) < sparseIndex.size());
            assert( sparseIndex[nodeId] == 0 );
            InsertAtIndex(T(nodeId), nodeId);
            return operator[](nodeId);
        }

        T& InsertINodeIfNotPresent(int nodeId) {
            assert(size_t(nodeId) < sparseIndex.size());
            if (sparseIndex[nodeId] == 0)
                InsertAtIndex(T(nodeId), nodeId);
            return operator[](nodeId);
        }

        auto& operator[](const size_t i) {
            assert(i < sparseIndex.size());
            assert( sparseIndex[i] != 0 );
            return denseData[sparseIndex[i]];
        }
        const auto& operator[](const size_t i) const {
            assert(i < sparseIndex.size());
            assert( sparseIndex[i] != 0 );
            return denseData[sparseIndex[i]];
        }

        bool isSet(size_t i) const {
            assert(i < sparseIndex.size());
            return (sparseIndex[i] != 0);
        }

        std::size_t GetMemFootPrint() const {
            std::size_t memFootPrint = 0;

            memFootPrint += sparseIndex.size() * sizeof(typename decltype(sparseIndex)::value_type);
            memFootPrint += denseData.size() * sizeof(typename decltype(denseData)::value_type);

            return memFootPrint;
        }
    };

    struct SearchQueueNode {
        SearchQueueNode(int index, float newPriorty)
            : heapPriority(newPriorty)
            , nodeIndex(index)
            {}

        float heapPriority;
        int nodeIndex;
    };

    /// Functor to define node priority.
    /// Needs to guarantee stable ordering, even if the sorting algorithm itself is not stable.
    struct ShouldMoveTowardsBottomOfPriorityQueue {
        inline bool operator() (const SearchQueueNode& lhs, const SearchQueueNode& rhs) const {
            return std::tie(lhs.heapPriority, lhs.nodeIndex) > std::tie(rhs.heapPriority, rhs.nodeIndex);
        }
    };


    // Reminder that std::priority does comparisons to push element back to the bottom. So using
    // ShouldMoveTowardsBottomOfPriorityQueue here means the smallest value will be top()
    typedef std::priority_queue<SearchQueueNode, std::vector<SearchQueueNode>, ShouldMoveTowardsBottomOfPriorityQueue> SearchPriorityQueue;

	struct SearchThreadData {

        static constexpr int SEARCH_FORWARD = 0;
        static constexpr int SEARCH_BACKWARD = 1;
        static constexpr int SEARCH_DIRECTIONS = 2;

		SparseData<SearchNode> allSearchedNodes[SEARCH_DIRECTIONS];
        SearchPriorityQueue openNodes[SEARCH_DIRECTIONS];
        std::vector<INode*> tmpNodesStore;
        int threadId = 0;

		SearchThreadData(size_t nodeCount, int curThreadId)
			// : allSearchedNodes(nodeCount)
            /*,*/ : threadId(curThreadId)
			{}

        void ResetQueue() { ZoneScoped; for (int i=0; i<SEARCH_DIRECTIONS; ++i) ResetQueue(i); }

        void ResetQueue(int i) { ZoneScoped; while (!openNodes[i].empty()) openNodes[i].pop(); }

		void Init(size_t sparseSize, size_t denseSize) {
            constexpr size_t tmpNodeStoreInitialReserve = 128;

            for (int i=0; i<SEARCH_DIRECTIONS; ++i) {
                allSearchedNodes[i].Reset(sparseSize);
                allSearchedNodes[i].denseData.reserve(denseSize + 1); // +1 for dummy record
            }
            tmpNodesStore.reserve(tmpNodeStoreInitialReserve);
            ResetQueue();
		}

        std::size_t GetMemFootPrint() {
            std::size_t memFootPrint = 0;

            for (int i=0; i<SEARCH_DIRECTIONS; ++i) {
                memFootPrint += allSearchedNodes[i].GetMemFootPrint();
                memFootPrint += openNodes[i].size() * sizeof(std::remove_reference_t<decltype(openNodes[0])>::value_type);
            }
            memFootPrint += tmpNodesStore.size() * sizeof(decltype(tmpNodesStore)::value_type);

            return memFootPrint;
        }
	};

    struct UpdateThreadData {
        std::vector<std::uint8_t> maxBlockBits;
        std::vector<INode*> relinkNodeGrid;
        SRectangle areaRelinkedInner;
        SRectangle areaRelinked;
        SRectangle areaMaxBlockBits;
        const MoveDef* moveDef;
        int threadId = 0;

		void InitUpdate(const SRectangle& area, const INode& topNode, const MoveDef& md, int newThreadId)
		{
            moveDef = &md;
            auto mapRect = MapToRectangle();
            
            areaRelinkedInner = SRectangle  ( topNode.xmin()
                                            , topNode.zmin()
                                            , topNode.xmax()
                                            , topNode.zmax());
            areaRelinked = SRectangle   ( topNode.xmin() - 1
                                        , topNode.zmin() - 1
                                        , topNode.xmax() + 1
                                        , topNode.zmax() + 1);
            areaMaxBlockBits = SRectangle   ( topNode.xmin() - md.xsizeh
                                            , topNode.zmin() - md.zsizeh
                                            , topNode.xmax() + md.xsizeh
                                            , topNode.zmax() + md.zsizeh);
            areaRelinked.ClampIn(mapRect);
            areaMaxBlockBits.ClampIn(mapRect);

            // area must be at least big enough for the unit to be queried from its center point.
            if (areaMaxBlockBits.GetWidth() < md.xsize){
                if (areaMaxBlockBits.x1 == 0)
                    areaMaxBlockBits.x2 = md.xsize;
                else
                    areaMaxBlockBits.x1 = mapRect.x2 - md.xsize;
            }
            if (areaMaxBlockBits.GetHeight() < md.zsize) {
                if (areaMaxBlockBits.z1 == 0)
                    areaMaxBlockBits.z2 = md.zsize;
                else
                    areaMaxBlockBits.z1 = mapRect.z2 - md.zsize;
            }
    
            maxBlockBits.reserve(areaMaxBlockBits.GetArea());
            relinkNodeGrid.reserve(areaRelinked.GetArea());

            threadId = newThreadId;
        }

        SRectangle MapToRectangle() {
            return SRectangle(0, 0, mapDims.mapx, mapDims.mapy);
        }

        void Reset() {
            areaRelinked = SRectangle(0, 0, 0, 0);
            areaMaxBlockBits = areaRelinked;
            areaRelinkedInner = areaRelinked;
            relinkNodeGrid.resize(0);
            relinkNodeGrid.shrink_to_fit();
            maxBlockBits.resize(0);
            maxBlockBits.shrink_to_fit();
            moveDef = nullptr;
        }

        std::size_t GetMemFootPrint() {
            std::size_t memFootPrint = 0;

            memFootPrint += maxBlockBits.size()   * sizeof(decltype(maxBlockBits)::value_type);
            memFootPrint += relinkNodeGrid.size() * sizeof(decltype(relinkNodeGrid)::value_type);

            return memFootPrint;
        }
    };
}

#endif