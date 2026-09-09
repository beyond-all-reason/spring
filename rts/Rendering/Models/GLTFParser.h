/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>
#include <cstdint>

#include "3DModelPiece.hpp"
#include "IModelParser.h"

#include "System/Threading/SpringThreading.h"

namespace fastgltf {
	class Asset;
}

namespace gltfmodel {
	enum class SourceConvention;
}

struct GLTFPiece : public S3DModelPiece {
	static constexpr size_t INVALID_NODE_INDEX = size_t(-1);
	size_t nodeIndex = INVALID_NODE_INDEX;
};

class CGLTFParser: public IModelParser
{
public:
	void Init() override {};
	void Kill() override {};

	void Load(S3DModel& model, const std::string& name) override;
private:
	GLTFPiece* AllocPiece();
	GLTFPiece* AllocRootEmptyPiece(
		S3DModel* model,
		const fastgltf::Asset& asset,
		size_t sceneIndex,
		gltfmodel::SourceConvention sourceConvention
	);
	GLTFPiece* LoadPiece(
		S3DModel* model,
		GLTFPiece* parentPiece,
		const fastgltf::Asset& asset,
		size_t nodeIndex,
		gltfmodel::SourceConvention sourceConvention
	);

	std::vector<GLTFPiece> piecePool;
	spring::mutex poolMutex;

	uint32_t numPoolPieces = 0;
};
