/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_MEM_H
#define SPRING_MEM_H

#include <vector>
#include <map>

#include <cassert>

#include "System/Threading/SpringThreading.h"
#include "System/Log/ILog.h"

namespace spring {
	void* AllocateAlignedMemory(size_t size, size_t alignment);
	void FreeAlignedMemory(void* ptr);
}

namespace spring {
    template <typename T>
    class StablePosAllocator {
    public:
        static constexpr bool reportWork = false;
        template<typename ...Args>
        static void myLog(Args&&... args) {
            if (!reportWork)
                return;
            LOG(std::forward<Args>(args)...);
        }
    public:
        StablePosAllocator() = default;
        StablePosAllocator(size_t initialSize) :StablePosAllocator() {
            data.reserve(initialSize);
        }
        void Reset() {
            data.clear();
            sizeToPositions.clear();
            positionToSize.clear();
        }

        size_t Allocate(size_t numElems, bool withMutex = false);
        void Free(size_t& firstElem, size_t numElems);
        const size_t GetSize() const { return data.size(); }
        std::vector<T>& GetData() { return data; }

        T& operator[](std::size_t idx) { return data[idx]; }
        const T& operator[](std::size_t idx) const { return data[idx]; }
    private:
        size_t AllocateImpl(size_t numElems);
    private:
        spring::mutex mut;
        std::vector<T> data;
        std::multimap<size_t, size_t> sizeToPositions;
        std::map<size_t, size_t> positionToSize;
    };


    template<typename T>
    inline size_t StablePosAllocator<T>::Allocate(size_t numElems, bool withMutex)
    {
        if (withMutex) {
            std::lock_guard<spring::mutex> lck(mutex);
            return AllocateImpl(numElems);
        }
        else
            return AllocateImpl(numElems);
    }

    template<typename T>
    inline size_t StablePosAllocator<T>::AllocateImpl(size_t numElems)
    {
        assert(numElems > 0);

        //no gaps
        if (positionToSize.empty()) {
            size_t returnPos = data.size();
            data.resize(data.size() + numElems);
            myLog("StablePosAllocator<T>::AllocateImpl(%u) = %u", uint32_t(numElems), uint32_t(returnPos));
            return returnPos;
        }

        //try to find gaps >= in size than requested
        for (auto it = sizeToPositions.lower_bound(numElems); it != sizeToPositions.end(); ++it) {
            if (it->first < numElems)
                continue;

            size_t returnPos = it->second;
            positionToSize.erase(it->second);

            if (it->first > numElems) {
                size_t gapSize = it->first - numElems;
                size_t gapPos = it->second + numElems;
                sizeToPositions.emplace(gapSize, gapPos);
                positionToSize.emplace(gapPos, gapSize);
            }

            sizeToPositions.erase(it);
            myLog("StablePosAllocator<T>::AllocateImpl(%u) = %u", uint32_t(numElems), uint32_t(returnPos));
            return returnPos;
        }

        //all gaps are too small
        size_t returnPos = data.size();
        data.resize(data.size() + numElems);
        myLog("StablePosAllocator<T>::AllocateImpl(%u) = %u", uint32_t(numElems), uint32_t(returnPos));
        return returnPos;
    }

    template<typename T>
    inline void StablePosAllocator<T>::Free(size_t& firstElem, size_t numElems)
    {
        assert(firstElem + numElems <= data.size());
        assert(numElems > 0);

        //lucky us
        if (firstElem + numElems == data.size()) {
            data.resize(firstElem);

            myLog("StablePosAllocator<T>::Free(%u, %u)", uint32_t(firstElem), uint32_t(numElems));
            firstElem = ~0u;
            return;
        }

        const auto eraseSizeToPositionsKV = [this](size_t size, size_t pos) {
            auto [beg, end] = sizeToPositions.equal_range(size);
            for (auto it = beg; it != end; /*noop*/)
                if (it->second == pos)
                    it = sizeToPositions.erase(it);
                else
                    ++it;
        };

        //check for merge opportunity before firstElem positionToSize
        if (!positionToSize.empty()) {
            auto positionToSizeBeforeIt = positionToSize.lower_bound(firstElem);
            if (positionToSizeBeforeIt == positionToSize.end())
                positionToSizeBeforeIt = positionToSize.begin();

            if (positionToSizeBeforeIt != positionToSize.begin())
                --positionToSizeBeforeIt;

            if (positionToSizeBeforeIt->first + positionToSizeBeforeIt->second == firstElem) {
                // [gapBefore][thisGap] ==> [newBiggerGap]

                //erase old sizeToPositions
                eraseSizeToPositionsKV(positionToSizeBeforeIt->second, positionToSizeBeforeIt->first);
                //patch up positionToSize
                positionToSizeBeforeIt->second += numElems;
                //emplace new sizeToPositions
                sizeToPositions.emplace(positionToSizeBeforeIt->second, positionToSizeBeforeIt->first);

                myLog("StablePosAllocator<T>::Free(%u, %u)", uint32_t(firstElem), uint32_t(numElems));
                firstElem = ~0u;
                return;
            }
        }

        //check for merge opportunity after firstElem positionToSize
        if (!positionToSize.empty()) {
            auto positionToSizeAfterIt = positionToSize.upper_bound(firstElem + numElems); //guaranteed to be after firstElem
            if (positionToSizeAfterIt != positionToSize.end()) {
                if (firstElem + numElems == positionToSizeAfterIt->first) {
                    // [thisGap][gapAfter] ==> [newBiggerGap]

                    //erase old sizeToPositions
                    eraseSizeToPositionsKV(positionToSizeAfterIt->second, positionToSizeAfterIt->first);
                    //emplace new positionToSize
                    positionToSize.emplace(firstElem, numElems + positionToSizeAfterIt->second);
                    //emplace new sizeToPositions
                    sizeToPositions.emplace(numElems + positionToSizeAfterIt->second, firstElem);
                    //erase old positionToSize
                    positionToSize.erase(positionToSizeAfterIt);

                    myLog("StablePosAllocator<T>::Free(%u, %u)", uint32_t(firstElem), uint32_t(numElems));
                    firstElem = ~0u;
                    return;
                }
            }
        }

        //no adjacent gaps found
        positionToSize.emplace(firstElem, numElems);
        sizeToPositions.emplace(numElems, firstElem);

        myLog("StablePosAllocator<T>::Free(%u, %u)", uint32_t(firstElem), uint32_t(numElems));
        firstElem = ~0u;
        return;
    }
}

#endif