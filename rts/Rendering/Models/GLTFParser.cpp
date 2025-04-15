/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "GLTFParser.h"
#include "3DModel.h"
#include "3DModelLog.h"

#include "Rendering/Textures/S3OTextureHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Misc/TracyDefs.h"
#include "System/Exceptions.h"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/math.hpp>

void CGLTFParser::Init()
{
}

void CGLTFParser::Kill()
{
}

void CGLTFParser::Load(S3DModel& model, const std::string& modelFilePath)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LOG_SL(LOG_SECTION_MODEL, L_INFO, "Loading model: %s", modelFilePath.c_str());

	const std::string modelPath = FileSystem::GetDirectory(modelFilePath);
	const std::string modelName = FileSystem::GetBasename(modelFilePath);

	CFileHandler file(modelFilePath, SPRING_VFS_ZIP);
	std::vector<uint8_t> fileBuf;
	if (!file.IsBuffered()) {
		const auto fs = file.FileSize();
		if (fs <= 0)
			throw content_error("An assimp model has invalid size of " + std::to_string(fs));

		fileBuf.resize(fs, 0);
		file.Read(fileBuf.data(), fileBuf.size());
	}
	else {
		fileBuf = std::move(file.GetBuffer());
	}

	auto gltfFile = fastgltf::GltfDataBuffer::FromBytes(reinterpret_cast<std::byte*>(fileBuf.data()), fileBuf.size());
	if (gltfFile.error() != fastgltf::Error::None) {
		// The file couldn't be loaded, or the buffer could not be allocated.
		throw content_error("Error loading GLTF file " + modelFilePath);
	}

	fastgltf::Parser parser;

	static constexpr auto PARSER_OPTION =
		fastgltf::Options::DontRequireValidAssetMember | 
		//fastgltf::Options::DecomposeNodeMatrices | // most likely sync unsafe, however this doesn't mean the transformation type is always matrix
		fastgltf::Options::GenerateMeshIndices;

	static constexpr auto PARSER_CATEGORIES =
		fastgltf::Category::Buffers |
		fastgltf::Category::BufferViews |
		fastgltf::Category::Accessors |
		fastgltf::Category::Images | //?
		fastgltf::Category::Animations |
		fastgltf::Category::Meshes |
		fastgltf::Category::Skins |
		fastgltf::Category::Nodes |
		fastgltf::Category::Scenes |
		fastgltf::Category::Asset;

	auto maybeGltf = parser.loadGltf(gltfFile.get(), modelPath, PARSER_OPTION, PARSER_CATEGORIES);
	if (auto error = maybeGltf.error(); error != fastgltf::Error::None) {
		throw content_error("Error loading GLTF file " + modelFilePath);
	}

	const auto& gltf = maybeGltf.get();

	if (gltf.scenes.empty()) {
		throw content_error("Error loading GLTF file " + modelFilePath);
	}

	model.name = modelFilePath;
	model.type = MODELTYPE_ASS; // Revise?
	model.numPieces = 0;
	model.texs[0] = "";//(header.texture1 == 0) ? "" : (char*)&fileBuf[header.texture1];
	model.texs[1] = "";//(header.texture2 == 0) ? "" : (char*)&fileBuf[header.texture2];
	model.mins = DEF_MIN_SIZE;
	model.maxs = DEF_MAX_SIZE;

	//textureHandlerS3O.PreloadTexture(&model);
	
	const auto defaultSceneIdx = gltf.defaultScene.value_or(0);
	const auto& defaultScene = gltf.scenes[defaultSceneIdx];
	model.FlattenPieceTree(LoadPiece(&model, nullptr, gltf, defaultScene.nodeIndices.front()));

	// set after the extrema are known
	//model.radius = (header.radius <= 0.01f) ? model.CalcDrawRadius() : header.radius;
	//model.height = (header.height <= 0.01f) ? model.CalcDrawHeight() : header.height;
	//model.relMidPos = float3(header.midx, header.midy, header.midz);

	fastgltf::iterateSceneNodes(gltf, 0, fastgltf::math::fmat4x4(),
		[&gltf, &model, this](const auto& node, const auto& mat) {
			auto* piece = AllocPiece();
			model.numPieces++;

			piece->SetParentModel(&model);
			piece->name = node.name;
			piece->children.reserve(node.children.size());

			if (!node.meshIndex.has_value())
				return;

			const auto& mesh = gltf.meshes[*node.meshIndex];

			for (const auto& prim: mesh.primitives) {
				for (const auto* primAttIt = prim.attributes.cbegin(); primAttIt != prim.attributes.cend(); ++primAttIt) {
					auto& accessor = gltf.accessors[primAttIt->accessorIndex];
					if (!accessor.bufferViewIndex.has_value())
						continue;
					/*
					fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(gltf, positionAccessor, [&](fastgltf::math::fvec3 pos, std::size_t idx) {
						vertices[idx].position = fastgltf::math::fvec3(pos.x(), pos.y(), pos.z());
						vertices[idx].uv = fastgltf::math::fvec2();
					});
					fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(gltf, texCoordAccessor, [&](fastgltf::math::fvec2 uv, std::size_t idx) {
						vertices[idx].uv = fastgltf::math::fvec2(uv.x(), uv.y());
					});
					*/

					switch (hashString(primAttIt->name.c_str(), primAttIt->name.length()))
					{
					case hashString("POSITION"):
						//resize accessor.count
						LOG("");
						break;
					case hashString("NORMAL"):
						LOG("");
						break;
					case hashString("TEXCOORD_0"):
						LOG("");
						break;
					case hashString("TEXCOORD_1"):
						LOG("");
						break;
					case hashString("TANGENT"):
						LOG("");
						break;
					default:
						break;
					}
				}

				assert(prim.indicesAccessor.has_value()); // We specify GenerateMeshIndices, so we should always have indices
			}
			LOG("node %s", node.name.c_str());
			//drawMesh(&viewer, *node.meshIndex, matrix);
	});

	LOG("Finished");
}

S3DModelPiece* CGLTFParser::AllocPiece()
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::lock_guard<spring::mutex> lock(poolMutex);

	// lazily reserve pool here instead of during Init
	// this way games using only one model-type do not
	// cause redundant allocation
	if (piecePool.empty())
		piecePool.resize(MAX_MODEL_OBJECTS * AVG_MODEL_PIECES);

	if (numPoolPieces >= piecePool.size()) {
		throw std::bad_alloc();
		return nullptr;
	}

	return &piecePool[numPoolPieces++];
}

S3DModelPiece* CGLTFParser::LoadPiece(S3DModel* model, S3DModelPiece* parentPiece, const fastgltf::Asset& asset, size_t nodeIndex)
{
	const auto& node = asset.nodes[nodeIndex];

	//fastgltf::math::fmat4x4& parentMatrix = parentPiece ? parentPiece->bposeMatrix : 

	auto* piece = AllocPiece();
	model->numPieces++;

	piece->SetParentModel(model);
	piece->parent = parentPiece;
	piece->name = node.name;
	piece->children.reserve(node.children.size());

	//nodeMatrix = getTransformMatrix(node, nodeMatrix);

	for (const auto childNodeIndex : node.children) {
		auto* childPiece = LoadPiece(model, piece, asset, childNodeIndex);
		piece->children.push_back(childPiece);
	}

	if (!node.meshIndex.has_value())
		return piece;

	auto& verts = piece->GetVerticesVec();
	auto& indcs = piece->GetIndicesVec();
	const auto& mesh = asset.meshes[*node.meshIndex];
	for (const auto& prim : mesh.primitives) {
		const size_t prevVertSize = verts.size();
		const size_t prevIndcSize = indcs.size();
		assert(prim.type == fastgltf::PrimitiveType::Triangles);
		for (const auto* primAttIt = prim.attributes.cbegin(); primAttIt != prim.attributes.cend(); ++primAttIt) {
			auto& accessor = asset.accessors[primAttIt->accessorIndex];

			if (primAttIt == prim.attributes.cbegin()) {
				verts.resize(prevVertSize + accessor.count);
			}

			if (!accessor.bufferViewIndex.has_value())
				continue;

			switch (hashString(primAttIt->name.c_str(), primAttIt->name.length()))
			{
			case hashString("POSITION"): {
				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, accessor, [&](const auto& val, std::size_t idx) {
					verts[prevVertSize + idx].pos = float3{ val.x(), val.y(), val.z() };
				});
			} break;
			case hashString("NORMAL"): {
				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, accessor, [&](const auto& val, std::size_t idx) {
					verts[prevVertSize + idx].normal = float3{ val.x(), val.y(), val.z() }.ANormalize();
				});
			} break;
			case hashString("TEXCOORD_0"): {
				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, accessor, [&](const auto& val, std::size_t idx) {
					verts[prevVertSize + idx].texCoords[0] = float2(val.x(), val.y());
				});
			} break;
			case hashString("TEXCOORD_1"): {
				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, accessor, [&](const auto& val, std::size_t idx) {
					verts[prevVertSize + idx].texCoords[1] = float2(val.x(), val.y());
				});
			} break;
			case hashString("TANGENT"): {
				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, accessor, [&](const auto& val, std::size_t idx) {
					verts[prevVertSize + idx].sTangent = (val.w() * float3{ val.x(), val.y(), val.z() }).ANormalize();
					verts[prevVertSize + idx].tTangent = verts[prevVertSize + idx].normal.cross(verts[prevVertSize + idx].sTangent).ANormalize();
				});
			} break;
			default:
				break;
			}
		}
		assert(prim.indicesAccessor.has_value());
		auto& accessor = asset.accessors[*prim.indicesAccessor];
		indcs.resize(prevIndcSize + accessor.count);

		assert(accessor.bufferViewIndex.has_value());
		fastgltf::iterateAccessorWithIndex<std::uint32_t>(asset, accessor, [&](std::uint32_t index, std::size_t idx) {
			indcs[idx] = index;
		});
	}
	LOG("");

	return piece;
}
