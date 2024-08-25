#pragma once

#include "ParticleGenerator.h"

struct GeoSquareParticleData {
	CR_DECLARE_STRUCT(GeoSquareParticleData)

	float3 p1;
	float w1;

	float3 p2;
	float w2;

	float3 v1;
	int32_t drawOrder;

	float3 v2;
	SColor color;

	AtlasedTexture texCoord;

	int32_t GetMaxNumQuads() const { return 1 * (texCoord != AtlasedTexture::DefaultAtlasTexture); }
	void Invalidate() {
		texCoord = AtlasedTexture::DefaultAtlasTexture;
	}
};
static_assert(sizeof(GeoSquareParticleData) % 16 == 0);

class GeoSquareParticleGenerator : public ParticleGenerator<GeoSquareParticleData, GeoSquareParticleGenerator> {
	friend class ParticleGenerator<GeoSquareParticleData, GeoSquareParticleGenerator>;
public:
	GeoSquareParticleGenerator() {}
	~GeoSquareParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};
