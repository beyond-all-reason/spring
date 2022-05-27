/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <vector>
#include <cassert>
#include <limits>

#include "SmoothHeightMesh.h"

#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "System/float3.h"
#include "System/SpringMath.h"
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"

SmoothHeightMesh smoothGround;


static float Interpolate(float x, float y, const int maxx, const int maxy, const float res, const float* heightmap)
{
	x = Clamp(x / res, 0.0f, (float)maxx);
	y = Clamp(y / res, 0.0f, (float)maxy);
	const int sx = x;
	const int sy = y;
	const float dx = (x - sx);
	const float dy = (y - sy);

	const int sxp1 = std::min(sx + 1, maxx);
	const int syp1 = std::min(sy + 1, maxy);

	const float& h1 = heightmap[sx   + sy   * maxx];
	const float& h2 = heightmap[sxp1 + sy   * maxx];
	const float& h3 = heightmap[sx   + syp1 * maxx];
	const float& h4 = heightmap[sxp1 + syp1 * maxx];

	const float hi1 = mix(h1, h2, dx);
	const float hi2 = mix(h3, h4, dx);
	return mix(hi1, hi2, dy);
}


void SmoothHeightMesh::Init(int2 max, int res, int smoothRad)
{
	Kill();

	fmaxx = max.x * SQUARE_SIZE;
	fmaxy = max.y * SQUARE_SIZE;
	fresolution = res * SQUARE_SIZE;

	resolution = res;
	maxx = max.x / resolution; // max.x;
	maxy = max.y / resolution; // max.y;

	smoothRadius = std::max(1, smoothRad);

	MakeSmoothMesh();
}

void SmoothHeightMesh::Kill() {
	// if (maximaHeightMap != nullptr) {
	// 	_mm_free(maximaHeightMap);
	// 	maximaHeightMap = nullptr;
	// }
	maximaMesh.clear();
	mesh.clear();
	origMesh.clear();
}



float SmoothHeightMesh::GetHeight(float x, float y)
{
	assert(!mesh.empty());
	return Interpolate(x, y, maxx, maxy, fresolution, &mesh[0]);
}

float SmoothHeightMesh::GetHeightAboveWater(float x, float y)
{
	assert(!mesh.empty());
	return std::max(0.0f, Interpolate(x, y, maxx, maxy, resolution, &mesh[0]));
}



float SmoothHeightMesh::SetHeight(int index, float h)
{
	return (mesh[index] = h);
}

float SmoothHeightMesh::AddHeight(int index, float h)
{
	return (mesh[index] += h);
}

float SmoothHeightMesh::SetMaxHeight(int index, float h)
{
	return (mesh[index] = std::max(h, mesh[index]));
}



inline static void FindMaximumColumnHeights(
	const int2 min,
	const int2 max,
	const int winSize,
	const int resolution,
	std::vector<float>& colsMaxima,
	std::vector<int>& maximaRows
) {
	// initialize the algorithm: find the maximum
	// height per column and the corresponding row
	for (int y = min.y; y <= std::min(max.y, min.y + winSize); ++y) {
		for (int x = min.x; x <= max.x; ++x)  {
			//const float curx = x * resolution;
			//const float cury = y * resolution;
			//const float curh = CGround::GetHeightReal(curx, cury);
			const float curh = readMap->GetCornerHeightMapSynced()[(x + y*mapDims.mapxp1)*resolution];

			if (curh > colsMaxima[x]) {
				colsMaxima[x] = curh;
				maximaRows[x] = y;
			}
		}
	}
}

inline static void AdvanceMaximaRows(
	const int y,
	const int minx,
	const int maxx,
	const int resolution,
	const std::vector<float>& colsMaxima,
	      std::vector<int>& maximaRows
) {
	//const float cury = y * resolution;

	// try to advance rows if they're equal to current maximum but are further away
	for (int x = minx; x <= maxx; ++x) {
		if (maximaRows[x] == (y - 1)) {
			//const float curx = x * resolution;
			//const float curh = CGround::GetHeightReal(curx, cury);
			const float curh = readMap->GetCornerHeightMapSynced()[(x + y*mapDims.mapxp1)*resolution];

			if (curh == colsMaxima[x]) {
				maximaRows[x] = y;
			}

			assert(curh <= colsMaxima[x]);
		}
	}
}



inline static void FindRadialMaximum(
	int mapx,
	int y,
	int minx,
	int maxx,
	int winSize,
	float resolution,
	const std::vector<float>& colsMaxima,
	      std::vector<float>& mesh
) {
	//const float cury = y * resolution;

	for (int x = minx; x <= maxx; ++x) {
		float maxRowHeight = -std::numeric_limits<float>::max();

		// find current maximum within radius smoothRadius
		// (in every column stack) along the current row
		const int startx = std::max(x - winSize, 0);
		const int endx = std::min(maxx, x + winSize);

		for (int i = startx; i <= endx; ++i) {
			// assert(i >= 0);
			// assert(i <= maxx);
			// assert(CGround::GetHeightReal(i * resolution, cury) <= colsMaxima[i]);

			maxRowHeight = std::max(colsMaxima[i], maxRowHeight);
		}

// #ifndef NDEBUG
// 		const float curx = x * resolution;
// 		assert(maxRowHeight <= readMap->GetCurrMaxHeight());
// 		assert(maxRowHeight >= CGround::GetHeightReal(curx, cury));

// 	#ifdef SMOOTHMESH_CORRECTNESS_CHECK
// 		// naive algorithm
// 		float maxRowHeightAlt = -std::numeric_limits<float>::max();

// 		for (float y1 = cury - smoothRadius; y1 <= cury + smoothRadius; y1 += resolution) {
// 			for (float x1 = curx - smoothRadius; x1 <= curx + smoothRadius; x1 += resolution) {
// 				maxRowHeightAlt = std::max(maxRowHeightAlt, CGround::GetHeightReal(x1, y1));
// 			}
// 		}

// 		assert(maxRowHeightAlt == maxRowHeight);
// 	#endif
// #endif

		mesh[x + y * mapx] = maxRowHeight;
	}
}



inline static void FixRemainingMaxima(
	const int minx,
	const int miny,
	const int2 max,
	const int winSize,
	const int resolution,
	std::vector<float>& colsMaxima,
	std::vector<int>& maximaRows
) {
	// fix remaining maximums after a pass
	const int nextrow = miny + winSize + 1;
	const int nextrowy = nextrow; // * resolution;

	for (int x = minx; x <= max.x; ++x) {
// #ifdef _DEBUG
// 		for (int y1 = std::max(min.y, min.y - winSize); y1 <= std::min(max.y, min.y + winSize); ++y1) {
// 			assert(CGround::GetHeightReal(x * resolution, y1 * resolution) <= colsMaxima[x]);
// 		}
// #endif
		//const int curx = x * resolution;

		if (maximaRows[x] <= (miny - winSize)) {
			// find a new maximum if the old one left the window
			colsMaxima[x] = -std::numeric_limits<float>::max();

			for (int y1 = std::max(0, miny - winSize + 1); y1 <= std::min(max.y, nextrow); ++y1) {
				//const float h = CGround::GetHeightReal(curx, y1 * resolution);
				const float h = readMap->GetCornerHeightMapSynced()[(x + y1*mapDims.mapxp1)*resolution];

				if (h > colsMaxima[x]) {
					colsMaxima[x] = h;
					maximaRows[x] = y1;
				} else if (colsMaxima[x] == h) {
					// if equal, move as far down as possible
					maximaRows[x] = y1;
				}
			}
		} else if (nextrow <= max.y) {
			// else, just check if a new maximum has entered the window
			//const float h = CGround::GetHeightReal(curx, nextrowy);
			const float h = readMap->GetCornerHeightMapSynced()[(x + nextrowy*mapDims.mapxp1)*resolution];

			if (h > colsMaxima[x]) {
				colsMaxima[x] = h;
				maximaRows[x] = nextrow;
			}
		}

		// assert(maximaRows[x] <= nextrow);
		// assert(maximaRows[x] >= min.y - winSize + 1);

// #ifdef _DEBUG
// 		for (int y1 = std::max(0, min.y - winSize + 1); y1 <= std::min(max.y, min.y + winSize + 1); ++y1) {
// 			assert(colsMaxima[x] >= CGround::GetHeightReal(curx, y1 * resolution));
// 		}
// #endif
	}
}



inline static void BlurHorizontal(
	const int2 mapSize,
	const int2 min,
	const int2 max,
	const int blurSize,
	const int resolution,
	const std::vector<float>& kernel,
	const std::vector<float>& mesh,
	      std::vector<float>& smoothed
) {
	const int lineSize = mapSize.x;

	for_mt(min.y, max.y+1, [&](const int y)
	{
		float avg = 0.0f;
		float lv = 0;
		float rv = 0;
		float weight = 1.f / ((float)blurSize * 2.f + 1.f);
		int li = min.x - blurSize;
		int ri = min.x + blurSize + 1;
		for (int x1 = li; x1 < ri; ++x1)
		 	avg += mesh[std::max(0, std::min(mapSize.x-1, x1)) + y * lineSize];

		for (int x = min.x; x < max.x; ++x)
		{
			avg += (-lv) + rv;

			//const float ghaw = CGround::GetHeightReal(x * resolution, y * resolution);
			const float ghaw = readMap->GetCornerHeightMapSynced()[(x + y*mapDims.mapxp1)*resolution];

			smoothed[x + y * lineSize] = std::max(ghaw, avg*weight);

			// #pragma message ("FIX ME")
			// smoothed[x + y * lineSize] = std::clamp(
			// 	smoothed[x + y * lineSize],
			// 	readMap->GetCurrMinHeight(),
			// 	readMap->GetCurrMaxHeight()
			// );

			// assert(smoothed[x + y * lineSize] <=          readMap->GetCurrMaxHeight()       );
			// assert(smoothed[x + y * lineSize] >=          readMap->GetCurrMinHeight()       );

			lv = mesh[std::max(0, std::min(mapSize.x-1, li)) + y * lineSize];
			rv = mesh[std::max(0, std::min(mapSize.x-1, ri)) + y * lineSize];
			li++; ri++;
		}
	});
}

inline static void BlurVertical(
	const int2 mapSize,
	const int2 min,
	const int2 max,
	const int blurSize,
	const int resolution,
	const std::vector<float>& kernel,
	const std::vector<float>& mesh,
	      std::vector<float>& smoothed
) {
	//SCOPED_TIMER("Sim::SmoothHeightMesh::BlurVertical");
	const int lineSize = mapSize.x;

	for_mt(min.x, max.x+1, [&](const int x)
	{
		float avg = 0.0f;
		float lv = 0;
		float rv = 0;
		float weight = 1.f / ((float)blurSize * 2.f + 1.f);
		int li = min.y - blurSize;
		int ri = min.y + blurSize + 1;
		for (int y1 = li; y1 < ri; ++y1)
			avg += mesh[ x + std::max(0, std::min(mapSize.y-1, y1)) * lineSize];

		for (int y = min.y; y < max.y; ++y)
		{
			avg += (-lv) + rv;

			//const float ghaw = CGround::GetHeightReal(x * resolution, y * resolution);
			const float ghaw = readMap->GetCornerHeightMapSynced()[(x + y*mapDims.mapxp1)*resolution];

			smoothed[x + y * lineSize] = std::max(ghaw, avg*weight);

			// #pragma message ("FIX ME")
			// smoothed[x + y * lineSize] = std::clamp(
			// 	smoothed[x + y * lineSize],
			// 	readMap->GetCurrMinHeight(),
			// 	readMap->GetCurrMaxHeight()
			// );

			// assert(smoothed[x + y * lineSize] <=          readMap->GetCurrMaxHeight()       );
			// assert(smoothed[x + y * lineSize] >=          readMap->GetCurrMinHeight()       );

			lv = mesh[ x + std::max(0, std::min(mapSize.y-1, li)) * lineSize];
			rv = mesh[ x + std::max(0, std::min(mapSize.y-1, ri)) * lineSize];
			li++; ri++;
		}
	});
}



inline static void CheckInvariants(
	int y,
	int maxx,
	int maxy,
	int winSize,
	float resolution,
	const std::vector<float>& colsMaxima,
	const std::vector<int>& maximaRows
) {
	// check invariants
	if (y < maxy) {
		for (int x = 0; x <= maxx; ++x) {
			assert(maximaRows[x] > y - winSize);
			assert(maximaRows[x] <= maxy);
			assert(colsMaxima[x] <=          readMap->GetCurrMaxHeight()       );
			assert(colsMaxima[x] >=          readMap->GetCurrMinHeight()       );
		}
	}
	for (int y1 = std::max(0, y - winSize + 1); y1 <= std::min(maxy, y + winSize + 1); ++y1) {
		for (int x1 = 0; x1 <= maxx; ++x1) {
			assert(CGround::GetHeightReal(x1 * resolution, y1 * resolution) <= colsMaxima[x1]);
		}
	}
}

// static int mapWidth = 0.f;

// static float GetLocalMaxima(int2 point, int2 windowSize) {
// 	float localMax = -std::numeric_limits<float>::max();

// 	auto heightMap = readMap->GetCornerHeightMapSynced();

// 	int yHalfWindow = windowSize.y / 2;
// 	int yStart = std::max(0, point.y - yHalfWindow);
// 	int yEnd = std::min(mapDims.mapy, yStart + yHalfWindow);

// 	int xHalfWindow = windowSize.x / 2;
// 	int xStart = std::max(0, point.x - xHalfWindow);
// 	int xEnd = std::min(mapDims.mapx, xStart + xHalfWindow);

// 	for (int y = yStart; y < yEnd; y++) {
// 		for (int x = xStart; x < xEnd; x++) {
// 			int pointIndex = x + y * mapWidth;
// 			localMax = std::max(localMax, heightMap[pointIndex]);
// 		}
// 	}

// 	return localMax;
// }

// static void BuildLocalMaximaGrid(std::pmr::vector<float> &maximaHeightMap, int2 point, int2 sampleSize) {
// 	int windowSize = 20;

// 	int yStart = std::max(0, point.y);
// 	int yEnd = std::min(mapDims.mapy, yStart + sampleSize.y);

// 	int xStart = std::max(0, point.x);
// 	int xEnd = std::min(mapDims.mapx, xStart + sampleSize.x);

// 	for (int y = yStart; y < yEnd; y++) {
// 		for (int x = xStart; x < xEnd; x++) {
// 			int pointIndex = x + y * mapWidth;
// 			maximaHeightMap[pointIndex] = GetLocalMaxima(int2(x, y), int2(windowSize, windowSize));
// 		}
// 	}
// }

// void SmoothHeightMesh::UpdateMapMaximaGrid() {
// 	constexpr int itemsPerChunk = CacheAlignedMemoryResource::cacheLineLength / sizeof(float);
// 	size_t chunks = chunksPerLine * mapDims.mapy;

// 	for_mt(0, chunks, [&, itemsPerChunk](const int i) {
// 		int y = i / chunksPerLine;
// 		int x = i % chunksPerLine;

// 		BuildLocalMaximaGrid(maximaHeightMap, int2(x, y), int2(itemsPerChunk, 1));
// 	});
// }


// // get built after initial changes applied
// void SmoothHeightMesh::BuildNewMapMaximaGrid() {
// 	constexpr int itemsPerChunk = CacheAlignedMemoryResource::cacheLineLength / sizeof(float);
// 	chunksPerLine = (mapDims.mapx / itemsPerChunk) + (mapDims.mapx % itemsPerChunk ? 1: 0);
// 	cacheAlignMapWidth = chunksPerLine * itemsPerChunk;

// 	size_t mapSize = mapDims.mapy*cacheAlignMapWidth;
// 	maximaHeightMap.resize(mapSize);

// 	//maximaHeightMap = (float*)_mm_malloc(memorySize, cacheLineLength);
// 	//std::pmr::monotonic_buffer_resource mbr(addr, memorySize, std::pmr::null_memory_resource());
// 	//maximaHeightMap = new std::pmr::vector<float>(mapSize, &mbr);
// 	//std::pmr::vector<float> maximaHeightMa(mapSize, &mbr);
// }

void SmoothHeightMesh::UpdateSmoothMesh()
{
	if (gs->frameNum % (GAME_SPEED/2) != 0) return;

	SCOPED_TIMER("Sim::SmoothHeightMesh::UpdateSmoothMesh");

	const int winSize = smoothRadius / resolution;
	const int blurSize = std::max(1, winSize / 2);
	constexpr int blurPassesCount = 1;

	int2 impactRadius{winSize, winSize};

	// area of map damaged.
	int2 damageMin{40,40};
	int2 damageMax{44,44};

	// area of the map which may need a max height change
	int2 updateLocation = damageMin - impactRadius;
	int2 updateLimit = damageMax + impactRadius;

	// area of map to load maximums for to update the updateLocation
	int2 min = updateLocation - impactRadius;
	int2 max = updateLocation + impactRadius;
	int2 map{maxx, maxy};

	updateLocation.x = std::clamp(updateLocation.x, 0, map.x);
	updateLocation.y = std::clamp(updateLocation.y, 0, map.y);
	updateLimit.x = std::clamp(updateLimit.x, 0, map.x);
	updateLimit.y = std::clamp(updateLimit.y, 0, map.y);
	min.x = std::clamp(min.x, 0, map.x);
	min.y = std::clamp(min.y, 0, map.y);
	max.x = std::clamp(max.x, 0, map.x);
	max.y = std::clamp(max.y, 0, map.y);

	FindMaximumColumnHeights(min, max, winSize, resolution, colsMaxima, maximaRows);

	for (int y = updateLocation.y; y <= updateLimit.y; ++y) {

		AdvanceMaximaRows(y, min.x, max.x, resolution, colsMaxima, maximaRows);
		FindRadialMaximum(map.x, y, updateLocation.x, updateLimit.x, winSize, resolution, colsMaxima, maximaMesh);
		FixRemainingMaxima(min.x, y, max, winSize, resolution, colsMaxima, maximaRows);

// #ifdef _DEBUG
// 		CheckInvariants(y, updateLimit.x, updateLimit.y, winSize, resolution, colsMaxima, maximaRows);
// #endif
	}

	// actually smooth with approximate Gaussian blur passes
	for (int numBlurs = blurPassesCount; numBlurs > 0; --numBlurs) {
		BlurHorizontal(map, updateLocation, updateLimit, blurSize, resolution, gaussianKernel, (numBlurs == blurPassesCount) ? maximaMesh : mesh, tempMesh); mesh.swap(tempMesh);
		BlurVertical(map, updateLocation, updateLimit, blurSize, resolution, gaussianKernel, mesh, tempMesh); mesh.swap(tempMesh);
	}
}

void SmoothHeightMesh::MakeSmoothMesh()
{
	ScopedOnceTimer timer("SmoothHeightMesh::MakeSmoothMesh");

	// info:
	//   height-value array has size <maxx + 1> * <maxy + 1>
	//   and represents a grid of <maxx> cols by <maxy> rows
	//   maximum legal index is ((maxx + 1) * (maxy + 1)) - 1
	//
	//   row-width (number of height-value corners per row) is (maxx + 1)
	//   col-height (number of height-value corners per col) is (maxy + 1)
	//
	//   1st row has indices [maxx*(  0) + (  0), maxx*(1) + (  0)] inclusive
	//   2nd row has indices [maxx*(  1) + (  1), maxx*(2) + (  1)] inclusive
	//   3rd row has indices [maxx*(  2) + (  2), maxx*(3) + (  2)] inclusive
	//   ...
	//   Nth row has indices [maxx*(N-1) + (N-1), maxx*(N) + (N-1)] inclusive
	//
	// use sliding window of maximums to reduce computational complexity
	const int winSize = smoothRadius / resolution;
	const int blurSize = std::max(1, winSize / 2);
	constexpr int blurPassesCount = 1;

	// const auto fillGaussianKernelFunc = [blurSize](std::vector<float>& gaussianKernel, const float sigma) {
	// 	gaussianKernel.resize(blurSize + 1);

	// 	const auto gaussianG = [](const int x, const float sigma) -> float {
	// 		// 0.3989422804f = 1/sqrt(2*pi)
	// 		return 0.3989422804f * math::expf(-0.5f * x * x / (sigma * sigma)) / sigma;
	// 	};

	// 	float sum = (gaussianKernel[0] = gaussianG(0, sigma));

	// 	for (int i = 1; i < blurSize + 1; ++i) {
	// 		sum += 2.0f * (gaussianKernel[i] = gaussianG(i, sigma));
	// 	}

	// 	for (auto& gk : gaussianKernel) {
	// 		gk /= sum;
	// 	}
	// };

	// constexpr float gSigma = 5.0f;
	// fillGaussianKernelFunc(gaussianKernel, gSigma);

	assert(mesh.empty());
	maximaMesh.resize((maxx) * (maxy), 0.0f);
	mesh.resize((maxx) * (maxy), 0.0f);
	tempMesh.resize((maxx) * (maxy), 0.0f);
	origMesh.resize((maxx) * (maxy), 0.0f);

	colsMaxima.clear();
	colsMaxima.resize(maxx, -std::numeric_limits<float>::max());
	maximaRows.clear();
	maximaRows.resize(maxx, -1);

	int2 min{0, 0};
	int2 max{maxx-1, maxy-1};
	int2 map{maxx, maxy};

// 	FindMaximumColumnHeights(int2{0, 0}, max, winSize, resolution, colsMaxima, maximaRows);

// 	for (int y = 0; y <= max.y; ++y) {
// 		AdvanceMaximaRows(y, 0, max.x, resolution, colsMaxima, maximaRows);
// 		FindRadialMaximum(map.x, y, 0, max.x, winSize, resolution, colsMaxima, maximaMesh);
// 		FixRemainingMaxima(0, y, max, winSize, resolution, colsMaxima, maximaRows);

// // #ifdef _DEBUG
// // 		CheckInvariants(y, maxx, maxy, winSize, resolution, colsMaxima, maximaRows);
// // #endif
// 	}

	// actually smooth with approximate Gaussian blur passes
	for (int numBlurs = blurPassesCount; numBlurs > 0; --numBlurs) {
		BlurHorizontal(map, min, max, blurSize, resolution, gaussianKernel, (numBlurs == blurPassesCount) ? maximaMesh : mesh, tempMesh); mesh.swap(tempMesh);
		BlurVertical(map, min, max, blurSize, resolution, gaussianKernel, mesh, tempMesh); mesh.swap(tempMesh);
	}

	// int i = 0;
	// for (int y =0; y < maxy; y++)
	// 	for (int x = 0; x < maxx; x++) {
	// 		maximaMesh[i++] = readMap->GetCornerHeightMapSynced()[(x + y*mapDims.mapxp1)*resolution];
	// 	}

	// <mesh> now contains the final smoothed heightmap, save it in origMesh
	std::copy(mesh.begin(), mesh.end(), origMesh.begin());
	// std::copy(maximaMesh.begin(), maximaMesh.end(), origMesh.begin());
	// std::copy(maximaMesh.begin(), maximaMesh.end(), mesh.begin());
}
