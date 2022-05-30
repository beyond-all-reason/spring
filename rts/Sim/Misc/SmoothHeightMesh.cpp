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

#include "System/Log/ILog.h"

constexpr int SMOOTH_MESH_UPDATE_DELAY = GAME_SPEED;
constexpr int samplesPerQuad = 32; 

SmoothHeightMesh smoothGround;


static float Interpolate(float x, float y, const int maxx, const int maxy, const float res, const float* heightmap)
{
	x = Clamp(x / res, 0.0f, ((float)maxx - 1));
	y = Clamp(y / res, 0.0f, ((float)maxy - 1));
	const int sx = x;
	const int sy = y;
	const float dx = (x - sx);
	const float dy = (y - sy);

	const int sxp1 = std::min(sx + 1, maxx - 1);
	const int syp1 = std::min(sy + 1, maxy - 1);

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
	maxx = max.x / resolution;
	maxy = max.y / resolution;

	smoothRadius = std::max(1, smoothRad);

	MakeSmoothMesh();
}

void SmoothHeightMesh::Kill() {
	while (!meshDamageTrack.damageQueue[0].empty()) { meshDamageTrack.damageQueue[0].pop(); }
	while (!meshDamageTrack.damageQueue[1].empty()) { meshDamageTrack.damageQueue[1].pop(); }
	while (!meshDamageTrack.horizontalBlurQueue.empty()) { meshDamageTrack.horizontalBlurQueue.pop(); }

	meshDamageTrack.damageMap.clear();
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

inline static float GetGroundHeight(int x, int y, int resolution) {
	const float* heightMap = readMap->GetCornerHeightMapSynced();
	const int baseIndex = (x + y*mapDims.mapxp1)*resolution;

	return heightMap[baseIndex];

	// float max = -std::numeric_limits<float>::max();
	// for (int y1 = y*resolution; y1 < (y+1)*resolution; ++y1) {
	// 	for (int x1 = x*resolution; x1 < (x+1)*resolution; ++x1) {
	// 		float h = heightMap[x1 + y1*mapDims.mapxp1];
	// 		if (h > max) max = h;
	// 	}
	// }
	// return max;
}

inline static void FindMaximumColumnHeights(
	const int2 map,
	const int y,
	const int minx,
	const int maxx,
	const int winSize,
	const int resolution,
	std::vector<float>& colsMaxima,
	std::vector<int>& maximaRows
) {
	// initialize the algorithm: find the maximum
	// height per column and the corresponding row

	const int miny = std::max(y - winSize, 0);
	const int maxy = std::min(y + winSize, map.y - 1);

	for (int y1 = miny; y1 <= maxy; ++y1) {
		for (int x = minx; x <= maxx; ++x)  {
			// const float curh = readMap->GetCornerHeightMapSynced()[(x + y*mapDims.mapxp1)*resolution];
			const float curh = GetGroundHeight(x, y1, resolution);

			// LOG("%s: y:%d x:%d cur: %f", __func__, y, x, curh);

			if (curh >= colsMaxima[x]) {
				colsMaxima[x] = curh;
				maximaRows[x] = y1;
			}
		}
	}
	// for (int x = minx; x <= maxx; ++x)
	// 	LOG("%s: y:%d col:%d row:%d max: %f", __func__, y, x, maximaRows[x], colsMaxima[x]);
}


inline static void FindRadialMaximum(
	const int2 map,
	int y,
	int minx,
	int maxx,
	int winSize,
	float resolution,
	const std::vector<float>& colsMaxima,
	      std::vector<float>& mesh
) {
	for (int x = minx; x <= maxx; ++x) {
		float maxRowHeight = -std::numeric_limits<float>::max();

		// find current maximum within radius smoothRadius
		// (in every column stack) along the current row
		const int startx = std::max(x - winSize, 0);
		const int endx = std::min(x + winSize, map.x - 1);

		// for (int i = startx; i <= endx; ++i) { // SSE candidate?
		// 	maxRowHeight = std::max(colsMaxima[i], maxRowHeight);
		// }

		const int endIdx = endx - 3;
		__m128 best = _mm_loadu_ps(&colsMaxima[startx]);
		for (int i = startx + 4; i < endIdx; i += 4) {
			__m128 next = _mm_loadu_ps(&colsMaxima[i]);
			best = _mm_max_ps(best, next);
		}

		{
			__m128 next = _mm_loadu_ps(&colsMaxima[endIdx]);
			best = _mm_max_ps(best, next);
		}

		{
			// split the four values into sets of two and compare
			__m128 bestAlt = _mm_movehl_ps(best, best);
			best = _mm_max_ps(best, bestAlt);

			// split the two values and compare
			bestAlt = _mm_shuffle_ps(best, best, _MM_SHUFFLE(0, 0, 0, 1));
			best = _mm_max_ss(best, bestAlt);
			_mm_store_ss(&maxRowHeight, best);
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

		mesh[x + y * map.x] = maxRowHeight;

		// LOG("%s: y:%d x:%d local max: %f", __func__, y, x, maxRowHeight);

		// for (int x = minx; x <= maxx; ++x)
		// 	LOG("%s: y:%d x:%d local max: %f", __func__, y, x, maxRowHeight);
	}
}



inline static void AdvanceMaxima(
	const int2 map,
	const int y,
	const int minx,
	const int maxx,
	const int winSize,
	const int resolution,
	std::vector<float>& colsMaxima,
	std::vector<int>& maximaRows
) {
	// fix remaining maximums after a pass
	const int miny = std::max(y - winSize, 0);
	const int virtualRow = y + winSize;
	const int maxy = std::min(virtualRow, map.y - 1);

	for (int x = minx; x <= maxx; ++x) {
// #ifdef _DEBUG
// 		for (int y1 = std::max(min.y, min.y - winSize); y1 <= std::min(max.y, min.y + winSize); ++y1) {
// 			assert(CGround::GetHeightReal(x * resolution, y1 * resolution) <= colsMaxima[x]);
// 		}
// #endif
		if (maximaRows[x] < miny) {
			// find a new maximum if the old one left the window
			colsMaxima[x] = -std::numeric_limits<float>::max();

			for (int y1 = miny; y1 <= maxy; ++y1) {
				// const float h = readMap->GetCornerHeightMapSynced()[(x + y1*mapDims.mapxp1)*resolution];
				const float h = GetGroundHeight(x, y1, resolution);

				if (h >= colsMaxima[x]) {
					colsMaxima[x] = h;
					maximaRows[x] = y1;
				}
			}
		} else if (virtualRow < map.y) {
			// else, just check if a new maximum has entered the window
			// const float h = readMap->GetCornerHeightMapSynced()[(x + maxy*mapDims.mapxp1)*resolution];
			const float h = GetGroundHeight(x, maxy, resolution);

			if (h >= colsMaxima[x]) {
				colsMaxima[x] = h;
				maximaRows[x] = maxy;
			}
		}

		// LOG("%s: y:%d x:%d column max: %f", __func__, y, x, colsMaxima[x]);

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
	const std::vector<float>& mesh,
	      std::vector<float>& smoothed
) {
	const int lineSize = mapSize.x;
	const int mapMaxX = mapSize.x - 1;

	//for_mt(min.y, max.y+1, [&](const int y)
	for (int y = min.y; y <= max.y; ++y)
	{
		float avg = 0.0f;
		float lv = 0;
		float rv = 0;
		float weight = 1.f / ((float)(blurSize*2 + 1));
		int li = min.x - blurSize;
		int ri = min.x + blurSize;
		for (int x1 = li; x1 <= ri; ++x1) {
		 	avg += mesh[std::max(0, std::min(x1, mapMaxX)) + y * lineSize];
		}
		ri++;

		for (int x = min.x; x <= max.x; ++x)
		{
			avg += (-lv) + rv;

			//const float ghaw = CGround::GetHeightReal(x * resolution, y * resolution);
			// const float ghaw = readMap->GetCornerHeightMapSynced()[(x + y*mapDims.mapxp1)*resolution];
			const float ghaw = GetGroundHeight(x, y, resolution);

			smoothed[x + y * lineSize] = std::max(ghaw, avg*weight);

			// LOG("%s: x: %d, y: %d, avg: %f (%f) (g: %f)", __func__, x, y, avg, avg*weight, ghaw);

			// #pragma message ("FIX ME")
			// smoothed[x + y * lineSize] = std::clamp(
			// 	smoothed[x + y * lineSize],
			// 	readMap->GetCurrMinHeight(),
			// 	readMap->GetCurrMaxHeight()
			// );

			// assert(smoothed[x + y * lineSize] <=          readMap->GetCurrMaxHeight()       );
			// assert(smoothed[x + y * lineSize] >=          readMap->GetCurrMinHeight()       );

			lv = mesh[std::max(0, std::min(li, mapMaxX)) + y * lineSize];
			rv = mesh[            std::min(ri, mapMaxX)  + y * lineSize];
			li++; ri++;
		}
	}//);
}

inline static void BlurVertical(
	const int2 mapSize,
	const int2 min,
	const int2 max,
	const int blurSize,
	const int resolution,
	const std::vector<float>& mesh,
	      std::vector<float>& smoothed
) {
	//SCOPED_TIMER("Sim::SmoothHeightMesh::BlurVertical");
	const int lineSize = mapSize.x;
	const int mapMaxY = mapSize.y - 1;

	//for_mt(min.x, max.x+1, [&](const int x)
	for (int x = min.x; x <= max.x; ++x)
	{
		float avg = 0.0f;
		float lv = 0;
		float rv = 0;
		float weight = 1.f / ((float)(blurSize*2 + 1));
		int li = min.y - blurSize;
		int ri = min.y + blurSize;
		for (int y1 = li; y1 <= ri; ++y1) {
			avg += mesh[x + std::max(0, std::min(y1, mapMaxY)) * lineSize];
		}
		// LOG("%s: starting average is: %f (%f) (w: %f)", __func__, avg, avg*weight, weight);
		ri++; // ri points to the next value to add

		for (int y = min.y; y <= max.y; ++y)
		{
			avg += (-lv) + rv;

			//const float ghaw = CGround::GetHeightReal(x * resolution, y * resolution);
			// const float ghaw = readMap->GetCornerHeightMapSynced()[(x + y*mapDims.mapxp1)*resolution];
			const float ghaw = GetGroundHeight(x, y, resolution);

			// LOG("%s: x: %d, y: %d, avg: %f (%f) (g: %f)", __func__, x, y, avg, avg*weight, ghaw);

			smoothed[x + y * lineSize] = std::max(ghaw, avg*weight);

			// #pragma message ("FIX ME")
			// smoothed[x + y * lineSize] = std::clamp(
			// 	smoothed[x + y * lineSize],
			// 	readMap->GetCurrMinHeight(),
			// 	readMap->GetCurrMaxHeight()
			// );

			// assert(smoothed[x + y * lineSize] <=          readMap->GetCurrMaxHeight()       );
			// assert(smoothed[x + y * lineSize] >=          readMap->GetCurrMinHeight()       );

			lv = mesh[ x + std::max(0, std::min(li, mapMaxY)) * lineSize];
			rv = mesh[ x +             std::min(ri, mapMaxY)  * lineSize];
			li++; ri++;

			// LOG("%s: for next line -%f +%f", __func__, lv, rv);
		}
	}//);
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

void SmoothHeightMesh::OnMapDamage(int x1, int y1, int x2, int y2) {

	const bool queueWasEmpty = meshDamageTrack.damageQueue[meshDamageTrack.activeBuffer].empty();
	const int res = resolution*samplesPerQuad;
	const int w = meshDamageTrack.width;
	const int h = meshDamageTrack.height;
	
	const int2 min  { std::max((x1 - smoothRadius) / res, 0)
					, std::max((y1 - smoothRadius) / res, 0)};
	const int2 max  { std::min((x2 + smoothRadius - 1) / res, (w-1))
					, std::min((y2 + smoothRadius - 1) / res, (h-1))};

	for (int y = min.y; y <= max.y; ++y) {
		int i = min.x + y*w;
		for (int x = min.x; x <= max.x; ++x, ++i) {
			if (!meshDamageTrack.damageMap[i]) {
				meshDamageTrack.damageMap[i] = true;
				meshDamageTrack.damageQueue[meshDamageTrack.activeBuffer].push(i);
			}
		}	
	}

	const bool queueWasUpdated = !meshDamageTrack.damageQueue[meshDamageTrack.activeBuffer].empty();

	if (queueWasEmpty && queueWasUpdated)
		meshDamageTrack.queueReleaseOnFrame = gs->frameNum + SMOOTH_MESH_UPDATE_DELAY;
}


void SmoothHeightMesh::UpdateSmoothMesh()
{
	SCOPED_TIMER("Sim::SmoothHeightMesh::UpdateSmoothMesh");

	const bool flushBuffer = !meshDamageTrack.activeBuffer;
	const bool activeBuffer = meshDamageTrack.activeBuffer;
	const bool currentWorkloadComplete = meshDamageTrack.damageQueue[flushBuffer].empty()
									  && meshDamageTrack.horizontalBlurQueue.empty()
									  && meshDamageTrack.verticalBlurQueue.empty();

	// LOG("%s: flush buffer is %d; damage queue is %I64d; blur queue is %I64d",
	// 	__func__,
	// 	(int)flushBuffer,
	// 	meshDamageTrack.damageQueue[flushBuffer].size(),
	// 	meshDamageTrack.blurQueue.size()
	// 	);

	if (currentWorkloadComplete){
		const bool activeBufferReady = !meshDamageTrack.damageQueue[activeBuffer].empty()
									&& gs->frameNum >= meshDamageTrack.queueReleaseOnFrame;
		if (activeBufferReady) {
			meshDamageTrack.activeBuffer = !meshDamageTrack.activeBuffer;
			// LOG("%s: opening new queue", __func__);
		}
		return;
	}

	bool updateMaxima = !meshDamageTrack.damageQueue[flushBuffer].empty();
	bool doHorizontalBlur = !meshDamageTrack.horizontalBlurQueue.empty();
	int damagedAreaIndex = 0;

	std::queue<int>* activeQueue = nullptr;

	if (updateMaxima) 
		activeQueue = &meshDamageTrack.damageQueue[flushBuffer];
	else if (doHorizontalBlur) {
		activeQueue = &meshDamageTrack.horizontalBlurQueue;
	} else {
		activeQueue = &meshDamageTrack.verticalBlurQueue;
	}

	damagedAreaIndex = activeQueue->front();
	activeQueue->pop();

	const int winSize = smoothRadius / resolution;
	const int blurSize = std::max(1, winSize / 2);

	int2 impactRadius{winSize, winSize};

	const int damageX = damagedAreaIndex % meshDamageTrack.width;
	const int damageY = damagedAreaIndex / meshDamageTrack.width;

	// area of map damaged.
	// area of the map which may need a max height change
	int2 damageMin{damageX*samplesPerQuad, damageY*samplesPerQuad};
	int2 damageMax = damageMin + int2{samplesPerQuad - 1, samplesPerQuad - 1};

	// area of map to load maximums for to update the updateLocation
	int2 min = damageMin - impactRadius;
	int2 max = damageMax + impactRadius;
	int2 map{maxx, maxy};

	damageMin.x = std::clamp(damageMin.x, 0, map.x);
	damageMin.y = std::clamp(damageMin.y, 0, map.y);
	damageMax.x = std::clamp(damageMax.x, 0, map.x);
	damageMax.y = std::clamp(damageMax.y, 0, map.y);
	min.x = std::clamp(min.x, 0, map.x - 1);
	min.y = std::clamp(min.y, 0, map.y - 1);
	max.x = std::clamp(max.x, 0, map.x - 1);
	max.y = std::clamp(max.y, 0, map.y - 1);

	if (updateMaxima) {

		// LOG("%s: quad index %d (%d,%d)-(%d,%d) (%d,%d)-(%d,%d) updating maxima",
		// 	__func__,
		// 	damagedAreaIndex,
		// 	damageMin.x, damageMin.y,
		// 	damageMax.x, damageMax.y,
		// 	min.x, min.y,
		// 	max.x, max.y
		// 	);

		// LOG("%s: quad area in world space (%f,%f) (%f,%f)", __func__
		// 	, (float)damageMin.x * fresolution
		// 	, (float)damageMin.y * fresolution
		// 	, (float)damageMax.x * fresolution
		// 	, (float)damageMax.y * fresolution
		// 	);

		for (int i = min.x; i <= max.x; ++i)
			colsMaxima[i] = -std::numeric_limits<float>::max();

		for (int i = min.x; i <= max.x; ++i)
			maximaRows[i] = -1;

		FindMaximumColumnHeights(map, damageMin.y, min.x, max.x, winSize, resolution, colsMaxima, maximaRows);

		for (int y = damageMin.y; y <= damageMax.y; ++y) {
			FindRadialMaximum(map, y, damageMin.x, damageMax.x, winSize, resolution, colsMaxima, maximaMesh);
			AdvanceMaxima(map, y+1, min.x, max.x, winSize, resolution, colsMaxima, maximaRows);
		}
		//std::copy(maximaMesh.begin(), maximaMesh.end(), mesh.begin());

		meshDamageTrack.horizontalBlurQueue.push(damagedAreaIndex);
		meshDamageTrack.damageMap[damagedAreaIndex] = false;
	} else {

		// LOG("%s: quad index %d (%d,%d)-(%d,%d) (%d,%d)-(%d,%d) applying blur",
		// 	__func__,
		// 	damagedAreaIndex,
		// 	damageMin.x, damageMin.y,
		// 	damageMax.x, damageMax.y,
		// 	min.x, min.y,
		// 	max.x, max.y
		// 	);

		// LOG("%s: quad area in world space (%f,%f) (%f,%f)", __func__
		// 	, (float)damageMin.x * fresolution
		// 	, (float)damageMin.y * fresolution
		// 	, (float)damageMax.x * fresolution
		// 	, (float)damageMax.y * fresolution
		// 	);

		// BlurHorizontal(map, damageMin, damageMax, blurSize, resolution, maximaMesh, mesh);
		// BlurVertical(map, damageMin, damageMax, blurSize, resolution, maximaMesh, mesh);
		if (doHorizontalBlur) {
			BlurHorizontal(map, damageMin, damageMax, blurSize, resolution, maximaMesh, tempMesh);
			meshDamageTrack.verticalBlurQueue.push(damagedAreaIndex);
		}
		else {
			BlurVertical(map, damageMin, damageMax, blurSize, resolution, tempMesh, mesh);

			for (int y = damageMin.y; y <= damageMax.y; ++y) {
				const int startIdx = damageMin.x + y*map.x;
				const int endIdx = damageMax.x + y*map.x + 1;
				std::copy(&mesh[startIdx], &mesh[endIdx], &tempMesh[startIdx]);
			}
		}
	}

	// std::copy(maximaMesh.begin(), maximaMesh.end(), mesh.begin());
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

	
	const int damageTrackWidth = maxx / samplesPerQuad + (maxx % samplesPerQuad ? 1 : 0);
	const int damageTrackHeight = maxy / samplesPerQuad + (maxy % samplesPerQuad ? 1 : 0);
	const int damageTrackQuads = damageTrackWidth * damageTrackHeight;
	meshDamageTrack.damageMap.clear();
	meshDamageTrack.damageMap.resize(damageTrackQuads);
	meshDamageTrack.width = damageTrackWidth;
	meshDamageTrack.height = damageTrackHeight;

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

 	FindMaximumColumnHeights(map, 0, 0, max.x, winSize, resolution, colsMaxima, maximaRows);

	for (int y = 0; y <= max.y; ++y) {
		//AdvanceMaximaRows(y, 0, max.x, resolution, colsMaxima, maximaRows);
		FindRadialMaximum(map, y, 0, max.x, winSize, resolution, colsMaxima, maximaMesh);
		AdvanceMaxima(map, y+1, 0, max.x, winSize, resolution, colsMaxima, maximaRows);

// #ifdef _DEBUG
// 		CheckInvariants(y, maxx, maxy, winSize, resolution, colsMaxima, maximaRows);
// #endif
	}

	// actually smooth with approximate Gaussian blur passes
	BlurHorizontal(map, min, max, blurSize, resolution, maximaMesh, tempMesh);
	BlurVertical(map, min, max, blurSize, resolution, tempMesh, mesh);

	// int i = 0;
	// for (int y =0; y < maxy; y++)
	// 	for (int x = 0; x < maxx; x++) {
	// 		maximaMesh[i++] = readMap->GetCornerHeightMapSynced()[(x + y*mapDims.mapxp1)*resolution];
	// 	}

	// <mesh> now contains the final smoothed heightmap, save it in origMesh
	std::copy(mesh.begin(), mesh.end(), origMesh.begin());
	std::copy(mesh.begin(), mesh.end(), tempMesh.begin());
	// std::copy(maximaMesh.begin(), maximaMesh.end(), origMesh.begin());
	// std::copy(maximaMesh.begin(), maximaMesh.end(), mesh.begin());
}
