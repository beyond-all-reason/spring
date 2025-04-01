/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AssParser.h"
#include "3DModel.h"
#include "3DModelLog.h"
#include "AssIO.h"

#include "Lua/LuaParser.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "System/SpringMath.h"
#include "System/ScopedFPUSettings.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"

#include "lib/assimp/include/assimp/config.h"
#include "lib/assimp/include/assimp/defs.h"
#include "lib/assimp/include/assimp/types.h"
#include "lib/assimp/include/assimp/scene.h"
#include "lib/assimp/include/assimp/postprocess.h"
#include "lib/assimp/include/assimp/Importer.hpp"
#include "lib/assimp/include/assimp/DefaultLogger.hpp"

#include <regex>
#include <algorithm>
#include <numeric>

#include "System/Misc/TracyDefs.h"


#define IS_QNAN(f) (f != f)

// triangulate guarantees the most complex mesh is a triangle
// sortbytype ensure only 1 type of primitive type per mesh is used
static constexpr unsigned int ASS_POSTPROCESS_OPTIONS =
	  aiProcess_RemoveComponent
	| aiProcess_FindInvalidData
	| aiProcess_CalcTangentSpace
	| aiProcess_GenSmoothNormals
	| aiProcess_Triangulate
	| aiProcess_GenUVCoords
	| aiProcess_SortByPType
	| aiProcess_JoinIdenticalVertices
	//| aiProcess_ImproveCacheLocality // FIXME crashes in an assert in VertexTriangleAdjancency.h (date 04/2011)
	| aiProcess_LimitBoneWeights
	| aiProcess_SplitLargeMeshes
	;

static constexpr unsigned int ASS_IMPORTER_OPTIONS =
	  aiComponent_CAMERAS
	| aiComponent_LIGHTS
	| aiComponent_TEXTURES
	| aiComponent_ANIMATIONS
	| aiComponent_MATERIALS
	;
static constexpr unsigned int ASS_LOGGING_OPTIONS =
	  Assimp::Logger::Debugging
	| Assimp::Logger::Info
	| Assimp::Logger::Err
	| Assimp::Logger::Warn
	;



static inline float3 aiVectorToFloat3(const aiVector3D v)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// no-op; AssImp's internal coordinate-system matches Spring's modulo handedness
	return {v.x, v.y, v.z};

	// Blender --> Spring
	// return float3(v.x, v.z, -v.y);
}

static inline CMatrix44f aiMatrixToMatrix(const aiMatrix4x4t<float>& m)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CMatrix44f n;

	n[ 0] = m.a1; n[ 1] = m.a2; n[ 2] = m.a3; n[ 3] = m.a4; // 1st column
	n[ 4] = m.b1; n[ 5] = m.b2; n[ 6] = m.b3; n[ 7] = m.b4; // 2nd column
	n[ 8] = m.c1; n[ 9] = m.c2; n[10] = m.c3; n[11] = m.c4; // 3rd column
	n[12] = m.d1; n[13] = m.d2; n[14] = m.d3; n[15] = m.d4; // 4th column

	// AssImp (row-major, RH) --> Spring (column-major, LH)
	return (n.Transpose());

	// Blender --> Spring
	// return (CMatrix44f(n.GetPos(), n.GetX(), n.GetZ(), -n.GetY()));
}

/*
static float3 aiQuaternionToRadianAngles(const aiQuaternion q1)
{
	const float sqw = q1.w * q1.w;
	const float sqx = q1.x * q1.x;
	const float sqy = q1.y * q1.y;
	const float sqz = q1.z * q1.z;
	// <unit> is 1 if normalised, otherwise correction factor
	const float unit = sqx + sqy + sqz + sqw;
	const float test = q1.x * q1.y + q1.z * q1.w;

	aiVector3D angles;

	if (test > (0.499f * unit)) {
		// singularity at north pole
		angles.x = 2.0f * math::atan2(q1.x, q1.w);
		angles.y = PI * 0.5f;
	} else if (test < (-0.499f * unit)) {
		// singularity at south pole
		angles.x = -2.0f * math::atan2(q1.x, q1.w);
		angles.y = -PI * 0.5f;
	} else {
		angles.x = math::atan2(2.0f * q1.y * q1.w - 2.0f * q1.x * q1.z,  sqx - sqy - sqz + sqw);
		angles.y = math::asin((2.0f * test) / unit);
		angles.z = math::atan2(2.0f * q1.x * q1.w - 2.0f * q1.y * q1.z, -sqx + sqy - sqz + sqw);
	}

	return (aiVectorToFloat3(angles));
}
*/




class AssLogStream : public Assimp::LogStream
{
public:
	void write(const char* message) override {
		LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "Assimp: %s", message);
	}
};



struct SPseudoAssPiece {
	std::string name;

	S3DModelPiece* parent;

	Transform bposeTransform;    /// bind-pose transform, including baked rots
	Transform bakedTransform;    /// baked local-space rotations

	float3 offset;     /// local (piece-space) offset wrt. parent piece
	float3 goffset;    /// global (model-space) offset wrt. root piece
	float scale{1.0f}; /// baked uniform scaling factor (assimp-only)

	bool hasBakedTra;

	// copy of S3DModelPiece::SetBakedTransform()
	void SetBakedTransform(const Transform& tra) {
		hasBakedTra = !tra.IsIdentity();
	}

	// copy of S3DModelPiece::ComposeTransform(), unused?
	Transform ComposeTransform(const float3& t, const float3& r, float s) const;

	// copy of S3DModelPiece::SetPieceTransform()
	// except there's no need to do it recursively
	void SetPieceTransform(const Transform& tra);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Impl {
	template<typename PieceObject>
	void LoadPieceTransformations(
		PieceObject* piece,
		const S3DModel* model,
		const aiNode* pieceNode,
		const LuaTable& pieceTable
	) {
		RECOIL_DETAILED_TRACY_ZONE;
		aiVector3D aiScaleVec;
		aiVector3D aiTransVec;
		aiQuaternion aiRotateQuat;

		// process transforms
		pieceNode->mTransformation.Decompose(aiScaleVec, aiRotateQuat, aiTransVec);

		// TODO remove bakedMatrix and do everything with basic transformations
		const aiMatrix3x3t<float> aiBakedRotMatrix = aiRotateQuat.GetMatrix();
		const aiMatrix4x4t<float> aiBakedMatrix = aiMatrix4x4t<float>(aiBakedRotMatrix);
		CMatrix44f bakedMatrix = aiMatrixToMatrix(aiBakedMatrix);

		// metadata-scaling
		float3 scales{ 1.0f, 1.0f, 1.0f };
		scales = pieceTable.GetFloat3("scale", aiVectorToFloat3(aiScaleVec));
		scales.x = pieceTable.GetFloat("scalex", scales.x);
		scales.y = pieceTable.GetFloat("scaley", scales.y);
		scales.z = pieceTable.GetFloat("scalez", scales.z);

		if (!epscmp(scales.x, scales.y, std::max(scales.x, scales.y) * float3::cmp_eps()) ||
			!epscmp(scales.y, scales.z, std::max(scales.y, scales.z) * float3::cmp_eps()) ||
			!epscmp(scales.z, scales.x, std::max(scales.z, scales.x) * float3::cmp_eps()))
		{
			LOG_SL(LOG_SECTION_MODEL, L_WARNING, "Recoil doesn't support non-uniform scaling");
		}
		piece->scale = scales.x;

		// metadata-translation
		piece->offset = pieceTable.GetFloat3("offset", aiVectorToFloat3(aiTransVec));
		piece->offset.x = pieceTable.GetFloat("offsetx", piece->offset.x);
		piece->offset.y = pieceTable.GetFloat("offsety", piece->offset.y);
		piece->offset.z = pieceTable.GetFloat("offsetz", piece->offset.z);

		// metadata-rotation
		// NOTE:
		//   these rotations are "pre-scripting" but "post-modelling"
		//   together with the (baked) aiRotateQuad they determine the
		//   model's pose *before* any animations execute
		//
		// float3 bakedRotAngles = pieceTable.GetFloat3("rotate", aiQuaternionToRadianAngles(aiRotateQuat) * math::RAD_TO_DEG);
		float3 bakedRotAngles = pieceTable.GetFloat3("rotate", ZeroVector);

		bakedRotAngles.x = pieceTable.GetFloat("rotatex", bakedRotAngles.x);
		bakedRotAngles.y = pieceTable.GetFloat("rotatey", bakedRotAngles.y);
		bakedRotAngles.z = pieceTable.GetFloat("rotatez", bakedRotAngles.z);
		bakedRotAngles *= math::DEG_TO_RAD;

		LOG_SL(LOG_SECTION_PIECE, L_INFO,
			"(%d:%s) Assimp offset (%f,%f,%f), rotate (%f,%f,%f,%f), scale (%f,%f,%f)",
			model->numPieces, piece->name.c_str(),
			aiTransVec.x, aiTransVec.y, aiTransVec.z,
			aiRotateQuat.w, aiRotateQuat.x, aiRotateQuat.y, aiRotateQuat.z,
			aiScaleVec.x, aiScaleVec.y, aiScaleVec.z
		);
		LOG_SL(LOG_SECTION_PIECE, L_INFO,
			"(%d:%s) Relative offset (%f,%f,%f), rotate (%f,%f,%f), scale (%f)",
			model->numPieces, piece->name.c_str(),
			piece->offset.x, piece->offset.y, piece->offset.z,
			bakedRotAngles.x, bakedRotAngles.y, bakedRotAngles.z,
			piece->scale
		);

		// construct 'baked' piece-space transform
		//
		// AssImp order is Translate * Rotate * Scale * v; the
		// translation and scale parts are split into <offset>
		// and <scales> so the baked part reduces to R
		//
		// note: for all non-AssImp models this is identity!

		Transform bakedTransform(CQuaternion::FromEulerYPRNeg(-bakedRotAngles) * CQuaternion(aiRotateQuat.x, aiRotateQuat.y, aiRotateQuat.z, aiRotateQuat.w), ZeroVector, 1.0f);
		piece->SetBakedTransform(bakedTransform);
	}

	std::vector<std::string> GetBoneNames(const aiScene* scene)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		std::vector<std::string> boneNames;
		for (size_t m = 0; m < scene->mNumMeshes; ++m) {
			for (size_t b = 0; b < scene->mMeshes[m]->mNumBones; ++b) {
				std::string boneName(scene->mMeshes[m]->mBones[b]->mName.data);
				auto it = std::find(boneNames.begin(), boneNames.end(), boneName);
				if (it == boneNames.end())
					boneNames.emplace_back(boneName);
			}
		}

		return boneNames;
	}

	std::vector<std::string> GetMeshNames(const aiScene* scene)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		std::vector<std::string> meshNames;
		for (uint32_t m = 0; m < scene->mNumMeshes; ++m) {
			meshNames.emplace_back(scene->mMeshes[m]->mName.data);
		}

		return meshNames;
	}

	aiNode* FindNode(const aiScene* scene, aiNode* node, const std::string& name)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		if (std::string(node->mName.C_Str()) == name)
			return node;

		for (uint32_t ci = 0; ci < node->mNumChildren; ++ci) {
			auto* childTargetNode = FindNode(scene, node->mChildren[ci], name);
			if (childTargetNode)
				return childTargetNode;
		}

		return nullptr;
	}

	aiNode* FindFallbackNode(const aiScene* scene)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		for (uint32_t ci = 0; ci < scene->mRootNode->mNumChildren; ++ci) {
			if (scene->mRootNode->mChildren[ci]->mNumChildren == 0) {
				return scene->mRootNode->mChildren[ci];
			}
		}

		return nullptr;
	}

	std::vector<Transform> GetMeshBoneTransforms(const aiScene* scene, const S3DModel* model, std::vector<SPseudoAssPiece>& meshPPs)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		std::vector<Transform> meshBoneTransform;

		for (auto& meshPP : meshPPs) {
			meshPP.SetPieceTransform(meshPP.parent->bposeTransform);
			meshBoneTransform.emplace_back(meshPP.bposeTransform);
		}

		return meshBoneTransform;
	}

	std::vector<CAssParser::MeshData> GetModelSpaceMeshes(const aiScene* scene, const S3DModel* model, const std::vector<Transform>& meshBoneTransforms)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		std::vector<uint32_t> meshVertexMapping;
		std::vector<CAssParser::MeshData> meshes;

		for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
			const aiMesh* mesh = scene->mMeshes[meshIndex];

			const auto& boneTra = meshBoneTransforms[meshIndex];

			std::vector<SVertexData> verts;
			std::vector<uint32_t   > indcs;
			uint32_t numUVs = 0;

			LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Fetching mesh %d from scene", meshIndex);
			LOG_SL(LOG_SECTION_PIECE, L_DEBUG,
				"Processing vertices for mesh %d (%d vertices)",
				meshIndex, mesh->mNumVertices);
			LOG_SL(LOG_SECTION_PIECE, L_DEBUG,
				"Normals: %s Tangents/Bitangents: %s TexCoords: %s",
				(mesh->HasNormals() ? "Y" : "N"),
				(mesh->HasTangentsAndBitangents() ? "Y" : "N"),
				(mesh->HasTextureCoords(0) ? "Y" : "N"));

			verts.reserve(mesh->mNumVertices);
			indcs.reserve(mesh->mNumFaces * 3);

			meshVertexMapping.clear();
			meshVertexMapping.reserve(mesh->mNumVertices);

			//bones info
			std::vector<std::vector<std::pair<uint16_t, float>>> vertexWeights(mesh->mNumVertices);

			for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++) {
				const aiBone* bone = mesh->mBones[boneIndex];
				for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; weightIndex++) {
					const auto& vertIndex = bone->mWeights[weightIndex].mVertexId;
					const auto& vertWeight = bone->mWeights[weightIndex].mWeight;
					const std::string boneName = std::string(bone->mName.data);

					auto boneID = spring::SafeCast<uint16_t>(model->FindPieceOffset(boneName));
					assert(boneID < INV_PIECE_NUM); // == INV_PIECE_NUM - invalid piece

					vertexWeights[vertIndex].emplace_back(boneID, vertWeight);
				}
			}

			for (auto& vertexWeight : vertexWeights) {
				std::stable_sort(vertexWeight.begin(), vertexWeight.end(), [](auto&& lhs, auto&& rhs) {
					if (lhs.second > rhs.second) return true;
					if (rhs.second > lhs.second) return false;

					if (lhs.first > rhs.first) return true;
					if (rhs.first > lhs.first) return false;

					return false;
					});
				vertexWeight.resize(4, std::make_pair(255, 0.0f));
			}

			// extract vertex data per mesh
			for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
				const aiVector3D& aiVertex = mesh->mVertices[vertexIndex];

				SVertexData vertex;

				// bones info
				vertex.SetBones(vertexWeights[vertexIndex]);

				// vertex coordinates
				vertex.pos = aiVectorToFloat3(aiVertex);

				if (mesh->HasNormals()) {
					// vertex normal
					const aiVector3D& aiNormal = mesh->mNormals[vertexIndex];

					if (IS_QNAN(aiNormal)) {
						LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Malformed normal (model->name=\"%s\" meshName=\"%s\" vertexIndex=%d x=%f y=%f z=%f)", model->name.c_str(), mesh->mName.C_Str(), vertexIndex, aiNormal.x, aiNormal.y, aiNormal.z);
						vertex.normal = float3{ 0.0f, 1.0f, 0.0f };
					}
					else {
						vertex.normal = (aiVectorToFloat3(aiNormal)).SafeANormalize();
					}
				}
				else {
					vertex.normal = float3{ 0.0f, 1.0f, 0.0f };
				}

				// vertex tangent, x is positive in texture axis
				if (mesh->HasTangentsAndBitangents()) {
					const aiVector3D& aiTangent = mesh->mTangents[vertexIndex];
					const aiVector3D& aiBitangent = mesh->mBitangents[vertexIndex];

					if (IS_QNAN(aiTangent.x) || IS_QNAN(aiTangent.y) || IS_QNAN(aiTangent.z)) {
						LOG_SL(LOG_SECTION_PIECE, L_INFO, "Malformed tangent (model->name=\"%s\" meshName=\"%s\" vertexIndex=%d x=%f y=%f z=%f)", model->name.c_str(), mesh->mName.C_Str(), vertexIndex, aiTangent.x, aiTangent.y, aiTangent.z);
						vertex.sTangent = float3{ 1.0f, 0.0f, 0.0f };
					}
					else {
						vertex.sTangent = (aiVectorToFloat3(aiTangent)).SafeANormalize();
					}

					if (IS_QNAN(aiBitangent.x) || IS_QNAN(aiBitangent.y) || IS_QNAN(aiBitangent.z)) {
						LOG_SL(LOG_SECTION_PIECE, L_INFO, "Malformed bitangent (model->name=\"%s\" meshName=\"%s\" vertexIndex=%d x=%f y=%f z=%f)", model->name.c_str(), mesh->mName.C_Str(), vertexIndex, aiBitangent.x, aiBitangent.y, aiBitangent.z);
						vertex.tTangent = vertex.normal.cross(vertex.sTangent);
					}
					else {
						vertex.tTangent = (aiVectorToFloat3(aiBitangent)).SafeANormalize();
					}

					vertex.tTangent *= -1.0f; // LH (assimp) to RH
				}

				// vertex tex-coords per channel
				for (uint32_t uvChanIndex = 0; uvChanIndex < NUM_MODEL_UVCHANNS; uvChanIndex++) {
					if (!mesh->HasTextureCoords(uvChanIndex))
						break;

					numUVs = uvChanIndex + 1;

					vertex.texCoords[uvChanIndex].x = mesh->mTextureCoords[uvChanIndex][vertexIndex].x;
					vertex.texCoords[uvChanIndex].y = mesh->mTextureCoords[uvChanIndex][vertexIndex].y;
				}

				vertex.pos      = (boneTra * float4{ vertex.pos     , 1.0f }).xyz;
				vertex.normal   = (boneTra * float4{ vertex.normal  , 0.0f }).xyz;
				vertex.sTangent = (boneTra * float4{ vertex.sTangent, 0.0f }).xyz;
				vertex.tTangent = (boneTra * float4{ vertex.tTangent, 0.0f }).xyz;

				meshVertexMapping.push_back(verts.size());
				verts.push_back(vertex);
			}

			// extract face data
			LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Processing faces for mesh %d (%d faces)", meshIndex, mesh->mNumFaces);

			/*
			 * since aiProcess_SortByPType is being used,
			 * we're sure we'll get only 1 type here,
			 * so combination check isn't needed, also
			 * anything more complex than triangles is
			 * being split thanks to aiProcess_Triangulate
			 */
			for (unsigned faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
				const aiFace& face = mesh->mFaces[faceIndex];

				// some models contain lines (mNumIndices == 2) which
				// we cannot render and they would need a 2nd drawcall)
				if (face.mNumIndices != 3)
					continue;

				for (unsigned vertexListID = 0; vertexListID < face.mNumIndices; ++vertexListID) {
					const unsigned int vertexFaceIdx = face.mIndices[vertexListID];
					const unsigned int vertexDrawIdx = meshVertexMapping[vertexFaceIdx];
					indcs.push_back(vertexDrawIdx);
				}
			}

			meshes.emplace_back(verts, indcs, numUVs);
		}

		return meshes;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

void CAssParser::Init()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// FIXME: non-optimal, maybe compute these ourselves (pre-TL cache size!)
	maxIndices = std::max(globalRendering->glslMaxRecommendedIndices, 1024);
	maxVertices = std::max(globalRendering->glslMaxRecommendedVertices, 1024);
	numPoolPieces = 0;

	Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
	// create a logger for debugging model loading issues
	Assimp::DefaultLogger::get()->attachStream(new AssLogStream(), ASS_LOGGING_OPTIONS);
}

void CAssParser::Kill()
{
	RECOIL_DETAILED_TRACY_ZONE;
	Assimp::DefaultLogger::kill();
	LOG_L(L_INFO, "[AssParser::%s] allocated %u pieces", __func__, numPoolPieces);

	// reuse piece innards when reloading
	// piecePool.clear();
	for (unsigned int i = 0; i < numPoolPieces; i++) {
		piecePool[i].Clear();
	}

	numPoolPieces = 0;
}

void CAssParser::Load(S3DModel& model, const std::string& modelFilePath)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LOG_SL(LOG_SECTION_MODEL, L_INFO, "Loading model: %s", modelFilePath.c_str());

	const std::string& modelPath = FileSystem::GetDirectory(modelFilePath);
	const std::string& modelName = FileSystem::GetBasename(modelFilePath);

	CFileHandler file(modelFilePath, SPRING_VFS_ZIP);

	std::vector<unsigned char> fileBuf;
	// load the lua metafile containing properties unique to Spring models (must return a table)
	std::string metaFileName = modelFilePath + ".lua";

	// try again without the model file extension
	if (!CFileHandler::FileExists(metaFileName, SPRING_VFS_ZIP))
		metaFileName = modelPath + modelName + ".lua";
	if (!CFileHandler::FileExists(metaFileName, SPRING_VFS_ZIP))
		LOG_SL(LOG_SECTION_MODEL, L_INFO, "No meta-file '%s'. Using defaults.", metaFileName.c_str());

	LuaParser metaFileParser(metaFileName, SPRING_VFS_ZIP, SPRING_VFS_ZIP);

	if (!metaFileParser.Execute())
		LOG_SL(LOG_SECTION_MODEL, L_INFO, "'%s': %s. Using defaults.", metaFileName.c_str(), metaFileParser.GetErrorLog().c_str());

	// get the (root-level) model table
	const LuaTable& modelTable = metaFileParser.GetRoot();

	if (!modelTable.IsValid())
		LOG_SL(LOG_SECTION_MODEL, L_INFO, "No valid model metadata in '%s' or no meta-file", metaFileName.c_str());


	Assimp::Importer importer;

	// speed-up processing by skipping things we don't need
	importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, ASS_IMPORTER_OPTIONS);
	importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT,   maxVertices);
	importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, maxIndices / 3);

	if (!file.IsBuffered()) {
		const auto fs = file.FileSize();
		if (fs <= 0)
			throw content_error("An assimp model has invalid size of " + std::to_string(fs));

		fileBuf.resize(fs, 0);
		file.Read(fileBuf.data(), fileBuf.size());
	} else {
		fileBuf = std::move(file.GetBuffer());
	}

	if (modelTable.GetBool("nodenamesfromids", false)) {
		assert(FileSystem::GetExtension(modelFilePath) == "dae");
		PreProcessFileBuffer(fileBuf);
	}


	// Read the model file to build a scene object
	LOG_SL(LOG_SECTION_MODEL, L_INFO, "Importing model file: %s", modelFilePath.c_str());

	const aiScene* scene = nullptr;

	{
		// ASSIMP spams many SIGFPEs atm in normal & tangent generation
		ScopedDisableFpuExceptions fe;
		scene = importer.ReadFileFromMemory(fileBuf.data(), fileBuf.size(), ASS_POSTPROCESS_OPTIONS);
	}

	if (scene == nullptr)
		throw content_error("[AssimpParser] Model Import: " + std::string(importer.GetErrorString()));

	LOG_SL(LOG_SECTION_MODEL, L_INFO,
		"Processing scene for model: %s (%d meshes / %d materials / %d textures)",
		modelFilePath.c_str(), scene->mNumMeshes, scene->mNumMaterials,
		scene->mNumTextures
	);

	ModelPieceMap pieceMap;
	ParentNameMap parentMap;

	model.name = modelFilePath;
	model.type = MODELTYPE_ASS;

	// Load textures
	FindTextures(&model, scene, modelTable, modelPath, modelName);
	LOG_SL(LOG_SECTION_MODEL, L_INFO, "Loading textures. Tex1: '%s' Tex2: '%s'", model.texs[0].c_str(), model.texs[1].c_str());

	textureHandlerS3O.PreloadTexture(&model, modelTable.GetBool("fliptextures", true), modelTable.GetBool("invertteamcolor", true));

	// Check if bones exist
	const auto boneNames = Impl::GetBoneNames(scene);
	const auto meshNames = !boneNames.empty() ? Impl::GetMeshNames(scene) : std::vector<std::string>{};

	// Load all pieces in the model
	LOG_SL(LOG_SECTION_MODEL, L_INFO, "Loading pieces from root node '%s'", scene->mRootNode->mName.data);
	LoadPiece(&model, scene->mRootNode, scene, modelTable, meshNames, pieceMap, parentMap);

	// Update piece hierarchy based on metadata
	BuildPieceHierarchy(&model, pieceMap, parentMap);

	// skinning support
	if (!meshNames.empty()) {
		// need matrices earlier than usual
		model.SetPieceMatrices();
		std::vector<SPseudoAssPiece> meshPseudoPieces(meshNames.size());
		auto mppIt = meshPseudoPieces.begin();
		for (const auto& meshName : meshNames) {
			aiNode* meshNode = nullptr;
			meshNode = Impl::FindNode(scene, scene->mRootNode, meshName);
			mppIt->name = meshName;
			if (!meshNode) {
				LOG_SL(LOG_SECTION_MODEL, L_ERROR, "An assimp model has invalid pieces hierarchy. Missing a mesh named: \"%s\" in model[\"%s\"] path: %s. Looking for a likely candidate", meshName.c_str(), modelName.c_str(), modelPath.c_str());

				/* Try to salvage the model since such "invalid" ones can actually be
				 * produced by industry standard tools (in particular, Blender). */
				meshNode = Impl::FindFallbackNode(scene);
				if (meshNode && meshNode->mParent)
					LOG_SL(LOG_SECTION_MODEL, L_WARNING, "Found a likely replacement candidate for mesh \"%s\" - node \"%s\". It might be incorrect!", meshName.c_str(), meshNode->mName.data);
				else
					throw content_error("An assimp model has invalid pieces hierarchy. Failed to find suitable replacement.");
			}

			std::string const parentName(meshNode->mParent->mName.C_Str());
			auto* parentPiece = model.FindPiece(parentName);
			assert(parentPiece);
			mppIt->parent = parentPiece;

			LoadPieceTransformations(&(*mppIt), &model, meshNode, modelTable);
			mppIt++;
		}
		const auto meshBoneTransforms = Impl::GetMeshBoneTransforms(scene, &model, meshPseudoPieces);
		const auto meshes = Impl::GetModelSpaceMeshes(scene, &model, meshBoneTransforms);

		// if numMeshes >= numBones reparent the whole meshes
		// else reparent meshes per-triangle
		if (meshNames.size() >= boneNames.size())
			ReparentCompleteMeshesToBones(&model, meshes);
		else
			ReparentMeshesTrianglesToBones(&model, meshes);
	}

	UpdatePiecesMinMaxExtents(&model);
	CalculateModelProperties(&model, modelTable);

	// Verbose logging of model properties
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->name: %s", model.name.c_str());
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->numobjects: %d", model.numPieces);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->radius: %f", model.radius);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->height: %f", model.height);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->mins: (%f,%f,%f)", model.mins[0], model.mins[1], model.mins[2]);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->maxs: (%f,%f,%f)", model.maxs[0], model.maxs[1], model.maxs[2]);
	LOG_SL(LOG_SECTION_MODEL, L_INFO, "Model %s Imported.", model.name.c_str());
}


void CAssParser::PreProcessFileBuffer(std::vector<unsigned char>& fileBuffer)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// the Collada specification requires node uid's to be unique
	// (names can be repeated) which certain exporters obey while
	// others do not
	// however, obedient exporters actually make life inconvenient
	// for modellers since assimp's Collada importer extracts node
	// names (aiNode::mName) from the *id* field
	// as a workaround, let the model metadata decide if id's and
	// names should be swapped before assimp processes the buffer
	const std::regex nodePattern{"<node id=\"([a-zA-Z0-9_-]+)\" name=\"([a-zA-Z0-9_-]+)\" type=\"([a-zA-Z]+)\">"};

	std::array<unsigned char, 1024> lineBuffer;
	std::cmatch matchGroups;

	const char* beg = reinterpret_cast<const char*>(fileBuffer.data());
	const char* end = reinterpret_cast<const char*>(fileBuffer.data() + fileBuffer.size());

	if (strstr(beg, "COLLADA") == nullptr)
		return;

	for (size_t i = 0, n = fileBuffer.size(); i < n; ) {
		matchGroups = std::cmatch{};

		if (!std::regex_search(beg + i, matchGroups, nodePattern))
			break;

		const std::string   id = matchGroups[1].str();
		const std::string name = matchGroups[2].str();
		const std::string type = matchGroups[3].str();

		assert(matchGroups[0].first  >= beg && matchGroups[0].first  < end);
		assert(matchGroups[0].second >= beg && matchGroups[0].second < end);

		// just swap id and name fields; preserves line length
		memset(lineBuffer.data(), 0, lineBuffer.size());
		snprintf(reinterpret_cast<char*>(lineBuffer.data()), lineBuffer.size(), "<node id=\"%s\" name=\"%s\" type=\"%s\">", name.c_str(), id.c_str(), type.c_str());
		memcpy(const_cast<char*>(matchGroups[0].first), lineBuffer.data(), matchGroups[0].length());

		i = matchGroups[0].second - beg;
	}
}

/*
void CAssParser::CalculateModelMeshBounds(S3DModel* model, const aiScene* scene)
{
	model->meshBounds.resize(scene->mNumMeshes * 2);

	// calculate bounds for each individual mesh of
	// the model; currently we have no use for this
	// and S3DModel has only one pair of bounds
	//
	for (size_t i = 0; i < scene->mNumMeshes; i++) {
		const aiMesh* mesh = scene->mMeshes[i];

		float3& mins = model->meshBounds[i*2 + 0];
		float3& maxs = model->meshBounds[i*2 + 1];

		mins = DEF_MIN_SIZE;
		maxs = DEF_MAX_SIZE;

		for (size_t vertexIndex= 0; vertexIndex < mesh->mNumVertices; vertexIndex++) {
			const aiVector3D& aiVertex = mesh->mVertices[vertexIndex];
			mins = std::min(mins, aiVectorToFloat3(aiVertex));
			maxs = std::max(maxs, aiVectorToFloat3(aiVertex));
		}

		if (mins == DEF_MIN_SIZE) { mins = ZeroVector; }
		if (maxs == DEF_MAX_SIZE) { maxs = ZeroVector; }
	}
}
*/



void CAssParser::LoadPieceTransformations(
	SAssPiece* piece,
	const S3DModel* model,
	const aiNode* pieceNode,
	const LuaTable& pieceTable
) {
	RECOIL_DETAILED_TRACY_ZONE;
	Impl::LoadPieceTransformations<SAssPiece>(piece, model, pieceNode, pieceTable);
}

void CAssParser::LoadPieceTransformations(
	SPseudoAssPiece* piece,
	const S3DModel* model,
	const aiNode* pieceNode,
	const LuaTable& pieceTable
) {
	RECOIL_DETAILED_TRACY_ZONE;
	Impl::LoadPieceTransformations<SPseudoAssPiece>(piece, model, pieceNode, pieceTable);
}

void CAssParser::UpdatePiecesMinMaxExtents(S3DModel* model)
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (auto* piece : model->pieceObjects) {
		for (const auto& vertex : piece->vertices) {
			piece->mins = float3::min(piece->mins, vertex.pos);
			piece->maxs = float3::max(piece->maxs, vertex.pos);
		}
	}
}

void CAssParser::SetPieceName(
	SAssPiece* piece,
	const S3DModel* model,
	const aiNode* pieceNode,
	ModelPieceMap& pieceMap
) {
	RECOIL_DETAILED_TRACY_ZONE;
	assert(piece->name.empty());
	piece->name = std::string(pieceNode->mName.data);

	if (piece->name.empty()) {
		if (piece == model->GetRootPiece()) {
			// root is always the first piece created, so safe to assign this
			piece->name = "$$root$$";
			return;
		}

		piece->name = "$$piece$$";
	}

	// find a new name if none given or if a piece with the same name already exists
	ModelPieceMap::const_iterator it = pieceMap.find(piece->name);

	for (unsigned int i = 0; it != pieceMap.end(); i++) {
		const std::string newPieceName = piece->name + IntToString(i, "%02i");

		if ((it = pieceMap.find(newPieceName)) == pieceMap.end()) {
			piece->name = newPieceName; break;
		}
	}

	assert(piece->name != "SpringHeight");
	assert(piece->name != "SpringRadius");
}

void CAssParser::SetPieceParentName(
	SAssPiece* piece,
	const S3DModel* model,
	const aiNode* pieceNode,
	const LuaTable& pieceTable,
	ParentNameMap& parentMap
) {
	RECOIL_DETAILED_TRACY_ZONE;
	// parent was updated in GetPieceTableRecursively
	if (parentMap.find(piece->name) != parentMap.end())
		return;

	// Get parent name from metadata or model
	if (pieceTable.KeyExists("parent")) {
		parentMap[piece->name] = pieceTable.GetString("parent", "");
		return;
	}

	if (pieceNode->mParent == nullptr)
		return;

	if (pieceNode->mParent->mParent != nullptr) {
		// parent is not the root
		parentMap[piece->name] = std::string(pieceNode->mParent->mName.data);
	} else {
		// parent is the root (which must already exist)
		assert(model->GetRootPiece() != nullptr);
		parentMap[piece->name] = (model->GetRootPiece())->name;
	}
}

void CAssParser::LoadPieceGeometry(SAssPiece* piece, const S3DModel* model, const aiNode* pieceNode, const aiScene* scene)
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::vector<unsigned> meshVertexMapping;

	// Get vertex data from node meshes
	for (unsigned meshListIndex = 0; meshListIndex < pieceNode->mNumMeshes; ++meshListIndex) {
		const unsigned int meshIndex = pieceNode->mMeshes[meshListIndex];
		const aiMesh* mesh = scene->mMeshes[meshIndex];

		LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Fetching mesh %d from scene", meshIndex);
		LOG_SL(LOG_SECTION_PIECE, L_DEBUG,
			"Processing vertices for mesh %d (%d vertices)",
			meshIndex, mesh->mNumVertices);
		LOG_SL(LOG_SECTION_PIECE, L_DEBUG,
			"Normals: %s Tangents/Bitangents: %s TexCoords: %s",
			(mesh->HasNormals() ? "Y" : "N"),
			(mesh->HasTangentsAndBitangents() ? "Y" : "N"),
			(mesh->HasTextureCoords(0) ? "Y" : "N"));

		piece->vertices.reserve(piece->vertices.size() + mesh->mNumVertices);
		piece->indices.reserve(piece->indices.size() + mesh->mNumFaces * 3);

		meshVertexMapping.clear();
		meshVertexMapping.reserve(mesh->mNumVertices);

		// extract vertex data per mesh
		for (unsigned vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
			const aiVector3D& aiVertex = mesh->mVertices[vertexIndex];

			SVertexData vertex;

			// vertex coordinates
			vertex.pos = aiVectorToFloat3(aiVertex);

			if (mesh->HasNormals()) {
				// vertex normal
				const aiVector3D& aiNormal = mesh->mNormals[vertexIndex];

				if (IS_QNAN(aiNormal)) {
					LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Malformed normal (model->name=\"%s\" piece->name=\"%s\" vertexIndex=%d x=%f y=%f z=%f)", model->name.c_str(), piece->name.c_str(), vertexIndex, aiNormal.x, aiNormal.y, aiNormal.z);
					vertex.normal = float3{ 0.0f, 1.0f, 0.0f };
				}
				else {
					vertex.normal = (aiVectorToFloat3(aiNormal)).SafeANormalize();
				}
			}
			else {
				vertex.normal = float3{ 0.0f, 1.0f, 0.0f };
			}

			// vertex tangent, x is positive in texture axis
			if (mesh->HasTangentsAndBitangents()) {
				const aiVector3D& aiTangent = mesh->mTangents[vertexIndex];
				const aiVector3D& aiBitangent = mesh->mBitangents[vertexIndex];

				if (IS_QNAN(aiTangent.x) || IS_QNAN(aiTangent.y) || IS_QNAN(aiTangent.z)) {
					LOG_SL(LOG_SECTION_PIECE, L_INFO, "Malformed tangent (model->name=\"%s\" piece->name=\"%s\" vertexIndex=%d x=%f y=%f z=%f)", model->name.c_str(), piece->name.c_str(), vertexIndex, aiTangent.x, aiTangent.y, aiTangent.z);
					vertex.sTangent = float3{1.0f, 0.0f, 0.0f};
				} else {
					vertex.sTangent = (aiVectorToFloat3(aiTangent)).SafeANormalize();
				}

				if (IS_QNAN(aiBitangent.x) || IS_QNAN(aiBitangent.y) || IS_QNAN(aiBitangent.z)) {
					LOG_SL(LOG_SECTION_PIECE, L_INFO, "Malformed bitangent (model->name=\"%s\" piece->name=\"%s\" vertexIndex=%d x=%f y=%f z=%f)", model->name.c_str(), piece->name.c_str(), vertexIndex, aiBitangent.x, aiBitangent.y, aiBitangent.z);
					vertex.tTangent = vertex.normal.cross(vertex.sTangent);
				} else {
					vertex.tTangent = (aiVectorToFloat3(aiBitangent)).SafeANormalize();
				}

				vertex.tTangent *= -1.0f; // LH (assimp) to RH
			}

			// vertex tex-coords per channel
			for (unsigned int uvChanIndex = 0; uvChanIndex < NUM_MODEL_UVCHANNS; uvChanIndex++) {
				if (!mesh->HasTextureCoords(uvChanIndex))
					break;

				piece->SetNumTexCoorChannels(uvChanIndex + 1);

				vertex.texCoords[uvChanIndex].x = mesh->mTextureCoords[uvChanIndex][vertexIndex].x;
				vertex.texCoords[uvChanIndex].y = mesh->mTextureCoords[uvChanIndex][vertexIndex].y;
			}

			meshVertexMapping.push_back(piece->vertices.size());
			piece->vertices.push_back(vertex);
		}

		// extract face data
		LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Processing faces for mesh %d (%d faces)", meshIndex, mesh->mNumFaces);

		/*
		 * since aiProcess_SortByPType is being used,
		 * we're sure we'll get only 1 type here,
		 * so combination check isn't needed, also
		 * anything more complex than triangles is
		 * being split thanks to aiProcess_Triangulate
		 */
		for (unsigned faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			const aiFace& face = mesh->mFaces[faceIndex];

			// some models contain lines (mNumIndices == 2) which
			// we cannot render and they would need a 2nd drawcall)
			if (face.mNumIndices != 3)
				continue;

			for (unsigned vertexListID = 0; vertexListID < face.mNumIndices; ++vertexListID) {
				const unsigned int vertexFaceIdx = face.mIndices[vertexListID];
				const unsigned int vertexDrawIdx = meshVertexMapping[vertexFaceIdx];
				piece->indices.push_back(vertexDrawIdx);
			}
		}
	}
}

void CAssParser::ReparentMeshesTrianglesToBones(S3DModel* model, const std::vector<CAssParser::MeshData>& meshes)
{
	RECOIL_DETAILED_TRACY_ZONE;

	auto GetBoneID = [](const SVertexData& vert, size_t wi) {
		return vert.boneIDsLow[wi] | (vert.boneIDsHigh[wi] << 8);
	};

	for (const auto& [verts, indcs, numUVs] : meshes) {
		for (size_t trID = 0; trID < indcs.size() / 3; ++trID) {
			std::array<uint32_t, 256> boneWeights = { 0 };

			for (size_t vi = 0; vi < 3; ++vi) {
				const auto& vert = verts[indcs[trID * 3 + vi]];

				for (size_t wi = 0; wi < 4; ++wi) {
					boneWeights[GetBoneID(vert, wi)] += vert.boneWeights[wi];
				}
			}

			const auto maxWeightedBoneID = std::distance(
				boneWeights.begin(),
				std::max_element(boneWeights.begin(), boneWeights.end())
			);
			assert(maxWeightedBoneID < INV_PIECE_NUM); // INV_PIECE_NUM - invalid bone

			auto* maxWeightedPiece = static_cast<SAssPiece*>(model->pieceObjects[maxWeightedBoneID]);
			maxWeightedPiece->SetNumTexCoorChannels(std::max(maxWeightedPiece->GetNumTexCoorChannels(), numUVs));

			auto& pieceVerts = maxWeightedPiece->vertices;
			auto& pieceIndcs = maxWeightedPiece->indices;

			for (size_t vi = 0; vi < 3; ++vi) {
				auto  targVert = verts[indcs[trID * 3 + vi]]; //copy

				// find if targVert is already added
				auto itTargVec = std::find_if(pieceVerts.begin(), pieceVerts.end(), [&targVert](const auto& vert) {
					return targVert.pos.equals(vert.pos) && targVert.normal.equals(vert.normal);
				});

				// new vertex
				if (itTargVec == pieceVerts.end()) {
					// make sure maxWeightedBoneID comes first. It's a must, even if it doesn't exist in targVert.boneIDs!
					const auto boneID0 = GetBoneID(targVert, 0);
					if (boneID0 != maxWeightedBoneID) {
						size_t itPos = 0;
						for (size_t jj = 1; jj < targVert.boneIDsLow.size(); ++jj) {
							if (GetBoneID(targVert, jj) == maxWeightedBoneID) {
								itPos = jj;
								break;
							}
						}
						if (itPos != 0) {
							// swap maxWeightedBoneID so it comes first in the boneIDs array
							std::swap(targVert.boneIDsLow[0], targVert.boneIDsLow[itPos]);
							std::swap(targVert.boneWeights[0], targVert.boneWeights[itPos]);
							std::swap(targVert.boneIDsHigh[0], targVert.boneIDsHigh[itPos]);
						}
						else {
							// maxWeightedBoneID doesn't even exist in this targVert
							// replace the bone with the least weight with maxWeightedBoneID and swap it be first
							targVert.boneIDsLow[3]  = static_cast<uint8_t>((maxWeightedBoneID     ) & 0xFF);
							targVert.boneWeights[3] = 0;
							targVert.boneIDsHigh[3] = static_cast<uint8_t>((maxWeightedBoneID >> 8) & 0xFF);
							std::swap(targVert.boneIDsLow[0], targVert.boneIDsLow[3]);
							std::swap(targVert.boneWeights[0], targVert.boneWeights[3]);
							std::swap(targVert.boneIDsHigh[0], targVert.boneIDsHigh[3]);

							// renormalize weights (optional but nice for debugging)
							const float sumWeights = static_cast<float>(std::reduce(targVert.boneWeights.begin(), targVert.boneWeights.end())) / 255.0;
							for (auto& bw : targVert.boneWeights) {
								bw = static_cast<uint8_t>(math::round(static_cast<float>(bw) / 255.0f / sumWeights));
							}
						}
					}

					pieceIndcs.emplace_back(static_cast<uint32_t>(pieceVerts.size()));
					pieceVerts.emplace_back(std::move(targVert));
				}
				else {
					pieceIndcs.emplace_back(static_cast<uint32_t>(std::distance(
						pieceVerts.begin(),
						itTargVec
					)));
				}
			}
		}
	}

	// transform model space mesh vertices into bone/piece space
	for (auto* piece : model->pieceObjects) {
		if (!piece->HasGeometryData())
			continue;

		const auto invMat = piece->bposeTransform.ToMatrix().InvertAffine();
		for (auto& vert : piece->vertices) {
			vert.pos      = (invMat * float4{ vert.pos     , 1.0f }).xyz;
			vert.normal   = (invMat * float4{ vert.normal  , 0.0f }).xyz;
			vert.sTangent = (invMat * float4{ vert.sTangent, 0.0f }).xyz;
			vert.tTangent = (invMat * float4{ vert.tTangent, 0.0f }).xyz;
		}
	}
}

void CAssParser::ReparentCompleteMeshesToBones(S3DModel* model, const std::vector<CAssParser::MeshData>& meshes)
{
	RECOIL_DETAILED_TRACY_ZONE;

	auto GetBoneID = [](const SVertexData& vert, size_t wi) {
		return vert.boneIDsLow[wi] | (vert.boneIDsHigh[wi] << 8);
	};

	for (const auto& [verts, indcs, numUVs] : meshes) {
		std::array<uint32_t, 256> boneWeights = { 0 };
		for (const auto& vert : verts) {
			for (size_t wi = 0; wi < 4; ++wi) {
				boneWeights[GetBoneID(vert, wi)] += vert.boneWeights[wi];
			}
		}
		const auto maxWeightedBoneID = std::distance(
			boneWeights.begin(),
			std::max_element(boneWeights.begin(), boneWeights.end())
		);
		assert(maxWeightedBoneID < 255); // 255 - invalid bone

		auto* maxWeightedPiece = static_cast<SAssPiece*>(model->pieceObjects[maxWeightedBoneID]);
		maxWeightedPiece->SetNumTexCoorChannels(std::max(maxWeightedPiece->GetNumTexCoorChannels(), numUVs));

		auto& pieceVerts = maxWeightedPiece->vertices;
		auto& pieceIndcs = maxWeightedPiece->indices;
		const auto indexOffset = static_cast<uint32_t>(pieceVerts.size());

		for (auto targVert : verts) { // deliberate copy
			// Unlike ReparentMeshesTrianglesToBones() do not check for already existing vertices
			// Just copy mesh as is. Modelers and assimp should have done necessary dedup for us.

			// make sure maxWeightedBoneID comes first. It's a must, even if it doesn't exist in targVert.boneIDs!
			const auto boneID0 = GetBoneID(targVert, 0);
			if (boneID0 != maxWeightedBoneID) {
				size_t itPos = 0;
				for (size_t jj = 1; jj < targVert.boneIDsLow.size(); ++jj) {
					if (GetBoneID(targVert, jj) == maxWeightedBoneID) {
						itPos = jj;
						break;
					}
				}
				if (itPos != 0) {
					// swap maxWeightedBoneID so it comes first in the boneIDs array
					std::swap(targVert.boneIDsLow[0], targVert.boneIDsLow[itPos]);
					std::swap(targVert.boneWeights[0], targVert.boneWeights[itPos]);
					std::swap(targVert.boneIDsHigh[0], targVert.boneIDsHigh[itPos]);
				}
				else {
					// maxWeightedBoneID doesn't even exist in this targVert
					// replace the bone with the least weight with maxWeightedBoneID and swap it be first
					targVert.boneIDsLow[3] = static_cast<uint8_t>((maxWeightedBoneID) & 0xFF);
					targVert.boneWeights[3] = 0;
					targVert.boneIDsHigh[3] = static_cast<uint8_t>((maxWeightedBoneID >> 8) & 0xFF);
					std::swap(targVert.boneIDsLow[0], targVert.boneIDsLow[3]);
					std::swap(targVert.boneWeights[0], targVert.boneWeights[3]);
					std::swap(targVert.boneIDsHigh[0], targVert.boneIDsHigh[3]);

					// renormalize weights (optional but nice for debugging)
					const float sumWeights = static_cast<float>(std::reduce(targVert.boneWeights.begin(), targVert.boneWeights.end())) / 255.0;
					for (auto& bw : targVert.boneWeights) {
						bw = static_cast<uint8_t>(math::round(static_cast<float>(bw) / 255.0f / sumWeights));
					}
				}
			}

			pieceVerts.emplace_back(std::move(targVert));
		}

		for (const auto indx : indcs) {
			pieceIndcs.emplace_back(indexOffset + indx);
		}
	}

	// transform model space mesh vertices into bone/piece space
	for (auto* piece : model->pieceObjects) {
		if (!piece->HasGeometryData())
			continue;

		const auto invMat = piece->bposeTransform.ToMatrix().InvertAffine();
		for (auto& vert : piece->vertices) {
			vert.pos      = (invMat * float4{ vert.pos     , 1.0f }).xyz;
			vert.normal   = (invMat * float4{ vert.normal  , 0.0f }).xyz;
			vert.sTangent = (invMat * float4{ vert.sTangent, 0.0f }).xyz;
			vert.tTangent = (invMat * float4{ vert.tTangent, 0.0f }).xyz;
		}
	}
}

// Not efficient, but there aren't that many pieces
// So fast anyway
static LuaTable GetPieceTableRecursively(
	const LuaTable& table,
	const std::string& name,
	const std::string& parentName,
	CAssParser::ParentNameMap& parentMap)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LuaTable ret = table.SubTable(name);
	if (ret.IsValid()) {
		if (!parentName.empty())
			parentMap[name] = parentName;
		return ret;
	}

	std::vector<std::string> keys;
	table.GetKeys(keys);
	for (const std::string& key: keys) {
		ret = GetPieceTableRecursively(table.SubTable(key), name, key, parentMap);
		if (ret.IsValid())
			break;
	}
	return ret;
}


SAssPiece* CAssParser::AllocPiece()
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

SAssPiece* CAssParser::LoadPiece(
	S3DModel* model,
	const aiNode* pieceNode,
	const aiScene* scene,
	const LuaTable& modelTable,
	const std::vector<std::string>& skipList,
	ModelPieceMap& pieceMap,
	ParentNameMap& parentMap
) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (std::find(skipList.begin(), skipList.end(), std::string(pieceNode->mName.data)) != skipList.end())
		return nullptr;

	++model->numPieces;

	SAssPiece* piece = AllocPiece();

	if (pieceNode->mParent == nullptr) {
		// set the model's root piece ASAP, needed in SetPiece*Name
		assert(pieceNode == scene->mRootNode);
		model->AddPiece(piece);
	}

	SetPieceName(piece, model, pieceNode, pieceMap);
	piece->SetParentModel(model);

	LOG_SL(LOG_SECTION_PIECE, L_INFO, "Converting node '%s' to piece '%s' (%d meshes).", pieceNode->mName.data, piece->name.c_str(), pieceNode->mNumMeshes);

	// Load additional piece properties from metadata
	const LuaTable& pieceTable = GetPieceTableRecursively(modelTable.SubTable("pieces"), piece->name, "", parentMap);

	if (pieceTable.IsValid())
		LOG_SL(LOG_SECTION_PIECE, L_INFO, "Found metadata for piece '%s'", piece->name.c_str());


	LoadPieceTransformations(piece, model, pieceNode, pieceTable);
	LoadPieceGeometry(piece, model, pieceNode, scene);
	SetPieceParentName(piece, model, pieceNode, pieceTable, parentMap);

	{
		// operator[] creates an empty string if piece is not in map
		const auto parentNameIt = parentMap.find(piece->name);
		const std::string& parentName = (parentNameIt != parentMap.end())? (parentNameIt->second).c_str(): "[null]";

		// Verbose logging of piece properties
		LOG_SL(LOG_SECTION_PIECE, L_INFO, "Loaded model piece: %s with %d meshes", piece->name.c_str(), pieceNode->mNumMeshes);
		LOG_SL(LOG_SECTION_PIECE, L_INFO, "piece->name: %s", piece->name.c_str());
		LOG_SL(LOG_SECTION_PIECE, L_INFO, "piece->parent: %s", parentName.c_str());
	}

	// Recursively process all child pieces
	for (unsigned int i = 0; i < pieceNode->mNumChildren; ++i) {
		LoadPiece(model, pieceNode->mChildren[i], scene, modelTable, skipList, pieceMap, parentMap);
	}

	pieceMap[piece->name] = piece;
	return piece;
}


// Because of metadata overrides we don't know the true hierarchy until all pieces have been loaded
void CAssParser::BuildPieceHierarchy(S3DModel* model, ModelPieceMap& pieceMap, const ParentNameMap& parentMap)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const char* fmt1 = "Missing piece '%s' declared as parent of '%s'.";
	const char* fmt2 = "Missing root piece (parent of orphan '%s')";

	// loop through all pieces and create missing hierarchy info
	for (auto it = pieceMap.cbegin(); it != pieceMap.cend(); ++it) {
		SAssPiece* piece = static_cast<SAssPiece*>(it->second);

		if (piece == model->GetRootPiece()) {
			assert(piece->parent == nullptr);
			assert(model->GetRootPiece() == piece);
			continue;
		}

		const auto parentNameIt = parentMap.find(piece->name);

		if (parentNameIt != parentMap.end()) {
			const std::string& parentName = parentNameIt->second;
			const auto pieceIt = pieceMap.find(parentName);

			// re-assign this piece to a different parent
			if (pieceIt != pieceMap.end()) {
				piece->parent = pieceIt->second;
				piece->parent->children.push_back(piece);
			} else {
				LOG_SL(LOG_SECTION_PIECE, L_ERROR, fmt1, parentName.c_str(), piece->name.c_str());
			}

			continue;
		}

		// piece with no named parent that isn't the root (orphaned)
		// link it to the root piece which has already been pre-added
		if ((piece->parent = model->GetRootPiece()) == nullptr) {
			LOG_SL(LOG_SECTION_PIECE, L_ERROR, fmt2, piece->name.c_str());
		} else {
			piece->parent->children.push_back(piece);
		}
	}

	model->FlattenPieceTree(model->GetRootPiece());
}


// Iterate over the model and calculate its overall dimensions
void CAssParser::CalculateModelDimensions(S3DModel* model, S3DModelPiece* piece)
{
	// TODO fix
	const CMatrix44f scaleRotMat = piece->ComposeTransform(ZeroVector, ZeroVector, piece->scale).ToMatrix();

	// cannot set this until parent relations are known, so either here or in BuildPieceHierarchy()
	piece->goffset = scaleRotMat.Mul(piece->offset) + ((piece->parent != nullptr)? piece->parent->goffset: ZeroVector);

	// update model min/max extents
	model->mins = float3::min(piece->goffset + piece->mins, model->mins);
	model->maxs = float3::max(piece->goffset + piece->maxs, model->maxs);

	piece->SetCollisionVolume(CollisionVolume('b', 'z', piece->maxs - piece->mins, (piece->maxs + piece->mins) * 0.5f));

	// Repeat with children
	for (S3DModelPiece* childPiece: piece->children) {
		CalculateModelDimensions(model, childPiece);
	}
}

// Calculate model radius from the min/max extents
void CAssParser::CalculateModelProperties(S3DModel* model, const LuaTable& modelTable)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CalculateModelDimensions(model, model->pieceObjects[0]);

	model->mins = modelTable.GetFloat3("mins", model->mins);
	model->maxs = modelTable.GetFloat3("maxs", model->maxs);

	model->radius = modelTable.GetFloat("radius", model->CalcDrawRadius());
	model->height = modelTable.GetFloat("height", model->CalcDrawHeight());

	model->relMidPos = modelTable.GetFloat3("midpos", model->CalcDrawMidPos());
}


static std::string FindTexture(std::string testTextureFile, const std::string& modelPath, const std::string& fallback)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (testTextureFile.empty())
		return fallback;

	// blender denotes relative paths with "//..", remove it
	if (testTextureFile.starts_with("//.."))
		testTextureFile = testTextureFile.substr(4);

	if (CFileHandler::FileExists(testTextureFile, SPRING_VFS_ZIP_FIRST))
		return testTextureFile;

	if (CFileHandler::FileExists("unittextures/" + testTextureFile, SPRING_VFS_ZIP_FIRST))
		return "unittextures/" + testTextureFile;

	if (CFileHandler::FileExists(modelPath + testTextureFile, SPRING_VFS_ZIP_FIRST))
		return modelPath + testTextureFile;

	return fallback;
}


static std::string FindTextureByRegex(const std::string& regex_path, const std::string& regex)
{
	RECOIL_DETAILED_TRACY_ZONE;
	//FIXME instead of ".*" only check imagetypes!
	const std::vector<std::string>& files = CFileHandler::FindFiles(regex_path, regex + ".*");

	if (!files.empty())
		return FindTexture(FileSystem::GetFilename(files[0]), "", "");

	return "";
}


void CAssParser::FindTextures(
	S3DModel* model,
	const aiScene* scene,
	const LuaTable& modelTable,
	const std::string& modelPath,
	const std::string& modelName
) {
	RECOIL_DETAILED_TRACY_ZONE;
	// 1. try to find by name (lowest priority)
	model->texs[0] = FindTextureByRegex("unittextures/", modelName);

	if (model->texs[0].empty()) model->texs[0] = FindTextureByRegex("unittextures/", modelName + "1");
	if (model->texs[1].empty()) model->texs[1] = FindTextureByRegex("unittextures/", modelName + "2");
	if (model->texs[0].empty()) model->texs[0] = FindTextureByRegex(modelPath, "tex1");
	if (model->texs[1].empty()) model->texs[1] = FindTextureByRegex(modelPath, "tex2");
	if (model->texs[0].empty()) model->texs[0] = FindTextureByRegex(modelPath, "diffuse");
	if (model->texs[1].empty()) model->texs[1] = FindTextureByRegex(modelPath, "glow"); // lowest-priority name

	// 2. gather model-defined textures of first material (medium priority)
	if (scene->mNumMaterials > 0) {
		constexpr unsigned int texTypes[] = {
			aiTextureType_SPECULAR,
			aiTextureType_UNKNOWN,
			aiTextureType_DIFFUSE,
			/*
			// TODO: support these too (we need to allow constructing tex1 & tex2 from several sources)
			aiTextureType_EMISSIVE,
			aiTextureType_HEIGHT,
			aiTextureType_NORMALS,
			aiTextureType_SHININESS,
			aiTextureType_OPACITY,
			*/
		};
		for (unsigned int texType: texTypes) {
			aiString textureFile;
			if (scene->mMaterials[0]->Get(AI_MATKEY_TEXTURE(texType, 0), textureFile) != aiReturn_SUCCESS)
				continue;

			assert(textureFile.length > 0);
			model->texs[0] = FindTexture(textureFile.data, modelPath, model->texs[0]);
		}
	}

	// 3. try to load from metafile (highest priority)
	model->texs[0] = FindTexture(modelTable.GetString("tex1", ""), modelPath, model->texs[0]);
	model->texs[1] = FindTexture(modelTable.GetString("tex2", ""), modelPath, model->texs[1]);
}

Transform SPseudoAssPiece::ComposeTransform(const float3& t, const float3& r, float s) const
{
	// NOTE:
	//   ORDER MATTERS (T(baked + script) * R(baked) * R(script) * S(baked))
	//   translating + rotating + scaling is faster than matrix-multiplying
	//   m is identity so m.SetPos(t)==m.Translate(t) but with fewer instrs
	Transform tra;
	tra.t = t;

	if (hasBakedTra)
		tra *= bakedTransform;

	tra *= Transform(CQuaternion::FromEulerYPRNeg(-r), ZeroVector, s);
	return tra;
}

void SPseudoAssPiece::SetPieceTransform(const Transform& tra)
{
	bposeTransform = tra * Transform{
		CQuaternion(),
		offset,
		scale
	};
}