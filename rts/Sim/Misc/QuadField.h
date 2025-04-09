/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QUAD_FIELD_H
#define QUAD_FIELD_H

#include <algorithm>
#include <array>
#include <vector>

#include "System/Misc/NonCopyable.h"
#include "System/Threading/ThreadPool.h"
#include "System/creg/creg_cond.h"
#include "System/float3.h"
#include "System/type2.h"

class CUnit;
class CFeature;
class CProjectile;
class CSolidObject;
class CPlasmaRepulser;
struct QuadFieldQuery;

template<typename T>
class QueryVectorCache {
public:
	typedef std::pair<bool, std::vector<T>> PairType;

	std::vector<T>* ReserveVector(size_t base = 0, size_t capa = 1024) {
		const auto pred = [](const PairType& p) { return (!p.first); };
		const auto iter = std::find_if(vectors.begin() + base, vectors.end(), pred);

		if (iter != vectors.end()) {
			iter->first = true;
			iter->second.clear();
			iter->second.reserve(capa);
			return &iter->second;
		}

		assert(false);
		return nullptr;
	}

	void ReserveAll(size_t capa) {
		for (size_t i = 0; i < vectors.size(); ++i) {
			ReserveVector(i, capa);
		}
	}

	void ReleaseVector(const std::vector<T>* released) {
		if (released == nullptr)
			return;

		const auto pred = [&](const PairType& p) { return (&p.second == released); };
		const auto iter = std::find_if(vectors.begin(), vectors.end(), pred);

		if (iter == vectors.end()) {
			assert(false);
			return;
		}

		iter->first = false;
	}
	void ReleaseAll() {
		for (auto& pair: vectors) {
			ReleaseVector(&pair.second);
		}
	}
private:
	// There should at most be 2 concurrent users of each vector type
	// using 3 to be safe, increase this number if the assertions below
	// fail
	std::array<PairType, 3> vectors = {{{false, {}}, {false, {}}, {false, {}}}};
};



class CQuadField : spring::noncopyable
{
	CR_DECLARE_STRUCT(CQuadField)
	CR_DECLARE_SUB(Quad)

public:


	void Init(int2 mapDims, int quadSize);
	void Kill();

	void GetQuads(QuadFieldQuery& qfq, float3 pos, float radius);
	void GetQuadsRectangle(QuadFieldQuery& qfq, const float3& mins, const float3& maxs);
	void GetQuadsOnRay(QuadFieldQuery& qfq, const float3& start, const float3& dir, float length);
	void GetQuadsOnWideRay(QuadFieldQuery& qfq, const float3& start, const float3& dir, float length, float width);

	void GetUnitsAndFeaturesColVol(
		const float3& pos,
		const float radius,
		std::vector<CUnit*>& units,
		std::vector<CFeature*>& features,
		std::vector<CPlasmaRepulser*>* repulsers = nullptr
	);

	/**
	 * Returns all units within @c radius of @c pos,
	 * and treats each unit as a 3D point object
	 */
	void GetUnits(QuadFieldQuery& qfq, const float3& pos, float radius);
	/**
	 * Returns all units within @c radius of @c pos,
	 * takes the 3D model radius of each unit into account,
 	 * and performs the search within a sphere or cylinder depending on @c spherical
	 */
	void GetUnitsExact(QuadFieldQuery& qfq, const float3& pos, float radius, bool spherical = true);
	/**
	 * Returns all units within the rectangle defined by
	 * mins and maxs, which extends infinitely along the y-axis
	 */
	void GetUnitsExact(QuadFieldQuery& qfq, const float3& mins, const float3& maxs);
	/**
	 * Returns all features within @c radius of @c pos,
	 * takes the 3D model radius of each feature into account,
	 * and performs the search within a sphere or cylinder depending on @c spherical
	 */
	void GetFeaturesExact(QuadFieldQuery& qfq, const float3& pos, float radius, bool spherical = true);
	/**
	 * Returns all features within the rectangle defined by
	 * mins and maxs, which extends infinitely along the y-axis
	 */
	void GetFeaturesExact(QuadFieldQuery& qfq, const float3& mins, const float3& maxs);

	void GetProjectilesExact(QuadFieldQuery& qfq, const float3& pos, float radius);
	void GetProjectilesExact(QuadFieldQuery& qfq, const float3& mins, const float3& maxs);

	void GetSolidsExact(
		QuadFieldQuery& qfq,
		const float3& pos,
		const float radius,
		const unsigned int physicalStateBits = 0xFFFFFFFF,
		const unsigned int collisionStateBits = 0xFFFFFFFF
	);

	bool NoSolidsExact(
		const float3& pos,
		const float radius,
		const unsigned int physicalStateBits = 0xFFFFFFFF,
		const unsigned int collisionStateBits = 0xFFFFFFFF
	);


	bool InsertUnitIf(CUnit* unit, const float3& wpos);
	bool RemoveUnitIf(CUnit* unit, const float3& wpos);

	void MovedUnit(CUnit* unit);
	void RemoveUnit(CUnit* unit);

	void AddFeature(CFeature* feature);
	void RemoveFeature(CFeature* feature);

	void MovedProjectile(CProjectile* projectile);
	void AddProjectile(CProjectile* projectile);
	void RemoveProjectile(CProjectile* projectile);

	void MovedRepulser(CPlasmaRepulser* repulser);
	void RemoveRepulser(CPlasmaRepulser* repulser);

	// Note: ensure ReleaseVector is called in the same thread as original quad field query generated.

	void ReleaseVector(std::vector<CUnit*>* v       , int onThread = 0) { tempUnits[onThread].ReleaseVector(v); }
	void ReleaseVector(std::vector<CFeature*>* v    , int onThread = 0) { tempFeatures[onThread].ReleaseVector(v); }
	void ReleaseVector(std::vector<CProjectile*>* v ) { tempProjectiles.ReleaseVector(v); }
	void ReleaseVector(std::vector<CSolidObject*>* v, int onThread = 0) { tempSolids[onThread].ReleaseVector(v); }
	void ReleaseVector(std::vector<int>* v          , int onThread = 0) { tempQuads[onThread].ReleaseVector(v); }

	struct Quad {
	public:
		CR_DECLARE_STRUCT(Quad)

		Quad() = default;
		Quad(const Quad& q) = delete;
		Quad(Quad&& q) { *this = std::move(q); }

		Quad& operator = (const Quad& q) = delete;
		Quad& operator = (Quad&& q) {
			units = std::move(q.units);
			teamUnits = std::move(q.teamUnits);
			features = std::move(q.features);
			projectiles = std::move(q.projectiles);
			repulsers = std::move(q.repulsers);
			return *this;
		}

		void PostLoad();
		void Resize(int numAllyTeams) { teamUnits.resize(numAllyTeams); }
		void Clear() {
			units.clear();
			// reuse inner vectors when reloading
			// teamUnits.clear();
			for (auto& v: teamUnits) {
				v.clear();
			}
			features.clear();
			projectiles.clear();
			repulsers.clear();
		}

	public:
		std::vector<CUnit*> units;
		std::vector< std::vector<CUnit*> > teamUnits;
		std::vector<CFeature*> features;
		std::vector<CProjectile*> projectiles;
		std::vector<CPlasmaRepulser*> repulsers;
	};

	const Quad& GetQuad(unsigned i) const {
		assert(i < baseQuads.size());
		return baseQuads[i];
	}
	const Quad& GetQuadAt(unsigned x, unsigned z) const {
		assert(unsigned(numQuadsX * z + x) < baseQuads.size());
		return baseQuads[numQuadsX * z + x];
	}


	int GetNumQuadsX() const { return numQuadsX; }
	int GetNumQuadsZ() const { return numQuadsZ; }

	int GetQuadSizeX() const { return quadSizeX; }
	int GetQuadSizeZ() const { return quadSizeZ; }

	constexpr static unsigned int BASE_QUAD_SIZE = 128;

private:
	int2 WorldPosToQuadField(const float3 p) const;
	int WorldPosToQuadFieldIdx(const float3 p) const;

private:
	std::vector<Quad> baseQuads;

	// preallocated vectors for Get*Exact functions
	std::array< QueryVectorCache<CUnit*>, ThreadPool::MAX_THREADS >  tempUnits;
	std::array< QueryVectorCache<CFeature*>, ThreadPool::MAX_THREADS >  tempFeatures;
	QueryVectorCache<CProjectile*> tempProjectiles;
	std::array< QueryVectorCache<CSolidObject*>, ThreadPool::MAX_THREADS > tempSolids;
	std::array< QueryVectorCache<int>, ThreadPool::MAX_THREADS > tempQuads;

	float2 invQuadSize;

	int numQuadsX;
	int numQuadsZ;

	int quadSizeX;
	int quadSizeZ;
};

extern CQuadField quadField;


struct QuadFieldQuery {
	~QuadFieldQuery() {
		quadField.ReleaseVector(units, threadOwner);
		quadField.ReleaseVector(features, threadOwner);
		quadField.ReleaseVector(projectiles);
		quadField.ReleaseVector(solids, threadOwner);
		quadField.ReleaseVector(quads, threadOwner);
	}

	std::vector<CUnit*>* units = nullptr;
	std::vector<CFeature*>* features = nullptr;
	std::vector<CProjectile*>* projectiles = nullptr;
	std::vector<CSolidObject*>* solids = nullptr;
	std::vector<int>* quads = nullptr;
	int threadOwner = 0;
};


#endif /* QUAD_FIELD_H */
