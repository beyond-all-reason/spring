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

        SparseData(size_t sparseSize) { Reset(sparseSize); }

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
            assert(index < sparseIndex.size());
            if (sparseIndex[index] == 0) {
                denseData.emplace_back(data);
                sparseIndex[index] = denseData.size() - 1;
            } else {
                denseData[sparseIndex[index]] = data;
            }
        }

        T& InsertINode(INode *node) {
            if (node == nullptr) return denseData[0];
            InsertAtIndex(T(node), node->GetIndex());
            return operator[](node->GetIndex());
        }

        T& InsertINodeIfNotPresent(INode *node) {
            if (node == nullptr) return denseData[0];
            if (sparseIndex[node->GetIndex()] == 0)
                InsertAtIndex(T(node), node->GetIndex());
            return operator[](node->GetIndex());
        }

        auto& operator[](const size_t i) {
            assert(i < sparseIndex.size());
            return denseData[sparseIndex[i]];
        }
        const auto& operator[](const size_t i) const {
            assert(i < sparseIndex.size());
            return denseData[sparseIndex[i]];
        }

        bool isSet(size_t i) {
            assert(i < sparseIndex.size());
            return (sparseIndex[i] != 0);
        }

        std::size_t GetMemFootPrint() const {
            std::size_t memFootPrint = sizeof(SparseData);

            memFootPrint += sparseIndex.size() * sizeof(typename decltype(sparseIndex)::value_type);
            memFootPrint += denseData.size() * sizeof(typename decltype(denseData)::value_type);

            return memFootPrint;
        }
    };

    struct SearchQueueNode {
		bool operator <  (const SearchQueueNode& n) const { return (heapPriority <  n.heapPriority); }
		bool operator >  (const SearchQueueNode& n) const { return (heapPriority >  n.heapPriority); }
		bool operator == (const SearchQueueNode& n) const { return (heapPriority == n.heapPriority); }
		bool operator <= (const SearchQueueNode& n) const { return (heapPriority <= n.heapPriority); }
		bool operator >= (const SearchQueueNode& n) const { return (heapPriority >= n.heapPriority); }

        SearchQueueNode(int index, float newPriorty)
            : heapPriority(newPriorty)
            , nodeIndex(index)
            {}

        float heapPriority;
        int nodeIndex;
    };

    // Reminder that std::priority does comparisons to push element back to the bottom. So using
    // std::greater here means the smallest value will be top()
    typedef std::priority_queue<SearchQueueNode, std::vector<SearchQueueNode>, std::greater<SearchQueueNode>> SearchPriorityQueue;

	struct SearchThreadData {
		SparseData<SearchNode> allSearchedNodes;
        SearchPriorityQueue openNodes;

		SearchThreadData(size_t nodeCount)
			: allSearchedNodes(nodeCount)
			{}

        void ResetQueue() { ZoneScoped; while (!openNodes.empty()) openNodes.pop(); }

		void Init(size_t sparseSize, size_t denseSize) {
            allSearchedNodes.denseData.reserve(denseSize + 1); // +1 for dummy record
			allSearchedNodes.Reset(sparseSize);
            ResetQueue();
		}

        std::size_t GetMemFootPrint() {
            std::size_t memFootPrint = sizeof(SearchThreadData);

            memFootPrint += allSearchedNodes.GetMemFootPrint();

            return memFootPrint;
        }
	};

    struct UpdateThreadData {
		// std::vector<SpeedModType> curSpeedMods;
		// std::vector<SpeedBinType> curSpeedBins;
        std::vector<std::uint8_t> maxBlockBits;
        std::vector<INode*> relinkNodeGrid;
        SRectangle areaUpdated;
        SRectangle areaRelinkedInner;
        SRectangle areaRelinked;
        SRectangle areaMaxBlockBits;
        const MoveDef* moveDef;
        int threadId = 0;

		void InitUpdate(const SRectangle& area, const INode& topNode, const MoveDef& md, int newThreadId)
		{
            moveDef = &md;
            auto mapRect = MapToRectangle();
            
            areaUpdated = area;
            areaRelinkedInner = SRectangle  ( topNode.xmin()
                                            , topNode.zmin()
                                            , topNode.xmax()
                                            , topNode.zmax());
            areaRelinked = SRectangle   ( topNode.xmin() - 1
                                        , topNode.zmin() - 1
                                        , topNode.xmax() + 1
                                        , topNode.zmax() + 1);
            areaMaxBlockBits = SRectangle   ( area.x1 - md.xsizeh
                                            , area.z1 - md.zsizeh
                                            , area.x2 + md.xsizeh
                                            , area.z2 + md.zsizeh);
            areaRelinked.ClampIn(mapRect);
            areaMaxBlockBits.ClampIn(mapRect);

            // curSpeedMods.reserve(area.GetArea());
            // curSpeedBins.reserve(area.GetArea());
            maxBlockBits.reserve(areaMaxBlockBits.GetArea());
            relinkNodeGrid.reserve(areaRelinked.GetArea());

            threadId = newThreadId;
        }

        SRectangle MapToRectangle() {
            return SRectangle(0, 0, mapDims.mapx, mapDims.mapy);
        }

        void Reset() {
            areaUpdated = SRectangle(0, 0, 0, 0);
            areaRelinked = areaUpdated;
            areaMaxBlockBits = areaUpdated;
            areaRelinkedInner = areaUpdated;
            // curSpeedMods.resize(0);
            // curSpeedMods.shrink_to_fit();
            // curSpeedBins.resize(0);
            // curSpeedBins.shrink_to_fit();
            relinkNodeGrid.resize(0);
            relinkNodeGrid.shrink_to_fit();
            maxBlockBits.resize(0);
            maxBlockBits.shrink_to_fit();
            moveDef = nullptr;
        }

        // std::size_t GetMemFootPrint() {
        //     std::size_t memFootPrint = sizeof(SearchThreadData);

        //     memFootPrint += allSearchedNodes.GetMemFootPrint();

        //     return memFootPrint;
        // }
    };
}

#endif