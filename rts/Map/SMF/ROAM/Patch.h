/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/GL/VAO.h"
#include "Rendering/GL/VertexArrayTypes.h"
#include "Map/MapDrawPassTypes.h"
#include "Game/Camera.h"
#include "System/Rectangle.h"
#include "System/type2.h"


#include <array>
#include <vector>


class CSMFGroundDrawer;
class CCamera;

// how many heightmap pixels a patch consists of
static constexpr int PATCH_SIZE = 128;

// debug (simulates fast pool exhaustion)
// #define NEW_POOL_SIZE (1 << 2)

class Patch; //declare it so that tritreenode can store its parent
// stores the triangle-tree structure, but no coordinates
struct TriTreeNode
{
	static TriTreeNode dummyNode;

	// all non-leaf nodes have both children, so just check for one
	bool IsLeaf() const { assert(!IsDummy()); return (LeftChild == &dummyNode); }
	bool IsBranch() const { assert(!IsDummy()); return (LeftChild != &dummyNode); }
	bool IsDummy() const { return (this == &dummyNode); }

	void Reset() {
		 LeftChild = &dummyNode;
		RightChild = &dummyNode;

		 BaseNeighbor = &dummyNode;
		 LeftNeighbor = &dummyNode;
		RightNeighbor = &dummyNode;
	}

	TriTreeNode*  LeftChild = &dummyNode;
	TriTreeNode* RightChild = &dummyNode;

	TriTreeNode*  BaseNeighbor = &dummyNode;
	TriTreeNode*  LeftNeighbor = &dummyNode;
	TriTreeNode* RightNeighbor = &dummyNode;

	Patch* parentPatch = nullptr; //triangles know their parent patch so they know of a neighbour's Split() func caused changes to them
};


// maintains a pool of TriTreeNodes, so we can (re)construct triangle-trees
// without dynamically (de)allocating nodes (note that InitPools() actually
// creates a pool for each worker thread to avoid locking)
class CTriNodePool
{
public:
	static void InitPools(bool shadowPass, size_t newPoolSize = NEW_POOL_SIZE);
	static void ResetAll(bool shadowPass);

	inline static CTriNodePool* GetPool(bool shadowPass);

public:
	void Resize(size_t poolSize);
	void Reset() { nextTriNodeIdx = 0; }
	bool Allocate(TriTreeNode*& left, TriTreeNode*& right);

	bool ValidNode(const TriTreeNode* n) const { return (n >= &tris.front() && n <= &tris.back()); }
	bool OutOfNodes() const { return (nextTriNodeIdx >= tris.size()); }

	size_t GetPoolSize() const { return tris.size(); }
	size_t GetNextTriNodeIdx() const { return nextTriNodeIdx; }
private:
	std::vector<TriTreeNode> tris;

	// index of next free TriTreeNode
	size_t nextTriNodeIdx = 0;

	// how many TriTreeNodes should be reserved per pool
	// (2M is a reasonable baseline for most large maps)
	// if a 32bit TriTreeNode struct is 28 bytes, the total ram usage of LOAM is 2M*28*2 = 104 MB
	static constexpr size_t NEW_POOL_SIZE = 1 << 21;

	static inline size_t CUR_POOL_SIZE = 0;                 // split over all threads
	static inline size_t MAX_POOL_SIZE = NEW_POOL_SIZE * 8; // upper limit for ResetAll
};



// stores information needed at the Patch level
class Patch
{
public:
	friend class CRoamMeshDrawer;
	friend class CPatchInViewChecker;

	Patch();
	Patch(const Patch&) = delete;
	Patch(Patch&& rhs) = default;
	~Patch();

	Patch& operator=(const Patch&) = delete;
	Patch& operator=(Patch&&) = default;

	void Init(CSMFGroundDrawer* drawer, int worldX, int worldZ);
	void Reset();

	TriTreeNode* GetBaseLeft()  { return &baseLeft;  }
	TriTreeNode* GetBaseRight() { return &baseRight; }

	bool IsVisible(const CCamera*) const;
	char IsDirty() const { return isDirty; }
	auto GetTriCount() const { return (indices.size() / 3); }

	void UpdateHeightMap(const SRectangle& rect = SRectangle(0, 0, PATCH_SIZE, PATCH_SIZE));

	float3 lastCameraPosition ; //the last camera position this patch was tesselated from

	//this specifies the manhattan distance from the camera during the last tesselation
	//note that this can only become lower, as we can only increase tesselation levels while maintaining no cracks
	float camDistanceLastTesselation;

	// create an approximate mesh

	bool Tessellate(const float3& camPos, int viewRadius, bool shadowPass);
	void ComputeVariance();

	void GenerateIndices();
	void Upload();
	void Draw() const;
	void DrawBorder() const;
	void SetSquareTexture(const DrawPass::e& drawPass) const;

	static void UpdateVisibility(CCamera* cam, std::vector<Patch>& patches, const int numPatchesX);
private:
	void UploadVertices();
	void UploadIndices();
	void UploadBorderVertices();

	void InitMainVAO() const;
	void InitBorderVAO() const;

	// recursive functions
	bool Split(TriTreeNode* tri);
	void RecursTessellate(TriTreeNode* tri, const int2 left, const int2 right, const int2 apex, const int varTreeIdx, const int curNodeIdx);
	void RecursRender(const TriTreeNode* tri, const int2 left, const int2 right, const int2 apex);

	float RecursComputeVariance(
		const   int2 left,
		const   int2 rght,
		const   int2 apex,
		const float3 hgts,
		const    int varTreeIdx,
		const    int curNodeIdx
	);

	void RecursGenBorderVertices(
		const TriTreeNode* tri,
		const int2 left,
		const int2 rght,
		const int2 apex,
		const int2 depth
	);

	float GetHeight(int2 pos);

	void GenerateBorderVertices();
private:
	// depth of variance tree; should be near SQRT(PATCH_SIZE) + 1
	static constexpr size_t VARIANCE_DEPTH = 12;

	CSMFGroundDrawer* smfGroundDrawer = nullptr;

	// pool used during Tessellate; each invoked Split allocates from this
	CTriNodePool* curTriPool = nullptr;
	float3 midPos;
	// does the variance-tree need to be recalculated for this Patch?
	bool isTesselated = false;
	bool isDirty = true;
	// Did the tesselation tree change from what we have stored in the VBO?
	bool isChanged = false;

	float varianceMaxLimit = std::numeric_limits<float>::max();
	float camDistLODFactor = 1.0f; // defines the LOD falloff in camera distance

	// world-coordinate offsets of this patch
	int2 coors = {-1, -1};

	int sumAffectedArea = 0;

	TriTreeNode baseLeft;  // left base-triangle tree node
	TriTreeNode baseRight; // right base-triangle tree node

	std::array<float, 1 << VARIANCE_DEPTH> varianceTrees[2];

	std::vector<float3> vertices;
	std::vector<uint32_t> indices;
	std::vector<VA_TYPE_C> borderVertices;

	// frame on which this patch was last visible, per pass
	// NOTE:
	//   shadow-mesh patches are only ever viewed by one camera
	//   normal-mesh patches can be viewed by *multiple* types!
	std::array<unsigned int, CCamera::CAMTYPE_VISCUL> lastDrawFrames = {};

	VBO vertVBO;
	VBO indxVBO;
	VBO borderVBO;

	VAO mainVAO;
	VAO borderVAO;
};