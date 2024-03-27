/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//
// ROAM Simplistic Implementation
// Added to Spring by Peter Sarkozy (mysterme AT gmail DOT com)
// Billion thanks to Bryan Turner (Jan, 2000)
//                    brturn@bellsouth.net
//
// Based on the Tread Marks engine by Longbow Digital Arts
//                               (www.LongbowDigitalArts.com)
// Much help and hints provided by Seumas McNally, LDA.
//

#include "Patch.h"
#include "RoamMeshDrawer.h"
#include "Map/ReadMap.h"
#include "Map/SMF/SMFGroundDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "System/Log/ILog.h"
#include "System/Threading/ThreadPool.h"
#include "xsimd/xsimd.hpp"

#include <climits>
#include <array>

#include <tracy/Tracy.hpp>


TriTreeNode TriTreeNode::dummyNode;

static std::array<CTriNodePool, CRoamMeshDrawer::MESH_COUNT> pools;

void CTriNodePool::InitPools(bool shadowPass, size_t newPoolSize)
{
	//ZoneScoped;
	for (int j = 0; newPoolSize > 0; j++) {
		try {
			size_t PoolSize =  std::max((CUR_POOL_SIZE = newPoolSize),newPoolSize); //the first pool should be larger, as only full retess uses threaded
			pools[shadowPass].Reset();
			pools[shadowPass].Resize(PoolSize + (PoolSize & 1));

			LOG_L(L_INFO, "[TriNodePool::%s] newPoolSize=" _STPF_ " PoolSize=" _STPF_ " (shadowPass=%d)", __func__, newPoolSize, PoolSize, shadowPass);
			break;
		} catch (const std::bad_alloc& e) {
			LOG_L(L_FATAL, "[TriNodePool::%s] exception \"%s\" (shadowPass=%d)", __func__, e.what(), shadowPass);

			// try again after reducing the wanted pool-size by a quarter
			newPoolSize = (MAX_POOL_SIZE = newPoolSize - (newPoolSize >> 2));
		}
	}
}

void CTriNodePool::ResetAll(bool shadowPass)
{
	//ZoneScoped;
	bool outOfNodes = pools[shadowPass].OutOfNodes();
	pools[shadowPass].Reset();
	if (!outOfNodes)
		return;
	if (CUR_POOL_SIZE >= MAX_POOL_SIZE)
		return;

	InitPools(shadowPass, std::min(CUR_POOL_SIZE * 2, MAX_POOL_SIZE));
}


CTriNodePool* CTriNodePool::GetPool(bool shadowPass)
{
	//ZoneScoped;
	return &(pools[shadowPass]);
}


void CTriNodePool::Resize(size_t poolSize)
{
	//ZoneScoped;
	// child nodes are always allocated in pairs, so poolSize must be even
	// (it does not technically need to be non-zero since patch root nodes
	// live outside the pool, but KISS)
	assert((poolSize & 1) == 0);
	assert(poolSize > 0);
	LOG_L(L_INFO, "[TriNodePool::%s] to " _STPF_,__func__, poolSize);

	tris.resize(poolSize);
}

bool CTriNodePool::Allocate(TriTreeNode*& left, TriTreeNode*& right)
{
	//ZoneScoped;
	// pool exhausted, make sure both child nodes are dummies
	if (OutOfNodes()) {
		LOG_L(L_WARNING, "[TriNodePool::%s] #nodes=" _STPF_ " #pool=" _STPF_ , __func__, nextTriNodeIdx, tris.size());

		left  = &TriTreeNode::dummyNode;
		right = &TriTreeNode::dummyNode;
		return false;
	}

	left  = &tris[nextTriNodeIdx++];
	right = &tris[nextTriNodeIdx++];

	left->Reset();
	right->Reset();
	return true;
}




Patch::Patch()
{
	//ZoneScoped;
	varianceTrees[0].fill(0.0f);
	varianceTrees[1].fill(0.0f);
}

Patch::~Patch()
{
	//ZoneScoped;
	//not really needed
	vertVBO = {};
	indxVBO = {};
	borderVBO = {};
}

void Patch::Init(CSMFGroundDrawer* _drawer, int patchX, int patchZ)
{
	//ZoneScoped;
	coors = { patchX, patchZ };

	smfGroundDrawer = _drawer;

	vertVBO   = { GL_ARRAY_BUFFER        , false, false };
	indxVBO   = { GL_ELEMENT_ARRAY_BUFFER, false, false };
	borderVBO = { GL_ARRAY_BUFFER        , false, false };

	vertices.resize((PATCH_SIZE + 1) * (PATCH_SIZE + 1));

	// initialize vertices
	unsigned int index = 0;
	for (int z = 0; z <= PATCH_SIZE; z++) {
		for (int x = 0; x <= PATCH_SIZE; x++) {
			vertices[index].x = x * SQUARE_SIZE;
			vertices[index].y = 0.0f;
			vertices[index].z = z * SQUARE_SIZE;
			index++;
		}
	}

	Reset();
	UpdateHeightMap();
	UploadVertices();
}

void Patch::Reset()
{
	//ZoneScoped;
	// reset the important relationships
	baseLeft  = {};
	baseRight = {};

	// attach the two base-triangles together
	baseLeft.BaseNeighbor  = &baseRight;
	baseRight.BaseNeighbor = &baseLeft;

	//Connect the base triangles to their parent
	baseLeft.parentPatch = this;
	baseRight.parentPatch = this;
	midPos.x = (coors.x + PATCH_SIZE / 2) * SQUARE_SIZE;
	midPos.z = (coors.y + PATCH_SIZE / 2) * SQUARE_SIZE;
	midPos.y = readMap->GetUnsyncedHeightInfo(coors.x / PATCH_SIZE, coors.y / PATCH_SIZE).z;

	//Reset camera
	lastCameraPosition.x = std::numeric_limits<float>::lowest();
	lastCameraPosition.y = std::numeric_limits<float>::lowest();
	lastCameraPosition.z = std::numeric_limits<float>::lowest();
	camDistanceLastTesselation = std::numeric_limits<float>::max();
}


void Patch::UpdateHeightMap(const SRectangle& rect)
{
	//ZoneScoped;
	midPos.y = readMap->GetUnsyncedHeightInfo(coors.x / PATCH_SIZE, coors.y / PATCH_SIZE).z;
	isDirty = true;
}


void Patch::UploadVertices()
{
	//ZoneScoped;
	vertVBO.Bind();
	vertVBO.New(vertices, GL_STATIC_DRAW);
	vertVBO.Unbind();
}

namespace {
	template<typename T>
	bool UploadStreamDrawData(VBO& vbo, GLenum target, const std::vector<T>& vec, size_t sizeUpMult, size_t sizeDownMult)
	{
		if (vec.empty())
			return false;

		static constexpr auto usage = GL_STREAM_DRAW;
		bool vboUpdated = false;

		vbo.Bind();
		if (const size_t sz = vec.size() * sizeof(T); (sz > vbo.GetSize() || vbo.GetSize() >= sz * sizeDownMult)) {
			// resize/remake the buffer without copying the old buffer content
			vbo.Unbind();
			vbo = VBO{ target, false, false };
			vbo.Bind();
			vbo.New(sz * sizeUpMult, usage, nullptr);
			vboUpdated = true;
		}
		vbo.SetBufferSubData(vec);
		vbo.Unbind();

		return vboUpdated;
	}
}

void Patch::UploadIndices()
{
	//ZoneScoped;
	if (UploadStreamDrawData(indxVBO, GL_ELEMENT_ARRAY_BUFFER, indices, 2, 8))
		InitMainVAO();
}

void Patch::UploadBorderVertices()
{
	//ZoneScoped;
	if (UploadStreamDrawData(borderVBO, GL_ARRAY_BUFFER, borderVertices, 2, 8))
		InitBorderVAO();
}

void Patch::InitMainVAO() const
{
	//ZoneScoped;
	mainVAO.Bind();

	indxVBO.Bind();
	vertVBO.Bind();

	glEnableVertexAttribArray(0);
	glVertexAttribDivisor(0, 0);

	glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, nullptr);

	mainVAO.Unbind();

	indxVBO.Unbind();
	vertVBO.Unbind();

	glDisableVertexAttribArray(0);
}

void Patch::InitBorderVAO() const
{
	//ZoneScoped;
	borderVAO.Bind();
	borderVBO.Bind();

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(0, 0);
	glVertexAttribDivisor(1, 0);

	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VA_TYPE_C), VA_TYPE_OFFSET(VA_TYPE_C, pos));
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true, sizeof(VA_TYPE_C), VA_TYPE_OFFSET(VA_TYPE_C, c));

	borderVAO.Unbind();
	borderVBO.Unbind();

	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);
}

// -------------------------------------------------------------------------------------------------
// Split a single Triangle and link it into the mesh.
// Will correctly force-split diamonds.
//
bool Patch::Split(TriTreeNode* tri)
{
	//ZoneScoped;
	if (tri->IsDummy())
		return false;

	// we are already split, no need to do it again
	if (!tri->IsLeaf())
		return true;

	// if this triangle is not in a proper diamond, force split our base-neighbor
	if (!tri->BaseNeighbor->IsDummy() && (tri->BaseNeighbor->BaseNeighbor != tri)) {
		Split(tri->BaseNeighbor);
		if (tri->BaseNeighbor->parentPatch != this)
			tri->BaseNeighbor->parentPatch->isChanged = true;
	}
	// create children and link into mesh, or make this triangle a leaf
	if (!curTriPool->Allocate(tri->LeftChild, tri->RightChild))
		return false;

	assert(tri->IsBranch());

	TriTreeNode* tlc = tri->LeftChild;
	TriTreeNode* trc = tri->RightChild;
	TriTreeNode* tln = tri->LeftNeighbor;
	TriTreeNode* trn = tri->RightNeighbor;
	TriTreeNode* tbn = tri->BaseNeighbor;

	// Set up parent patches so they notify them of changes
	tri->LeftChild->parentPatch = tri->parentPatch;
	tri->RightChild->parentPatch = tri->parentPatch;
	tri->parentPatch->isChanged = true;

	assert(!tlc->IsDummy());
	assert(!trc->IsDummy());

	{
		// fill in the information we can get from the parent (neighbor pointers)
		tlc->BaseNeighbor = tln;
		tlc->LeftNeighbor = trc;
	}
	{
		trc-> BaseNeighbor = trn;
		trc->RightNeighbor = tlc;
	}

	// link our left-neighbor to the new children
	if (!tln->IsDummy()) {
		if (tln->BaseNeighbor == tri)
			tln->BaseNeighbor = tlc;
		else if (tln->LeftNeighbor == tri)
			tln->LeftNeighbor = tlc;
		else if (tln->RightNeighbor == tri)
			tln->RightNeighbor = tlc;
		else
			;// illegal Left neighbor
	}

	// link our right-neighbor to the new children
	if (!trn->IsDummy()) {
		if (trn->BaseNeighbor == tri)
			trn->BaseNeighbor = trc;
		else if (trn->RightNeighbor == tri)
			trn->RightNeighbor = trc;
		else if (trn->LeftNeighbor == tri)
			trn->LeftNeighbor = trc;
		else
			;// illegal Right neighbor
	}

	// link our base-neighbor to the new children
	if (!tbn->IsDummy()) {
		if (tbn->IsBranch()) {
			assert(curTriPool->ValidNode(tbn->LeftChild));
			assert(curTriPool->ValidNode(tbn->RightChild));

			if (!(tlc->RightNeighbor = tbn->RightChild)->IsDummy())
				tbn->RightChild->LeftNeighbor = tlc;

			if (!(trc->LeftNeighbor = tbn->LeftChild)->IsDummy())
				tbn->LeftChild->RightNeighbor = trc;
		} else {
			// base Neighbor (in a diamond with us) was not split yet, do so now
			// FIXME: if pool ran out above, this will fail and leave a LOD-crack
			Split(tbn);
			if (tbn->parentPatch != this)
				tbn->parentPatch->isChanged = true;

		}
	} else {
		// edge triangle, trivial case
		tlc->RightNeighbor = &TriTreeNode::dummyNode;
		trc-> LeftNeighbor = &TriTreeNode::dummyNode;
	}

	return true;
}


// ---------------------------------------------------------------------
// Tessellate a Patch.
// Will continue to split until the variance metric is met.
//
void Patch::RecursTessellate(TriTreeNode* tri, const int2 left, const int2 right, const int2 apex, const int varTreeIdx, const int curNodeIdx)
{
	//ZoneScoped;
	// bail if we can not tessellate further in at least one dimension
	if ((abs(left.x - right.x) <= 1) && (abs(left.y - right.y) <= 1))
		return;
	if (tri->IsDummy())
		return;

	// default > 1; when variance isn't saved this issues further tessellation
	float triVariance = 10.0f;

	if (curNodeIdx < (1 << VARIANCE_DEPTH)) {
		// make maximum tessellation-level dependent on camDistLODFactor
		// huge cliffs cause huge variances and would otherwise always tessellate
		// regardless of the actual camera distance (-> huge/distfromcam ~= huge)
		const int sizeX = std::max(left.x - right.x, right.x - left.x);
		const int sizeY = std::max(left.y - right.y, right.y - left.y);
		const int size  = std::max(sizeX, sizeY);

		// take distance, variance and patch size into consideration
		triVariance = (std::min(varianceTrees[varTreeIdx][curNodeIdx], varianceMaxLimit) * PATCH_SIZE * size) * camDistLODFactor;
	}

	// stop tesselation
	if (triVariance <= 1.0f)
		return;

	// since we can 'retesselate' to a deeper depth, to preserve the trinodepool we will only split if its unsplit
	if (!tri->IsBranch()) {
		Split(tri);
		// we perform the split, and if the result is not a branch (e.g. couldnt split) we bail
		if(!tri->IsBranch())
			return;
	}
	// triangle was split, also try to split its children
	const int2 center = {(left.x + right.x) >> 1, (left.y + right.y) >> 1};

	RecursTessellate(tri-> LeftChild,  apex, left, center, varTreeIdx, (curNodeIdx << 1)    );
	RecursTessellate(tri->RightChild, right, apex, center, varTreeIdx, (curNodeIdx << 1) + 1);
}


// ---------------------------------------------------------------------
// Render the tree.
//

void Patch::RecursRender(const TriTreeNode* tri, const int2 left, const int2 right, const int2 apex)
{
	//ZoneScoped;
	if (tri->IsDummy())
		return;
	if (tri->IsLeaf()) {
		const int apexIndex = apex.x + apex.y * (PATCH_SIZE + 1);
		const int leftIndex = left.x + left.y * (PATCH_SIZE + 1);
		const int rightIndex = right.x + right.y * (PATCH_SIZE + 1);

		// Rotate the triangles if their hypotenuse becomes shorter
		if (!tri->BaseNeighbor->IsDummy() && (tri->BaseNeighbor->BaseNeighbor == tri)) {
			const int2 baseNeighborApex = left + right - apex;

			if (baseNeighborApex.x >= 0 && baseNeighborApex.y >= 0 && baseNeighborApex.x < PATCH_SIZE + 1 && baseNeighborApex.y < PATCH_SIZE + 1) {
				const float apexHeight = GetHeight(apex);
				const float leftHeight = GetHeight(left);
				const float rightHeight = GetHeight(right);

				float heightDiff = std::abs(leftHeight - rightHeight);

				const int baseNeighborApexIndex = baseNeighborApex.x + baseNeighborApex.y * (PATCH_SIZE + 1);
				const float baseNeighborApexHeight = GetHeight(baseNeighborApex);

				float heightDiff2 = std::abs(apexHeight - baseNeighborApexHeight);

				if (heightDiff2 < heightDiff - 0.0001f) {
					indices.push_back(apexIndex);
					indices.push_back(leftIndex);
					indices.push_back(baseNeighborApexIndex);
					return;
				}
			}
		}

		indices.push_back(apexIndex);
		indices.push_back(leftIndex);
		indices.push_back(rightIndex);
		return;
	}

	const int2 center = {(left.x + right.x) >> 1, (left.y + right.y) >> 1};

	RecursRender(tri-> LeftChild,  apex, left, center);
	RecursRender(tri->RightChild, right, apex, center);
}


void Patch::GenerateIndices()
{
	//ZoneScoped;
	indices.clear();
	RecursRender(&baseLeft,  int2(         0, PATCH_SIZE), int2(PATCH_SIZE,          0), int2(         0,          0));
	RecursRender(&baseRight, int2(PATCH_SIZE,          0), int2(         0, PATCH_SIZE), int2(PATCH_SIZE, PATCH_SIZE));
}

float Patch::GetHeight(int2 pos)
{
	//ZoneScoped;
	const auto* uhm = readMap->GetCornerHeightMapUnsynced();
	return uhm[(coors.y + pos.y) * mapDims.mapxp1 + (coors.x + pos.x)];
}

// ---------------------------------------------------------------------
// Computes Variance over the entire tree.  Does not examine node relationships.
//
float Patch::RecursComputeVariance(
	const   int2 left,
	const   int2 rght,
	const   int2 apex,
	const float3 hgts,
	const    int varTreeIdx,
	const    int curNodeIdx
) {
	/*      A
	 *     /|\
	 *    / | \
	 *   /  |  \
	 *  /   |   \
	 * L----M----R
	 *
	 * first compute the XZ coordinates of 'M' (hypotenuse middle)
	 */
	const int2 mpos = {(left.x + rght.x) >> 1, (left.y + rght.y) >> 1};

	// get the height value at M
	const float mhgt = GetHeight(mpos);

	// variance of this triangle is the actual height at its hypotenuse
	// midpoint minus the interpolated height; use values passed on the
	// stack instead of re-accessing the heightmap
	float mvar = math::fabs(mhgt - ((hgts.x + hgts.y) * 0.5f));

	// shore lines get more variance for higher accuracy
	// NOTE: .x := height(L), .y := height(R), .z := height(A)
	if ((hgts.x * hgts.y) < 0.0f || (hgts.x * mhgt) < 0.0f || (hgts.y * mhgt) < 0.0f)
		mvar = std::max(mvar * 1.5f, 20.0f);

	#if 0
	mvar = MAX(abs(left.x - rght.x), abs(left.y - rght.y)) * mvar;
	#endif

	// save some CPU, only calculate variance down to a 4x4 block
	if ((abs(left.x - rght.x) >= 4) || (abs(left.y - rght.y) >= 4)) {
		const float3 hgts1 = {hgts.z, hgts.x, mhgt};
		const float3 hgts2 = {hgts.y, hgts.z, mhgt};

		// final variance for this node is the max of its own variance and that of its children
		mvar = std::max(mvar, RecursComputeVariance(apex, left, mpos, hgts1, varTreeIdx, (curNodeIdx << 1)    ));
		mvar = std::max(mvar, RecursComputeVariance(rght, apex, mpos, hgts2, varTreeIdx, (curNodeIdx << 1) + 1));
	}

	// NOTE: Variance is never zero
	mvar = std::max(0.001f, mvar);

	// store the final variance for this node
	if (curNodeIdx < (1 << VARIANCE_DEPTH))
		varianceTrees[varTreeIdx][curNodeIdx] = mvar;

	return mvar;
}


// ---------------------------------------------------------------------
// Compute the variance tree for each of the Binary Triangles in this patch.
//
void Patch::ComputeVariance()
{
	//ZoneScoped;
	{
		const   int2 left = {         0, PATCH_SIZE};
		const   int2 rght = {PATCH_SIZE,          0};
		const   int2 apex = {         0,          0};
		const float3 hgts = {
			GetHeight(left),
			GetHeight(rght),
			GetHeight(apex),
		};

		RecursComputeVariance(left, rght, apex, hgts, 0, 1);
	}

	{
		const   int2 left = {PATCH_SIZE,          0};
		const   int2 rght = {         0, PATCH_SIZE};
		const   int2 apex = {PATCH_SIZE, PATCH_SIZE};
		const float3 hgts = {
			GetHeight(left),
			GetHeight(rght),
			GetHeight(apex),
		};

		RecursComputeVariance(left, rght, apex, hgts, 1, 1);
	}

	// Clear the dirty flag for this patch
	isDirty = false;
}


// ---------------------------------------------------------------------
// Create an approximate mesh.
//
bool Patch::Tessellate(const float3& camPos, int viewRadius, bool shadowPass)
{
	//ZoneScoped;
	isTesselated = true;

	// Set/Update LOD params (FIXME: wrong height?)

	curTriPool = CTriNodePool::GetPool(shadowPass);

	// MAGIC NUMBER 1: scale factor to reduce LOD with camera distance
	camDistLODFactor  = midPos.distance(camPos);
	camDistanceLastTesselation = camDistLODFactor; //store distance from camera
	camDistLODFactor *= (300.0f / viewRadius);
	camDistLODFactor  = std::max(1.0f, camDistLODFactor);
	camDistLODFactor  = 1.0f / camDistLODFactor;

	// MAGIC NUMBER 2:
	//   regulates how deeply areas are tessellated by clamping variances to it
	//   (the maximum tessellation is still untouched, this reduces the maximum
	//   far-distance LOD while the param above defines an overall falloff-rate)
	varianceMaxLimit = viewRadius * 0.35f;

	{
		// split each of the base triangles
		const int2 left = {coors.x,              coors.y + PATCH_SIZE};
		const int2 rght = {coors.x + PATCH_SIZE, coors.y             };
		const int2 apex = {coors.x,              coors.y             };

		RecursTessellate(&baseLeft, left, rght, apex, 0, 1);
	}
	{
		const int2 left = {coors.x + PATCH_SIZE, coors.y             };
		const int2 rght = {coors.x,              coors.y + PATCH_SIZE};
		const int2 apex = {coors.x + PATCH_SIZE, coors.y + PATCH_SIZE};

		RecursTessellate(&baseRight, left, rght, apex, 1, 1);
	}

	// mark patches that are totally flat and did not get split in RecursTessellate
	// as 'changed', so their vertices can be updated
	if (baseLeft.IsLeaf() && baseRight.IsLeaf()) isChanged = true;

	lastCameraPosition = camPos;
	return (!curTriPool->OutOfNodes());
}


// ---------------------------------------------------------------------
// Render the mesh.
//

void Patch::Draw() const
{
	//ZoneScoped;
	if (indices.empty())
		return;

	mainVAO.Bind();
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
	mainVAO.Unbind();
}


void Patch::DrawBorder() const
{
	//ZoneScoped;
	if (borderVertices.empty())
		return;

	borderVAO.Bind();
	glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(borderVertices.size()));
	borderVAO.Unbind();
}

void Patch::RecursGenBorderVertices(
	const TriTreeNode* tri,
	const int2 left,
	const int2 rght,
	const int2 apex,
	const int2 depth
) {
	//ZoneScoped;
	if (tri->IsDummy())
		return;

	if (tri->IsLeaf()) {
		const float3& v1 = vertices[(apex.x + apex.y * (PATCH_SIZE + 1))];
		const float3& v2 = vertices[(left.x + left.y * (PATCH_SIZE + 1))];
		const float3& v3 = vertices[(rght.x + rght.y * (PATCH_SIZE + 1))];

		static constexpr unsigned char white[] = {255, 255, 255, 255};
		static constexpr unsigned char trans[] = {255, 255, 255,   0};

		if ((depth.x & 1) == 0) {
			borderVertices.push_back(VA_TYPE_C{ v2,                 {white}});
			borderVertices.push_back(VA_TYPE_C{{v2.x, -1.0f, v2.z}, {trans}});
			borderVertices.push_back(VA_TYPE_C{ v3                , {white}});

			borderVertices.push_back(VA_TYPE_C{{v2.x, -1.0f, v2.z}, {trans}});
			borderVertices.push_back(VA_TYPE_C{{v3.x, -1.0f, v3.z}, {trans}});
			borderVertices.push_back(VA_TYPE_C{ v3                , {white}});
			return;
		}

		if (depth.y) {
			// left child
			borderVertices.push_back(VA_TYPE_C{ v1                , {white}});
			borderVertices.push_back(VA_TYPE_C{{v1.x, -1.0f, v1.z}, {trans}});
			borderVertices.push_back(VA_TYPE_C{ v2                , {white}});

			borderVertices.push_back(VA_TYPE_C{{v1.x, -1.0f, v1.z}, {trans}});
			borderVertices.push_back(VA_TYPE_C{{v2.x, -1.0f, v2.z}, {trans}});
			borderVertices.push_back(VA_TYPE_C{ v2               , {white}});
		} else {
			// right child
			borderVertices.push_back(VA_TYPE_C{ v3                , {white}});
			borderVertices.push_back(VA_TYPE_C{{v3.x, -1.0f, v3.z}, {trans}});
			borderVertices.push_back(VA_TYPE_C{ v1                , {white}});

			borderVertices.push_back(VA_TYPE_C{{v3.x, -1.0f, v3.z}, {trans}});
			borderVertices.push_back(VA_TYPE_C{{v1.x, -1.0f, v1.z}, {trans}});
			borderVertices.push_back(VA_TYPE_C{ v1                , {white}});
		}

		return;
	}

	const int2 center = {(left.x + rght.x) >> 1, (left.y + rght.y) >> 1};

	// at even depths, descend down left *and* right children since both
	// are on the patch-edge; returns are needed for gcc's TCO (although
	// unlikely to be applied)
	if ((depth.x & 1) == 0) {
		       RecursGenBorderVertices(tri-> LeftChild, apex, left, center, {depth.x + 1, !depth.y});
		return RecursGenBorderVertices(tri->RightChild, rght, apex, center, {depth.x + 1,  depth.y});
	}

	// at odd depths (where only one triangle is on the edge), always force
	// a left-bias for the next call so the recursion ends up at the correct
	// leafs
	if (depth.y) {
		return RecursGenBorderVertices(tri-> LeftChild, apex, left, center, {depth.x + 1, true});
	} else {
		return RecursGenBorderVertices(tri->RightChild, rght, apex, center, {depth.x + 1, true});
	}
}

void Patch::GenerateBorderVertices()
{
	//ZoneScoped;
	if (!isTesselated)
		return;

	isTesselated = false;

	borderVertices.clear();
	borderVertices.reserve((PATCH_SIZE + 1) * 2);

	static constexpr auto PS = PATCH_SIZE;
	// border vertices are always part of base-level triangles
	// that have either no left or no right neighbor, i.e. are
	// on the map edge
	if (baseLeft . LeftNeighbor->IsDummy()) RecursGenBorderVertices(&baseLeft , { 0, PS}, {PS,  0}, { 0,  0}, {1,  true}); // left border
	if (baseLeft .RightNeighbor->IsDummy()) RecursGenBorderVertices(&baseLeft , { 0, PS}, {PS,  0}, { 0,  0}, {1, false}); // right border
	if (baseRight.RightNeighbor->IsDummy()) RecursGenBorderVertices(&baseRight, {PS,  0}, { 0, PS}, {PS, PS}, {1, false}); // bottom border
	if (baseRight. LeftNeighbor->IsDummy()) RecursGenBorderVertices(&baseRight, {PS,  0}, { 0, PS}, {PS, PS}, {1,  true}); // top border
}


void Patch::Upload()
{
	//ZoneScoped;
	UploadIndices();
	UploadBorderVertices();
	isChanged = false;
}

void Patch::SetSquareTexture(const DrawPass::e& drawPass) const
{
	//ZoneScoped;
	smfGroundDrawer->SetupBigSquare(drawPass, coors.x / PATCH_SIZE, coors.y / PATCH_SIZE);
}

void Patch::UpdateVisibility(CCamera* cam, std::vector<Patch>& patches, const int numPatchesX)
{
	//ZoneScoped;
	assert(cam->GetCamType() < CCamera::CAMTYPE_VISCUL);

	const float minHeight = readMap->GetCurrMinHeight() - 100.0f;
	const float maxHeight = readMap->GetCurrMaxHeight() + 100.0f;

	static constexpr float wsEdge = PATCH_SIZE * SQUARE_SIZE;

	const int drawQuadsX = mapDims.mapx / PATCH_SIZE;
	const int drawQuadsZ = mapDims.mapy / PATCH_SIZE;

	for (int x = 0; x < drawQuadsX; ++x) {
		for (int z = 0; z < drawQuadsZ; ++z) {
			const auto& uhmi = readMap->GetUnsyncedHeightInfo(x, z);

			AABB aabb{
				{ (x + 0) * wsEdge, uhmi.x, (z + 0) * wsEdge },
				{ (x + 1) * wsEdge, uhmi.y, (z + 1) * wsEdge }
			};

			if (!cam->InView(aabb))
				continue;

			patches[z * numPatchesX + x].lastDrawFrames[cam->GetCamType()] = globalRendering->drawFrame;
		}
	}
}

bool Patch::IsVisible(const CCamera* cam) const {
	//ZoneScoped;
	return (lastDrawFrames[cam->GetCamType()] >= globalRendering->drawFrame);
}