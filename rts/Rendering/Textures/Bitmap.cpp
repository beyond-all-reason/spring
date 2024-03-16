/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <utility>
#include <cstring>
#include <memory>
#include <span>

#include <IL/il.h>
#include <SDL_video.h>

#include "Rendering/GL/myGL.h"
#ifndef HEADLESS
	#include "System/TimeProfiler.h"
#endif

#include "Bitmap.h"
#include "Rendering/GlobalRendering.h"
#include "System/bitops.h"
#include "System/ScopedFPUSettings.h"
#include "System/ContainerUtil.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"
#include "System/SpringMem.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"
#include "System/Threading/ThreadPool.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Threading/SpringThreading.h"

#include <tracy/Tracy.hpp>

struct InitializeOpenIL {
	InitializeOpenIL() { ilInit(); }
	~InitializeOpenIL() { ilShutDown(); }
} static initOpenIL;

class TexMemPool;
class TexNoMemPool;

class ITexMemPool {
public:
	void GrabLock() { bmpMutex.lock(); }
	void FreeLock() { bmpMutex.unlock(); }

	bool NoCurrentAllocations() const {
		return numAllocs == numFrees;
	}

	virtual ~ITexMemPool() {}

	virtual size_t Size() const = 0;
	virtual size_t AllocIdx(size_t size) = 0;
	virtual size_t AllocIdxRaw(size_t size) = 0;

	uint8_t* Alloc(size_t size) {
		std::scoped_lock lck(bmpMutex);
		return (AllocRaw(size));
	}
	virtual uint8_t* AllocRaw(size_t size) = 0;

	void Free(uint8_t* mem, size_t size) {
		std::scoped_lock lck(bmpMutex);
		FreeRaw(mem, size);
	}
	virtual void FreeRaw(uint8_t* mem, size_t size) = 0;
	virtual void Resize(size_t size) = 0;

	virtual bool Defrag() = 0;

	virtual const uint8_t* GetRawMem(size_t memIdx) const = 0;
	virtual       uint8_t* GetRawMem(size_t memIdx)       = 0;

	spring::mutex& GetMutex() { return bmpMutex; }
public:
	static void Init(size_t size);
	static void Kill();
	static inline std::unique_ptr<ITexMemPool> texMemPool = {};
protected:
	size_t numAllocs = 0;
	size_t allocSize = 0;
	size_t numFrees = 0;
	size_t freeSize = 0;

	// libIL is not thread-safe, neither are {Alloc,Free}
	spring::mutex bmpMutex;
};

class TexMemPool : public ITexMemPool {
private:
	// (index, size)
	using FreePair = std::pair<size_t, size_t>;

	std::span<uint8_t> memArray;
	std::vector<FreePair> freeList;
private:
	const uint8_t* Base() const { return memArray.data(); }
	      uint8_t* Base()       { return memArray.data(); }
public:
	~TexMemPool() override {
		spring::FreeAlignedMemory(memArray.data());
		memArray = {};
		freeList = {};
	}

	size_t Size() const override { return (memArray.size()); }
	size_t AllocIdx(size_t size) override { return (Alloc(size) - Base()); }
	size_t AllocIdxRaw(size_t size) override { return (AllocRaw(size) - Base()); }

	uint8_t* AllocRaw(size_t size) override {
		size = AlignUp(size, sizeof(uint64_t));

		uint8_t* mem = nullptr;

		size_t bestPair = size_t(-1);
		size_t bestSize = size_t(-1);
		size_t diffSize = size_t(-1);

		for (bool runDefrag: {true, false}) {
			bestPair = size_t(-1);
			bestSize = size_t(-1);
			diffSize = size_t(-1);

			// find chunk with smallest size difference
			for (size_t i = 0, n = freeList.size(); i < n; i++) {
				if (freeList[i].second < size)
					continue;

				if ((freeList[i].second - size) > diffSize)
					continue;

				bestSize = freeList[bestPair = i].second;
				diffSize = std::min(bestSize - size, diffSize);
			}

			if (bestPair == size_t(-1)) {
				if (runDefrag && DefragRaw())
					continue;

				// give up
				LOG_L(L_ERROR, "[TexMemPool::%s] failed to allocate bitmap of size " _STPF_ "u from pool of total size " _STPF_ "u", __func__, size, Size());
				throw std::bad_alloc();
				return mem;
			}

			break;
		}

		mem = &memArray[freeList[bestPair].first];

		if (bestSize > size) {
			freeList[bestPair].first += size;
			freeList[bestPair].second -= size;
		} else {
			// exact match, erase
			freeList[bestPair] = freeList.back();
			freeList.pop_back();
		}

		numAllocs += 1;
		allocSize += size;

		return mem;
	}

	void FreeRaw(uint8_t* mem, size_t size) {
		//ZoneScoped;
		if (mem == nullptr)
			return;

		if (size == 0)
			return;

		size = AlignUp(size, sizeof(uint64_t));

		memset(mem, 0, size);
		freeList.emplace_back(mem - memArray.data(), size);

		#if 0
		{
			// check if freed mem overlaps any existing chunks
			const FreePair& p = freeList.back();

			for (size_t i = 0, n = freeList.size() - 1; i < n; i++) {
				const FreePair& c = freeList[i];

				assert(!((p.first < c.first) && (p.first + p.second) > c.first));
				assert(!((c.first < p.first) && (c.first + c.second) > p.first));
			}
		}
		#endif

		numFrees += 1;
		freeSize += size;
		allocSize -= size;

		// most bitmaps are transient, so keep the list short
		// longer-lived textures should be allocated ASAP s.t.
		// rest of the pool remains unfragmented
		// TODO: split into power-of-two subpools?
		if (freeList.size() >= 64 || freeSize >= (memArray.size() >> 4))
			DefragRaw();
	}

	void Resize(size_t size) {
		//ZoneScoped;
		size = AlignUp(size, sizeof(uint64_t));

		if (size <= Size())
			return;

		std::scoped_lock lck(bmpMutex);

		if (memArray.empty()) {
			freeList.reserve(32);
			freeList.emplace_back(0, size);

			const size_t oldSize = Size();
			memArray = std::span(
				reinterpret_cast<uint8_t*>(spring::ReallocateAlignedMemory(memArray.data(), size, 64)),
				size
			);
			std::fill(memArray.begin() + oldSize, memArray.end(), 0);
		} else {
			assert(size > Size());

			freeList.emplace_back(Size(), size - Size());

			const size_t oldSize = Size();
			memArray = std::span(
				reinterpret_cast<uint8_t*>(spring::ReallocateAlignedMemory(memArray.data(), size, 64)),
				size
			);
			std::fill(memArray.begin() + oldSize, memArray.end(), 0);
		}

		LOG_L(L_INFO, "[TexMemPool::%s] poolSize=" _STPF_ "u allocSize=" _STPF_ "u texCount=" _STPF_ "u", __func__, size, allocSize, numAllocs - numFrees);
	}

	bool Defrag() override {
		//ZoneScoped;
		if (freeList.empty())
			return false;

		std::scoped_lock lck(bmpMutex);
		return (DefragRaw());
	}

	const uint8_t* GetRawMem(size_t memIdx) const override { return ((memIdx == size_t(-1))? nullptr: (Base() + memIdx)); }
	      uint8_t* GetRawMem(size_t memIdx)       override { return ((memIdx == size_t(-1))? nullptr: (Base() + memIdx)); }

private:
	bool DefragRaw() {
		//ZoneScoped;
		const auto sortPred = [](const FreePair& a, const FreePair& b) { return (a.first < b.first); };
		const auto accuPred = [](const FreePair& a, const FreePair& b) { return FreePair{0, a.second + b.second}; };

		std::sort(freeList.begin(), freeList.end(), sortPred);

		// merge adjacent chunks
		for (size_t i = 0, n = freeList.size(); i < n; /*NOOP*/) {
			FreePair& currPair = freeList[i++];

			for (size_t j = i; j < n; j++) {
				FreePair& nextPair = freeList[j];

				assert(!((currPair.first + currPair.second) > nextPair.first));

				if ((currPair.first + currPair.second) != nextPair.first)
					break;

				currPair.second += nextPair.second;
				nextPair.second = 0;

				i += 1;
			}
		}


		size_t i = 0;
		size_t j = 0;

		// cleanup zero-length chunks
		while (i < freeList.size()) {
			freeList[j] = freeList[i];

			j += (freeList[i].second != 0);
			i += 1;
		}

		if (j >= freeList.size())
			return false;

		// shrink
		freeList.resize(j);

		const auto freeBeg  = freeList.begin();
		const auto freeEnd  = freeList.end();
		      auto freePair = FreePair{0, 0};

		freePair = std::accumulate(freeBeg, freeEnd, freePair, accuPred);
		freeSize = freePair.second;
		return true;
	}
};

class TexNoMemPool : public ITexMemPool {
public:
	size_t Size() const override { return 0; }
	size_t AllocIdx(size_t size) override { return reinterpret_cast<std::uintptr_t>(Alloc(size)); }
	size_t AllocIdxRaw(size_t size) override { return reinterpret_cast<std::uintptr_t>(AllocRaw(size)); }

	uint8_t* AllocRaw(size_t size) override
	{
		if (size == 0)
			return nullptr;

		numAllocs += 1;
		allocSize += size;

		return static_cast<uint8_t*>(spring::AllocateAlignedMemory(size, sizeof(uint64_t)));
	}
	void FreeRaw(uint8_t* mem, size_t size) override
	{
		if (size == 0 || mem == nullptr)
			return;

		numFrees += 1;
		freeSize += size;
		allocSize -= size;

		spring::FreeAlignedMemory(mem);
	}
	void Resize(size_t size) override {}
	bool Defrag() override { return true; }
	const uint8_t* GetRawMem(size_t memIdx) const override { return (memIdx == size_t(-1)) ? nullptr : reinterpret_cast<uint8_t*>(memIdx); }
		  uint8_t* GetRawMem(size_t memIdx)       override { return (memIdx == size_t(-1)) ? nullptr : reinterpret_cast<uint8_t*>(memIdx); }
};

void ITexMemPool::Init(size_t size)
{
	//ZoneScoped;
	if (size == 0) {
		if (texMemPool == nullptr || typeid(*texMemPool.get()) != typeid(TexNoMemPool))
			texMemPool = std::make_unique<TexNoMemPool>();
	}
	else {
		if (texMemPool == nullptr || typeid(*texMemPool.get()) != typeid(  TexMemPool))
			texMemPool = std::make_unique<  TexMemPool>();
	}
	texMemPool->Resize(size);
	texMemPool->Defrag();
}

void ITexMemPool::Kill()
{
	//ZoneScoped;
	texMemPool = {};
}


// static bool IsValidImageType(int type) {
// 	// this is a minimal list of file formats that (should) be available at all platforms
// 	static constexpr int typeList[] = {
// 		IL_PNG, IL_JPG, IL_TGA, IL_DDS, IL_BMP, IL_TIF, IL_HDR, IL_EXR
// 	};
//
// 	return std::find(std::cbegin(typeList), std::cend(typeList), type) != std::cend(typeList);
// }

static bool IsValidImageFormat(int format) {
	//ZoneScoped;
	static constexpr int formatList[] = {
		IL_RGBA, IL_RGB, IL_BGRA, IL_BGR,
		IL_COLOUR_INDEX, IL_LUMINANCE, IL_LUMINANCE_ALPHA
	};
	return std::find(std::cbegin(formatList), std::cend(formatList), format) != std::cend(formatList);
}



//////////////////////////////////////////////////////////////////////
// BitmapAction
//////////////////////////////////////////////////////////////////////

#ifndef HEADLESS
class BitmapAction {
public:
	BitmapAction() = delete;
	BitmapAction(CBitmap* bmp_)
		: bmp{ bmp_ }
	{}

	BitmapAction(const BitmapAction& ba) = delete;
	BitmapAction(BitmapAction&& ba) noexcept = delete;

	BitmapAction& operator=(const BitmapAction& ba) = delete;
	BitmapAction& operator=(BitmapAction&& ba) noexcept = delete;

	virtual void CreateAlpha(uint8_t red, uint8_t green, uint8_t blue) = 0;
	virtual void ReplaceAlpha(float a) = 0;
	virtual void SetTransparent(const SColor& c, const SColor trans = SColor(0, 0, 0, 0)) = 0;

	virtual void Renormalize(const float3& newCol) = 0;
	virtual void Blur(int iterations = 1, float weight = 1.0f) = 0;
	virtual void Fill(const SColor& c) = 0;

	virtual void InvertColors() = 0;
	virtual void InvertAlpha() = 0;
	virtual void MakeGrayScale() = 0;
	virtual void Tint(const float tint[3]) = 0;

	virtual CBitmap CreateRescaled(int newx, int newy) = 0;

	static std::unique_ptr<BitmapAction> GetBitmapAction(CBitmap* bmp);
protected:
	CBitmap* bmp;
};

template<typename T, uint32_t ch>
class TBitmapAction : public BitmapAction {
public:
	static constexpr size_t PixelTypeSize = sizeof(T) * ch;

	using ChanType  = T;
	using PixelType = T[ch];

	using AccumChanType = typename std::conditional<std::is_same_v<T, float>, float, uint32_t>::type;

	using ChanTypeRep  = uint8_t[sizeof(T) *  1];
	using PixelTypeRep = uint8_t[PixelTypeSize ];
public:
	TBitmapAction() = delete;
	TBitmapAction(CBitmap* bmp_)
		: BitmapAction(bmp_)
	{}

	constexpr const ChanType GetMaxNormValue() const {
		if constexpr (std::is_same_v<T, float>) {
			return 1.0f;
		}
		else {
			return std::numeric_limits<T>::max();
		}
	}

	PixelType& GetRef(uint32_t xyOffset) {
		auto* mem = bmp->GetRawMem();
		assert(mem && xyOffset >= 0 && xyOffset <= bmp->GetMemSize() - sizeof(PixelTypeRep));
		//return *static_cast<PT*>(static_cast<PTR*>(mem[xyOffset]));
		return *(reinterpret_cast<PixelType*>(&mem[PixelTypeSize * xyOffset]));
	}

	ChanType& GetRef(uint32_t xyOffset, uint32_t chan) {
		assert(chan >= 0 && chan < 4);
		return GetRef(xyOffset)[chan];
	}

	void CreateAlpha(uint8_t red, uint8_t green, uint8_t blue) override;
	void ReplaceAlpha(float a) override;
	void SetTransparent(const SColor& c, const SColor trans) override;

	void Renormalize(const float3& newCol) override;
	void Blur(int iterations = 1, float weight = 1.0f) override;
	void Fill(const SColor& c) override;

	void InvertColors() override;
	void InvertAlpha() override;
	void MakeGrayScale() override;
	void Tint(const float tint[3]) override;

	CBitmap CreateRescaled(int newx, int newy) override;
};

//fugly way to make CH compile time constant
#define GET_BITMAP_ACTION_HELPER(CH) do { \
	if (bmp->channels == CH) { \
		switch (bmp->dataType) { \
			case GL_FLOAT         : { \
				return std::make_unique<TBitmapAction<float   , CH>>(bmp); \
			} break; \
			case GL_UNSIGNED_SHORT: { \
				return std::make_unique<TBitmapAction<uint16_t, CH>>(bmp); \
			} break; \
			case GL_UNSIGNED_BYTE : { \
				return std::make_unique<TBitmapAction<uint8_t , CH>>(bmp); \
			} break; \
		} \
	} \
} while (0)

std::unique_ptr<BitmapAction> BitmapAction::GetBitmapAction(CBitmap* bmp)
{
	GET_BITMAP_ACTION_HELPER(4);
	GET_BITMAP_ACTION_HELPER(3);
	GET_BITMAP_ACTION_HELPER(2);
	GET_BITMAP_ACTION_HELPER(1);

	assert(false);
	return nullptr;
}

#undef GET_BITMAP_ACTION_HELPER

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::CreateAlpha(uint8_t red, uint8_t green, uint8_t blue)
{
	//ZoneScoped;
	//if constexpr needed here to avoid compilation errors
	if constexpr (ch != 4) {
		assert(false);
		return;
	}
	else {
		const ChanType N = GetMaxNormValue();
		const float4 fRGBA = SColor{ red, green, blue, 0u };
		const PixelType tRGBA = {
			static_cast<ChanType>(fRGBA.r * N),
			static_cast<ChanType>(fRGBA.g * N),
			static_cast<ChanType>(fRGBA.b * N),
			static_cast<ChanType>(fRGBA.a * N)
		};

		float3 aCol;
		for (int a = 0; a < 3; ++a) {
			float cCol = 0.0f;
			int numCounted = 0;

			for (int y = 0; y < bmp->ysize; ++y) {
				int32_t yOffset = (y * bmp->xsize);
				for (int x = 0; x < bmp->xsize; ++x) {
					auto& pixel = GetRef(yOffset + x);

					if (pixel[3] == ChanType{ 0 })
						continue;
					if (pixel[0] == tRGBA[0] && pixel[1] == tRGBA[1] && pixel[2] == tRGBA[2])
						continue;

					cCol += static_cast<float>(pixel[a]);
					numCounted += 1;
				}
			}

			if (numCounted != 0)
				aCol[a] = static_cast<float>(cCol / GetMaxNormValue() / numCounted);
		}

		const SColor c(red, green, blue);
		const SColor a(aCol.x, aCol.y, aCol.z, 0.0f);
		SetTransparent(c, a);
	}
}

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::ReplaceAlpha(float a)
{
	if (ch != 4) {
		assert(false);
		return;
	}

	for (int32_t y = 0; y < bmp->ysize; ++y) {
		int32_t yOffset = (y * bmp->xsize);
		for (int32_t x = 0; x < bmp->xsize; ++x) {
			auto& alpha = GetRef(yOffset + x, ch - 1);
			alpha = static_cast<ChanType>(GetMaxNormValue() * a);
		}
	}
}

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::SetTransparent(const SColor& c, const SColor t)
{
	//ZoneScoped;
	//if constexpr needed here to avoid compilation errors
	if constexpr (ch != 4) {
		assert(false);
		return;
	}
	else {
		const ChanType N = GetMaxNormValue();

		const float4 fC = c;
		const float4 fT = t;
		const PixelType tC = {
			static_cast<ChanType>(fC.r * N),
			static_cast<ChanType>(fC.g * N),
			static_cast<ChanType>(fC.b * N),
			static_cast<ChanType>(fC.a * N)
		};
		const PixelType tT = {
			static_cast<ChanType>(fT.r * N),
			static_cast<ChanType>(fT.g * N),
			static_cast<ChanType>(fT.b * N),
			static_cast<ChanType>(fT.a * N)
		};

		for (int y = 0; y < bmp->ysize; ++y) {
			int32_t yOffset = (y * bmp->xsize);
			for (int x = 0; x < bmp->xsize; ++x) {
				auto& pixel = GetRef(yOffset + x);

				if (pixel[0] == tC[0] && pixel[1] == tC[1] && pixel[2] == tC[2]) {
					pixel[0] = tT[0];
					pixel[1] = tT[1];
					pixel[2] = tT[2];
					pixel[3] = tT[3];
				}
			}
		}
	}
}

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::Renormalize(const float3& newCol)
{
	//ZoneScoped;
	if (ch != 4) {
		assert(false);
		return;
	}

	float3 aCol;
	float3 colorDif;

	for (int a = 0; a < 3; ++a) {
		float cCol = 0.0f;
		int numCounted = 0;

		for (int y = 0; y < bmp->ysize; ++y) {
			int32_t yOffset = (y * bmp->xsize);
			for (int x = 0; x < bmp->xsize; ++x) {
				auto& pixel = GetRef(yOffset + x);

				if (pixel[3] != ChanType{ 0 }) {
					cCol += static_cast<float>(pixel[a]);
					numCounted += 1;
				}
			}
		}

		if (numCounted != 0)
			aCol[a] = static_cast<float>(cCol / GetMaxNormValue() / numCounted);

		//cCol /= xsize*ysize; //??
		colorDif[a] = newCol[a] - aCol[a];
	}

	for (int a = 0; a < 3; ++a) {
		for (int y = 0; y < bmp->ysize; ++y) {
			int32_t yOffset = (y * bmp->xsize);
			for (int x = 0; x < bmp->xsize; ++x) {
				auto& pixel = GetRef(yOffset + x);

				float nc = static_cast<float>(pixel[a]) / GetMaxNormValue() + colorDif[a];
				pixel[a] = static_cast<ChanType>(std::max(0.0f, nc * GetMaxNormValue()));
			}
		}
	}
}

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::Blur(int iterations, float weight)
{
	//ZoneScoped;
	// We use an axis-separated blur algorithm. Applies blurkernel in both the x
	// and y dimensions. This 3x1 blur kernel is equivalent to a 3x3 kernel in
	// both the x and y dimensions.
	// See more info
	// https://www.rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
	static constexpr float blurkernel[3] = {
		1.0f / 4.0f, 2.0f / 4.0f, 1.0f / 4.0f
	};

	// Two temporaries are required in order to perform axis separated gaussian
	// blur with an additional weight from the source pixel specified by `weight`.
	// The first blur pass, when dimension == 0, applies blur in the x
	// dimension on bitmaps[0] and saves the result to bitmaps[1]. The second blur
	// pass, when dimension == 1, applies blur in the y dimension on bitmaps[1]
	// and saves the result in bitmaps[2]. Additionally, the second blur pass adds
	// an additional `weight` from bitmaps[0] to the final result in bitmaps[2].
	CBitmap tmp(nullptr, bmp->xsize, bmp->ysize, bmp->channels, bmp->dataType);
	CBitmap tmp2(nullptr, bmp->xsize, bmp->ysize, bmp->channels, bmp->dataType);

	std::array<CBitmap*, 3> bitmaps = {bmp, &tmp, &tmp2};
	std::array<std::unique_ptr<BitmapAction>, 3> actions = {
		BitmapAction::GetBitmapAction(bitmaps[0]),
		BitmapAction::GetBitmapAction(bitmaps[1]),
		BitmapAction::GetBitmapAction(bitmaps[2])
	};

	using ThisType = decltype(this);

	for (int iter = 0; iter < iterations; ++iter) {
		for(int dimension = 0; dimension < 2; ++dimension) {

			CBitmap* src = bitmaps[dimension];
			CBitmap* dst = bitmaps[dimension + 1];

			auto& srcAction = actions[dimension];
			auto& dstAction = actions[dimension + 1];

			for_mt(0, src->ysize, [&](const int y) {
				for (int x = 0; x < src->xsize; x++) {
					int yBaseOffset = (y * src->xsize);
					for (int a = 0; a < src->channels; a++) {
						float fragment = 0.0f;

						for (int i = 0; i < 3; ++i) {
							int yoffset = dimension == 1 ? (i - 1) : 0;
							int xoffset = dimension == 0 ? (i - 1) : 0;

							const int tx = x + xoffset;
							const int ty = y + yoffset;

							xoffset *= ((tx >= 0) && (tx < src->xsize));
							yoffset *= ((ty >= 0) && (ty < src->ysize));

							const int offset = (yoffset * src->xsize + xoffset);

							auto& srcChannel = static_cast<ThisType>(srcAction.get())->GetRef(yBaseOffset + x + offset, a);

							fragment += (blurkernel[i] * srcChannel);
						}

						if (dimension == 1) {
							auto& srcChannel = static_cast<ThisType>(actions[0].get())->GetRef(yBaseOffset + x, a);

							fragment += (blurkernel[1] * blurkernel[1]) * (weight - 1.0f) * srcChannel;
						}

						auto& dstChannel = static_cast<ThisType>(dstAction.get())->GetRef(yBaseOffset + x, a);

						if constexpr (std::is_same_v<ChanType, float>) {
							dstChannel = static_cast<ChanType>(std::max(fragment, 0.0f));
						}
						else {
							dstChannel = static_cast<ChanType>(std::clamp(fragment, 0.0f, static_cast<float>(GetMaxNormValue())));
						}
					}
				}
			});
		}

		std::swap(actions[0], actions[2]);
		std::swap(bitmaps[0], bitmaps[2]);
	}

}

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::Fill(const SColor& c)
{
	//if constexpr needed here to avoid compilation errors
	if constexpr (ch != 4) {
		assert(false);
		return;
	}
	else {
		const ChanType N = GetMaxNormValue();
		const float4 fRGBA = c;
		const PixelType tRGBA = {
			static_cast<ChanType>(fRGBA.r * N),
			static_cast<ChanType>(fRGBA.g * N),
			static_cast<ChanType>(fRGBA.b * N),
			static_cast<ChanType>(fRGBA.a * N)
		};

		for (uint32_t i = 0, n = bmp->xsize * bmp->ysize; i < n; i++) {
			auto& pixel = GetRef(i);
			memcpy(&pixel, &tRGBA, PixelTypeSize);
		}
	}
}

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::InvertColors()
{
	//ZoneScoped;
	if (ch != 4) {
		assert(false);
		return;
	}

	for (int y = 0; y < bmp->ysize; ++y) {
		uint32_t yOffset = (y * bmp->xsize);
		for (int x = 0; x < bmp->xsize; ++x) {
			auto& pixel = GetRef(yOffset + x);

			// do not invert alpha
			for (int a = 0; a < ch - 1; ++a)
				pixel[a] = GetMaxNormValue() - std::clamp(pixel[a], ChanType{ 0 }, GetMaxNormValue());
		}
	}
}

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::InvertAlpha()
{
	//ZoneScoped;
	if (ch != 4) {
		assert(false);
		return;
	}

	for (int y = 0; y < bmp->ysize; ++y) {
		uint32_t yOffset = (y * bmp->xsize);
		for (int x = 0; x < bmp->xsize; ++x) {
			auto& pixel = GetRef(yOffset + x);

			pixel[ch - 1] = GetMaxNormValue() - std::clamp(pixel[ch - 1], ChanType{ 0 }, GetMaxNormValue());
		}
	}
}

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::MakeGrayScale()
{
	//ZoneScoped;
	if (ch != 4) {
		assert(false);
		return;
	}

	const ChanType N = GetMaxNormValue();

	for (int y = 0; y < bmp->ysize; ++y) {
		uint32_t yOffset = (y * bmp->xsize);
		for (int x = 0; x < bmp->xsize; ++x) {
			auto& pixel = GetRef(yOffset + x);

			float3 rgb = {
				static_cast<float>(pixel[0]) / N,
				static_cast<float>(pixel[1]) / N,
				static_cast<float>(pixel[2]) / N
			};

			const float luma =
				(rgb.r * 0.299f) +
				(rgb.g * 0.587f) +
				(rgb.b * 0.114f);

			const AccumChanType val = std::max(
				static_cast<AccumChanType>((256.0f / 255.0f) * luma),
				AccumChanType(0)
			);

			if constexpr (std::is_same_v<ChanType, float>) {
				pixel[0] = val;
				pixel[1] = val;
				pixel[2] = val;
			}
			else {
				const ChanType cval = static_cast<ChanType>( std::min(val, static_cast<AccumChanType>(N)) );
				pixel[0] = cval;
				pixel[1] = cval;
				pixel[2] = cval;
			}
		}
	}
}

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::Tint(const float tint[3])
{
	//ZoneScoped;
	if (ch != 4) {
		assert(false);
		return;
	}

	const AccumChanType N = GetMaxNormValue();

	for (int y = 0; y < bmp->ysize; ++y) {
		uint32_t yOffset = (y * bmp->xsize);
		for (int x = 0; x < bmp->xsize; ++x) {
			auto& pixel = GetRef(yOffset + x);

			// don't touch the alpha channel
			for (int a = 0; a < ch - 1; ++a) {
				AccumChanType val = pixel[a] * tint[a];
				if constexpr (std::is_same_v<ChanType, float>) {
					pixel[a] = static_cast<ChanType>(std::max  (val, AccumChanType{ 0 }   ));
				}
				else {
					pixel[a] = static_cast<ChanType>(std::clamp(val, AccumChanType{ 0 }, N));
				}
			}
		}
	}
}

template<typename T, uint32_t ch>
CBitmap TBitmapAction<T, ch>::CreateRescaled(int newx, int newy)
{
	//ZoneScoped;
	CBitmap dst;

	if (ch != 4) {
		assert(false);
		dst.AllocDummy();
		return dst;
	}

	using ThisType = decltype(this);
	const AccumChanType N = GetMaxNormValue();

	dst.Alloc(newx, newy, bmp->channels, bmp->dataType);
	auto dstAction = BitmapAction::GetBitmapAction(&dst);

	const float dx = static_cast<float>(bmp->xsize) / static_cast<float>(newx);
	const float dy = static_cast<float>(bmp->ysize) / static_cast<float>(newy);

	float cy = 0;
	for (int y = 0; y < newy; ++y) {
		const int sy = (int)cy;
		cy += dy;
		int ey = (int)cy;
		if (ey == sy)
			ey = sy + 1;

		float cx = 0;
		for (int x = 0; x < newx; ++x) {
			const int sx = (int)cx;
			cx += dx;
			int ex = (int)cx;
			if (ex == sx)
				ex = sx + 1;

			std::array<AccumChanType, ch> rgba = {0};

			for (int y2 = sy; y2 < ey; ++y2) {
				for (int x2 = sx; x2 < ex; ++x2) {
					const int index = y2 * bmp->xsize + x2;
					auto& srcPixel = GetRef(index);

					for (int a = 0; a < ch; ++a)
						rgba[a] += srcPixel[a];
				}
			}
			const int denom = ((ex - sx) * (ey - sy));

			const int index = (y * dst.xsize + x);
			auto& dstPixel = static_cast<ThisType>(dstAction.get())->GetRef(index);

			for (int a = 0; a < ch; ++a) {
				if constexpr (std::is_same_v<ChanType, float>) {
					dstPixel[a] = static_cast<ChanType>(std::max  (rgba[a] / denom, AccumChanType{ 0 }   ));
				}
				else {
					dstPixel[a] = static_cast<ChanType>(std::clamp(rgba[a] / denom, AccumChanType{ 0 }, N));
				}
			}
		}
	}

	return dst;
}
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBitmap::~CBitmap()
{
	ITexMemPool::texMemPool->Free(GetRawMem(), GetMemSize());
}

CBitmap::CBitmap()
	: xsize(0)
	, ysize(0)
	, channels(4)
	, dataType(0x1401)
	, compressed(false)
{}

CBitmap::CBitmap(const uint8_t* data, int _xsize, int _ysize, int _channels, uint32_t reqDataType)
	: xsize(_xsize)
	, ysize(_ysize)
	, channels(_channels)
	, dataType(reqDataType == 0 ? 0x1401/*GL_UNSIGNED_BYTE*/ : reqDataType)
	, compressed(false)
{
#ifndef HEADLESS
	assert(GetMemSize() > 0);
#endif
	memIdx = ITexMemPool::texMemPool->AllocIdx(GetMemSize());

	if (data != nullptr) {
		assert(!((GetRawMem() < data) && (GetRawMem() + GetMemSize()) > data));
		assert(!((data < GetRawMem()) && (data + GetMemSize()) > GetRawMem()));

		std::memcpy(GetRawMem(), data, GetMemSize());
	} else {
		std::memset(GetRawMem(), 0, GetMemSize());
	}
}


CBitmap& CBitmap::operator=(const CBitmap& bmp)
{
	//ZoneScoped;
	if (this != &bmp) {
		// NB: Free preserves size for asserts
		ITexMemPool::texMemPool->Free(GetRawMem(), GetMemSize());

		if (bmp.GetRawMem() != nullptr) {
			assert(!bmp.compressed);
#ifndef HEADLESS
			assert(bmp.GetMemSize() != 0);
#endif
			assert(!((    GetRawMem() < bmp.GetRawMem()) && (    GetRawMem() +     GetMemSize()) > bmp.GetRawMem()));
			assert(!((bmp.GetRawMem() <     GetRawMem()) && (bmp.GetRawMem() + bmp.GetMemSize()) >     GetRawMem()));

			memIdx = ITexMemPool::texMemPool->AllocIdx(bmp.GetMemSize());

			std::memcpy(GetRawMem(), bmp.GetRawMem(), bmp.GetMemSize());
		} else {
			memIdx = size_t(-1);
		}

		xsize = bmp.xsize;
		ysize = bmp.ysize;
		channels = bmp.channels;
		dataType = bmp.dataType;
		compressed = bmp.compressed;

		#ifndef HEADLESS
		textype = bmp.textype;

		ddsimage = bmp.ddsimage;
		#endif
	}

	assert(GetMemSize() == bmp.GetMemSize());
	assert((GetRawMem() != nullptr) == (bmp.GetRawMem() != nullptr));
	return *this;
}

CBitmap& CBitmap::operator=(CBitmap&& bmp) noexcept
{
	//ZoneScoped;
	if (this != &bmp) {
		std::swap(memIdx, bmp.memIdx);
		std::swap(xsize, bmp.xsize);
		std::swap(ysize, bmp.ysize);
		std::swap(channels, bmp.channels);
		std::swap(dataType, bmp.dataType);
		std::swap(compressed, bmp.compressed);

		#ifndef HEADLESS
		std::swap(textype, bmp.textype);

		std::swap(ddsimage, bmp.ddsimage);
		#endif
	}

	return *this;
}


bool CBitmap::CanBeKilled()
{
	//ZoneScoped;
	return ITexMemPool::texMemPool->NoCurrentAllocations();
}

void CBitmap::InitPool(size_t size)
{
	//ZoneScoped;
	// only allow expansion; config-size is in MB
	size *= (1024 * 1024);
	ITexMemPool::Init(size);
	ITexMemPool::texMemPool->Resize(size);
	ITexMemPool::texMemPool->Defrag();
}

void CBitmap::KillPool()
{
	//ZoneScoped;
	assert(CanBeKilled());
	ITexMemPool::Kill();
}


const uint8_t* CBitmap::GetRawMem() const { return ITexMemPool::texMemPool->GetRawMem(memIdx); }
      uint8_t* CBitmap::GetRawMem()       { return ITexMemPool::texMemPool->GetRawMem(memIdx); }

void CBitmap::Alloc(int w, int h, int c, uint32_t glType)
{
	//ZoneScoped;
	if (!Empty())
		ITexMemPool::texMemPool->Free(GetRawMem(), GetMemSize());

	dataType = glType;
	const uint32_t dts = GetDataTypeSize();

	memIdx = ITexMemPool::texMemPool->AllocIdx((xsize = w) * (ysize = h) * (channels = c) * dts);
	memset(GetRawMem(), 0, GetMemSize());
}

void CBitmap::AllocDummy(const SColor fill)
{
	//ZoneScoped;
	compressed = false;

	Alloc(1, 1, sizeof(SColor), dataType);
	Fill(fill);
}

uint32_t CBitmap::GetDataTypeSize(uint32_t glType)
{
	//ZoneScoped;
	switch (glType) {
	case GL_FLOAT:
		return sizeof(float);
	case GL_INT: [[fallthrough]];
	case GL_UNSIGNED_INT:
		return sizeof(uint32_t);
	case GL_SHORT: [[fallthrough]];
	case GL_UNSIGNED_SHORT:
		return sizeof(uint16_t);
	case GL_BYTE: [[fallthrough]];
	case GL_UNSIGNED_BYTE:
		return sizeof(uint8_t);
	default:
		assert(false);
		return 0;
	}
}

int32_t CBitmap::GetExtFmt(uint32_t ch)
{
	//ZoneScoped;
	static constexpr std::array extFormats = { 0, GL_RED, GL_RG , GL_RGB , GL_RGBA }; // GL_R is not accepted for [1]
	return extFormats[ch];
}

int32_t CBitmap::ExtFmtToChannels(int32_t extFmt)
{
	//ZoneScoped;
	// IL_COLOUR_INDEX is transformed elsewhere

	switch (extFmt) {
	case GL_DEPTH_COMPONENT: [[fallthrough]];
	case GL_LUMINANCE: [[fallthrough]];
	case GL_ALPHA: [[fallthrough]];
	case GL_RED:
		return 1;
	case GL_LUMINANCE_ALPHA: [[fallthrough]];
	case GL_RG:
		return 2;
	case GL_BGR: [[fallthrough]];
	case GL_RGB:
		return 3;
	case GL_BGRA: [[fallthrough]];
	case GL_RGBA:
		return 4;
	default:
		assert(false);
		return 0;
	}
}

#ifndef HEADLESS
int32_t CBitmap::GetIntFmt() const
{
	//ZoneScoped;
	constexpr uint32_t intFormats[3][5] = {
			{ 0, GL_R8   , GL_RG8  , GL_RGB8  , GL_RGBA8   },
			{ 0, GL_R16  , GL_RG16 , GL_RGB16 , GL_RGBA16  },
			{ 0, GL_R32F , GL_RG32F, GL_RGB32F, GL_RGBA32F }
	};
	switch (dataType) {
	case GL_FLOAT:
		return intFormats[2][channels];
	case GL_UNSIGNED_SHORT:
		return intFormats[1][channels];
	case GL_UNSIGNED_BYTE:
		return intFormats[0][channels];
	default:
		assert(false);
		return 0;
	}
}

#else
int32_t CBitmap::GetIntFmt() const { return 0; }
#endif

bool CBitmap::CondReinterpret(int w, int h, int c, uint32_t dt)
{
	//ZoneScoped;
#ifdef HEADLESS
	return true;
#else
	if (w * h * c * GetDataTypeSize(dt) != GetMemSize())
		return false;

	xsize = w;
	ysize = h;
	channels = c;
	dataType = dt;

	return true;
#endif
}

bool CBitmap::Load(std::string const& filename, float defaultAlpha, uint32_t reqChannel, uint32_t reqDataType, bool forceReplaceAlpha)
{
	//ZoneScoped;
	bool isLoaded = false;
	bool isValid  = false;
	bool hasAlpha = false;

	// LHS is only true for "image.dds", "IMAGE.DDS" would be loaded by IL
	// which does not vertically flip DDS images by default, unlike nv_dds
	// most Spring games do not seem to store DDS buildpics pre-flipped so
	// files ending in ".DDS" would appear upside-down if loaded by nv_dds
	//
	// const bool loadDDS = (filename.find(".dds") != std::string::npos || filename.find(".DDS") != std::string::npos);
	const bool loadDDS = (FileSystem::GetExtension(filename) == "dds"); // always lower-case
	const bool flipDDS = (filename.find("unitpics") == std::string::npos); // keep buildpics as-is

	const size_t curMemSize = GetMemSize();

	channels = 4;
	textype = GL_TEXTURE_2D;

	#define BITMAP_USE_NV_DDS
	#ifdef BITMAP_USE_NV_DDS
	if (loadDDS) {
		#ifndef HEADLESS
		compressed = true;
		xsize = 0;
		ysize = 0;
		channels = 0;

		ddsimage.clear();
		if (!ddsimage.load(filename, flipDDS))
			return false;

		xsize = ddsimage.get_width();
		ysize = ddsimage.get_height();
		channels = ddsimage.get_components();
		switch (ddsimage.get_type()) {
			case nv_dds::TextureFlat :
				textype = GL_TEXTURE_2D;
				break;
			case nv_dds::Texture3D :
				textype = GL_TEXTURE_3D;
				break;
			case nv_dds::TextureCubemap :
				textype = GL_TEXTURE_CUBE_MAP;
				break;
			case nv_dds::TextureNone :
			default :
				break;
		}
		return true;
		#else
		// allocate a dummy texture, dds aren't supported in headless
		AllocDummy();
		return true;
		#endif
	}

	compressed = false;
	#else
	compressed = loadDDS;
	#endif


	CFileHandler file(filename);
	std::vector<uint8_t> buffer;

	if (!file.FileExists()) {
		AllocDummy();
		return false;
	}

	if (!file.IsBuffered()) {
		buffer.resize(file.FileSize(), 0);
		file.Read(buffer.data(), buffer.size());
	} else {
		// steal if file was loaded from VFS
		buffer = std::move(file.GetBuffer());
	}


	{
		std::scoped_lock lck(ITexMemPool::texMemPool->GetMutex());

		// do not preserve the image origin since IL does not
		// vertically flip DDS images by default, unlike nv_dds
		ilOriginFunc((loadDDS && flipDDS)? IL_ORIGIN_LOWER_LEFT: IL_ORIGIN_UPPER_LEFT);
		ilEnable(IL_ORIGIN_SET);

		ILuint imageID = 0;
		ilGenImages(1, &imageID);
		ilBindImage(imageID);

		ILint currFormat;
		{
			// do not signal floating point exceptions in devil library
			ScopedDisableFpuExceptions fe;

			isLoaded = !!ilLoadL(IL_TYPE_UNKNOWN, buffer.data(), static_cast<ILuint>(buffer.size()));
			currFormat = ilGetInteger(IL_IMAGE_FORMAT);
			isValid = (isLoaded && IsValidImageFormat(currFormat));
			dataType = ilGetInteger(IL_IMAGE_TYPE);
			// auto bpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);

			// want to keep the format RGB* no matter the source swizzle
			switch (currFormat)
			{
			case IL_COLOUR_INDEX: {
				switch (auto pt = ilGetInteger(IL_PALETTE_TYPE))
				{
				case IL_PAL_BGR24: [[fallthrough]];
				case IL_PAL_BGR32: [[fallthrough]];
				case IL_PAL_RGB24: [[fallthrough]];
				case IL_PAL_RGB32: {
					currFormat = GL_RGB;
					hasAlpha = false;
				} break;
				case IL_PAL_RGBA32: [[fallthrough]];
				case IL_PAL_BGRA32: {
					currFormat = GL_RGBA;
					hasAlpha = true;
				} break;
				default: {
					assert(false);
				} break;
				}
			} break;
			case IL_ALPHA: {
				hasAlpha = true;
			} break;
			case IL_BGR: [[fallthrough]];
			case IL_RGB: {
				currFormat = GL_RGB;
				hasAlpha = false;
			} break;
			case IL_BGRA: [[fallthrough]];
			case IL_RGBA: {
				currFormat = GL_RGBA;
				hasAlpha = true;
			} break;
			case IL_LUMINANCE: {
				hasAlpha = false;
			} break;
			case IL_LUMINANCE_ALPHA: {
				hasAlpha = true;
			} break;
			default:
				assert(false);
				break;
			}

			// FPU control word has to be restored as well
			streflop::streflop_init<streflop::Simple>();
		}

		if (isValid) {
			{
				// conditional transformation
				ILenum dstFormat;
				if (reqChannel == 0) {
					dstFormat = currFormat;
					channels = ExtFmtToChannels(dstFormat);
				}
				else {
					dstFormat = GetExtFmt(reqChannel);
					channels = reqChannel;
				}

				if (reqDataType != 0)
					dataType = reqDataType;

				ilConvertImage(dstFormat, dataType);
			}

			xsize = ilGetInteger(IL_IMAGE_WIDTH);
			ysize = ilGetInteger(IL_IMAGE_HEIGHT);

			ITexMemPool::texMemPool->FreeRaw(GetRawMem(), curMemSize);
			memIdx = ITexMemPool::texMemPool->AllocIdxRaw(GetMemSize());

			for (const ILubyte* imgData = ilGetData(); imgData != nullptr; imgData = nullptr) {
				std::memset(GetRawMem(), 0xFF   , GetMemSize());
				std::memcpy(GetRawMem(), imgData, GetMemSize());
			}
		}

		ilDisable(IL_ORIGIN_SET);
		ilDeleteImages(1, &imageID);
	}

	// has to be outside the mutex scope; AllocDummy will acquire it again and
	// LOG can indirectly cause other bitmaps to be loaded through FontTexture
	if (!isValid) {
		LOG_L(L_ERROR, "[BMP::%s] invalid bitmap \"%s\" (loaded=%d)", __func__, filename.c_str(), isLoaded);
		AllocDummy();
		return false;
	}

	if (!hasAlpha || forceReplaceAlpha)
		ReplaceAlpha(defaultAlpha);

	return true;
}


bool CBitmap::LoadGrayscale(const std::string& filename)
{
	//ZoneScoped;
	const size_t curMemSize = GetMemSize();

	compressed = false;
	channels = 1;


	CFileHandler file(filename);

	if (!file.FileExists())
		return false;

	std::vector<uint8_t> buffer;

	if (!file.IsBuffered()) {
		buffer.resize(file.FileSize() + 1, 0);
		file.Read(buffer.data(), file.FileSize());
	} else {
		// steal if file was loaded from VFS
		buffer = std::move(file.GetBuffer());
	}

	{
		std::scoped_lock lck(ITexMemPool::texMemPool->GetMutex());

		ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
		ilEnable(IL_ORIGIN_SET);

		ILuint imageID = 0;
		ilGenImages(1, &imageID);
		ilBindImage(imageID);

		const bool success = !!ilLoadL(IL_TYPE_UNKNOWN, buffer.data(), buffer.size());
		ilDisable(IL_ORIGIN_SET);

		if (!success)
			return false;

		ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
		xsize = ilGetInteger(IL_IMAGE_WIDTH);
		ysize = ilGetInteger(IL_IMAGE_HEIGHT);

		ITexMemPool::texMemPool->FreeRaw(GetRawMem(), curMemSize);
		memIdx = ITexMemPool::texMemPool->AllocIdxRaw(GetMemSize());

		for (const ILubyte* imgData = ilGetData(); imgData != nullptr; imgData = nullptr) {
			std::memset(GetRawMem(), 0xFF, GetMemSize());
			std::memcpy(GetRawMem(), imgData, GetMemSize());
		}

		ilDeleteImages(1, &imageID);
	}

	return true;
}

namespace {
	bool SaveToFile(const ILchar* p, const std::string& ext)
	{
		//ZoneScoped;
		bool success = false;

		switch (hashString(ext)) {
			case hashString("bmp"): { success = ilSave(IL_BMP, p); } break;
			case hashString("jpg"): { success = ilSave(IL_JPG, p); } break;
			case hashString("png"): { success = ilSave(IL_PNG, p); } break;
			case hashString("tga"): { success = ilSave(IL_TGA, p); } break;
			case hashString("tif"): [[fallthrough]];
			case hashString("tiff"): { success = ilSave(IL_TIF, p); } break;
			case hashString("dds"): { success = ilSave(IL_DDS, p); } break;
			case hashString("raw"): { success = ilSave(IL_RAW, p); } break;
			case hashString("pbm"): [[fallthrough]];
			case hashString("pgm"): [[fallthrough]];
			case hashString("ppm"): [[fallthrough]];
			case hashString("pnm"): { success = ilSave(IL_PNM, p); } break;
		}

		assert(ilGetError() == IL_NO_ERROR);
		while (auto err = ilGetError() != IL_NO_ERROR);

		return success;
	}
}

bool CBitmap::Save(const std::string& filename, bool dontSaveAlpha, bool logged, unsigned quality) const
{
	//ZoneScoped;
	if (compressed) {
		#ifndef HEADLESS
		return ddsimage.save(filename);
		#else
		return false;
		#endif
	}

	if (GetMemSize() == 0)
		return false;

	// operator= acquires ITexMemPool's mutex internally
	CBitmap flippedCopy = *this;
	flippedCopy.ReverseYAxis();

	std::unique_lock lck(ITexMemPool::texMemPool->GetMutex());

	// clear any previous errors
	while (ilGetError() != IL_NO_ERROR);

	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	ilHint(IL_COMPRESSION_HINT, IL_NO_COMPRESSION);
	ilSetInteger(IL_JPG_QUALITY, quality);

	ILuint imageID = 0;
	ilGenImages(1, &imageID);
	ilBindImage(imageID);

	static constexpr ILuint Channels2Formats[] = {
		0,
		IL_LUMINANCE,
		IL_LUMINANCE_ALPHA,
		IL_RGB,
		IL_RGBA
	};

	ilTexImage(xsize, ysize, 1, channels, Channels2Formats[channels], dataType, flippedCopy.GetRawMem());
	assert(ilGetError() == IL_NO_ERROR);

	if (dontSaveAlpha && (channels == 2 || channels == 4)) {
		ilConvertImage(Channels2Formats[channels - 1], dataType);
		assert(ilGetError() == IL_NO_ERROR);
	}

	const std::string& fsImageExt = FileSystem::GetExtension(filename);
	const std::string& fsFullPath = dataDirsAccess.LocateFile(filename, FileQueryFlags::WRITE);
	const std::wstring& ilFullPath = std::wstring(fsFullPath.begin(), fsFullPath.end());

	if (logged)
		LOG("[CBitmap::%s] saving \"%s\" to \"%s\" (IL_VERSION=%d IL_UNICODE=%d)", __func__, filename.c_str(), fsFullPath.c_str(), IL_VERSION, sizeof(ILchar) != 1);

	const ILchar* p = (sizeof(ILchar) != 1)?
		reinterpret_cast<const ILchar*>(ilFullPath.data()):
		reinterpret_cast<const ILchar*>(fsFullPath.data());

	bool success = SaveToFile(p, fsImageExt);

	if (logged) {
		if (success) {
			LOG("[CBitmap::%s] saved \"%s\" to \"%s\"", __func__, filename.c_str(), fsFullPath.c_str());
		} else {
			LOG("[CBitmap::%s] error 0x%x saving \"%s\" to \"%s\"", __func__, ilGetError(), filename.c_str(), fsFullPath.c_str());
		}
	}

	ilDeleteImages(1, &imageID);
	ilDisable(IL_ORIGIN_SET);

	lck.unlock(); //unlock explicitly because Free(flippedCopy) is locking the same mutex

	return success;
}


bool CBitmap::SaveGrayScale(const std::string& filename) const
{
	//ZoneScoped;
	if (compressed)
		return false;

	//the code below only works under these assumptions
	if (channels != 4 && dataType != IL_UNSIGNED_SHORT) {
		assert(false);
		return false;
	}

	CBitmap bmp = *this;

	if (uint8_t* mem = bmp.GetRawMem(); mem != nullptr) {
		// approximate luminance
		bmp.MakeGrayScale();

		// convert RGBA tuples to normalized FLT32 values expected by SaveFloat; GBA are destroyed
		for (int y = 0; y < ysize; ++y) {
			for (int x = 0; x < xsize; ++x) {
				*reinterpret_cast<float*>(&mem[(y * xsize + x) * 4]) = static_cast<float>(mem[(y * xsize + x) * 4 + 0] / 255.0f);
			}
		}

		bool r = bmp.CondReinterpret(xsize, ysize, 1, IL_FLOAT);
		assert(r);

		return bmp.SaveFloat(filename);
	}

	return false;
}


bool CBitmap::SaveFloat(std::string const& filename) const
{
	//ZoneScoped;
	if (GetMemSize() == 0 || channels != 1 || dataType != IL_FLOAT)
		return false;

	std::scoped_lock lck(ITexMemPool::texMemPool->GetMutex());

	using ConvertType = uint16_t;
	constexpr ConvertType ConvertTypeMAX = std::numeric_limits<ConvertType>::max();
	constexpr uint32_t ConvertTypeDevIL = IL_UNSIGNED_SHORT;

	// seems IL_ORIGIN_SET only works in ilLoad and not in ilTexImage nor in ilSaveImage
	// so we need to flip the image ourselves
	const auto* f32b = reinterpret_cast<const float*>(GetRawMem());
	      auto* ctb  = reinterpret_cast<ConvertType*>(ITexMemPool::texMemPool->AllocRaw(channels * xsize * ysize * sizeof(ConvertType)));

	const auto* f32e = f32b + channels * xsize * ysize;
	const auto* f32mem = f32b;
	      auto* ctmem = ctb;

	while (f32mem != f32e) {
		*ctmem = static_cast<ConvertType>(std::clamp(*f32mem, 0.0f, 1.0f) * ConvertTypeMAX);
		f32mem++; ctmem++;
	}

	// clear any previous errors
	while (ilGetError() != IL_NO_ERROR);

	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	ilHint(IL_COMPRESSION_HINT, IL_NO_COMPRESSION);
	ilSetInteger(IL_JPG_QUALITY, 80);

	ILuint imageID = 0;
	ilGenImages(1, &imageID);
	ilBindImage(imageID);
	// note: DevIL only generates a 16bit grayscale PNG when format is IL_UNSIGNED_SHORT!
	//       IL_FLOAT is converted to RGB with 8bit colordepth!
	ilTexImage(xsize, ysize, 1, channels, IL_LUMINANCE, ConvertTypeDevIL, ctb);
	assert(ilGetError() == IL_NO_ERROR);

	ITexMemPool::texMemPool->FreeRaw(reinterpret_cast<uint8_t*>(ctb), channels * xsize * ysize * sizeof(ConvertType));

	const std::string fsImageExt = FileSystem::GetExtension(filename);
	const std::string fsFullPath = dataDirsAccess.LocateFile(filename, FileQueryFlags::WRITE);
	const std::wstring ilFullPath = std::wstring(fsFullPath.begin(), fsFullPath.end());

	const ILchar* p = (sizeof(ILchar) != 1) ?
		reinterpret_cast<const ILchar*>(ilFullPath.data()) :
		reinterpret_cast<const ILchar*>(fsFullPath.data());

	bool success = SaveToFile(p, fsImageExt);

	ilDeleteImages(1, &imageID);
	return success;
}


#ifndef HEADLESS
uint32_t CBitmap::CreateTexture(float aniso, float lodBias, bool mipmaps, uint32_t texID) const
{
	//ZoneScoped;
	if (compressed)
		return CreateDDSTexture(texID, aniso, lodBias, mipmaps);

	if (GetMemSize() == 0)
		return 0;

	// jcnossen: Some drivers return "2.0" as a version string,
	// but switch to software rendering for non-power-of-two textures.
	// GL_ARB_texture_non_power_of_two indicates that the hardware will actually support it.
	if (!globalRendering->supportNonPowerOfTwoTex && (xsize != next_power_of_2(xsize) || ysize != next_power_of_2(ysize))) {
		CBitmap bm = CreateRescaled(next_power_of_2(xsize), next_power_of_2(ysize));
		return bm.CreateTexture(aniso, mipmaps);
	}

	if (texID == 0)
		glGenTextures(1, &texID);

	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (lodBias != 0.0f)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, lodBias);
	if (aniso > 0.0f)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);

	if (mipmaps) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glBuildMipmaps(GL_TEXTURE_2D, GetIntFmt(), xsize, ysize, GetExtFmt(), dataType, GetRawMem());
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GetIntFmt(), xsize, ysize, 0, GetExtFmt(), dataType, GetRawMem());
	}

	return texID;
}


static void HandleDDSMipmap(GLenum target, bool mipmaps, int num_mipmaps)
{
	//ZoneScoped;
	if (num_mipmaps > 0) {
		// dds included the MipMaps use them
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	} else {
		if (mipmaps && IS_GL_FUNCTION_AVAILABLE(glGenerateMipmap)) {
			// create the mipmaps at runtime
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glGenerateMipmap(target);
		} else {
			// no mipmaps
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
	}
}

uint32_t CBitmap::CreateDDSTexture(uint32_t texID, float aniso, float lodBias, bool mipmaps) const
{
	//ZoneScoped;
	glPushAttrib(GL_TEXTURE_BIT);

	if (texID == 0)
		glGenTextures(1, &texID);

	switch (ddsimage.get_type()) {
		case nv_dds::TextureNone:
			glDeleteTextures(1, &texID);
			texID = 0;
			break;

		case nv_dds::TextureFlat:    // 1D, 2D, and rectangle textures
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, texID);

			if (!ddsimage.upload_texture2D(0, GL_TEXTURE_2D)) {
				glDeleteTextures(1, &texID);
				texID = 0;
				break;
			}

			if (lodBias != 0.0f)
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, lodBias);
			if (aniso > 0.0f)
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);

			HandleDDSMipmap(GL_TEXTURE_2D, mipmaps, ddsimage.get_num_mipmaps());
			break;

		case nv_dds::Texture3D:
			glEnable(GL_TEXTURE_3D);
			glBindTexture(GL_TEXTURE_3D, texID);

			if (!ddsimage.upload_texture3D()) {
				glDeleteTextures(1, &texID);
				texID = 0;
				break;
			}

			if (lodBias != 0.0f)
				glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_LOD_BIAS, lodBias);

			HandleDDSMipmap(GL_TEXTURE_3D, mipmaps, ddsimage.get_num_mipmaps());
			break;

		case nv_dds::TextureCubemap:
			glEnable(GL_TEXTURE_CUBE_MAP);
			glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

			if (!ddsimage.upload_textureCubemap()) {
				glDeleteTextures(1, &texID);
				texID = 0;
				break;
			}

			if (lodBias != 0.0f)
				glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_LOD_BIAS, lodBias);
			if (aniso > 0.0f)
				glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);

			HandleDDSMipmap(GL_TEXTURE_CUBE_MAP, mipmaps, ddsimage.get_num_mipmaps());
			break;

		default:
			assert(false);
			break;
	}

	glPopAttrib();
	return texID;
}
#else  // !HEADLESS

uint32_t CBitmap::CreateTexture(float aniso, float lodBias, bool mipmaps, uint32_t texID) const {
	//ZoneScoped;
	return 0;
}

uint32_t CBitmap::CreateDDSTexture(uint32_t texID, float aniso, float lodBias, bool mipmaps) const {
	//ZoneScoped;
	return 0;
}
#endif // !HEADLESS


void CBitmap::CreateAlpha(uint8_t red, uint8_t green, uint8_t blue)
{
	//ZoneScoped;
#ifndef HEADLESS
	if (compressed)
		return;

	auto action = BitmapAction::GetBitmapAction(this);
	action->CreateAlpha(red, green, blue);
#endif
}


void CBitmap::SetTransparent(const SColor& c, const SColor trans)
{
	//ZoneScoped;
#ifndef HEADLESS
	if (compressed)
		return;

	auto action = BitmapAction::GetBitmapAction(this);
	action->SetTransparent(c, trans);
#endif
}


void CBitmap::Renormalize(const float3& newCol)
{
	//ZoneScoped;
#ifndef HEADLESS
	if (compressed)
		return;

	auto action = BitmapAction::GetBitmapAction(this);
	action->Renormalize(newCol);
#endif
}

void CBitmap::Blur(int iterations, float weight)
{
	//ZoneScoped;
#ifndef HEADLESS
	if (compressed)
		return;


	auto action = BitmapAction::GetBitmapAction(this);
	action->Blur(iterations, weight);
#endif
}


void CBitmap::Fill(const SColor& c)
{
	//ZoneScoped;
#ifndef HEADLESS
	if (compressed)
		return;

	auto action = BitmapAction::GetBitmapAction(this);
	action->Fill(c);
#endif
}

void CBitmap::ReplaceAlpha(float a)
{
	//ZoneScoped;
#ifndef HEADLESS
	if (compressed)
		return;

	auto action = BitmapAction::GetBitmapAction(this);
	action->ReplaceAlpha(a);
#endif
}


void CBitmap::CopySubImage(const CBitmap& src, int xpos, int ypos)
{
	//ZoneScoped;
	if ((xpos + src.xsize) > xsize || (ypos + src.ysize) > ysize) {
		LOG_L(L_WARNING, "CBitmap::CopySubImage src image does not fit into dst!");
		return;
	}

	if (compressed || src.compressed) {
		LOG_L(L_WARNING, "CBitmap::CopySubImage can't copy compressed textures!");
		return;
	}

	const uint8_t* srcMem = src.GetRawMem();
	      uint8_t* dstMem =     GetRawMem();

	const auto dts = GetDataTypeSize();
	for (int y = 0; y < src.ysize; ++y) {
		const int pixelDst = (((ypos + y) *     xsize) + xpos) * channels * dts;
		const int pixelSrc = ((        y  * src.xsize) +    0) * channels * dts;

		// copy the whole line
		std::copy(&srcMem[pixelSrc], &srcMem[pixelSrc] + src.xsize * channels * dts, &dstMem[pixelDst]);
	}
}


CBitmap CBitmap::CanvasResize(const int newx, const int newy, const bool center) const
{
	//ZoneScoped;
	CBitmap bm;

	if (xsize > newx || ysize > newy) {
		LOG_L(L_WARNING, "CBitmap::CanvasResize can only upscale (tried to resize %ix%i to %ix%i)!", xsize,ysize,newx,newy);
		bm.AllocDummy();
		return bm;
	}

	const int borderLeft = (center) ? (newx - xsize) / 2 : 0;
	const int borderTop  = (center) ? (newy - ysize) / 2 : 0;

	bm.Alloc(newx, newy, channels, dataType);
	bm.CopySubImage(*this, borderLeft, borderTop);

	return bm;
}


SDL_Surface* CBitmap::CreateSDLSurface()
{
	//ZoneScoped;
	SDL_Surface* surface = nullptr;

	if (channels < 3 && GetDataTypeSize() != 1) {
		LOG_L(L_WARNING, "CBitmap::CreateSDLSurface works only with 24bit RGB and 32bit RGBA pictures!");
		return surface;
	}

	// this will only work with 24bit RGB and 32bit RGBA pictures
	// note: does NOT create a copy of mem, must keep this around
	surface = SDL_CreateRGBSurfaceFrom(GetRawMem(), xsize, ysize, 8 * channels, xsize * channels, 0x000000FF, 0x0000FF00, 0x00FF0000, (channels == 4) ? 0xFF000000 : 0);

	if (surface == nullptr)
		LOG_L(L_WARNING, "CBitmap::CreateSDLSurface Failed!");

	return surface;
}


CBitmap CBitmap::CreateRescaled(int newx, int newy) const
{
	//ZoneScoped;
	newx = std::max(1, newx);
	newy = std::max(1, newy);

#ifndef HEADLESS
	if (compressed) {
		LOG_L(L_WARNING, "CBitmap::CreateRescaled doesn't work with compressed textures!");
		CBitmap bm;
		bm.AllocDummy();
		return bm;
	}

	if (channels != 4) {
		LOG_L(L_WARNING, "CBitmap::CreateRescaled only works with RGBA data!");
		CBitmap bm;
		bm.AllocDummy();
		return bm;
	}

	auto action = BitmapAction::GetBitmapAction(const_cast<CBitmap*>(this));
	return action->CreateRescaled(newx, newy);
#else
	CBitmap bm;
	bm.AllocDummy();
	return bm;
#endif
}


void CBitmap::InvertColors()
{
	//ZoneScoped;
#ifndef HEADLESS
	if (compressed)
		return;

	auto action = BitmapAction::GetBitmapAction(this);
	action->InvertColors();
#endif
}


void CBitmap::InvertAlpha()
{
	//ZoneScoped;
#ifndef HEADLESS
	if (compressed)
		return; // Don't try to invert DDS

	auto action = BitmapAction::GetBitmapAction(this);
	action->InvertAlpha();
#endif
}


void CBitmap::MakeGrayScale()
{
	//ZoneScoped;
#ifndef HEADLESS
	if (compressed)
		return;

	auto action = BitmapAction::GetBitmapAction(this);
	action->MakeGrayScale();
#endif
}

void CBitmap::Tint(const float tint[3])
{
	//ZoneScoped;
#ifndef HEADLESS
	if (compressed)
		return;

	auto action = BitmapAction::GetBitmapAction(this);
	action->Tint(tint);
#endif
}


void CBitmap::ReverseYAxis()
{
	//ZoneScoped;
#ifndef HEADLESS
	if (compressed)
		return; // don't try to flip DDS

	const auto dts = GetDataTypeSize();
	const auto memSize = xsize * channels * dts;

	uint8_t* tmp = ITexMemPool::texMemPool->Alloc(memSize);
	uint8_t* mem = GetRawMem();

	for (int y = 0; y < (ysize / 2); ++y) {
		const int pixelL = (((y            ) * xsize) + 0) * channels * dts;
		const int pixelH = (((ysize - 1 - y) * xsize) + 0) * channels * dts;

		// copy the whole line
		std::copy(mem + pixelH, mem + pixelH + memSize, tmp         );
		std::copy(mem + pixelL, mem + pixelL + memSize, mem + pixelH);
		std::copy(tmp, tmp + memSize, mem + pixelL);
	}

	ITexMemPool::texMemPool->Free(tmp, memSize);
#endif
}
