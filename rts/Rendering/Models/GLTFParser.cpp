/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include <vector>
#include <numeric>
#include <algorithm>
#include <optional>
#include <unordered_set>
#include <unordered_map>

#include "GLTFParser.h"
#include "3DModel.hpp"
#include "3DModelLog.h"
#include "ModelUtils.h"

#include "Lua/LuaParser.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Misc/TracyDefs.h"
#include "System/Exceptions.h"
#include "System/UnorderedSet.hpp"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/math.hpp>
#include <simdjson.h>


namespace Impl {
	Transform TRStoTransform(const fastgltf::TRS& trs) {
		assert(float3(trs.scale.x(), trs.scale.y(), trs.scale.z()) == float3(trs.scale.x()));
		return Transform{
			CQuaternion{ trs.rotation.x(), trs.rotation.y(), trs.rotation.z(), trs.rotation.w()},
			float3{ trs.translation.x(), trs.translation.y(), trs.translation.z() },
			trs.scale.x()
		};
	}
	Transform MatrixToTransform(const fastgltf::math::fmat4x4& matrix) {
		CMatrix44f mat;
		memcpy(&mat.m[0], matrix.data(), sizeof(CMatrix44f));
		auto [t, r, s] = mat.DecomposeIntoTRS();
		assert(s == float3(s));
		return Transform{ r, t, s.x };
	}

	Transform GetNodeTransform(const fastgltf::Node& node) {
		return fastgltf::visit_exhaustive(fastgltf::visitor{
			[](const fastgltf::math::fmat4x4& matrix) {
				return Impl::MatrixToTransform(matrix);
			},
			[](const fastgltf::TRS& trs) {
				return Impl::TRStoTransform(trs);
			}
		}, node.transform);
	}

	template<typename PrimContainer>
	void ReadGeometryData(const fastgltf::Asset& asset, const PrimContainer& primitives, std::vector<SVertexData>& verts, std::vector<uint32_t>& indcs, size_t nodeIdx, const fastgltf::Skin* skinPtr = nullptr) {
			for (const auto& prim : primitives) {
				const size_t prevVertSize = verts.size();
				const size_t prevIndcSize = indcs.size();

				bool seenNormal = false;
				bool seenTangents = false;
				bool seenTex2 = false;

				std::vector<std::array<std::pair<uint16_t, float>, 8>> vertexWeights;

				if (prim.type != fastgltf::PrimitiveType::Triangles) {
					throw content_error("A GLTF model has invalid primitive type " + std::to_string(static_cast<uint32_t>(prim.type)));
				}

				for (const auto* primAttIt = prim.attributes.cbegin(); primAttIt != prim.attributes.cend(); ++primAttIt) {
					auto& accessor = asset.accessors[primAttIt->accessorIndex];

					if (primAttIt == prim.attributes.cbegin()) {
						verts.resize(prevVertSize + accessor.count);
						vertexWeights.resize(accessor.count);
						for (auto& weights : vertexWeights) {
							weights.fill(std::make_pair(SVertexData::INVALID_BONEID, 0.0f));
						}
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
						seenNormal = true;
					} break;
					// note the UV.y is inverted!!!
					case hashString("TEXCOORD_0"): {
						fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, accessor, [&](const auto& val, std::size_t idx) {
							verts[prevVertSize + idx].texCoords[0] = float2(val.x(), 1.0f - val.y());
						});
					} break;
					case hashString("TEXCOORD_1"): {
						fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, accessor, [&](const auto& val, std::size_t idx) {
							verts[prevVertSize + idx].texCoords[1] = float2(val.x(), 1.0f - val.y());
						});
						seenTex2 = true;
					} break;
					case hashString("TANGENT"): {
						fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, accessor, [&](const auto& val, std::size_t idx) {
							verts[prevVertSize + idx].sTangent = float3{ val.x(), val.y(), val.z() }.ANormalize();
							verts[prevVertSize + idx].tTangent = val.w() * verts[prevVertSize + idx].normal.cross(verts[prevVertSize + idx].sTangent).ANormalize();
						});
						seenTangents = true;
					} break;
					case hashString("JOINTS_0"): {
						assert(skinPtr);
						fastgltf::iterateAccessorWithIndex<fastgltf::math::uvec4>(asset, accessor, [&](const auto& val, std::size_t idx) {
							auto& vertexWeight = vertexWeights[idx];
							vertexWeight[0].first = static_cast<uint16_t>(skinPtr->joints[val.x()]);
							vertexWeight[1].first = static_cast<uint16_t>(skinPtr->joints[val.y()]);
							vertexWeight[2].first = static_cast<uint16_t>(skinPtr->joints[val.z()]);
							vertexWeight[3].first = static_cast<uint16_t>(skinPtr->joints[val.w()]);
						});
					} break;
					case hashString("JOINTS_1"): {
						assert(skinPtr);
						fastgltf::iterateAccessorWithIndex<fastgltf::math::uvec4>(asset, accessor, [&](const auto& val, std::size_t idx) {
							auto& vertexWeight = vertexWeights[idx];
							vertexWeight[4].first = static_cast<uint16_t>(skinPtr->joints[val.x()]);
							vertexWeight[5].first = static_cast<uint16_t>(skinPtr->joints[val.y()]);
							vertexWeight[6].first = static_cast<uint16_t>(skinPtr->joints[val.z()]);
							vertexWeight[7].first = static_cast<uint16_t>(skinPtr->joints[val.w()]);
						});
					} break;
					case hashString("WEIGHTS_0"): {
						fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, accessor, [&](const auto& val, std::size_t idx) {
							auto& vertexWeight = vertexWeights[idx];
							vertexWeight[0].second = val.x();
							vertexWeight[1].second = val.y();
							vertexWeight[2].second = val.z();
							vertexWeight[3].second = val.w();
						});
					} break;
					case hashString("WEIGHTS_1"): {
						fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, accessor, [&](const auto& val, std::size_t idx) {
							auto& vertexWeight = vertexWeights[idx];
							vertexWeight[4].second = val.x();
							vertexWeight[5].second = val.y();
							vertexWeight[6].second = val.z();
							vertexWeight[7].second = val.w();
						});
					} break;
					default:
						break;
					}
				}

			for (auto& vertexWeight : vertexWeights) {
				for (auto& w : vertexWeight) {
					if (w.second == 0.0f)
						w.first = SVertexData::INVALID_BONEID;
				}

				std::stable_sort(vertexWeight.begin(), vertexWeight.end(), [](const auto& lhs, const auto& rhs) {
					return std::forward_as_tuple(lhs.second, lhs.first) > std::forward_as_tuple(rhs.second, rhs.first);
				});

				// non-skinning case
				if (auto& [boneId, boneWeight] = vertexWeight.front(); boneId == SVertexData::INVALID_BONEID) {
					boneId = nodeIdx;
					boneWeight = 1.0f;
				}
			}

			for (size_t i = prevVertSize; i < verts.size(); ++i) {
				verts[i].SetBones(vertexWeights[i - prevVertSize]);
			}

			assert(prim.indicesAccessor.has_value());
			auto& accessor = asset.accessors[*prim.indicesAccessor];
			indcs.resize(prevIndcSize + accessor.count);

			assert(accessor.bufferViewIndex.has_value());
			fastgltf::iterateAccessorWithIndex<uint32_t>(asset, accessor, [&](std::uint32_t index, std::size_t idx) {
				indcs[idx] = index;
			});

			// certain post-processing
			if (!seenTex2)
				std::for_each(verts.begin(), verts.end(), [](auto& vert) { vert.texCoords[1] = vert.texCoords[0]; });

			if (!seenNormal)
				ModelUtils::CalculateNormals(verts, indcs);

			if (!seenTangents)
				ModelUtils::CalculateTangents(verts, indcs);
		}
	}

	template<typename UM>
	void ReplaceNodeIndexWithPieceIndex(std::vector<SVertexData>& verts, const UM& nodeIdxToPieceIdx) {
		for (auto& vert : verts) {
			for (size_t wi = 0; wi < vert.boneIDsLow.size(); ++wi) {
				const auto nodeIdx = Skinning::GetBoneID(vert, wi);
				if (nodeIdx == INV_PIECE_NUM)
					continue;

				const auto pIt = nodeIdxToPieceIdx.find(nodeIdx);
				assert(pIt != nodeIdxToPieceIdx.end());
				vert.boneIDsLow [wi] = static_cast<uint8_t>((pIt->second >> 0) & 0xFF);
				vert.boneIDsHigh[wi] = static_cast<uint8_t>((pIt->second >> 8) & 0xFF);
			}
		}
	}

	template<typename UM>
	void TransformSkinsToModelSpace(std::vector<SVertexData>& verts, size_t nodeIdx, const UM& transforms) {
		auto it = transforms.find(nodeIdx);
		assert(it != transforms.end());

		// Skins are expected to be in the model space anyway,
		// So this whole function is mostly a precaution
		if likely(it->second.IsIdentity())
			return;

		for (auto& vert : verts) {
			vert.TransformBy(it->second);
		}
	}

	void ParseSceneExtra(simdjson::dom::object* extras, std::size_t objectIndex, fastgltf::Category objectType, void* userPointer) {
		if (objectType != fastgltf::Category::Scenes)
			return;

		if (!extras)
			return;

		auto ParseFloat3 = [](const simdjson::dom::element& value) -> std::optional<float3> {
			if (!value.is_array() || value.get_array().size() != 3)
				return std::nullopt;

			float3 f3{};
			for (size_t i = 0; i < 3; ++i) {
				f3[i] = static_cast<float>(value.get_array().at(i).get_double());
			}

			return std::make_optional(std::move(f3));
		};

		auto& optionalModelParams = *reinterpret_cast<ModelUtils::ModelParams*>(userPointer);
		for (const auto& [key, value] : *extras) {
			switch (hashStringLower(key.data(), key.size())) {
			case hashString("tex1"): {
				if (value.is_string())
					optionalModelParams.texs[0] = value;
			} break;
			case hashString("tex2"): {
				if (value.is_string())
					optionalModelParams.texs[1] = value;
			} break;
			case hashString("midpos"): {
				optionalModelParams.relMidPos = ParseFloat3(value);
			} break;
			case hashString("mins"): {
				optionalModelParams.mins = ParseFloat3(value);
			} break;
			case hashString("maxs"): {
				optionalModelParams.maxs = ParseFloat3(value);
			} break;
			case hashString("height"): {
				optionalModelParams.height = static_cast<float>(value.get_double());
			} break;
			case hashString("radius"): {
				optionalModelParams.radius = static_cast<float>(value.get_double());
			} break;
			case hashString("fliptextures"): {
				if (value.is_bool())
					optionalModelParams.flipTextures = value;
			} break;
			case hashString("invertteamcolor"): {
				if (value.is_bool())
					optionalModelParams.invertTeamColor = value;
			} break;
			case hashString("s3ocompat"): {
				if (value.is_bool())
					optionalModelParams.s3oCompat = value;
			} break;
			default: {
				/*NO-OP*/
			} break;
			}
		}
	}

	void FindTextures(S3DModel* model, const fastgltf::Asset& asset, const std::string& modelBaseName, const ModelUtils::ModelParams& optionalModelParams)
	{
		std::string fullPath;
		for (int i = 0; i < 2; ++i) {
			if (optionalModelParams.texs[i].has_value()) {
				fullPath = "unittextures/" + *optionalModelParams.texs[i];
				if (CFileHandler::FileExists(fullPath, SPRING_VFS_ZIP_FIRST)) {
					model->texs[i] = fullPath;
					continue;
				}
			}
		}

		// TODO parse asset?
		// TODO guess the texture?
	}

	auto GetModelTransforms(const fastgltf::Asset& asset, std::size_t sceneIndex, const Transform& sceneTransform = Transform{}) {
		auto& scene = asset.scenes[sceneIndex];

		spring::unordered_map<size_t, Transform> transforms(asset.nodes.size());

		auto function = [&](auto& self, size_t nodeIdx, const Transform& parentTransform) -> void {
			const auto& node = asset.nodes[nodeIdx];

			const auto& [it, noDup] = transforms.emplace(
				nodeIdx,
				parentTransform * GetNodeTransform(node)
			);
			assert(noDup);

			for (auto& child : node.children) {
				self(self, child, it->second);
			}
		};

		for (auto& node : scene.nodeIndices) {
			function(function, node, sceneTransform);
		}

		return transforms;
	}
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

	fastgltf::Parser parser;

	ModelUtils::ModelParams optionalModelParams;
	parser.setExtrasParseCallback(&Impl::ParseSceneExtra);
	parser.setUserPointer(&optionalModelParams);

	auto maybeGltf = parser.loadGltf(gltfFile.get(), modelPath, PARSER_OPTION, PARSER_CATEGORIES);
	if (auto error = maybeGltf.error(); error != fastgltf::Error::None) {
		throw content_error("Error loading GLTF file " + modelFilePath);
	}

	const auto& asset = maybeGltf.get();
	if (asset.scenes.empty()) {
		throw content_error("Error loading GLTF file " + modelFilePath);
	}

	//////// Lua metafile
	// load the lua metafile containing properties unique to Recoil models (must return a table)
	std::string metaFileName = modelFilePath + ".lua";

	// try again without the model file extension
	if (!CFileHandler::FileExists(metaFileName, SPRING_VFS_ZIP))
		metaFileName = modelPath + modelName + ".lua";

	std::optional<LuaParser> metaFileParser;
	LuaTable modelTable;

	if (!CFileHandler::FileExists(metaFileName, SPRING_VFS_ZIP)) {
		LOG_SL(LOG_SECTION_MODEL, L_INFO, "No meta-file found for the GLTF model '%s'.", metaFileName.c_str());
	} else {
		metaFileParser.emplace(metaFileName, SPRING_VFS_ZIP, SPRING_VFS_ZIP);
		if (!metaFileParser->Execute())
			throw content_error("Failed to load " + metaFileName + ": " + metaFileParser->GetErrorLog());

		modelTable = metaFileParser->GetRoot();
	}

	if (!modelTable.IsValid()) {
		LOG_SL(LOG_SECTION_MODEL, L_INFO, "No valid model metadata in '%s' or no meta-file", metaFileName.c_str());
	}

	// optionalModelParams will contain all non-empty data from the modelTable
	ModelUtils::GetModelParams(modelTable, optionalModelParams);

	// Load textures
	Impl::FindTextures(&model, asset, modelName, optionalModelParams);
	LOG_SL(LOG_SECTION_MODEL, L_INFO, "Loading textures. Tex1: '%s' Tex2: '%s'", model.texs[0].c_str(), model.texs[1].c_str());

	textureHandlerS3O.PreloadTexture(
		&model,
		optionalModelParams.flipTextures.value_or(false),
		optionalModelParams.invertTeamColor.value_or(false)
	);

	model.name = modelFilePath;
	model.type = MODELTYPE_ASS; // Revise?
	model.numPieces = 0;
	model.mins = DEF_MIN_SIZE;
	model.maxs = DEF_MAX_SIZE;

	// GLTF model MUST be exported with Z axis UP. We will rotate it here by ourselves
	const auto initTransform = (optionalModelParams.s3oCompat.value_or(false)) ?
		Transform(CQuaternion(0, -math::HALFSQRT2, -math::HALFSQRT2, 0)): // Rotate so xyz ==> (-x,z, y)
		Transform(CQuaternion( math::HALFSQRT2, 0, 0, -math::HALFSQRT2)); // Rotate so xyz ==> ( x,z,-y)

	const auto defaultSceneIdx = asset.defaultScene.value_or(0);

	auto* rootPiece = AllocRootEmptyPiece(&model, initTransform, asset, defaultSceneIdx);
	model.FlattenPieceTree(rootPiece);

	spring::unordered_map<size_t, size_t> nodeIdxToPieceIdx;
	for (size_t pi = 0; pi < model.pieceObjects.size(); ++pi) {
		const auto* piece = static_cast<const GLTFPiece*>(model.pieceObjects[pi]);
		if (piece->nodeIndex == GLTFPiece::INVALID_NODE_INDEX)
			continue;

		nodeIdxToPieceIdx[piece->nodeIndex] = pi;
	}

	// conceptually almost the same as AllocRootEmptyPiece + SetPieceMatrices
	// except this one doesn't ignore nodes with skinned meshes
	const auto modelTransforms = Impl::GetModelTransforms(asset, defaultSceneIdx, initTransform);

	std::vector<Skinning::SkinnedMesh> allSkinnedMeshes;

	for (size_t ni = 0; ni < asset.nodes.size(); ++ni) {
		const auto& node = asset.nodes[ni];
		if (!node.meshIndex.has_value())
			continue;

		if (!node.skinIndex.has_value())
			continue;

		const auto& skin = asset.skins[*node.skinIndex];
		const auto& mesh = asset.meshes[*node.meshIndex];
		auto& skinnedMesh = allSkinnedMeshes.emplace_back();

		Impl::ReadGeometryData(asset, mesh.primitives, skinnedMesh.verts, skinnedMesh.indcs, ni, &skin);
		Impl::TransformSkinsToModelSpace(skinnedMesh.verts, ni, modelTransforms);
		Impl::ReplaceNodeIndexWithPieceIndex(skinnedMesh.verts, nodeIdxToPieceIdx);
	}

	// non-skinning case
	if (allSkinnedMeshes.empty()) {
		for (size_t pi = 0; pi < model.pieceObjects.size(); ++pi) {
			auto* piece = static_cast<GLTFPiece*>(model.pieceObjects[pi]);
			if (piece->nodeIndex == GLTFPiece::INVALID_NODE_INDEX)
				continue;

			if (piece->GetVerticesVec().empty())
				continue;

			Impl::ReplaceNodeIndexWithPieceIndex(piece->GetVerticesVec(), nodeIdxToPieceIdx);
		}
	}

	spring::unordered_set<size_t> allBones;
	for (const auto& skin : asset.skins) {
		for (size_t ji = 0; ji < skin.joints.size(); ++ji) {
			allBones.emplace(skin.joints[ji]);
		}
	}

	if (!allSkinnedMeshes.empty()) {
		// Skinning::<> code below needs correct bposeTransforms
		model.SetPieceMatrices();

		// if numMeshes >= numBones reparent the whole meshes
		// else reparent meshes per-triangle
		if (allSkinnedMeshes.size() >= allBones.size())
			Skinning::ReparentCompleteMeshesToBones(&model, allSkinnedMeshes);
		else
			Skinning::ReparentMeshesTrianglesToBones(&model, allSkinnedMeshes);
	}

	// will also calculate pieces / model bounding box
	ModelUtils::ApplyModelProperties(&model, optionalModelParams);

	ModelLog::LogModelProperties(model);
}

GLTFPiece* CGLTFParser::AllocPiece()
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

GLTFPiece* CGLTFParser::AllocRootEmptyPiece(S3DModel* model, const Transform& parentTransform, const fastgltf::Asset& asset, size_t sceneIndex)
{
	const auto& scene = asset.scenes[sceneIndex];

	auto* piece = AllocPiece();
	model->numPieces++;

	piece->SetParentModel(model);

	auto bakedTransform = parentTransform;
	// only rotation is allowed because of Spring-isms
	bakedTransform.t = float3{};
	bakedTransform.s = 1.0f;
	piece->SetBakedTransform(bakedTransform); // bakedRotAngles are not read or supported for GLTF
	piece->offset = parentTransform.t;
	piece->scale = parentTransform.s;


	piece->parent = nullptr;
	piece->name = scene.name;
	piece->children.reserve(scene.nodeIndices.size());

	for (const auto childNodeIndex : scene.nodeIndices) {
		auto* childPiece = LoadPiece(model, piece, asset, childNodeIndex);
		if (childPiece)
			piece->children.push_back(childPiece);
	}

	return piece;
}


GLTFPiece* CGLTFParser::LoadPiece(S3DModel* model, GLTFPiece* parentPiece, const fastgltf::Asset& asset, size_t nodeIndex)
{
	const auto& node = asset.nodes[nodeIndex];

	// skip skinned meshes (handled separately)
	if (node.skinIndex.has_value())
		return nullptr;

	auto* piece = AllocPiece();
	model->numPieces++;

	piece->SetParentModel(model);
	piece->parent = parentPiece;
	piece->name = node.name;
	piece->children.reserve(node.children.size());
	piece->nodeIndex = nodeIndex;

	Transform pieceTransform = Impl::GetNodeTransform(node);
	
	auto bakedTransform = pieceTransform;
	// by idiotic Spring convention bakedTransform should only contain rotation
	bakedTransform.t = float3{};
	bakedTransform.s = 1.0f;

	piece->SetBakedTransform(bakedTransform); // bakedRotAngles are not read or supported for GLTF
	piece->offset = pieceTransform.t;
	piece->scale = pieceTransform.s;

	for (const auto childNodeIndex : node.children) {
		auto* childPiece = LoadPiece(model, piece, asset, childNodeIndex);
		if (childPiece)
			piece->children.push_back(childPiece);
	}

	if (!node.meshIndex.has_value())
		return piece;

	auto& verts = piece->GetVerticesVec();
	auto& indcs = piece->GetIndicesVec();
	const auto& mesh = asset.meshes[*node.meshIndex];
	Impl::ReadGeometryData(asset, mesh.primitives, verts, indcs, nodeIndex, nullptr);

	return piece;
}