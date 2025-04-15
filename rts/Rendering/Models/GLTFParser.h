/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>
#include <cstdint>

#include "3DModel.h"
#include "IModelParser.h"

namespace fastgltf {
	class Asset;
}

class CGLTFParser: public IModelParser
{
public:
	void Init() override;
	void Kill() override;

	void Load(S3DModel& model, const std::string& name) override;
private:
	S3DModelPiece* AllocPiece();
	S3DModelPiece* LoadPiece(S3DModel* model, S3DModelPiece* parentPiece, const fastgltf::Asset& asset, size_t nodeIndex);

	std::vector<S3DModelPiece> piecePool;
	spring::mutex poolMutex;

	uint32_t numPoolPieces = 0;
};
