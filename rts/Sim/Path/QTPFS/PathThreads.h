#ifndef PATH_THREADS_H__
#define PATH_THREADS_H__

#include <cstddef>

#include "Node.h"
#include "NodeHeap.h"

#include <queue>

namespace QTPFS {

    // per thread
    template<typename T>
    struct SparseData {
        std::vector<int> sparseIndex;
        std::vector<T> denseData;

        SparseData(size_t sparseSize) { Reset(sparseSize); }

        void Reset(size_t sparseSize) {
            sparseIndex.assign(sparseSize, 0);
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
		//binary_heap<SearchQueueNode> openNodes;

		SearchThreadData(size_t nodeCount)
			: allSearchedNodes(nodeCount)
			{}

        void ResetQueue() {
            //openNodes.clear();
            while (!openNodes.empty())
                openNodes.pop();
        }

		void Init(size_t sparseSize, size_t denseSize) {
            allSearchedNodes.denseData.reserve(denseSize + 1); // +1 for dummy record
			allSearchedNodes.Reset(sparseSize);
            ResetQueue();
		}
	};

}

#endif